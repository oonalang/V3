#pragma once

#include "../AstralPrtctn/json.hpp"
#include <jni.h>
#include <curl/curl.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/md5.h>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <android/log.h>

using json = nlohmann::json;
extern JavaVM* jvm;
std::string g_Token, g_Auth;
bool bValid = false;
std::string EXP = " ";
std::string usedKey = " ";
std::string mod_status = " ";
std::string max_dev = " ";
std::string userType = "PREMIUM PAID";
int keyUserCount = 0;
time_t expiryTimestamp = 0;

time_t parseExpiryDate(const std::string& dateStr) {
    struct tm tm = {};
    std::istringstream ss(dateStr);
    ss >> std::get_time(&tm, "%Y-%m-%d");

    tm.tm_hour = 23;  
    tm.tm_min = 59;  
    tm.tm_sec = 59;  
      
    return mktime(&tm);
}

std::string getExpiryCountdown() {
    if (expiryTimestamp == 0) {
        return "Unknown";
    }

    time_t now = time(nullptr);
    if (now > expiryTimestamp) {
        return "EXPIRED";
    }

    time_t diff = expiryTimestamp - now;
    int days = diff / (24 * 3600);
    diff = diff % (24 * 3600);
    int hours = diff / 3600;
    diff = diff % 3600;
    int minutes = diff / 60;
    int seconds = diff % 60;

    std::stringstream ss;
    ss << days << "d " << hours << "h " << minutes << "m " << seconds << "s";
    return ss.str();
}
JNIEnv* AttachCurrentThread3(JavaVM* vm) {
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        vm->AttachCurrentThread(&env, nullptr);
    }
    return env;
}

const char *GetAndroidID(JNIEnv *env, jobject context) {
    jclass contextClass = env->FindClass("android/content/Context");
    jmethodID getContentResolverMethod = env->GetMethodID(contextClass, "getContentResolver", "()Landroid/content/ContentResolver;");
    jclass settingSecureClass = env->FindClass("android/provider/Settings$Secure");
    jmethodID getStringMethod = env->GetStaticMethodID(settingSecureClass, "getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;");

    auto obj = env->CallObjectMethod(context, getContentResolverMethod);
    auto str = (jstring) env->CallStaticObjectMethod(settingSecureClass, getStringMethod, obj, env->NewStringUTF("android_id"));
    return env->GetStringUTFChars(str, 0);
}

bool isVipKey(const std::string& key) {
    return key.substr(0, 4) == "VIP-";
}

const char *GetDeviceModel(JNIEnv *env) {
    jclass buildClass = env->FindClass("android/os/Build");
    jfieldID modelId = env->GetStaticFieldID(buildClass, "MODEL", "Ljava/lang/String;");

    auto str = (jstring) env->GetStaticObjectField(buildClass, modelId);
    return env->GetStringUTFChars(str, 0);
}

const char *GetDeviceBrand(JNIEnv *env) {
    jclass buildClass = env->FindClass("android/os/Build");
    jfieldID modelId = env->GetStaticFieldID(buildClass, "BRAND", "Ljava/lang/String;");

    auto str = (jstring) env->GetStaticObjectField(buildClass, modelId);
    return env->GetStringUTFChars(str, 0);
}

const char *GetDeviceUniqueIdentifier(JNIEnv *env, const char *uuid) {
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

std::string CalcMD5(std::string s) {
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

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) userp;
    
    mem->memory = (char *) realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        return 0;
    }
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

// The licence host answers refusals (403) with an HTML block page. Its text is
// the only thing that says *why* the request was refused, so the login label
// shows a short hint taken from it instead of a bare status code.
inline std::string ExtractResponseHint(const std::string &body)
{
    std::string text;
    bool inTag = false;
    for (char c : body) {
        if (c == '<') { inTag = true; continue; }
        if (c == '>') { inTag = false; text += ' '; continue; }
        if (inTag) continue;
        text += c;
    }

    std::string out;
    bool lastWasSpace = true;
    for (char c : text) {
        const bool isSpace = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
        if (isSpace) {
            if (!lastWasSpace) out += ' ';
        } else {
            out += c;
        }
        lastWasSpace = isSpace;
    }
    while (!out.empty() && out.front() == ' ') out.erase(out.begin());
    while (!out.empty() && out.back() == ' ') out.pop_back();
    if (out.size() > 90) out = out.substr(0, 90) + "...";
    return out;
}

struct LicenceHttpResult {
    long status;
    CURLcode code;
    std::string body;
};

// One attempt at the licence POST. `primeSession` fetches the site landing page
// first so the POST carries the ci_session cookie the host's anti-bot rule
// expects from a browser, and `forceHttp11` keeps the request on the protocol
// script clients normally get accepted on.
inline LicenceHttpResult PostLicenceAttempt(const std::string &url,
                                            const std::string &referer,
                                            const std::string &postFields,
                                            const char *userAgent,
                                            bool primeSession,
                                            bool forceHttp11)
{
    LicenceHttpResult result;
    result.status = 0;
    result.code = CURLE_OK;

    struct MemoryStruct chunk{};
    chunk.memory = (char *) calloc(1, 1);
    chunk.size = 0;

    CURL *curl = curl_easy_init();
    if (curl == nullptr) {
        free(chunk.memory);
        result.code = CURLE_FAILED_INIT;
        return result;
    }

    std::string origin = referer;
    while (!origin.empty() && origin.back() == '/') origin.pop_back();

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    headers = curl_slist_append(headers, "Accept: application/json, text/plain, */*");
    headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");
    headers = curl_slist_append(headers, "X-Requested-With: XMLHttpRequest");
    headers = curl_slist_append(headers, (std::string("Referer: ") + referer).c_str());
    headers = curl_slist_append(headers, (std::string("Origin: ") + origin).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);
    if (forceHttp11)
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long) CURL_HTTP_VERSION_1_1);
    if (primeSession)
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");   // turn on libcurl's cookie engine

    if (primeSession) {
        curl_easy_setopt(curl, CURLOPT_URL, referer.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        const CURLcode warmup = curl_easy_perform(curl);
        chunk.size = 0;
        if (chunk.memory != nullptr)
            chunk.memory[0] = '\0';
        if (warmup != CURLE_OK) {
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            result.code = warmup;
            return result;
        }
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 0L);
    }

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
    result.code = curl_easy_perform(curl);
    if (result.code == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.status);
    result.body.assign(chunk.memory != nullptr ? chunk.memory : "");

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(chunk.memory);
    return result;
}

int ShowSoftKeyboardInput() {
    jint result;
    jint flags = 0;
    
    JNIEnv *env;
    jvm->AttachCurrentThread(&env, NULL);
    
    jclass looperClass = env->FindClass("android/os/Looper");
    auto prepareMethod = env->GetStaticMethodID(looperClass, "prepare", "()V");
    env->CallStaticVoidMethod(looperClass, prepareMethod);
    
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    
    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    
    jclass contextClass = env->FindClass("android/content/Context");
    jfieldID fieldINPUT_METHOD_SERVICE = env->GetStaticFieldID(contextClass, "INPUT_METHOD_SERVICE", "Ljava/lang/String;");
    jobject INPUT_METHOD_SERVICE = env->GetStaticObjectField(contextClass, fieldINPUT_METHOD_SERVICE);
    jmethodID getSystemServiceMethod = env->GetMethodID(contextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject callObjectMethod = env->CallObjectMethod(mInitialApplication, getSystemServiceMethod, INPUT_METHOD_SERVICE);
    
    jclass classInputMethodManager = env->FindClass("android/view/inputmethod/InputMethodManager");
    jmethodID toggleSoftInputId = env->GetMethodID(classInputMethodManager, "toggleSoftInput", "(II)V");
    
    if (result) {
        env->CallVoidMethod(callObjectMethod, toggleSoftInputId, 2, flags);
    } else {
        env->CallVoidMethod(callObjectMethod, toggleSoftInputId, flags, flags);
    }
    
    env->DeleteLocalRef(classInputMethodManager);
    env->DeleteLocalRef(callObjectMethod);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(activityThreadClass);
    jvm->DetachCurrentThread();
    
    return result;
}

int PollUnicodeChars() {
    JNIEnv *env;
    jvm->AttachCurrentThread(&env, NULL);
    
    jclass looperClass = env->FindClass("android/os/Looper");
    auto prepareMethod = env->GetStaticMethodID(looperClass, "prepare", "()V");
    env->CallStaticVoidMethod(looperClass, prepareMethod);
    
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    
    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    
    jclass keyEventClass = env->FindClass("android/view/KeyEvent");
    jmethodID getUnicodeCharMethod = env->GetMethodID(keyEventClass, "getUnicodeChar", "(I)I");
    
    ImGuiIO& io = ImGui::GetIO();
    
    int return_key = env->CallIntMethod(keyEventClass, getUnicodeCharMethod);
    
    env->DeleteLocalRef(keyEventClass);
    env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(activityThreadClass);
    jvm->DetachCurrentThread();
    
    return return_key;
}

std::string getClipboard() {
    std::string result;
    JNIEnv *env;
    
    jvm->AttachCurrentThread(&env, NULL);
    
    auto looperClass = env->FindClass("android/os/Looper");
    auto prepareMethod = env->GetStaticMethodID(looperClass, "prepare", "()V");
    env->CallStaticVoidMethod(looperClass, prepareMethod);
    
    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    
    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    
    auto contextClass = env->FindClass("android/content/Context");
    auto getSystemServiceMethod = env->GetMethodID(contextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
    
    auto str = env->NewStringUTF("clipboard");
    auto clipboardManager = env->CallObjectMethod(mInitialApplication, getSystemServiceMethod, str);
    env->DeleteLocalRef(str);
    
    jclass ClipboardManagerClass = env->FindClass("android/content/ClipboardManager");
    auto getText = env->GetMethodID(ClipboardManagerClass, "getText", "()Ljava/lang/CharSequence;");

    jclass CharSequenceClass = env->FindClass("java/lang/CharSequence");
    auto toStringMethod = env->GetMethodID(CharSequenceClass, "toString", "()Ljava/lang/String;");

    auto text = env->CallObjectMethod(clipboardManager, getText);
    if (text) {
        str = (jstring) env->CallObjectMethod(text, toStringMethod);
        result = env->GetStringUTFChars(str, 0);
        env->DeleteLocalRef(str);
        env->DeleteLocalRef(text);
    }
    env->DeleteLocalRef(CharSequenceClass);
    env->DeleteLocalRef(ClipboardManagerClass);
    env->DeleteLocalRef(clipboardManager);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(activityThreadClass);
    jvm->DetachCurrentThread();
    return result.c_str();
}

std::string Login(const char *user_key) {
    // Fast path: if we already authenticated successfully this session, don't block
    // the UI on another full network round-trip just to draw the menu open.
    if (bValid) {
        ApplyForbidKickOffOnLogin();
        return "OK";
    }
    if (!jvm) {
        return "JavaVM unavailable";
    }
    JNIEnv *env = nullptr;
    bool attachedHere = false;
    if (jvm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (jvm->AttachCurrentThread(&env, 0) == JNI_OK) {
            attachedHere = true;
        }
    }
    if (!env) {
        return "JNI environment unavailable";
    }
    
    auto looperClass = env->FindClass("android/os/Looper");
    if (looperClass) {
        auto prepareMethod = env->GetStaticMethodID(looperClass, "prepare", "()V");
        if (prepareMethod) {
            env->CallStaticVoidMethod(looperClass, prepareMethod);
        }
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
        env->DeleteLocalRef(looperClass);
    }
    
    // Declared up front: every goto login_cleanup below is reached before the
    // original declarations, and C++ forbids a goto jumping over non-trivial
    // initialization (clang rejects the jump even for the JNI pointer locals).
    jclass activityThreadClass = nullptr;
    jfieldID sCurrentActivityThreadField = nullptr;
    jobject sCurrentActivityThread = nullptr;
    jfieldID mInitialApplicationField = nullptr;
    jobject mInitialApplication = nullptr;
    std::string hwid;
    std::string UUID;
    std::string errMsg;
    activityThreadClass = env->FindClass("android/app/ActivityThread");
    if (!activityThreadClass) {
        errMsg = "ActivityThread class not found";
        goto login_cleanup;
    }
    sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    if (!sCurrentActivityThreadField) {
        errMsg = "sCurrentActivityThread field not found";
        goto login_cleanup;
    }
    sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    if (!sCurrentActivityThread) {
        errMsg = "sCurrentActivityThread is null";
        goto login_cleanup;
    }
    
    mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    if (!mInitialApplicationField) {
        errMsg = "mInitialApplication field not found";
        goto login_cleanup;
    }
    mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    if (!mInitialApplication) {
        errMsg = "mInitialApplication is null";
        goto login_cleanup;
    }
    
    hwid = user_key;
    hwid += GetAndroidID(env, mInitialApplication);
    hwid += GetDeviceModel(env);
    hwid += GetDeviceBrand(env);
    UUID = GetDeviceUniqueIdentifier(env, hwid.c_str());
    
    {
        // Scoped so the goto cleanup above does not jump into these initializations.
        std::string api_url = oxorany("https://xlreyt.x10.mx/connect");
        std::string site_root = "https://xlreyt.x10.mx/";
        std::string postFields = "game=CODMGR&user_key=" + std::string(user_key) + "&serial=" + UUID;

        // Attempt 1: the plain browser-style POST.
        LicenceHttpResult attempt = PostLicenceAttempt(
                api_url, site_root, postFields,
                "Mozilla/5.0 (Linux; Android 13) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Mobile Safari/537.36",
                false, false);

        // The host refuses requests it does not trust with a 403 block page (no
        // user agent, script user agents, or a POST that arrives without a site
        // session). When that happens, go through the website itself: pull the
        // landing page for its ci_session cookie, then POST over HTTP/1.1 with a
        // desktop user agent.
        if (attempt.code != CURLE_OK || attempt.status != 200) {
            LicenceHttpResult retry = PostLicenceAttempt(
                    api_url, site_root, postFields,
                    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
                    true, true);
            __android_log_print(ANDROID_LOG_INFO, "AstralLogin",
                                "licence POST first=%ld/%d retry=%ld/%d",
                                attempt.status, (int) attempt.code,
                                retry.status, (int) retry.code);
            attempt = retry;
        }

        if (attempt.code != CURLE_OK) {
            errMsg = curl_easy_strerror(attempt.code);
        } else if (attempt.status != 200) {
            errMsg = "Server error: HTTP " + std::to_string(attempt.status);
            const std::string hint = ExtractResponseHint(attempt.body);
            if (!hint.empty())
                errMsg += " - " + hint;
        } else {
            try {
                json result = json::parse(attempt.body);
                if (result["status"] == true) {
                    std::string token = result["data"]["token"].get<std::string>();
                    time_t rng = result["data"]["rng"].get<time_t>();
                    EXP = result["data"]["EXP"].get<std::string>();
                    expiryTimestamp = parseExpiryDate(EXP);
                    if (rng + 30 > time(0)) {
                        std::string auth = "CODMGR";
                        auth += "-";
                        auth += user_key;
                        auth += "-";
                        auth += UUID;
                        auth += "-";
                        auth += "Vm8Lk7Uj2JmsjCPVPVjrLa7zgfx3uz9E";
                        std::string outputAuth = CalcMD5(auth);
                        g_Token = token;
                        g_Auth = outputAuth;
                        bValid = g_Token == g_Auth;
                    }
                } else {
                    errMsg = result["reason"].get<std::string>();
                }
            } catch (std::exception &e) {
                errMsg = e.what();
            }
        }
    }
    
    
login_cleanup:
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
    if (attachedHere) {
        jvm->DetachCurrentThread();
    }
    if (bValid) {
        return "OK";
    }
    // Never return an empty string: the UI treats "" as "still in flight", so an
    // empty result left the login window spinning on "Authorizing..." forever.
    // This happens when HTTP/parse succeeded but the token check failed (or curl
    // never initialized) without setting errMsg.
    if (errMsg.empty()) {
        errMsg = "Login failed: could not verify your license key";
    }
    return errMsg;
}
