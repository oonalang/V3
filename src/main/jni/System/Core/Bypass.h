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

// Every one of these is a blind write into the protection library at a
// hard-coded RVA, executed during the game's own startup. They now go through
// SafePatchHex, which refuses anything that is not mapped executable code and
// logs what it did -- a stale RVA is a log line instead of a dead process.
inline void InitializeProtection() {
    if (SafeModeEnabled())
    {
        SAFELOG("protection: safe mode -- libanogs patches skipped");
        return;
    }

    static const uintptr_t kProtectionRvAs[] = {
        0x204218, 0x3893D8, 0x455A80, 0x497244, 0x4AFC1C,
    };
    int applied = 0;
    for (uintptr_t rva : kProtectionRvAs) {
        if (SafePatchHex("libanogs.so", rva, armFalse, "protection stub"))
            ++applied;
    }
    // The entry page holds the library's ELF header and its trampolines. Only
    // the executable half is patched: the old loop stepped 4 bytes from offset
    // 0x1 and wrote over the header and section table too, which corrupts the
    // library instead of disabling the checks inside it.
    applied += SafePatchRange("libanogs.so", 0x1, 0x1000, 4, armFalse, "protection entry page");
    SAFELOG("protection: %d patches applied", applied);
}
    
    
