#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <jni.h>
#include "Includes/Logger.h"
#include "Includes/Jni_Stuff.h"

extern JavaVM* VM;
extern char logintext[4096];

std::string GetFilesDirPath(JavaVM* vm) {
    if (!vm) {
        LOGE("Failed to get JavaVM");
        return "";
    }
    JNIEnv* env = AttachCurrentThread3(vm);
    if (!env) {
        LOGE("Failed to get JNIEnv");
        return "";
    }

    jclass activityThreadClass = env->FindClass("android/app/ActivityThread");
    if (!activityThreadClass) {
        LOGE("Failed to find android/app/ActivityThread");
        return "";
    }

    jobject application = nullptr;
    jmethodID currentApplicationMethod = env->GetStaticMethodID(
        activityThreadClass, "currentApplication", "()Landroid/app/Application;");
    if (currentApplicationMethod) {
        application = env->CallStaticObjectMethod(activityThreadClass, currentApplicationMethod);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            application = nullptr;
        }
    }

    jobject activityThread = nullptr;
    if (!application) {
        jmethodID currentActivityThreadMethod = env->GetStaticMethodID(
            activityThreadClass, "currentActivityThread", "()Landroid/app/ActivityThread;");
        if (currentActivityThreadMethod) {
            activityThread = env->CallStaticObjectMethod(activityThreadClass, currentActivityThreadMethod);
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                activityThread = nullptr;
            }
        }
        if (!activityThread) {
            LOGE("Failed to get current ActivityThread");
            return "";
        }

        jmethodID getApplicationMethod = env->GetMethodID(
            activityThreadClass, "getApplication", "()Landroid/app/Application;");
        application = env->CallObjectMethod(activityThread, getApplicationMethod);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            application = nullptr;
        }
    }

    if (!application) {
        LOGE("Failed to get Application instance");
        return "";
    }

    jclass contextClass = env->GetObjectClass(application);
    jmethodID getFilesDirMethod = env->GetMethodID(contextClass, "getFilesDir", "()Ljava/io/File;");
    jobject filesDir = env->CallObjectMethod(application, getFilesDirMethod);
    if (!filesDir) {
        LOGE("Failed to get filesDir");
        return "";
    }

    jclass fileClass = env->GetObjectClass(filesDir);
    jmethodID getAbsolutePathMethod = env->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
    jstring absolutePath = static_cast<jstring>(env->CallObjectMethod(filesDir, getAbsolutePathMethod));
    if (!absolutePath) {
        LOGE("Failed to get absolute path");
        return "";
    }

    const char* pathChars = env->GetStringUTFChars(absolutePath, nullptr);
    std::string path(pathChars);
    env->ReleaseStringUTFChars(absolutePath, pathChars);

    if (activityThread) env->DeleteLocalRef(activityThread);
    env->DeleteLocalRef(application);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(filesDir);
    env->DeleteLocalRef(fileClass);
    env->DeleteLocalRef(absolutePath);

    return path;
}

bool SaveLoginTextToFile(const char* logintext) {
    if (!logintext) {
        return false;
    }
    std::string filesDir = GetFilesDirPath(VM);
    if (filesDir.empty()) {
        return false;
    }

    std::string textfile = filesDir + "/codm.ini";
    std::ofstream file(textfile.c_str(), std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file << logintext;
    file.close();
    return true;
}

bool LoadTextFromFile() {
    std::string filesDir = GetFilesDirPath(VM);
    if (filesDir.empty()) {
        return false;
    }

    std::string textfile = filesDir + "/codm.ini";
    std::ifstream file(textfile.c_str()); 
    std::stringstream buffer;
    
    if (file.is_open()) {
        buffer << file.rdbuf();
        file.close();
        std::string content = buffer.str();
        if (content.empty()) {
            logintext[0] = '\0';
        } else {
            strncpy(logintext, content.c_str(), sizeof(logintext));
            logintext[sizeof(logintext) - 1] = '\0';
        }
        return true;
    }

    return false;
}

#endif
