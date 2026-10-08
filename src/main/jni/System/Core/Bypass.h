#pragma once

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

inline void InitializeProtection() {
    static bool s_protectionInitialized = false;
    if (s_protectionInitialized) {
        return;
    }
    s_protectionInitialized = true;

    if (VM == nullptr) {
        return;
    }

    MemoryPatch::createWithHex("libanogs.so", 0x204218, "00 00 80 D2 C0 03 5F D6").Modify();
    MemoryPatch::createWithHex("libanogs.so", 0x3893D8, "00 00 80 D2 C0 03 5F D6").Modify();
    MemoryPatch::createWithHex("libanogs.so", 0x455A80, "00 00 80 D2 C0 03 5F D6").Modify();
    MemoryPatch::createWithHex("libanogs.so", 0x497244, "00 00 80 D2 C0 03 5F D6").Modify();
    MemoryPatch::createWithHex("libanogs.so", 0x4AFC1C, "00 00 80 D2 C0 03 5F D6", 32);
    for (auto offs : range{0x1, 0x1000}) {
        MemoryPatch::createWithHex("libanogs.so", offs, armFalse).Modify();
    }
}