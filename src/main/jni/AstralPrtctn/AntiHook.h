#include "Includes/SafePatch.h"
#include <sys/prctl.h>
#include <dirent.h>
#include <dlfcn.h>
void* GetAnogs() {
    static void* cached = nullptr;
    if (cached) return cached;
    FILE* fp = fopen(OBFUSCATE("/proc/self/maps"), "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, OBFUSCATE("libanogs.so")) && strstr(line, OBFUSCATE("r-xp"))) {
                uintptr_t start = 0;
                sscanf(line, OBFUSCATE("%lx"), &start);
                uintptr_t end = 0;
                char perms[5] = {0};
                sscanf(line, OBFUSCATE("%lx-%lx %4s"), &start, &end, perms);
                if (start && end > start) {
                    size_t size = end - start;
                    void* address = (void*)start;
                    void* temp = mmap(nullptr, size, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                    if (temp != MAP_FAILED) {
                        memcpy(temp, address, size);
                        void* remapped = mremap(temp, size, size, MREMAP_MAYMOVE | MREMAP_FIXED, address);
                        if (remapped != MAP_FAILED) {
                            int prot = PROT_READ | PROT_EXEC;
                            mprotect(address, size, prot);
                        } else {
                            munmap(temp, size);
                        }
                    }
                    fclose(fp);
                    cached = (void*)start;
                    return cached;
                }
            }
        }
        fclose(fp);
    }
    return nullptr;
}

uintptr_t EncOff(uintptr_t offs) {
    static uintptr_t key = 0;
    if (!key) {
        key = (uintptr_t)GetAnogs;
        key = (key << 16) | (key >> 48);
        if (key == 0) key = 1;
    }
    return offs ^ key;
}

uintptr_t DecOff(uintptr_t encrypted) {
    static uintptr_t key = 0;
    if (!key) {
        key = (uintptr_t)GetAnogs;
        key = (key << 16) | (key >> 48);
        if (key == 0) key = 1;
    }
    return encrypted ^ key;
}

void Kooh(uintptr_t encoffs, void* hook, void** original) {
    void* Anogs = GetAnogs();
    if (Anogs) {
        uintptr_t real = DecOff(encoffs);
        void* target = (void*)((uintptr_t)Anogs + real);
        Tools::Hook(target, hook, original);
    }
}

void ApplyHook(uintptr_t& offs, void* hook, void** orig) {
    Kooh(offs, hook, orig);
    usleep(3000000);
    static uintptr_t key = 0;
    if (!key) {
        key = (uintptr_t)ApplyHook;
        key = (key << 16) | (key >> 48);
        if (key == 0) key = 1;
    }
    offs = offs ^ key;
}

// ============================================================================
// STEALTH LAYER -- ported from the VAR3 build, which is the lineage that runs on
// this game build. It is what keeps the anti-cheat from terminating the client
// once this module has hooks in it:
//
//   1. PR_SET_DUMPABLE = 0 -- the kernel then refuses /proc/<pid>/mem and
//      /proc/<pid>/maps opens and process_vm_readv() from any process without
//      CAP_SYS_PTRACE (i.e. from every memory tool running as a normal app).
//   2. ptrace() and process_vm_readv/writev are hooked to fail with EPERM, so
//      userspace inspection of this process gets nothing even if it gets past
//      the flag.
//
// The comment in the VAR3 source is worth keeping: read/open/openat must NOT be
// hooked globally. They run for every operation in the process and break GMS and
// the launcher -- that was a crash in its own right.
// ============================================================================

#ifndef PTRACE_TRACEME
#define PTRACE_TRACEME      0
#endif
#ifndef PTRACE_PEEKTEXT
#define PTRACE_PEEKTEXT     1
#endif
#ifndef PTRACE_PEEKDATA
#define PTRACE_PEEKDATA     2
#endif
#ifndef PTRACE_PEEKUSER
#define PTRACE_PEEKUSER     3
#endif
#ifndef PTRACE_POKETEXT
#define PTRACE_POKETEXT     4
#endif
#ifndef PTRACE_POKEDATA
#define PTRACE_POKEDATA     5
#endif
#ifndef PTRACE_POKEUSER
#define PTRACE_POKEUSER     6
#endif
#ifndef PTRACE_GETREGS
#define PTRACE_GETREGS      12
#endif
#ifndef PTRACE_SETREGS
#define PTRACE_SETREGS      13
#endif
#ifndef PTRACE_ATTACH
#define PTRACE_ATTACH       16
#endif
#ifndef PTRACE_DETACH
#define PTRACE_DETACH       17
#endif
#ifndef PTRACE_GETREGSET
#define PTRACE_GETREGSET    0x4204
#endif
#ifndef PTRACE_SETREGSET
#define PTRACE_SETREGSET    0x4205
#endif
#ifndef PTRACE_SEIZE
#define PTRACE_SEIZE        0x4206
#endif

#define ANTI_LOG_TAG "ANTIHOOK"
#define ANTI_LOGI(...) __android_log_print(ANDROID_LOG_INFO,  ANTI_LOG_TAG, __VA_ARGS__)
#define ANTI_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, ANTI_LOG_TAG, __VA_ARGS__)

static long (*g_orig_ptrace)(int, pid_t, void*, void*) = nullptr;

static long Hook_ptrace(int request, pid_t pid, void* addr, void* data) {
    switch (request) {
        case PTRACE_ATTACH:
        case PTRACE_SEIZE:
        case PTRACE_PEEKTEXT:
        case PTRACE_PEEKDATA:
        case PTRACE_POKETEXT:
        case PTRACE_POKEDATA:
        case PTRACE_PEEKUSER:
        case PTRACE_POKEUSER:
        case PTRACE_GETREGS:
        case PTRACE_SETREGS:
        case PTRACE_GETREGSET:
        case PTRACE_SETREGSET:
            errno = EPERM;
            return -1;
        case PTRACE_TRACEME:
            return 0;
        default:
            break;
    }
    if (g_orig_ptrace) return g_orig_ptrace(request, pid, addr, data);
    errno = EPERM;
    return -1;
}

static ssize_t (*g_orig_process_vm_readv)(pid_t, const struct iovec*, unsigned long,
                                          const struct iovec*, unsigned long, unsigned long) = nullptr;

static ssize_t Hook_process_vm_readv(pid_t, const struct iovec*, unsigned long,
                                     const struct iovec*, unsigned long, unsigned long) {
    errno = EPERM;
    return -1;
}

static ssize_t (*g_orig_process_vm_writev)(pid_t, const struct iovec*, unsigned long,
                                           const struct iovec*, unsigned long, unsigned long) = nullptr;

static ssize_t Hook_process_vm_writev(pid_t, const struct iovec*, unsigned long,
                                      const struct iovec*, unsigned long, unsigned long) {
    errno = EPERM;
    return -1;
}

static bool g_stealthInstalled = false;

inline void InstallStealth() {
    if (g_stealthInstalled) return;
    g_stealthInstalled = true;

    // STEP 1 -- primary defence.
    if (prctl(PR_SET_DUMPABLE, 0, 0, 0, 0) == 0)
        ANTI_LOGI("PR_SET_DUMPABLE=0 (kernel-level /proc/mem + process_vm block)");
    else
        ANTI_LOGE("PR_SET_DUMPABLE=0 failed (errno %d)", errno);

    // STEP 2 -- userspace backup. Only the syscalls that exist purely for
    // cross-process memory access are hooked; the libraries may not be loaded
    // yet, in which case dlsym returns null and we simply skip that one.
    void* a = (void*) dlsym(RTLD_DEFAULT, "ptrace");
    if (a && SafeDobbyHookAddr(a, (void*) Hook_ptrace, (void**) &g_orig_ptrace, "ptrace"))
        ANTI_LOGI("ptrace hooked");

    a = (void*) dlsym(RTLD_DEFAULT, "process_vm_readv");
    if (a && SafeDobbyHookAddr(a, (void*) Hook_process_vm_readv, (void**) &g_orig_process_vm_readv, "process_vm_readv"))
        ANTI_LOGI("process_vm_readv hooked");

    a = (void*) dlsym(RTLD_DEFAULT, "process_vm_writev");
    if (a && SafeDobbyHookAddr(a, (void*) Hook_process_vm_writev, (void**) &g_orig_process_vm_writev, "process_vm_writev"))
        ANTI_LOGI("process_vm_writev hooked");
}
