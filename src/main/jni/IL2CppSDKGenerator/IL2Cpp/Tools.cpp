#include <android/log.h>
#include <libgen.h>
#include <inttypes.h>
#include <jni.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/md5.h>
#include <atomic>
#include <string>
#include <vector>
#include <utility>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include "Tools.h"

#if defined(__arm__)
#define process_vm_readv_syscall 376
#define process_vm_writev_syscall 377
#elif defined(__aarch64__)
#define process_vm_readv_syscall 270
#define process_vm_writev_syscall 271
#elif defined(__i386__)
#define process_vm_readv_syscall 347
#define process_vm_writev_syscall 348
#else
#define process_vm_readv_syscall 310
#define process_vm_writev_syscall 311
#endif

pid_t target_pid = -1;
#define INRANGE(x, a, b)        (x >= a && x <= b)
#define getBits(x)              (INRANGE(x,'0','9') ? (x - '0') : ((x&(~0x20)) - 'A' + 0xa))
#define getByte(x)              (getBits(x[0]) << 4 | getBits(x[1]))

ssize_t process_v(pid_t __pid, const struct iovec *__local_iov, unsigned long __local_iov_count, const struct iovec *__remote_iov, unsigned long __remote_iov_count, unsigned long __flags, bool iswrite) {
    return syscall((iswrite ? process_vm_writev_syscall : process_vm_readv_syscall), __pid, __local_iov, __local_iov_count, __remote_iov, __remote_iov_count, __flags);
}

bool pvm(void *address, void *buffer, size_t size, bool write = false) {
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buffer;
    local[0].iov_len = size;
    remote[0].iov_base = address;
    remote[0].iov_len = size;

    if (target_pid == -1) {
        target_pid = getpid();
    }

    ssize_t bytes = process_v(target_pid, local, 1, remote, 1, 0, write);
    return bytes == size;
}

void Tools::Hook(void *target, void *replace, void **backup) {
    unsigned long page_size = sysconf(_SC_PAGESIZE);
    unsigned long size = page_size * sizeof(uintptr_t);
    void *p = (void *) ((uintptr_t) target - ((uintptr_t) target % page_size) - page_size);
    if (mprotect(p, (size_t) size, PROT_EXEC | PROT_READ | PROT_WRITE) == 0) {
		DobbyHook(target, replace, backup);
    }
}

bool Tools::Read(void *addr, void *buffer, size_t length) {
	return memcpy(buffer, addr, length) != 0;
}

bool Tools::Write(void *addr, void *buffer, size_t length) {
	return memcpy(addr, buffer, length) != 0;
}

bool Tools::ReadAddr(void *addr, void *buffer, size_t length) {
	unsigned long page_size = sysconf(_SC_PAGESIZE);
	unsigned long size = page_size * sizeof(uintptr_t);
	return mprotect((void *) ((uintptr_t) addr - ((uintptr_t) addr % page_size) - page_size), (size_t) size, PROT_EXEC | PROT_READ | PROT_WRITE) == 0 && memcpy(buffer, addr, length) != 0;
}

bool Tools::WriteAddr(void *addr, void *buffer, size_t length) {
	unsigned long page_size = sysconf(_SC_PAGESIZE);
    unsigned long size = page_size * sizeof(uintptr_t);
    return mprotect((void *) ((uintptr_t) addr - ((uintptr_t) addr % page_size) - page_size), (size_t) size, PROT_EXEC | PROT_READ | PROT_WRITE) == 0 && memcpy(addr, buffer, length) != 0;
}

bool Tools::PVM_ReadAddr(void *addr, void *buffer, size_t length) {
    return pvm(addr, buffer, length, false);
}

bool Tools::PVM_WriteAddr(void *addr, void *buffer, size_t length) {
    return pvm(addr, buffer, length, true);
}

static uint8_t tools_isvalid_buf[4] = {0,0,0,0};

// ---------------------------------------------------------------------------
// Readable-memory probe for Tools::IsPtrValid.
//
// process_vm_readv is the cheap and precise fast path, but some Android builds
// block that raw syscall for untrusted_app SELinux domains. When that happens
// pvm() returns false for EVERY address, which made every guard that used
// IsPtrValid() reject valid objects -- including the skin ctor-hook installer,
// so all 19 skin hooks were skipped and the skins lists stayed empty.
//
// Fallback: a /proc/self/maps range check over a fixed-size POD double buffer.
// The parser fills the inactive half and publishes it with an atomic index, so
// readers never allocate, never take a lock and never observe a half-written
// table. This guard is called from the render thread (menu/ESP), the game
// thread (triggerbot / hitbox hooks) and the skin thread at the same time: the
// previous std::vector version cleared and reallocated the table under those
// readers, which is a use-after-free -- a crash, not a slow path.
// ---------------------------------------------------------------------------
static const int TOOLS_MAX_RANGES = 4096;   // /proc/self/maps is a few hundred lines

struct ToolsRangeBits { uintptr_t lo; uintptr_t hi; bool exec; };

static ToolsRangeBits       g_toolsRanges[2][TOOLS_MAX_RANGES];
static std::atomic<int>     g_toolsRangeCount[2];
static std::atomic<int>     g_toolsActiveRangeSet{0};
static std::atomic<uint64_t> g_toolsRangesStampMs{0};
static std::atomic<int>     g_toolsParseLock{0};
static std::atomic<int>     g_toolsPvmBlocked{-1};   // -1 unknown, 1 blocked, 0 working

static uint64_t ToolsNowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000ull + (uint64_t) ts.tv_nsec / 1000000ull;
}

static void ToolsLogOnce(const char *msg, bool blocked) {
    static std::atomic<int> logged{0};
    int expected = 0;
    if (logged.compare_exchange_strong(expected, 1, std::memory_order_acq_rel))
        __android_log_print(ANDROID_LOG_WARN, "MWD-ASTRAL", "%s (%s)", msg,
                            blocked ? "using /proc/self/maps fallback" : "process_vm_readv is fine");
}

// Does process_vm_readv work at all on this device? Probing our own code page
// answers the question without depending on any game object.
static bool ToolsPvmWorks() {
    int probe = 0;
    return pvm((void *) &ToolsPvmWorks, &probe, sizeof probe);
}

// Fill 'slot' from /proc/self/maps, return the number of readable ranges found.
static int ToolsBuildRangeTable(int slot) {
    int n = 0;
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) return 0;
    char line[512];
    while (n < TOOLS_MAX_RANGES && fgets(line, sizeof line, f)) {
        uintptr_t lo = 0, hi = 0;
        char perms[8] = {0};
        if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %7s", &lo, &hi, perms) == 3 &&
            perms[0] == 'r' && hi > lo) {
            g_toolsRanges[slot][n].lo = lo;
            g_toolsRanges[slot][n].hi = hi;
            // perms is "rwxp": index 2 is the execute bit. Only executable pages
            // may be handed to an inline hook.
            g_toolsRanges[slot][n].exec = (perms[2] == 'x');
            ++n;
        }
    }
    fclose(f);
    return n;
}

static void ToolsRefreshRanges() {
    // One parser at a time; everybody else keeps using the published snapshot.
    int expected = 0;
    if (!g_toolsParseLock.compare_exchange_strong(expected, 1, std::memory_order_acq_rel))
        return;

    const int slot = 1 - g_toolsActiveRangeSet.load(std::memory_order_relaxed);
    const int n = ToolsBuildRangeTable(slot);
    if (n > 0) {
        // Publish complete tables only: the count is written first, then the
        // index is released, so a reader can never see a truncated table.
        g_toolsRangeCount[slot].store(n, std::memory_order_relaxed);
        g_toolsActiveRangeSet.store(slot, std::memory_order_release);
    }
    g_toolsRangesStampMs.store(ToolsNowMs(), std::memory_order_relaxed);
    g_toolsParseLock.store(0, std::memory_order_release);
}

// True when [addr, addr+len) lies completely inside one readable mapping.
// When requireExec is set the mapping must also be executable: that is the test
// a hook target has to pass before we patch it.
static bool ToolsAddrReadable(void *addr, size_t len, bool requireExec = false) {
    const uintptr_t a = (uintptr_t) addr;
    if (a == 0 || a + len < a)
        return false;

    const uint64_t now = ToolsNowMs();
    const uint64_t stamp = g_toolsRangesStampMs.load(std::memory_order_relaxed);
    const int active = g_toolsActiveRangeSet.load(std::memory_order_acquire);
    if (g_toolsRangeCount[active].load(std::memory_order_relaxed) <= 0 ||
        now < stamp || now - stamp > 250) {
        ToolsRefreshRanges();
    }

    const int slot = g_toolsActiveRangeSet.load(std::memory_order_acquire);
    const int count = g_toolsRangeCount[slot].load(std::memory_order_relaxed);
    if (count <= 0)
        return false;

    // Ranges come out of /proc/self/maps already sorted by start address.
    int lo = 0, hi = count;
    while (lo < hi) {
        const int mid = lo + (hi - lo) / 2;
        if (a < g_toolsRanges[slot][mid].lo)          hi = mid;
        else if (a >= g_toolsRanges[slot][mid].hi)    lo = mid + 1;
        else return (a + len <= g_toolsRanges[slot][mid].hi) &&
                     (!requireExec || g_toolsRanges[slot][mid].exec);
    }
    return false;
}

// Report how pointer validation is working on this device (used by the boot log).
void Tools::Diag(char *buffer, size_t size) {
    const int blocked = g_toolsPvmBlocked.load(std::memory_order_relaxed);
    const int slot = g_toolsActiveRangeSet.load(std::memory_order_acquire);
    snprintf(buffer, size, "pvm=%s maps_ranges=%d maps_age_ms=%llu",
             blocked == 1 ? "blocked" : (blocked == 0 ? "ok" : "unknown"),
             g_toolsRangeCount[slot].load(std::memory_order_relaxed),
             (unsigned long long)(ToolsNowMs() - g_toolsRangesStampMs.load(std::memory_order_relaxed)));
}

// Executable-memory probe used before installing any inline hook. A stale
// offset (game updated, dump from another build) points into data or the middle
// of an unrelated function; patching that takes the whole process down later,
// usually during the next load. Refusing it here turns "random crash" into a
// single log line naming the address.
bool Tools::IsExecPtr(void *addr) {
    if (addr == nullptr) return false;
    // process_vm_readv can prove a page is readable, but the execute bit only
    // exists in the maps snapshot, so both questions are answered there.
    return ToolsAddrReadable(addr, sizeof(void *), true);
}

bool Tools::IsPtrValid(void *addr) {
    // A valid pointer is non-null and points into readable mapped memory.
    if (addr == nullptr) return false;
    if (pvm(addr, tools_isvalid_buf, sizeof tools_isvalid_buf)) return true;

    const int blocked = g_toolsPvmBlocked.load(std::memory_order_relaxed);
    if (blocked == 0)
        return false;                       // pvm works, so this address really is bad

    if (blocked < 0) {
        // First failure: work out whether the syscall is blocked for us at all.
        const bool isBlocked = !ToolsPvmWorks();
        g_toolsPvmBlocked.store(isBlocked ? 1 : 0, std::memory_order_relaxed);
        ToolsLogOnce("IsPtrValid: process_vm_readv probe failed", isBlocked);
        if (!isBlocked)
            return false;
    }

    // pvm is unusable here: fall back to the live /proc/self/maps snapshot
    // instead of blindly rejecting, which would silently disable every guarded
    // hook (that is exactly what emptied the skins lists).
    return ToolsAddrReadable(addr, sizeof(void *));
}

uintptr_t Tools::GetBaseAddress(const char *name) {
    uintptr_t base = 0;
    char line[512];

    FILE *f = fopen("/proc/self/maps", "r");

    if (!f) {
        return 0;
    }
    while (fgets(line, sizeof line, f)) {
        uintptr_t tmpBase;
        char tmpName[256];
        if (sscanf(line, "%" PRIXPTR "-%*" PRIXPTR "%*s %*s %*s %*s %s", &tmpBase, tmpName) > 0) {
            if (!strcmp(basename(tmpName), name)) {
                base = tmpBase;
                break;
            }
        }
    }

    fclose(f);
    return base;
}

uintptr_t Tools::GetRealOffsets(const char *libraryName, uintptr_t relativeAddr) {
	uintptr_t libBase = Tools::GetBaseAddress(libraryName);
	if (libBase == 0)
		return 0;
	return (reinterpret_cast<uintptr_t>(libBase + relativeAddr));
}


uintptr_t Tools::GetEndAddress(const char *name) {
    uintptr_t end = 0;
    char line[512];

    FILE *f = fopen("/proc/self/maps", "r");

    if (!f) {
        return 0;
    }

    bool found = false;
    while (fgets(line, sizeof line, f)) {
        uintptr_t tmpEnd;
        char tmpName[256];
        if (sscanf(line, "%*" PRIXPTR "-%" PRIXPTR "%*s %*s %*s %*s %s", &tmpEnd, tmpName) > 0) {
            if (!strcmp(basename(tmpName), name)) {
                if (!found) {
                    found = true;
                }
            } else {
                if (found) {
                    end = tmpEnd;
                    break;
                }
            }
        }
    }

    fclose(f);
    return end;
}


uintptr_t Tools::FindPattern(const char *lib, const char *pattern) {
    auto start = GetBaseAddress(lib);
    if (!start)
        return 0;
    auto end = GetEndAddress(lib);
    if (!end)
        return 0;
    auto curPat = reinterpret_cast<const unsigned char *>(pattern);
    uintptr_t firstMatch = 0;
    for (uintptr_t pCur = start; pCur < end; ++pCur) {
        if (*(uint8_t *) curPat == (uint8_t) '\?' || *(uint8_t *) pCur == getByte(curPat)) {
            if (!firstMatch) {
                firstMatch = pCur;
            }
            curPat += (*(uint16_t *) curPat == (uint16_t) '\?\?' || *(uint8_t *) curPat != (uint8_t) '\?') ? 2 : 1;
            if (!*curPat) {
                return firstMatch;
            }
            curPat++;
            if (!*curPat) {
                return firstMatch;
            }
        } else if (firstMatch) {
            pCur = firstMatch;
            curPat = reinterpret_cast<const unsigned char *>(pattern);
            firstMatch = 0;
        }
    }
    return 0;
}

std::string Tools::RandomString(const int len) {
    static const char alphanumerics[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    srand((unsigned) time(0) * getpid());
    std::string tmp;
    tmp.reserve(len);
    for (int i = 0; i < len; ++i) {
        tmp += alphanumerics[rand() % (sizeof(alphanumerics) - 1)];
    }
    return tmp;
}

std::string Tools::GetPackageName(JNIEnv *env, jobject context) {
    jclass contextClass = env->FindClass("android/content/Context");
    jmethodID getPackageNameId = env->GetMethodID(contextClass, "getPackageName", "()Ljava/lang/String;");
    auto str = (jstring) env->CallObjectMethod(context, getPackageNameId);
    return env->GetStringUTFChars(str, 0);
}

const char *Tools::GetAndroidID(JNIEnv *env, jobject context) {
    jclass contextClass = env->FindClass("android/content/Context");
    jmethodID getContentResolverMethod = env->GetMethodID(contextClass, "getContentResolver", "()Landroid/content/ContentResolver;");
    jclass settingSecureClass = env->FindClass("android/provider/Settings$Secure");
    jmethodID getStringMethod = env->GetStaticMethodID(settingSecureClass, "getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;");

    auto obj = env->CallObjectMethod(context, getContentResolverMethod);
    auto str = (jstring) env->CallStaticObjectMethod(settingSecureClass, getStringMethod, obj, env->NewStringUTF("android_id"));
    return env->GetStringUTFChars(str, 0);
}

const char *Tools::GetDeviceModel(JNIEnv *env) {
	jclass buildClass = env->FindClass("android/os/Build");
	jfieldID modelId = env->GetStaticFieldID(buildClass, "MODEL", "Ljava/lang/String;");
	
	auto str = (jstring) env->GetStaticObjectField(buildClass, modelId);
	return env->GetStringUTFChars(str, 0);
}

const char *Tools::GetDeviceBrand(JNIEnv *env) {
	jclass buildClass = env->FindClass("android/os/Build");
	jfieldID modelId = env->GetStaticFieldID(buildClass, "BRAND", "Ljava/lang/String;");
	
	auto str = (jstring) env->GetStaticObjectField(buildClass, modelId);
	return env->GetStringUTFChars(str, 0);
}

const char *Tools::GetDeviceUniqueIdentifier(JNIEnv *env, const char *uuid) {
    jclass uuidClass = env->FindClass("java/util/UUID");

    auto len = strlen(uuid);

    jbyteArray myJByteArray = env->NewByteArray(len);
    env->SetByteArrayRegion(myJByteArray, 0, len, (jbyte *) uuid);

    jmethodID nameUUIDFromBytesMethod = env->GetStaticMethodID(uuidClass, "nameUUIDFromBytes", "([B)Ljava/util/UUID;");
    jmethodID toStringMethod = env->GetMethodID(uuidClass, "toString", "()Ljava/lang/String;");

    auto obj = env->CallStaticObjectMethod(uuidClass, nameUUIDFromBytesMethod, myJByteArray);
    auto str = (jstring) env->CallObjectMethod(obj, toStringMethod);
    return env->GetStringUTFChars(str, 0);
}

std::string Tools::CalcMD5(std::string s) {
    std::string result;

    unsigned char hash[MD5_DIGEST_LENGTH];
    char tmp[4];

    MD5_CTX md5;
    MD5_Init(&md5);
    MD5_Update(&md5, s.c_str(), s.length());
    MD5_Final(hash, &md5);
	
    for (unsigned char i : hash) {
        sprintf(tmp, "%02x", i);
        result += tmp;
    }
	
    return result;
}
