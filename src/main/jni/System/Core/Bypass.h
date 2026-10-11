#pragma once

#include "Includes/SafePatch.h"
#include <unistd.h>   // usleep (bounded wait for the game's libraries)

bool (*orig_bypass)(void *ins);
bool hook_bypass(void *ins) {
    return false;
}

#if defined(__aarch64__)

inline const char *armFalse = "00 00 80 D2 C0 03 5F D6";

#endif

struct range {
    uintptr_t Irt, Ind;
    struct Iter {
      uintptr_t val;
      bool operator!=(const Iter& it) const {
        return val <= it.val; 
      }
      uintptr_t operator*() const {
        return val; 
      }
      void operator++() {
        val += 4; 
      }
    };
    Iter begin() const { 
      return {Irt}; 
    }
    Iter end() const {
      return {Ind}; 
    }
};

// ── anti-cheat neutralisation ───────────────────────────────────────────────
// Ported from the VAR3 build -- the one that survives this game build. Each
// address below is an entry point in the game's protection library that reports
// on the client; hooking it to `hook_bypass` (which returns false) is what keeps
// the process alive while the rest of the module patches game code.
//
// This is the difference that matters. The old code wrote "mov x0, #0; ret"
// over a *different* address list (and over the library's ELF header), which
// left these detectors running: a client whose hooks are found gets terminated
// during its own load, which from the outside is "black screen, then the game is
// gone". hook_bypass was defined in this file and never installed anywhere.
inline const uintptr_t kProtectionHookRvAs[] = {
    0x202680,
    0x204218,
    0x258B6C,
    0x259670,
    0x3055A0,
    0x3075C4,
    0x30E234,
    0x35140C,
    0x303A68,
    0x47D0C8,
    0x245F34,
    0x2B8CDC,
    0x4AB5EC,
    0x4E70A4,
    0x39CD30,
    0x259010,
    0x44BC90,
    0x4A9A00,
    0x4952E8,
    0x4951C8,
    0x49521C,
    0x42BA18,
    0x420578,
    0x1FF3A4,
    0x455A80,
    0x4F03D4,
    0x39AE94,
    0x41BA40,
    0x1EE554,
    0x1F815C,
    0x203460,
    0x24D638,
    0x24EDD0,
    0x257B90,
    0x28F894,
    0x2A9338,
    0x2ABBC8,
    0x438154,
    0x44A714,
    0x48CF20
};

// ── wait for a game library to appear ────────────────────────────────────────
// The protection library is mapped by the *game*, not by this module: at
// JNI_OnLoad only our own .so is in memory, so every libanogs.so target resolves
// to null. Hooking before that point used to be a silent no-op for the whole
// anti-cheat bypass (the client then gets terminated by the detector during its
// own load -- the reported "black screen, then the game is gone"). Wait for the
// mapping, bounded, and say so in the log when it never shows up.
inline bool WaitForLibrary(const char *lib, int timeoutMs)
{
    const int steps = timeoutMs > 0 ? timeoutMs / 250 : 0;
    for (int i = 0; i < steps; ++i) {
        if (Tools::GetBaseAddress(lib) != 0)
            return true;
        usleep(250 * 1000);
    }
    return Tools::GetBaseAddress(lib) != 0;
}

inline void InitializeProtection() {
    if (SafeModeEnabled())
    {
        SAFELOG("protection: safe mode -- anti-cheat hooks skipped");
        return;
    }

    // libanogs.so loads after this module does. Installing these hooks before it
    // is mapped installs nothing and leaves the anti-cheat free to kill the
    // client mid-load. Wait for it (the game maps it early in its own boot).
    if (!WaitForLibrary("libanogs.so", 120 * 1000)) {
        SAFEERR("protection: libanogs.so never mapped -- anti-cheat bypass NOT installed");
        return;
    }

    const int total = (int) (sizeof(kProtectionHookRvAs) / sizeof(kProtectionHookRvAs[0]));
    static bool done[sizeof(kProtectionHookRvAs) / sizeof(kProtectionHookRvAs[0])] = {};
    int installed = 0;

    // Two passes: while the library is still being mapped the range probe can
    // read a half-populated map and refuse a target that is fine a moment later.
    for (int pass = 0; pass < 2; ++pass) {
        int thisPass = 0;
        for (int i = 0; i < total; ++i) {
            if (done[i])
                continue;
            char label[96];
            snprintf(label, sizeof label, "protection 0x%lx", (unsigned long) kProtectionHookRvAs[i]);
            if (SafeToolsHook("libanogs.so", kProtectionHookRvAs[i], (void *) hook_bypass,
                              (void **) &orig_bypass, label)) {
                done[i] = true;
                ++installed;
                ++thisPass;
            }
        }
        if (installed == total || thisPass == 0)
            break;
        usleep(1500 * 1000);
    }

    SAFELOG("protection: %d/%d anti-cheat hooks installed", installed, total);
}
    
    
