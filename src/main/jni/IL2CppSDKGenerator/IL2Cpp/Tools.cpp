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
// process_vm_readv is the cheap fast path, but some Android builds block the
// raw syscall for untrusted_app SELinux domains. When that happens pvm()
// returns false for EVERY address, which made every guard that used
// IsPtrValid() reject valid objects — including the skin ctor-hook installer,
// so all 19 skin hooks were skipped and the skins lists stayed empty.
//
// Fallback: a cached /proc/self/maps range check. The map list is refreshed at
// most every 250 ms; per-call cost is a binary search over the cached ranges,
// so the per-frame ESP/aim callers stay cheap.
// ---------------------------------------------------------------------------
static std::vector<std::pair<uintptr_t, uintptr_t>> g_readableRanges;
static uint64_t g_readableCacheMs = 0;

static uint64_t ToolsNowMs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

static void ToolsRefreshReadableRanges() {
    g_readableRanges.clear();
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof line, f)) {
        uintptr_t lo = 0, hi = 0;
        char perms[8] = {0};
        if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %7s", &lo, &hi, perms) == 3 &&
            perms[0] == 'r' && hi > lo) {
            g_readableRanges.emplace_back(lo, hi);
        }
    }
    fclose(f);
}

static bool ToolsAddrReadable(void *addr) {
    const uintptr_t a = (uintptr_t)addr;
    uint64_t now = ToolsNowMs();
    if (g_readableRanges.empty() || now < g_readableCacheMs || now - g_readableCacheMs > 250) {
        ToolsRefreshReadableRanges();
        g_readableCacheMs = now;
    }
    // binary search for a range containing 'a'
    size_t lo = 0, hi = g_readableRanges.size();
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (a < g_readableRanges[mid].first)      hi = mid;
        else if (a >= g_readableRanges[mid].second) lo = mid + 1;
        else return true;
    }
    return false;
}

bool Tools::IsPtrValid(void *addr) {
    // A valid pointer is non-null and points into readable mapped memory.
    if (addr == nullptr) return false;
    if (pvm(addr, tools_isvalid_buf, sizeof tools_isvalid_buf)) return true;
    // pvm failed: either the page really is unreadable, or this device blocks
    // process_vm_readv entirely. Confirm through /proc/self/maps instead of
    // blindly rejecting (a blind reject silently disabled every guarded hook).
    return ToolsAddrReadable(addr);
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
