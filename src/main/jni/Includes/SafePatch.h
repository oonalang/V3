#ifndef SAFEPATCH_H
#define SAFEPATCH_H
#pragma once

// ============================================================================
// Verified patches + hooks, plus a safe mode that can be flipped without a
// rebuild.
//
// Every hook and every byte patch in this module writes to a hard-coded RVA.
// When that RVA does not belong to the game build that is actually running, the
// write lands in .rodata, in the middle of an unrelated function or in an
// unmapped page, and the game dies during its next load. From the outside that
// looks exactly like "black screen, then crash, no menu, no login".
//
// These wrappers refuse any target that cannot be proven to be mapped
// executable code, and they check that the write actually landed. A refused
// patch is a log line instead of a dead process.
// ============================================================================

#include <android/log.h>
#include <sys/stat.h>
#include <cstdint>
#include <cstdio>

#include "IL2CppSDKGenerator/IL2Cpp/Tools.h"            // Tools::IsExecPtr + DobbyHook
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "IL2CppSDKGenerator/KittyMemory/KittyMemory.h"  // KittyMemory::getAbsoluteAddress

#define SAFELOG(...) ((void)__android_log_print(ANDROID_LOG_INFO,  "MWD-ASTRAL", __VA_ARGS__))
#define SAFEERR(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "MWD-ASTRAL", __VA_ARGS__))

// ── safe mode ────────────────────────────────────────────────────────────────
// Create an empty file at any of these paths and the module still loads and
// still draws its overlay, but touches nothing inside the game: no hooks, no
// patches. That is how "our hooks killed it" is told apart from "our overlay
// killed it" without guessing.
inline bool SafeModeEnabled()
{
    static int cached = -1;
    if (cached >= 0)
        return cached == 1;

    static const char *kMarkers[] = {
        "/storage/emulated/0/Android/data/com.garena.game.codm/files/astral_safe_mode",
        "/sdcard/astral_safe_mode",
        "/data/local/tmp/astral_safe_mode",
    };

    bool found = false;
    for (const char *path : kMarkers) {
        struct stat st;
        if (stat(path, &st) == 0) {
            SAFELOG("SAFE MODE: marker found at %s", path);
            found = true;
            break;
        }
    }

    cached = found ? 1 : 0;
    SAFELOG("boot: safe mode %s",
            found ? "ON -- every game hook and patch is disabled" : "off");
    return found;
}

// ── target validation ────────────────────────────────────────────────────────
// A patch or hook target must be 4-byte aligned and inside executable memory.
// The probe covers the 16 bytes an inline hook's trampoline replaces, so it
// cannot pass on a target whose tail runs off the end of the code mapping.
inline bool SafeTargetOk(void *addr, const char *label, size_t len = 16)
{
    if (addr == nullptr) {
        SAFEERR("refused %s: offset did not resolve (library not mapped yet?)", label);
        return false;
    }
    if (((uintptr_t) addr & 3) != 0) {
        SAFEERR("refused %s: %p is not 4-byte aligned", label, addr);
        return false;
    }
    if (!Tools::IsExecPtr(addr) || (len > 8 && !Tools::IsExecPtr((char *) addr + 8))) {
        SAFEERR("refused %s: %p is not executable code -- RVA does not match this build",
                label, addr);
        return false;
    }
    return true;
}

// Builds a patch object without touching the bytes, but only after proving the
// target is readable executable code: MemoryPatch's constructor reads the
// original bytes, so an unmapped RVA is a crash on its own.
inline MemoryPatch SafeBuildPatch(const char *lib, uintptr_t rva, const char *hex, const char *label)
{
    MemoryPatch empty;
    if (SafeModeEnabled())
        return empty;

    void *addr = (void *) KittyMemory::getAbsoluteAddress(lib, rva);
    char name[160];
    snprintf(name, sizeof name, "%s (%s+0x%lx)", label, lib, (unsigned long) rva);
    if (!SafeTargetOk(addr, name, 8))
        return empty;

    MemoryPatch patch = MemoryPatch::createWithHex(lib, rva, hex);
    if (!patch.isValid())
        SAFEERR("refused %s: patch object invalid", name);
    return patch;
}

// ── verified byte patch (build + apply in one step) ──────────────────────────
inline bool SafePatchHex(const char *lib, uintptr_t rva, const char *hex, const char *label)
{
    if (SafeModeEnabled())
        return false;

    void *addr = (void *) KittyMemory::getAbsoluteAddress(lib, rva);
    char name[160];
    snprintf(name, sizeof name, "%s (%s+0x%lx)", label, lib, (unsigned long) rva);
    if (!SafeTargetOk(addr, name, 8))
        return false;

    MemoryPatch patch = MemoryPatch::createWithHex(lib, rva, hex);
    if (!patch.isValid()) {
        SAFEERR("refused %s: patch object invalid", name);
        return false;
    }

    const uint32_t before = *(volatile uint32_t *) addr;
    if (!patch.Modify()) {
        SAFEERR("failed %s: Modify refused (mprotect?)", name);
        return false;
    }
    if (*(volatile uint32_t *) addr == before) {
        SAFEERR("failed %s: bytes unchanged after Modify", name);
        return false;
    }
    SAFELOG("patched %s", name);
    return true;
}

// Toggle-time apply for the patches features switch on later (wallhack, no
// crouch, ...). Same proof, done at the moment of the write.
inline bool SafeModify(MemoryPatch &patch, const char *label)
{
    if (SafeModeEnabled())
        return false;
    if (!patch.isValid()) {
        SAFEERR("refused %s: patch was never built (target not mapped at boot?)", label);
        return false;
    }
    if (!SafeTargetOk((void *) patch.get_TargetAddress(), label)) {
        SAFEERR("refused %s: target is not executable code", label);
        return false;
    }
    const uint32_t before = *(volatile uint32_t *) patch.get_TargetAddress();
    if (!patch.Modify()) {
        SAFEERR("failed %s: Modify refused", label);
        return false;
    }
    if (*(volatile uint32_t *) patch.get_TargetAddress() == before) {
        SAFEERR("failed %s: bytes unchanged after Modify", label);
        return false;
    }
    return true;
}

// ── verified inline hook (Dobby) ─────────────────────────────────────────────
inline bool SafeDobbyHookAddr(void *addr, void *replace, void **orig, const char *label)
{
    if (SafeModeEnabled())
        return false;
    if (replace == nullptr)
        return false;
    if (!SafeTargetOk(addr, label))
        return false;

    const uint32_t before = *(volatile uint32_t *) addr;
    DobbyHook(addr, replace, orig);
    if (*(volatile uint32_t *) addr == before) {
        SAFEERR("failed %s: trampoline did not land at %p", label, addr);
        return false;
    }
    SAFELOG("hooked %s at %p", label, addr);
    return true;
}

inline bool SafeDobbyHook(const char *lib, uintptr_t rva, void *replace, void **orig, const char *label)
{
    char name[160];
    snprintf(name, sizeof name, "%s (%s+0x%lx)", label, lib, (unsigned long) rva);
    return SafeDobbyHookAddr((void *) KittyMemory::getAbsoluteAddress(lib, rva), replace, orig, name);
}

// A range of RVAs, used to blank the protection library's entry page. Only
// aligned executable targets are touched: the raw loop this replaces also wrote
// into the ELF header and section table, corrupting the library instead of
// disabling the checks inside it.
inline int SafePatchRange(const char *lib, uintptr_t from, uintptr_t to, uintptr_t step,
                          const char *hex, const char *label)
{
    if (SafeModeEnabled())
        return 0;

    int applied = 0, skipped = 0;
    for (uintptr_t rva = from; rva + 8 <= to; rva += step) {
        void *addr = (void *) KittyMemory::getAbsoluteAddress(lib, rva);
        if (addr == nullptr || ((uintptr_t) addr & 3) != 0 || !Tools::IsExecPtr(addr)) {
            ++skipped;
            continue;
        }
        if (SafePatchHex(lib, rva, hex, label))
            ++applied;
    }
    SAFELOG("%s: %d applied, %d skipped (not executable code)", label, applied, skipped);
    return applied;
}

#endif // SAFEPATCH_H
