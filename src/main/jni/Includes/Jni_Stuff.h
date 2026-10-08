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

    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }

    if (bValid) {
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
        return "OK";
    }

    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    if (!activityThreadClass) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (attachedHere) jvm->DetachCurrentThread();
        return "ActivityThread unavailable";
    }

    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
    if (!sCurrentActivityThreadField) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteLocalRef(activityThreadClass);
        if (attachedHere) jvm->DetachCurrentThread();
        return "ActivityThread field unavailable";
    }

    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
    if (!sCurrentActivityThread) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteLocalRef(activityThreadClass);
        if (attachedHere) jvm->DetachCurrentThread();
        return "ActivityThread instance unavailable";
    }

    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    if (!mInitialApplicationField) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteLocalRef(sCurrentActivityThread);
        env->DeleteLocalRef(activityThreadClass);
        if (attachedHere) jvm->DetachCurrentThread();
        return "Application field unavailable";
    }

    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
    env->DeleteLocalRef(sCurrentActivityThread);
    env->DeleteLocalRef(activityThreadClass);

    if (!mInitialApplication) {
        if (attachedHere) jvm->DetachCurrentThread();
        return "Application instance unavailable";
    }

    std::string hwid = user_key ? user_key : "";
    hwid += GetAndroidID(env, mInitialApplication);
    hwid += GetDeviceModel(env);
    hwid += GetDeviceBrand(env);
    std::string UUID = GetDeviceUniqueIdentifier(env, hwid.c_str());
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
    std::string errMsg;
    usedKey = user_key ? user_key : "";

    if (isVipKey(usedKey)) {
        userType = "PREMIUM PAID";
    } else {
        userType = "PREMIUM PAID";
    }

    struct MemoryStruct chunk{};
    chunk.memory = (char *) calloc(1, 1);
    chunk.size = 0;

    CURL *curl = nullptr;
    CURLcode res = CURLE_OK;
    curl = curl_easy_init();

    if (curl) {
        std::string api_url = oxorany("https://xlreyt.x10.mx/connect");
        curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "Accept: application/json");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Linux; Android 13) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Mobile Safari/537.36");
        char data[4096];
        snprintf(data, sizeof(data), "game=CODMGR&user_key=%s&serial=%s", user_key ? user_key : "", UUID.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 12L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 6L);
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            if (httpCode != 200) {
                errMsg = "Server error: HTTP " + std::to_string(httpCode);
            } else {
                try {
                    json result = json::parse(chunk.memory);
                    if (result.contains("status") && result["status"] == true) {
                        if (result.contains("data") && result["data"].contains("token") && result["data"].contains("rng") && result["data"].contains("EXP")) {
                            std::string token = result["data"]["token"].get<std::string>();
                            time_t rng = result["data"]["rng"].get<time_t>();
                            EXP = result["data"]["EXP"].get<std::string>();
                            expiryTimestamp = parseExpiryDate(EXP);

                            if (rng + 30 > time(0)) {
                                std::string auth = "CODMGR";
                                auth += "-";
                                auth += user_key ? user_key : "";
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
                            errMsg = "Invalid server response";
                        }
                    } else if (result.contains("reason")) {
                        errMsg = result["reason"].get<std::string>();
                    } else {
                        errMsg = "Invalid server response";
                    }
                } catch (std::exception &e) {
                    errMsg = e.what();
                }
            }
        } else {
            errMsg = curl_easy_strerror(res);
        }
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    free(chunk.memory);
    chunk.memory = nullptr;
    env->DeleteLocalRef(mInitialApplication);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
    if (attachedHere) {
        jvm->DetachCurrentThread();
    }
    return bValid ? "OK" : errMsg;
}
