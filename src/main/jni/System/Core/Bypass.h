#pragma once

#include "Includes/SafePatch.h"

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

inline void InitializeProtection() {
    if (SafeModeEnabled())
    {
        SAFELOG("protection: safe mode -- anti-cheat hooks skipped");
        return;
    }

    int installed = 0;
    for (uintptr_t rva : kProtectionHookRvAs) {
        char label[96];
        snprintf(label, sizeof label, "protection 0x%lx", (unsigned long) rva);
        if (SafeToolsHook("libanogs.so", rva, (void *) hook_bypass, (void **) &orig_bypass, label))
            ++installed;
    }
    SAFELOG("protection: %d/%d anti-cheat hooks installed", installed,
            (int) (sizeof(kProtectionHookRvAs) / sizeof(kProtectionHookRvAs[0])));
}
    
    
