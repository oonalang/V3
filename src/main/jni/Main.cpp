#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "ImGui/Call_ImGui.h"
#include "IL2CppSDKGenerator/BasicStructs/Call_BasicStructs.h"
#include "IL2CppSDKGenerator/IL2Cpp/Call_IL2Cpp.h"
#include "Hacks/Hacks.h"
#include "IL2CppSDKGenerator/KittyMemory/MemoryPatch.h"
#include "foxcheats/include/ScanEngine.hpp"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string>
#include <functional>
#include <cstring>
#include <cfloat>
#include <mutex>
#include <thread>
#include <cmath>
#include <jni.h>
#include <pthread.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "oxorany/source/oxorany.h"
#include "oxorany/source/oxorany.cpp"
#include "oxorany/source/oxorany_include.h"

#include "MainFeatureIncludes.h"

#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_activity.h>
#include <android/native_window_jni.h>
#include <dlfcn.h>
#include <sys/system_properties.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace android::anwcreator::detail::types
{
    enum class SurfaceControlFlags : uint32_t
    {
        eHidden          = 0x00000004,
        eSkipScreenshot  = 0x00000040,
        eSecure          = 0x00000080,
        eNonPremultiplied= 0x00000100,
        eOpaque          = 0x00000400,
        eNoColorFill     = 0x00004000,
    };

    enum class DisplayRotation : int32_t
    {
        Rotation0   = 0,
        Rotation90  = 1,
        Rotation180 = 2,
        Rotation270 = 3
    };

    template <typename enum_t, typename = std::enable_if_t<std::is_enum_v<enum_t>>>
    constexpr enum_t operator|(enum_t lhs, enum_t rhs)
    {
        using underlying_t = std::underlying_type_t<enum_t>;
        return static_cast<enum_t>(static_cast<underlying_t>(lhs) | static_cast<underlying_t>(rhs));
    }

    template <typename enum_t, typename = std::enable_if_t<std::is_enum_v<enum_t>>>
    constexpr enum_t operator|=(enum_t &lhs, enum_t rhs)
    {
        return (lhs = lhs | rhs);
    }
}

namespace android::anwcreator::detail::jni
{
    struct JNIEnvironment
    {
        JNIEnv *env;
        JavaVM *vm;

        JNIEnvironment(JavaVM *javaVM) : env(nullptr), vm(javaVM)
        {
            if (vm)
                vm->AttachCurrentThread(&env, nullptr);
        }

        ~JNIEnvironment() {}

        inline bool IsValid() const { return env != nullptr; }
        inline operator JNIEnv *() const { return env; }
        inline JNIEnv *operator->() const { return env; }

        inline bool CheckException(const char * /*context*/ = nullptr)
        {
            if (!env || !env->ExceptionCheck())
                return false;
            env->ExceptionDescribe();
            env->ExceptionClear();
            return true;
        }
    };

    struct LocalRef
    {
        JNIEnv *env;
        jobject obj;

        LocalRef(JNIEnv *e, jobject o) : env(e), obj(o) {}
        ~LocalRef() { if (env && obj) env->DeleteLocalRef(obj); }
        LocalRef(const LocalRef &) = delete;
        LocalRef &operator=(const LocalRef &) = delete;
        LocalRef(LocalRef &&other) noexcept : env(other.env), obj(other.obj) { other.obj = nullptr; }

        inline operator jobject() const { return obj; }
        inline operator jclass() const { return reinterpret_cast<jclass>(obj); }
        inline jobject get() const { return obj; }
        inline jclass getClass() const { return reinterpret_cast<jclass>(obj); }
        inline bool IsValid() const { return obj != nullptr; }
        inline explicit operator bool() const { return obj != nullptr; }
    };

    struct GlobalRef
    {
        JNIEnv *env;
        jobject obj;

        GlobalRef(JNIEnv *e, jobject o) : env(e), obj(nullptr)
        {
            if (e && o) obj = e->NewGlobalRef(o);
        }

        ~GlobalRef() { Release(); }

        void Release()
        {
            if (env && obj) {
                env->DeleteGlobalRef(obj);
                obj = nullptr;
            }
        }

        GlobalRef(const GlobalRef &) = delete;
        GlobalRef &operator=(const GlobalRef &) = delete;
        GlobalRef(GlobalRef &&other) noexcept : env(other.env), obj(other.obj) { other.obj = nullptr; }

        inline operator jobject() const { return obj; }
        inline jobject get() const { return obj; }
        inline bool IsValid() const { return obj != nullptr; }
        inline explicit operator bool() const { return obj != nullptr; }
    };
}

namespace android::anwcreator::detail::framework
{
    struct DisplayMetrics
    {
        int32_t widthPixels;
        int32_t heightPixels;
        float   density;
        int32_t densityDpi;
    };

    struct DisplayInfo
    {
        int32_t                width;
        int32_t                height;
        types::DisplayRotation rotation;
        float                  refreshRate;
    };

    class Display
    {
    public:
        static DisplayInfo GetDisplayInfo(JNIEnv *env, ANativeActivity *activity)
        {
            DisplayInfo info{};
            if (!env || !activity || !activity->clazz) return info;
            jni::LocalRef activityCls(env, env->GetObjectClass(activity->clazz));
            if (!activityCls) return info;
            jmethodID getWindowManager = env->GetMethodID(activityCls, "getWindowManager", "()Landroid/view/WindowManager;");
            if (!getWindowManager) return info;
            jni::LocalRef windowManager(env, env->CallObjectMethod(activity->clazz, getWindowManager));
            if (!windowManager || jni::JNIEnvironment(activity->vm).CheckException("Activity.getWindowManager()")) return info;
            jni::LocalRef windowManagerCls(env, env->GetObjectClass(windowManager));
            jmethodID getDefaultDisplay = env->GetMethodID(windowManagerCls, "getDefaultDisplay", "()Landroid/view/Display;");
            if (!getDefaultDisplay) return info;
            jni::LocalRef display(env, env->CallObjectMethod(windowManager, getDefaultDisplay));
            if (!display || jni::JNIEnvironment(activity->vm).CheckException("WindowManager.getDefaultDisplay()")) return info;
            jni::LocalRef displayMetricsCls(env, env->FindClass("android/util/DisplayMetrics"));
            jmethodID displayMetricsCtor = env->GetMethodID(displayMetricsCls, "<init>", "()V");
            jni::LocalRef displayMetrics(env, env->NewObject(displayMetricsCls, displayMetricsCtor));
            jni::LocalRef displayCls(env, env->GetObjectClass(display));
            jmethodID getRealMetrics = env->GetMethodID(displayCls, "getRealMetrics", "(Landroid/util/DisplayMetrics;)V");
            env->CallVoidMethod(display, getRealMetrics, displayMetrics.get());
            jfieldID widthPixelsField  = env->GetFieldID(displayMetricsCls, "widthPixels", "I");
            jfieldID heightPixelsField = env->GetFieldID(displayMetricsCls, "heightPixels", "I");
            info.width  = env->GetIntField(displayMetrics, widthPixelsField);
            info.height = env->GetIntField(displayMetrics, heightPixelsField);
            jmethodID getRotation = env->GetMethodID(displayCls, "getRotation", "()I");
            int32_t   rotation    = env->CallIntMethod(display, getRotation);
            info.rotation         = static_cast<types::DisplayRotation>(rotation);
            jmethodID getRefreshRate = env->GetMethodID(displayCls, "getRefreshRate", "()F");
            if (getRefreshRate) info.refreshRate = env->CallFloatMethod(display, getRefreshRate);
            return info;
        }
    };

    class SurfaceControl
    {
    public:
        struct Builder
        {
            JNIEnv *env;
            jobject builder;
            jclass  builderClass;

            Builder(JNIEnv *e) : env(e), builder(nullptr), builderClass(nullptr)
            {
                builderClass = env->FindClass("android/view/SurfaceControl$Builder");
                if (!builderClass) return;
                jmethodID ctor = env->GetMethodID(builderClass, "<init>", "()V");
                builder        = env->NewObject(builderClass, ctor);
            }

            ~Builder()
            {
                if (env && builder) env->DeleteLocalRef(builder);
                if (env && builderClass) env->DeleteLocalRef(builderClass);
            }

            Builder &SetName(const char *name)
            {
                if (!builder) return *this;
                jmethodID setName = env->GetMethodID(builderClass, "setName", "(Ljava/lang/String;)Landroid/view/SurfaceControl$Builder;");
                if (setName) {
                    jni::LocalRef jName(env, env->NewStringUTF(name));
                    builder = env->CallObjectMethod(builder, setName, jName.get());
                }
                return *this;
            }

            Builder &SetParent(jobject parent)
            {
                if (!builder || !parent) return *this;
                jmethodID setParent = env->GetMethodID(builderClass, "setParent", "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Builder;");
                if (setParent) builder = env->CallObjectMethod(builder, setParent, parent);
                return *this;
            }

            Builder &SetBufferSize(int32_t width, int32_t height)
            {
                if (!builder) return *this;
                jmethodID setBufferSize = env->GetMethodID(builderClass, "setBufferSize", "(II)Landroid/view/SurfaceControl$Builder;");
                if (setBufferSize) builder = env->CallObjectMethod(builder, setBufferSize, width, height);
                return *this;
            }

            Builder &SetFormat(int32_t format)
            {
                if (!builder) return *this;
                jmethodID setFormat = env->GetMethodID(builderClass, "setFormat", "(I)Landroid/view/SurfaceControl$Builder;");
                if (setFormat) builder = env->CallObjectMethod(builder, setFormat, (jint)format);
                return *this;
            }

            Builder &SetOpaque(bool opaque)
            {
                if (!builder) return *this;
                jmethodID setOpaque = env->GetMethodID(builderClass, "setOpaque", "(Z)Landroid/view/SurfaceControl$Builder;");
                if (setOpaque) builder = env->CallObjectMethod(builder, setOpaque, (jboolean)opaque);
                return *this;
            }

            Builder &SetFlags(uint32_t flags, uint32_t mask)
            {
                if (!builder) return *this;
                jmethodID setFlags = env->GetMethodID(builderClass, "setFlags", "(II)Landroid/view/SurfaceControl$Builder;");
                if (setFlags) builder = env->CallObjectMethod(builder, setFlags, static_cast<jint>(flags), static_cast<jint>(mask));
                return *this;
            }

            Builder &SetSkipScreenshot(bool skip)
            {
                if (skip) {
                    constexpr uint32_t FLAG_SKIP_SCREENSHOT = 0x40;
                    SetFlags(FLAG_SKIP_SCREENSHOT, FLAG_SKIP_SCREENSHOT);
                }
                return *this;
            }

            jobject Build()
            {
                if (!builder) return nullptr;
                jmethodID buildMethod = env->GetMethodID(builderClass, "build", "()Landroid/view/SurfaceControl;");
                if (!buildMethod) return nullptr;
                return env->CallObjectMethod(builder, buildMethod);
            }

            inline bool IsValid() const { return builder != nullptr; }
        };

        class Transaction
        {
        public:
            JNIEnv *env;
            jobject transaction;
            jclass  transactionClass;

            Transaction(JNIEnv *e) : env(e), transaction(nullptr), transactionClass(nullptr)
            {
                transactionClass = env->FindClass("android/view/SurfaceControl$Transaction");
                if (!transactionClass) return;
                jmethodID ctor = env->GetMethodID(transactionClass, "<init>", "()V");
                transaction     = env->NewObject(transactionClass, ctor);
            }

            ~Transaction()
            {
                if (env && transaction) env->DeleteLocalRef(transaction);
                if (env && transactionClass) env->DeleteLocalRef(transactionClass);
            }

            Transaction &SetAlpha(jobject surfaceControl, float alpha)
            {
                if (!transaction || !surfaceControl) return *this;
                jmethodID setAlpha = env->GetMethodID(transactionClass, "setAlpha", "(Landroid/view/SurfaceControl;F)Landroid/view/SurfaceControl$Transaction;");
                if (setAlpha) transaction = env->CallObjectMethod(transaction, setAlpha, surfaceControl, alpha);
                return *this;
            }

            Transaction &SetLayer(jobject surfaceControl, int32_t z)
            {
                if (!transaction || !surfaceControl) return *this;
                jmethodID setLayer = env->GetMethodID(transactionClass, "setLayer", "(Landroid/view/SurfaceControl;I)Landroid/view/SurfaceControl$Transaction;");
                if (setLayer) transaction = env->CallObjectMethod(transaction, setLayer, surfaceControl, z);
                return *this;
            }

            Transaction &SetPosition(jobject surfaceControl, float x, float y)
            {
                if (!transaction || !surfaceControl) return *this;
                jmethodID m = env->GetMethodID(transactionClass,"setPosition","(Landroid/view/SurfaceControl;FF)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction,m,surfaceControl,x,y);
                return *this;
            }

            Transaction &SetMatrix(jobject surfaceControl, float dsdx, float dtdx, float dtdy, float dsdy)
            {
                if (!transaction || !surfaceControl) return *this;
                jmethodID m = env->GetMethodID(transactionClass,"setMatrix","(Landroid/view/SurfaceControl;FFFF)Landroid/view/SurfaceControl$Transaction;");
                if (m) transaction = env->CallObjectMethod(transaction,m,surfaceControl,dsdx,dtdx,dtdy,dsdy);
                return *this;
            }

            Transaction &Show(jobject surfaceControl)
            {
                if (!transaction || !surfaceControl) return *this;
                jmethodID show = env->GetMethodID(transactionClass, "show", "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (show) transaction = env->CallObjectMethod(transaction, show, surfaceControl);
                return *this;
            }

            Transaction &Hide(jobject surfaceControl)
            {
                if (!transaction || !surfaceControl) return *this;
                jmethodID hide = env->GetMethodID(transactionClass, "hide", "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (hide) transaction = env->CallObjectMethod(transaction, hide, surfaceControl);
                return *this;
            }

            Transaction &SetParent(jobject child, jobject newParent)
            {
                if (!transaction || !child || !newParent) return *this;
                jmethodID setParent = env->GetMethodID(transactionClass, "setParent", "(Landroid/view/SurfaceControl;Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (setParent) transaction = env->CallObjectMethod(transaction, setParent, child, newParent);
                return *this;
            }

            Transaction &Remove(jobject surfaceControl)
            {
                if (!transaction || !surfaceControl) return *this;
                jmethodID remove = env->GetMethodID(transactionClass, "remove", "(Landroid/view/SurfaceControl;)Landroid/view/SurfaceControl$Transaction;");
                if (remove) transaction = env->CallObjectMethod(transaction, remove, surfaceControl);
                return *this;
            }

            void Apply()
            {
                if (!transaction) return;
                jmethodID apply = env->GetMethodID(transactionClass, "apply", "()V");
                if (apply) env->CallVoidMethod(transaction, apply);
            }

            inline bool IsValid() const { return transaction != nullptr; }
        };

        static jobject GetParentSurfaceControl(JNIEnv *env, ANativeActivity *activity)
        {
            if (!env || !activity || !activity->clazz) return nullptr;
            jni::LocalRef activityCls(env, env->GetObjectClass(activity->clazz));
            jmethodID getWindow = env->GetMethodID(activityCls, "getWindow", "()Landroid/view/Window;");
            if (!getWindow) return nullptr;
            jni::LocalRef window(env, env->CallObjectMethod(activity->clazz, getWindow));
            if (!window) return nullptr;
            jni::LocalRef windowCls(env, env->GetObjectClass(window));
            jmethodID getDecorView = env->GetMethodID(windowCls, "getDecorView", "()Landroid/view/View;");
            if (!getDecorView) return nullptr;
            jni::LocalRef decorView(env, env->CallObjectMethod(window, getDecorView));
            if (!decorView) return nullptr;
            jni::LocalRef viewCls(env, env->GetObjectClass(decorView));
            jmethodID getViewRootImpl = env->GetMethodID(viewCls, "getViewRootImpl", "()Landroid/view/ViewRootImpl;");
            if (!getViewRootImpl) return nullptr;
            jni::LocalRef viewRootImpl(env, env->CallObjectMethod(decorView, getViewRootImpl));
            if (!viewRootImpl) return nullptr;
            jni::LocalRef viewRootImplCls(env, env->GetObjectClass(viewRootImpl));
            jmethodID getSurfaceControl = env->GetMethodID(viewRootImplCls, "getSurfaceControl", "()Landroid/view/SurfaceControl;");
            if (!getSurfaceControl) return nullptr;
            return env->CallObjectMethod(viewRootImpl, getSurfaceControl);
        }
    };

    class Surface
    {
    public:
        static jobject CreateFromSurfaceControl(JNIEnv *env, jobject surfaceControl)
        {
            if (!env || !surfaceControl) return nullptr;
            jni::LocalRef surfaceCls(env, env->FindClass("android/view/Surface"));
            if (!surfaceCls) return nullptr;
            jmethodID ctor = env->GetMethodID(surfaceCls, "<init>", "(Landroid/view/SurfaceControl;)V");
            if (!ctor) return nullptr;
            return env->NewObject(surfaceCls, ctor, surfaceControl);
        }
    };
}

namespace android::anwcreator::detail
{
    struct WindowContext
    {
        std::unique_ptr<jni::GlobalRef> surfaceControl;
        std::unique_ptr<jni::GlobalRef> surface;
        std::unique_ptr<jni::GlobalRef> parentSurfaceControl;
        ANativeWindow                  *nativeWindow;
        int32_t                         width;
        int32_t                         height;
        bool                            skipScreenshot;

        WindowContext() : surfaceControl(nullptr), surface(nullptr), parentSurfaceControl(nullptr), nativeWindow(nullptr), width(0), height(0), skipScreenshot(false) {}
        ~WindowContext() { Release(); }

        void Release()
        {
            if (nativeWindow) {
                ANativeWindow_release(nativeWindow);
                nativeWindow = nullptr;
            }
            surface.reset();
            surfaceControl.reset();
            parentSurfaceControl.reset();
        }
    };
}

namespace android
{
    class ANwCreator
    {
    public:
        struct DisplayInfo
        {
            int32_t theta; int32_t width; int32_t height; float refreshRate;
            DisplayInfo() : theta(0), width(0), height(0), refreshRate(60.0f) {}
        };

        struct CreateOptions
        {
            const char *name; int32_t width; int32_t height; bool skipScreenshot;
            CreateOptions() : name(""), width(-1), height(-1), skipScreenshot(false) {}
        };

    public:
        static DisplayInfo GetDisplayInfo(ANativeActivity *activity)
        {
            DisplayInfo result{};
            if (!activity || !activity->vm || !activity->clazz) return result;
            anwcreator::detail::jni::JNIEnvironment jniEnv(activity->vm);
            if (!jniEnv.IsValid()) return result;
            auto displayInfo = anwcreator::detail::framework::Display::GetDisplayInfo(jniEnv, activity);
            result.width       = displayInfo.width;
            result.height      = displayInfo.height;
            result.theta       = 90 * static_cast<int32_t>(displayInfo.rotation);
            result.refreshRate = displayInfo.refreshRate;
            return result;
        }

        static ANativeWindow *Create(ANativeActivity *activity, const CreateOptions &options = CreateOptions())
        {
            if (!activity || !activity->vm || !activity->clazz) return nullptr;
            anwcreator::detail::jni::JNIEnvironment jniEnv(activity->vm);
            if (!jniEnv.IsValid()) return nullptr;

            int32_t width  = options.width;
            int32_t height = options.height;
            if (width <= 0 || height <= 0) {
                auto displayInfo = anwcreator::detail::framework::Display::GetDisplayInfo(jniEnv, activity);
                width  = displayInfo.width;
                height = displayInfo.height;
            }

            jobject parentSC = anwcreator::detail::framework::SurfaceControl::GetParentSurfaceControl(jniEnv, activity);
            if (!parentSC && jniEnv.CheckException("GetParentSurfaceControl")) return nullptr;

            anwcreator::detail::framework::SurfaceControl::Builder builder(jniEnv);
            if (!builder.IsValid()) return nullptr;

            builder.SetName(options.name)
                .SetBufferSize(width, height)
                .SetFormat(1)
                .SetOpaque(false)
                .SetSkipScreenshot(options.skipScreenshot);

            if (parentSC) builder.SetParent(parentSC);

            jobject localSurfaceControl = builder.Build();
            if (!localSurfaceControl || jniEnv.CheckException("Builder.build()")) {
                if (parentSC) jniEnv->DeleteLocalRef(parentSC);
                return nullptr;
            }

            auto context = std::make_unique<anwcreator::detail::WindowContext>();
            context->width = width; context->height = height; context->skipScreenshot = options.skipScreenshot;
            context->surfaceControl = std::make_unique<anwcreator::detail::jni::GlobalRef>(jniEnv, localSurfaceControl);
            if (parentSC) context->parentSurfaceControl = std::make_unique<anwcreator::detail::jni::GlobalRef>(jniEnv, parentSC);

            {
                anwcreator::detail::framework::SurfaceControl::Transaction transaction(jniEnv);
                if (transaction.IsValid()) {
                    transaction.SetAlpha(context->surfaceControl->get(), 1.0f)
                        .SetLayer(context->surfaceControl->get(), 0x7FFFFFFE)
                        .SetPosition(context->surfaceControl->get(), 0.0f, 0.0f)
                        .SetMatrix(context->surfaceControl->get(), 1.0f, 0.0f, 0.0f, 1.0f)
                        .Show(context->surfaceControl->get())
                        .Apply();
                }
            }

            jobject localSurface = anwcreator::detail::framework::Surface::CreateFromSurfaceControl(jniEnv, context->surfaceControl->get());
            if (!localSurface || jniEnv.CheckException("Create Surface")) {
                jniEnv->DeleteLocalRef(localSurfaceControl);
                if (parentSC) jniEnv->DeleteLocalRef(parentSC);
                return nullptr;
            }

            context->surface = std::make_unique<anwcreator::detail::jni::GlobalRef>(jniEnv, localSurface);
            context->nativeWindow = ANativeWindow_fromSurface(jniEnv, context->surface->get());
            if (!context->nativeWindow) {
                jniEnv->DeleteLocalRef(localSurface);
                jniEnv->DeleteLocalRef(localSurfaceControl);
                if (parentSC) jniEnv->DeleteLocalRef(parentSC);
                return nullptr;
            }

            ANativeWindow *result = context->nativeWindow;
            m_windowContexts.emplace(result, std::move(context));
            jniEnv->DeleteLocalRef(localSurface);
            jniEnv->DeleteLocalRef(localSurfaceControl);
            if (parentSC) jniEnv->DeleteLocalRef(parentSC);
            return result;
        }

        static void Destroy(ANativeActivity *activity, ANativeWindow *nativeWindow)
        {
            if (!nativeWindow) return;
            auto it = m_windowContexts.find(nativeWindow);
            if (it == m_windowContexts.end()) { ANativeWindow_release(nativeWindow); return; }
            auto &context = it->second;
            if (activity && activity->vm && context->surfaceControl && context->surfaceControl->IsValid()) {
                anwcreator::detail::jni::JNIEnvironment jniEnv(activity->vm);
                if (jniEnv.IsValid()) {
                    anwcreator::detail::framework::SurfaceControl::Transaction transaction(jniEnv);
                    if (transaction.IsValid()) transaction.Remove(context->surfaceControl->get()).Apply();
                }
            }
            context->Release();
            m_windowContexts.erase(it);
        }

        static bool IsValid(ANativeWindow *nativeWindow) { return nativeWindow && m_windowContexts.count(nativeWindow) > 0; }

        static bool GetWindowSize(ANativeWindow *nativeWindow, int32_t *outWidth, int32_t *outHeight)
        {
            auto it = m_windowContexts.find(nativeWindow);
            if (it == m_windowContexts.end()) return false;
            if (outWidth) *outWidth = it->second->width;
            if (outHeight) *outHeight = it->second->height;
            return true;
        }

        static void EnsureVisible(ANativeActivity *activity, ANativeWindow *nativeWindow)
        {
            if (!activity || !activity->vm || !nativeWindow) return;
            auto it = m_windowContexts.find(nativeWindow);
            if (it == m_windowContexts.end()) return;
            auto &context = it->second;
            if (!context || !context->surfaceControl || !context->surfaceControl->IsValid()) return;
            anwcreator::detail::jni::JNIEnvironment jniEnv(activity->vm);
            if (!jniEnv.IsValid()) return;
            anwcreator::detail::framework::SurfaceControl::Transaction transaction(jniEnv);
            if (!transaction.IsValid()) return;

            jobject currentParent = anwcreator::detail::framework::SurfaceControl::GetParentSurfaceControl(jniEnv, activity);
            bool needReparent = false;
            if (currentParent && context->parentSurfaceControl && context->parentSurfaceControl->IsValid()) {
                if (!jniEnv->IsSameObject(currentParent, context->parentSurfaceControl->get())) needReparent = true;
            } else if (currentParent && !context->parentSurfaceControl) needReparent = true;

            if (needReparent && currentParent) {
                transaction.SetParent(context->surfaceControl->get(), currentParent);
                context->parentSurfaceControl = std::make_unique<anwcreator::detail::jni::GlobalRef>(jniEnv, currentParent);
            }

            float scaleX = 1.0f, scaleY = 1.0f;
            auto displayInfo = anwcreator::detail::framework::Display::GetDisplayInfo(jniEnv, activity);
            if (context->width > 0 && context->height > 0 && displayInfo.width > 0 && displayInfo.height > 0) {
                int32_t targetW = displayInfo.width;
                int32_t targetH = displayInfo.height;
                const bool bufferLandscape = context->width >= context->height;
                const bool targetLandscape = targetW >= targetH;
                if (bufferLandscape != targetLandscape) std::swap(targetW, targetH);
                scaleX = (float)targetW / (float)context->width;
                scaleY = (float)targetH / (float)context->height;
                if (!(scaleX > 0.0f) || !(scaleY > 0.0f) || scaleX > 4.0f || scaleY > 4.0f) scaleX = scaleY = 1.0f;
            }

            transaction
                .SetAlpha(context->surfaceControl->get(), 1.0f)
                .SetLayer(context->surfaceControl->get(), 0x7FFFFFFE)
                .SetPosition(context->surfaceControl->get(), 0.0f, 0.0f)
                .SetMatrix(context->surfaceControl->get(), scaleX, 0.0f, 0.0f, scaleY)
                .Show(context->surfaceControl->get())
                .Apply();

            if (currentParent) jniEnv->DeleteLocalRef(currentParent);
        }

    private:
        inline static std::unordered_map<ANativeWindow *, std::unique_ptr<anwcreator::detail::WindowContext>> m_windowContexts;
    };
}

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>
#include <android/native_activity.h>

namespace android {
class HideRecAImGui {
public:
    struct Options { ANativeActivity* activity=nullptr; bool skipScreenshot=true; };
    using SwapBuffersFn=EGLBoolean (*)(EGLDisplay,EGLSurface);
    HideRecAImGui():HideRecAImGui(Options{}){}
    explicit HideRecAImGui(const Options& options);
    ~HideRecAImGui();
    bool RenderDrawData(ImDrawData* drawData,EGLDisplay gameDisplay,EGLSurface gameSurface, EGLContext gameContext,SwapBuffersFn swapBuffersFn);
    void Shutdown();
    constexpr operator bool() const{return m_state;}
private:
    bool EnsureEnvironment(EGLDisplay gameDisplay,EGLContext gameContext,int32_t width,int32_t height);
    bool InitEnvironment(EGLDisplay gameDisplay,EGLContext gameContext,int32_t width,int32_t height);
    void UnInitEnvironment();
    EGLConfig FindConfig(EGLDisplay display,EGLContext gameContext);
private:
    bool m_state=false;
    int32_t m_screenWidth=-1,m_screenHeight=-1;
    Options m_options{};
    ANativeWindow* m_nativeWindow=nullptr;
    EGLDisplay m_display=EGL_NO_DISPLAY;
    EGLSurface m_surface=EGL_NO_SURFACE;
    EGLContext m_overlayContext=EGL_NO_CONTEXT;
    EGLContext m_gameContext=EGL_NO_CONTEXT;
};
}

#include <EGL/eglext.h>
#include <cmath>

namespace android {
HideRecAImGui::HideRecAImGui(const Options& options):m_options(options){}
HideRecAImGui::~HideRecAImGui(){UnInitEnvironment();}

EGLConfig HideRecAImGui::FindConfig(EGLDisplay display,EGLContext gameContext){
    (void)gameContext;
    const EGLint attrs[]={
        EGL_SURFACE_TYPE,EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE,0x00000040,
        EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,
        EGL_NONE
    };
    EGLConfig cfg=nullptr; EGLint count=0;
    if(eglChooseConfig(display,attrs,&cfg,1,&count)==EGL_TRUE&&count>0&&cfg)return cfg;
    EGLint total=0;
    if(eglGetConfigs(display,nullptr,0,&total)!=EGL_TRUE||total<=0)return nullptr;
    std::vector<EGLConfig> configs((size_t)total);
    if(eglGetConfigs(display,configs.data(),total,&total)!=EGL_TRUE)return nullptr;
    for(int i=0;i<total;i++){
        EGLint st=0,rt=0,r=0,g=0,b=0,a=0;
        eglGetConfigAttrib(display,configs[i],EGL_SURFACE_TYPE,&st);
        eglGetConfigAttrib(display,configs[i],EGL_RENDERABLE_TYPE,&rt);
        eglGetConfigAttrib(display,configs[i],EGL_RED_SIZE,&r);
        eglGetConfigAttrib(display,configs[i],EGL_GREEN_SIZE,&g);
        eglGetConfigAttrib(display,configs[i],EGL_BLUE_SIZE,&b);
        eglGetConfigAttrib(display,configs[i],EGL_ALPHA_SIZE,&a);
        if((st&EGL_WINDOW_BIT)&&(rt&0x00000040)&&r>=8&&g>=8&&b>=8&&a>=8)return configs[i];
    }
    return nullptr;
}

bool HideRecAImGui::InitEnvironment(EGLDisplay gameDisplay,EGLContext gameContext,int32_t width,int32_t height){
    if(!m_options.activity||gameDisplay==EGL_NO_DISPLAY||gameContext==EGL_NO_CONTEXT||width<=0||height<=0)return false;
    ANwCreator::CreateOptions opt;
    opt.name="HideRecSecure"; opt.width=width; opt.height=height; opt.skipScreenshot=m_options.skipScreenshot;
    m_nativeWindow=ANwCreator::Create(m_options.activity,opt);
    if(!m_nativeWindow)return false;
    m_display=gameDisplay; m_gameContext=gameContext;
    EGLConfig config=FindConfig(m_display,gameContext);
    if(!config){UnInitEnvironment();return false;}
    EGLint format=0;
    if(eglGetConfigAttrib(m_display,config,EGL_NATIVE_VISUAL_ID,&format)!=EGL_TRUE){UnInitEnvironment();return false;}
    ANativeWindow_setBuffersGeometry(m_nativeWindow,width,height,format);
    m_surface=eglCreateWindowSurface(m_display,config,m_nativeWindow,nullptr);
    if(m_surface==EGL_NO_SURFACE){UnInitEnvironment();return false;}
    const EGLint ctxAttrs[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    m_overlayContext=eglCreateContext(m_display,config,gameContext,ctxAttrs);
    if(m_overlayContext==EGL_NO_CONTEXT){UnInitEnvironment();return false;}
    EGLint realW=0,realH=0;
    if(eglQuerySurface(m_display,m_surface,EGL_WIDTH,&realW)==EGL_TRUE&&realW>0)m_screenWidth=realW;else m_screenWidth=width;
    if(eglQuerySurface(m_display,m_surface,EGL_HEIGHT,&realH)==EGL_TRUE&&realH>0)m_screenHeight=realH;else m_screenHeight=height;
    m_state=true;
    ANwCreator::EnsureVisible(m_options.activity,m_nativeWindow);
    return true;
}

bool HideRecAImGui::EnsureEnvironment(EGLDisplay gameDisplay,EGLContext gameContext,int32_t width,int32_t height){
    if(gameDisplay==EGL_NO_DISPLAY||gameContext==EGL_NO_CONTEXT||width<=0||height<=0)return false;
    if(m_state&&(m_display!=gameDisplay||m_gameContext!=gameContext||m_screenWidth!=width||m_screenHeight!=height))UnInitEnvironment();
    if(!m_state&&!InitEnvironment(gameDisplay,gameContext,width,height))return false;
    return true;
}

bool HideRecAImGui::RenderDrawData(ImDrawData* drawData,EGLDisplay gameDisplay,EGLSurface gameSurface, EGLContext gameContext,SwapBuffersFn swapBuffersFn){
    if(!drawData||!swapBuffersFn||gameSurface==EGL_NO_SURFACE)return false;
    int32_t wantedW=(int32_t)std::lround(drawData->DisplaySize.x);
    int32_t wantedH=(int32_t)std::lround(drawData->DisplaySize.y);
    if(wantedW<=0||wantedH<=0)return false;
    if(!EnsureEnvironment(gameDisplay,gameContext,wantedW,wantedH))return false;
    EGLDisplay oldDisplay=eglGetCurrentDisplay();
    EGLSurface oldDraw=eglGetCurrentSurface(EGL_DRAW);
    EGLSurface oldRead=eglGetCurrentSurface(EGL_READ);
    EGLContext oldContext=eglGetCurrentContext();
    if(m_overlayContext==EGL_NO_CONTEXT||eglMakeCurrent(m_display,m_surface,m_surface,m_overlayContext)!=EGL_TRUE)return false;
    EGLint vw=0,vh=0;
    if(eglQuerySurface(m_display,m_surface,EGL_WIDTH,&vw)!=EGL_TRUE||vw<=0)vw=m_screenWidth;
    if(eglQuerySurface(m_display,m_surface,EGL_HEIGHT,&vh)!=EGL_TRUE||vh<=0)vh=m_screenHeight;
    glViewport(0,0,vw,vh);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.f,0.f,0.f,0.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(drawData);
    EGLBoolean ok=swapBuffersFn(m_display,m_surface);
    if(oldDisplay!=EGL_NO_DISPLAY&&oldContext!=EGL_NO_CONTEXT) eglMakeCurrent(oldDisplay,oldDraw,oldRead,oldContext);
    else eglMakeCurrent(m_display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    return ok==EGL_TRUE;
}

void HideRecAImGui::Shutdown(){UnInitEnvironment();}
void HideRecAImGui::UnInitEnvironment(){
    m_state=false;
    if(m_display!=EGL_NO_DISPLAY&&m_surface!=EGL_NO_SURFACE){
        if(eglGetCurrentSurface(EGL_DRAW)==m_surface) eglMakeCurrent(m_display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
        eglDestroySurface(m_display,m_surface);
    }
    if(m_display!=EGL_NO_DISPLAY&&m_overlayContext!=EGL_NO_CONTEXT) eglDestroyContext(m_display,m_overlayContext);
    m_surface=EGL_NO_SURFACE; m_overlayContext=EGL_NO_CONTEXT;
    if(m_nativeWindow){ANwCreator::Destroy(m_options.activity,m_nativeWindow);m_nativeWindow=nullptr;}
    m_display=EGL_NO_DISPLAY; m_gameContext=EGL_NO_CONTEXT;
    m_screenWidth=m_screenHeight=-1;
}
}

#include <jni.h>
#include <android/native_activity.h>
#include <android/log.h>
#include <cstring>

static ANativeActivity g_nativeActivity{};
static bool g_initialized = false;
static JavaVM* g_vm = nullptr;

extern "C" void SetANativeActivityVM(JavaVM* vm) { g_vm = vm; }

static jobject GetCurrentActivity(JNIEnv* env)
{
    jclass activityThreadCls = env->FindClass("android/app/ActivityThread");
    if (!activityThreadCls) return nullptr;
    jmethodID currentActivityThread = env->GetStaticMethodID(activityThreadCls, "currentActivityThread", "()Landroid/app/ActivityThread;");
    if (!currentActivityThread) return nullptr;
    jobject activityThread = env->CallStaticObjectMethod(activityThreadCls, currentActivityThread);
    if (!activityThread) return nullptr;
    jfieldID mActivitiesField = env->GetFieldID(activityThreadCls, "mActivities", "Landroid/util/ArrayMap;");
    if (!mActivitiesField) return nullptr;
    jobject mActivities = env->GetObjectField(activityThread, mActivitiesField);
    if (!mActivities) return nullptr;
    jclass arrayMapCls = env->GetObjectClass(mActivities);
    jmethodID valuesMethod = env->GetMethodID(arrayMapCls, "values", "()Ljava/util/Collection;");
    if (!valuesMethod) return nullptr;
    jobject values = env->CallObjectMethod(mActivities, valuesMethod);
    if (!values) return nullptr;
    jclass collectionCls = env->GetObjectClass(values);
    jmethodID iteratorMethod = env->GetMethodID(collectionCls, "iterator", "()Ljava/util/Iterator;");
    if (!iteratorMethod) return nullptr;
    jobject iterator = env->CallObjectMethod(values, iteratorMethod);
    if (!iterator) return nullptr;
    jclass iteratorCls = env->GetObjectClass(iterator);
    jmethodID hasNext = env->GetMethodID(iteratorCls, "hasNext", "()Z");
    jmethodID next = env->GetMethodID(iteratorCls, "next", "()Ljava/lang/Object;");
    while (env->CallBooleanMethod(iterator, hasNext)) {
        jobject record = env->CallObjectMethod(iterator, next);
        if (!record) continue;
        jclass recordCls = env->GetObjectClass(record);
        jfieldID activityField = env->GetFieldID(recordCls, "activity", "Landroid/app/Activity;");
        if (!activityField) continue;
        jobject activity = env->GetObjectField(record, activityField);
        if (activity) return activity;
    }
    return nullptr;
}

extern "C" ANativeActivity* GetANativeActivity()
{
    if (g_initialized) return &g_nativeActivity;
    if (!g_vm) return nullptr;
    JNIEnv* env = nullptr;
    if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return nullptr;
    jobject activity = GetCurrentActivity(env);
    if (!activity) return nullptr;
    memset(&g_nativeActivity, 0, sizeof(g_nativeActivity));
    g_nativeActivity.vm    = g_vm;
    g_nativeActivity.env   = env;
    g_nativeActivity.clazz = env->NewGlobalRef(activity);
    g_initialized = true;
    return &g_nativeActivity;
}

#include <jni.h>
#include <android/native_activity.h>
#include <EGL/egl.h>

extern "C" ANativeActivity* GetANativeActivity();
extern "C" void SetANativeActivityVM(JavaVM* vm);

static android::HideRecAImGui* g_zel_imgui=nullptr;
static JavaVM* g_zel_vm=nullptr;

inline void ZEL_SetVM(JavaVM* vm){g_zel_vm=vm;SetANativeActivityVM(vm);}

inline bool ZEL_Render(ImDrawData* drawData,EGLDisplay dpy,EGLSurface surface,EGLContext context,
                       android::HideRecAImGui::SwapBuffersFn swapFn){
    if(!g_zel_imgui){
        ANativeActivity* activity=GetANativeActivity();
        if(!activity)return false;
        android::HideRecAImGui::Options opts;
        opts.activity=activity;
        opts.skipScreenshot=true;
        g_zel_imgui=new android::HideRecAImGui(opts);
    }
    return g_zel_imgui->RenderDrawData(drawData,dpy,surface,context,swapFn);
}

inline void ZEL_Shutdown(){
    if(g_zel_imgui){delete g_zel_imgui;g_zel_imgui=nullptr;}
}
inline bool ZEL_IsReady(){return g_zel_imgui&&(bool)*g_zel_imgui;}

class _BYTE;
class _BOOL4;
class _BOOL8;
class _WORD;
class _DWORD;
class _QWORD;

#define CREATE_COLOR(r, g, b, a) new float[4] {(float)(r) / 255.0f, (float)(g) / 255.0f, (float)(b) / 255.0f, (float)(a) / 255.0f}
bool g_clearMousePos = false;
bool ClearDisplay = false;
bool ShowFPS;
bool SnowB = false;
bool HIDEESP = false;
float SnowBsize = 0.0f;
bool isSpeedHackEnabled = false;
float speedHackMultiplier = 1.0f;
bool RedWallhackShow = false;
bool isJumpAdjustmentEnabled = false;
float jumpHeightMultiplier = 1.0f;
char logintext[4096];
float menu[4] = {142.0f / 255.0f, 134.0f / 255.0f, 246.0f / 255.0f, 1.0f};

float g_LastLogoOpacity = 1.0f;
float g_LastLogoSize = 1.0f;
int g_LogoHideDelayFrames = 0;
int g_LogoHideDelay = 40;

#define _BYTE uint8_t
#define _WORD  uint8_t
#define _DWORD uint64_t
#define _QWORD uint64_t
#define _BOOL4 uint8_t

#include <fstream>
using namespace std;

#include <Substrate/SubstrateHook.h>
#include <Substrate/CydiaSubstrate.h>

ImFont* F50 = nullptr;
ImFont* F107 = nullptr;
ImFont* SOCIAL = nullptr;
ImFont* Bold = nullptr;
JavaVM* jvm = nullptr;
JavaVM* VM = nullptr;

namespace font {
    ImFont* icomoon_logo = nullptr;
    ImFont* inter_semibold = nullptr;
    ImFont* icomoon_page = nullptr;
    // Fonts taken straight from the eliwoahzja/Lumin framework (Fonts/LuminFonts.h).
    ImFont* lumin_medium = nullptr;   // Inter Medium  (secondary / helper text)
    ImFont* lumin_icon = nullptr;     // uicons-regular-rounded (Lumin's glyph set)
}

static int g_GlWidth, g_GlHeight;
static bool g_App = false;

struct My_Patches
{
    MemoryPatch A1, grap, NoCrouch, SpeedhackX, SpeedhackX1, NoWingsuit, fpss, frame;
    MemoryPatch NoCrouchPawn, NoCrouchPlayerPawn;
} Patches;

float AVIWA = 119.167f;
bool wallh;
bool active = false;
float AimSmooth = 1.0f;
bool showKeyboard = false;

// The MISC tab text fields (report spoof / rename card) are button-like custom
// widgets: ImGui clears the active id as soon as the finger lifts, so gating the
// virtual keyboard on IsItemActive() made it flash for a single frame and
// vanish before anything could be typed. Track the focused field ourselves;
// only the focused field's buffer receives the virtual keyboard input, and the
// keyboard stays open until the user closes it or focuses another field.
static char *g_miscFieldBuf = nullptr;
static size_t g_miscFieldSize = 0;
static const char *g_miscFieldKbId = nullptr;
static void MiscTextFieldFocus(const char *kbId, char *buf, size_t size)
{
    g_miscFieldBuf = buf;
    g_miscFieldSize = size;
    g_miscFieldKbId = kbId;
    showKeyboard = true;
}
static void RenderMiscVirtualKeyboard()
{
    if (!showKeyboard || g_miscFieldBuf == nullptr)
    {
        if (!showKeyboard)
        {
            g_miscFieldBuf = nullptr;
            g_miscFieldKbId = nullptr;
        }
        return;
    }
    RenderVirtualKeyboard(g_miscFieldKbId, g_miscFieldBuf, g_miscFieldSize, &showKeyboard);
    if (!showKeyboard)
    {
        g_miscFieldBuf = nullptr;
        g_miscFieldKbId = nullptr;
    }
}
static bool g_RuntimeClearDisplayInit = false;

struct ClearDisplayDefaultInit {
    ClearDisplayDefaultInit() { Config.ExtraMenu.ClearDisplay = true; }
} g_ClearDisplayDefaultInit;

struct sRegion { uintptr_t start, end; };

std::chrono::steady_clock::time_point appStartTime = std::chrono::steady_clock::now();

static float veh_min = 0.1f;
static float veh_max = 5.0f;
static bool windowCollapsed = false;
static double collapseBarLastActiveTime = 0.0;
static float collapseBarOpacityAnim = 1.0f;
static float collapseBarPressAnim = 0.0f;
static float collapseBarEnterAnim = 1.0f;
static float collapseBarRestoreAnim = 1.0f;
static bool collapseBarRestoreActive = false;
static bool collapseBarWasCollapsed = false;
static float uncollapseOpenAnim = 1.0f;
static bool dark = true;
static float tabAlpha = 0.0f;
static float tabAdd = 0.0f;
static int page = 1;
static int activeTab = 1;
bool g_LogoPreviewMode = false;
static bool isLogin = false;
static std::string err;
static std::string storedKey = "";
static char s[256];
static bool g_LoginTextLoaded = false;

// Non-blocking login: the LOG IN button starts an in-flight request on a worker
// thread; the draw code renders a loading indicator while it runs, then consumes
// the result. That keeps the menu responsive even when the licence panel is slow.
static std::string g_loginResult;
static std::mutex  g_loginMutex;
static float g_loginStart = 0.0f;
static bool g_loginInFlight = false;

// Lumin-style failure feedback: the whole card accent turns red for a moment
// and the Activate button shakes horizontally, decaying over 0.8s.
static bool g_licenseInvalid = false;
static float g_licenseInvalidTimer = 0.0f;
static std::string g_licenseErrorMsg;
static void LuminFlagInvalid(const char* msg)
{
    g_licenseInvalid = true;
    g_licenseInvalidTimer = 0.0f;
    g_licenseErrorMsg = msg ? msg : "";
}

static bool g_ShowRadialMenu = true;
static void SaveLoginTextForAttempt(const char* text) {
    strncpy(logintext, text, sizeof(logintext) - 1);
    logintext[sizeof(logintext) - 1] = '\0';
}

static void StartLoginAttempt(const char* key) {
    // Defensive: do not start a network request for an empty or whitespace-only key.
    if (!key || !*key) {
        return;
    }
    bool hasNonWhitespace = false;
    for (const char* p = key; *p; ++p) {
        if (!std::isspace(static_cast<unsigned char>(*p))) {
            hasNonWhitespace = true;
            break;
        }
    }
    if (!hasNonWhitespace) {
        return;
    }

    const std::string attemptKey = key;
    {
        std::lock_guard<std::mutex> lock(g_loginMutex);
        g_loginResult.clear();
    }
    err.clear(); // drop any stale error so it can't overlap this attempt's status
    SaveLoginTextForAttempt(key);
    g_loginInFlight = true;
    g_loginStart = ImGui::GetTime();
    std::thread([attemptKey]() {
        const std::string result = Login(attemptKey.c_str());
        std::lock_guard<std::mutex> lock(g_loginMutex);
        g_loginResult = result;
    }).detach();
}

static std::string TakeLoginResult() {
    std::lock_guard<std::mutex> lock(g_loginMutex);
    const std::string result = g_loginResult;
    if (!result.empty())
        g_loginResult.clear();
    return result;
}

// Consume any login result that completed while the login window was not visible
// (for example a very fast credential that already authenticated before the first
// overlay frame). This keeps g_loginInFlight from becoming a permanently stuck state
// and ensures a successful login still transitions isLogin / g_ShowRadialMenu.
static void ConsumePendingLoginResult() {
    const std::string loginResult = TakeLoginResult();
    if (!loginResult.empty()) {
        // Mirror the render-thread handling used in the login window so the app
        // still reaches the menu even if the login UI never painted.
        LOGI("login: result consumed (%s)", loginResult.c_str());
        err = loginResult;
        if (err == "OK") {
            showKeyboard = false;
            g_LoginTextLoaded = true;
            err.clear();
            isLogin = true;
            g_ShowRadialMenu = true;
            LOGI("login: menu transition done");
            ApplyForbidKickOffOnLogin();
        }
    }
}


std::vector<sRegion> trapRegions;
uintptr_t address = 0;
std::string md5(std::string s);
uintptr_t g_il2cpp;
static bool isMenuVisible = true;

// ================= PIZZA MENU  =================

int TABG = 1;

// ============================================================================
//  UI 
// ============================================================================
namespace ModernUI {

    void RenderMenuEdgeLightning(ImDrawList* back, ImDrawList* front,
                                  const ImVec2& menuMin, const ImVec2& menuMax)
    {
        if (!back || !front || menuMax.x <= menuMin.x || menuMax.y <= menuMin.y)
            return;

        const float t = (float)ImGui::GetTime();
        const float w = menuMax.x - menuMin.x;
        const float h = menuMax.y - menuMin.y;
        const float perimeter = 2.0f * (w + h);
        if (perimeter <= 1.0f) return;

        float hr, hg, hb;
        ImGui::ColorConvertHSVtoRGB(ImClamp(main_runtime_theme::g_menuHue, 0.0f, 1.0f), 0.50f, 0.97f, hr, hg, hb);
        const int cr = (int)(hr * 255.0f);
        const int cg = (int)(hg * 255.0f);
        const int cb = (int)(hb * 255.0f);

        auto clamp01 = [](float v) -> float { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); };
        auto clampAlpha = [](float a) -> int { return a < 0.0f ? 0 : (a > 255.0f ? 255 : (int)a); };
        auto hash32 = [](uint32_t x) -> uint32_t { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16; return x; };
        auto randSigned = [&](int i, uint32_t salt) -> float { uint32_t hsh = hash32((uint32_t)i * 0x9E3779B9u ^ salt); return ((float)(hsh & 0x00FFFFFFu) / 8388607.5f) - 1.0f; };
        auto valueNoise = [&](float x, uint32_t salt) -> float {
            int i0 = (int)std::floor(x); int i1 = i0 + 1; float f = x - (float)i0;
            float s = f * f * (3.0f - 2.0f * f);
            float a = randSigned(i0, salt); float b = randSigned(i1, salt);
            return a + (b - a) * s;
        };
        auto perimeterPoint = [&](float d, ImVec2& tangent, ImVec2& normal) -> ImVec2 {
            d = std::fmod(d, perimeter); if (d < 0.0f) d += perimeter;
            if (d < w) { tangent = ImVec2(1.0f, 0.0f); normal = ImVec2(0.0f, -1.0f); return ImVec2(menuMin.x + d, menuMin.y); }
            d -= w;
            if (d < h) { tangent = ImVec2(0.0f, 1.0f); normal = ImVec2(1.0f, 0.0f); return ImVec2(menuMax.x, menuMin.y + d); }
            d -= h;
            if (d < w) { tangent = ImVec2(-1.0f, 0.0f); normal = ImVec2(0.0f, 1.0f); return ImVec2(menuMax.x - d, menuMax.y); }
            d -= w; tangent = ImVec2(0.0f, -1.0f); normal = ImVec2(-1.0f, 0.0f); return ImVec2(menuMin.x, menuMax.y - d);
        };

        int samples = (int)(perimeter / 5.0f);
        if (samples < 180) samples = 180;
        if (samples > 520) samples = 520;

        std::vector<ImVec2> path;
        path.reserve((size_t)samples);
        const float travel = t * 13.0f;

        for (int i = 0; i < samples; ++i) {
            const float u = (float)i / (float)samples;
            const float d = perimeter * u;
            ImVec2 tangent, normal;
            ImVec2 base = perimeterPoint(d, tangent, normal);

            const float n1 = valueNoise((float)i * 0.34f - travel, 0xA17C23D5u);
            const float n2 = valueNoise((float)i * 0.83f - travel * 1.91f, 0xD04F918Bu);
            const float n3 = std::sin(d * 0.115f - t * 8.6f) * 0.50f;
            const float n4 = std::sin(d * 0.247f - t * 14.3f + 1.7f) * 0.26f;

            const float outward = 4.3f + n1 * 4.6f + n2 * 2.0f + n3 * 1.7f + n4 * 1.0f;
            const float along = valueNoise((float)i * 0.51f - travel * 0.72f, 0x51F29A37u) * 1.65f;

            path.emplace_back(base.x + normal.x * outward + tangent.x * along,
                              base.y + normal.y * outward + tangent.y * along);
        }

        float flicker = 0.78f
            + 0.06f * std::sin(t * 31.0f)
            + 0.04f * std::sin(t * 53.0f + 0.8f)
            + 0.03f * std::sin(t * 79.0f + 2.1f);
        const float flashWave = 0.5f + 0.5f * std::sin(t * 17.0f + std::sin(t * 3.3f) * 2.0f);
        flicker += std::pow(flashWave, 9.0f) * 0.14f;
        flicker = clamp01(flicker);

        const int haloWide = clampAlpha(16.0f + flicker * 30.0f);
        const int haloMid  = clampAlpha(30.0f + flicker * 44.0f);
        const int coreSoft = clampAlpha(60.0f + flicker * 52.0f);
        const int coreHot  = clampAlpha(95.0f + flicker * 45.0f);

        for (int i = 0; i < samples; ++i) {
            const ImVec2& a = path[(size_t)i];
            const ImVec2& b = path[(size_t)((i + 1) % samples)];
            back->AddLine(a, b, IM_COL32(cr, cg, cb, haloWide), 10.0f);
            back->AddLine(a, b, IM_COL32(cr, cg, cb, haloMid),  6.0f);
            front->AddLine(a, b, IM_COL32(235, 235, 235, coreSoft), 2.2f);
            front->AddLine(a, b, IM_COL32(235, 235, 235, coreHot), 1.0f);
        }

        const float headD = std::fmod(t * 245.0f, perimeter);
        const float hotSpan = 210.0f;
        for (int i = 0; i < samples; ++i) {
            const float d = perimeter * ((float)i / (float)samples);
            float delta = std::fabs(d - headD);
            if (delta > perimeter * 0.5f) delta = perimeter - delta;
            if (delta > hotSpan) continue;

            float hot = 1.0f - (delta / hotSpan);
            hot = hot * hot * (3.0f - 2.0f * hot);
            const float rapid = 0.72f + 0.28f * std::sin(t * 46.0f + d * 0.08f);
            hot *= rapid;

            const ImVec2& a = path[(size_t)i];
            const ImVec2& b = path[(size_t)((i + 1) % samples)];
            back->AddLine(a, b, IM_COL32(cr, cg, cb, clampAlpha(22.0f + hot * 60.0f)), 8.0f);
            front->AddLine(a, b, IM_COL32(235, 235, 235, clampAlpha(55.0f + hot * 75.0f)), 1.6f);
        }
    }

    // Keeps the wheel hub (its center) inside the display so it can always be
    // grabbed again after a drag or a resolution/rotation change.
    inline ImVec2 ClampWheelPos(const ImVec2& pos, const ImVec2& displaySize)
    {
        const float slack = 230.0f; // half of the wheel window
        ImVec2 out = pos;
        if (displaySize.x <= 0.0f || displaySize.y <= 0.0f)
            return out; // display size not known yet
        out.x = ImClamp(out.x, -slack, ImMax(-slack, displaySize.x - slack));
        out.y = ImClamp(out.y, -slack, ImMax(-slack, displaySize.y - slack));
        return out;
    }

    // Keeps the menu header on screen so the container can always be dragged back.
    inline ImVec2 ClampMenuPos(const ImVec2& pos, const ImVec2& windowSize, const ImVec2& displaySize)
    {
        if (displaySize.x <= 0.0f || displaySize.y <= 0.0f)
            return pos; // display size not known yet

        // Keep the whole card on screen. The old version only guaranteed 140px
        // (x) / 90px (y) of the card stayed visible, so a stale position saved by
        // an earlier build left most of the menu -- including the entire sidebar
        // -- hanging off the left edge of the screen.
        ImVec2 out = pos;
        if (windowSize.x <= displaySize.x)
            out.x = ImClamp(out.x, 0.0f, displaySize.x - windowSize.x);
        else
            out.x = (displaySize.x - windowSize.x) * 0.5f;

        if (windowSize.y <= displaySize.y)
            out.y = ImClamp(out.y, 0.0f, displaySize.y - windowSize.y);
        else
            out.y = (displaySize.y - windowSize.y) * 0.5f;

        return out;
    }

    int RenderCategoryWheel(const ImVec2& defaultCenter)
    {
        const float windowSize  = 460.0f;
        const float outerRadius = 195.0f;
        const float hubRadius   = 55.0f;
        constexpr float kPi = 3.14159265358979323846f;
        const int sliceCount = 6;
        const char* labels[sliceCount] = { "VISUAL", "COMBAT", "MEMORY", "SKINS", "MISC", "SETTINGS" };

        const ImVec2 wheelDisplaySize = ImGui::GetIO().DisplaySize;

        // Centered on the display by default, then free-draggable; the position
        // the user drags it to is remembered in ui_layout.ini.
        static ImVec2 wheelPos(0.0f, 0.0f);
        static ImVec2 wheelPosApplied(-99999.0f, -99999.0f);
        static int  wheelHoldSlice = -1;
        static bool wheelDragging  = false;
        static bool wheelPosInit   = false;
        if (!wheelPosInit)
        {
            wheelPosInit = true;
            const ui_layout::State& layout = ui_layout::Get();
            wheelPos = layout.hasWheel
                ? ImVec2(layout.wheelX, layout.wheelY)
                : ImVec2(defaultCenter.x - windowSize * 0.5f, defaultCenter.y - windowSize * 0.5f);
        }
        const ImVec2 wheelDrawPos = ClampWheelPos(wheelPos, wheelDisplaySize);

        if (wheelDrawPos.x != wheelPosApplied.x || wheelDrawPos.y != wheelPosApplied.y)
        {
            ImGui::SetNextWindowPos(wheelDrawPos, ImGuiCond_Always);
            wheelPosApplied = wheelDrawPos;
        }
        ImGui::SetNextWindowSize(ImVec2(windowSize, windowSize), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        int selected = 0;
        const ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        if (ImGui::Begin("##astavex_radial_category_wheel", nullptr, flags)) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImVec2 wp = ImGui::GetWindowPos();
            const ImVec2 c(wp.x + windowSize * 0.5f, wp.y + windowSize * 0.5f);

            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::InvisibleButton("##astavex_radial_hitbox", ImVec2(windowSize, windowSize));

            const ImVec2 mouse = ImGui::GetIO().MousePos;
            const float mx = mouse.x - c.x;
            const float my = mouse.y - c.y;
            const float r  = std::sqrt(mx * mx + my * my);
            const float startAngle = -kPi * 0.5f;
            const float sliceAngle = (2.0f * kPi) / (float)sliceCount;

            float rawAngle = std::atan2(my, mx) - startAngle;
            while (rawAngle < 0.0f) rawAngle += 2.0f * kPi;
            while (rawAngle >= 2.0f * kPi) rawAngle -= 2.0f * kPi;

            int hoverSlice = -1;
            if (r >= hubRadius && r <= outerRadius)
                hoverSlice = (int)(rawAngle / sliceAngle);

            // Drag & drop: press the wheel, move it and release to keep the new
            // spot (stored in ui_layout.ini). A plain tap still picks a tab.
            const bool wheelHitboxActive = ImGui::IsItemActive();
            if (wheelHitboxActive && !wheelDragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 6.0f))
                wheelDragging = true;

            if (wheelDragging)
            {
                wheelPos = ClampWheelPos(wheelPos + ImGui::GetIO().MouseDelta, wheelDisplaySize);
                ImGui::SetWindowPos(wheelPos, ImGuiCond_Always);
                wheelPosApplied = wheelPos;
            }
            else if (wheelHitboxActive && wheelHoldSlice < 0)
            {
                wheelHoldSlice = hoverSlice;
            }

            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                if (wheelDragging)
                {
                    ui_layout::RememberWheel(wheelPos.x, wheelPos.y);
                    wheelDragging = false;
                    wheelHoldSlice = -1;
                }
                else if (wheelHoldSlice >= 0)
                {
                    if (hoverSlice == wheelHoldSlice)
                        selected = hoverSlice + 1;
                    wheelHoldSlice = -1;
                }
            }

            float hr, hg, hb;
            ImGui::ColorConvertHSVtoRGB(ImClamp(main_runtime_theme::g_menuHue, 0.0f, 1.0f), 0.50f, 0.97f, hr, hg, hb);
            const int cr = (int)(hr * 255.0f);
            const int cg = (int)(hg * 255.0f);
            const int cb = (int)(hb * 255.0f);

            for (int s = 0; s < sliceCount; ++s) {
                const int arcSteps = 16;
                ImVec2 poly[arcSteps + 2];
                poly[0] = c;
                const float a0 = startAngle + sliceAngle * (float)s;
                const float a1 = a0 + sliceAngle;
                for (int k = 0; k <= arcSteps; ++k) {
                    const float a = a0 + (a1 - a0) * ((float)k / (float)arcSteps);
                    poly[k + 1] = ImVec2(c.x + std::cos(a) * outerRadius,
                                         c.y + std::sin(a) * outerRadius);
                }
                const bool hot = (hoverSlice == s);
                dl->AddConvexPolyFilled(poly, arcSteps + 2,
                    hot ? IM_COL32(cr, cg, cb, 110) : IM_COL32(14, 14, 14, 235));
            }

            for (int s = 0; s < sliceCount; ++s) {
                const float a = startAngle + sliceAngle * (float)s;
                ImVec2 p0(c.x + std::cos(a) * hubRadius,   c.y + std::sin(a) * hubRadius);
                ImVec2 p1(c.x + std::cos(a) * outerRadius, c.y + std::sin(a) * outerRadius);
                dl->AddLine(p0, p1, IM_COL32(cr, cg, cb, 190), 1.6f);
            }
            dl->AddCircleFilled(c, hubRadius, IM_COL32(5, 5, 5, 245), 48);
            dl->AddCircle(c, hubRadius, IM_COL32(cr, cg, cb, 255), 48, 2.0f);
            dl->AddCircle(c, hubRadius - 4.0f, IM_COL32(235, 235, 235, 160), 48, 1.0f);

            const int ringPts = 160;
            ImVec2 ring[ringPts];
            const float t = (float)ImGui::GetTime();
            float wheelFlicker = 0.72f + 0.16f * std::sin(t * 37.0f) + 0.10f * std::sin(t * 61.0f + 0.7f);
            if (wheelFlicker < 0.42f) wheelFlicker = 0.42f;
            if (wheelFlicker > 1.0f)  wheelFlicker = 1.0f;

            for (int i = 0; i < ringPts; ++i) {
                const float a = (2.0f * kPi) * ((float)i / (float)ringPts);
                const float jitter =
                    std::sin(a * 17.0f - t * 5.6f) * 2.5f +
                    std::sin(a * 41.0f - t * 10.3f + 1.2f) * 1.4f +
                    std::sin(a * 73.0f - t * 14.7f + 0.4f) * 0.7f;
                const float rr = outerRadius + 4.0f + jitter;
                ring[i] = ImVec2(c.x + std::cos(a) * rr, c.y + std::sin(a) * rr);
            }
            for (int i = 0; i < ringPts; ++i) {
                const ImVec2& a = ring[i];
                const ImVec2& b = ring[(i + 1) % ringPts];
                dl->AddLine(a, b, IM_COL32(cr, cg, cb, (int)(30.0f + wheelFlicker * 45.0f)), 8.0f);
                dl->AddLine(a, b, IM_COL32(cr, cg, cb, (int)(90.0f + wheelFlicker * 70.0f)), 2.4f);
                dl->AddLine(a, b, IM_COL32(235, 235, 235, (int)(110.0f + wheelFlicker * 45.0f)), 1.0f);
            }

            for (int s = 0; s < sliceCount; ++s) {
                const float a = startAngle + sliceAngle * ((float)s + 0.5f);
                const float labelRadius = 122.0f;
                ImVec2 p(c.x + std::cos(a) * labelRadius, c.y + std::sin(a) * labelRadius);
                ImVec2 ts = ImGui::CalcTextSize(labels[s]);
                ImU32 col = (hoverSlice == s) ? IM_COL32(235, 235, 235, 255) : IM_COL32(180, 180, 184, 255);
                dl->AddText(ImVec2(p.x - ts.x * 0.5f, p.y - ts.y * 0.5f), col, labels[s]);
            }

            const char* hubLine1 = "ETHNIR";
            const char* hubLine2 = "NOIR";
            const ImVec2 hubSize1 = ImGui::CalcTextSize(hubLine1);
            const ImVec2 hubSize2 = ImGui::CalcTextSize(hubLine2);
            const float hubLineGap = 1.0f;
            const float hubTotalH  = hubSize1.y + hubSize2.y + hubLineGap;
            dl->AddText(ImVec2(c.x - hubSize1.x * 0.5f, c.y - hubTotalH * 0.5f), IM_COL32(255, 255, 255, 245), hubLine1);
            dl->AddText(ImVec2(c.x - hubSize2.x * 0.5f, c.y - hubTotalH * 0.5f + hubSize1.y + hubLineGap), IM_COL32(255, 255, 255, 245), hubLine2);
        }
        ImGui::End();
        ImGui::PopStyleVar(3);
        return selected;
    }
}

// ---------------------------------------------------------------------------
// Panic mode
// Each tab gets a PANIC button that switches the features living on that tab
// back off. Flipping the flags is enough for everything the hooks read every
// frame; the values that are "active while > 0/1" (multipliers, slide distance,
// hitbox scale, ...) are reset to their neutral value so the hooks really go
// idle again. Memory patches (WallHack / NoCrouch) are restored by the
// per-frame patch sync lower down and A-Fire releases any trigger it holds.
// ---------------------------------------------------------------------------
inline void PanicVisualTab()
{
    Config.ESPMenu.isPlayerLine = false;
    Config.ESPMenu.Box = false;
    Config.ESPMenu.Skeleton = false;
    Config.ESPMenu.Health = false;
    Config.ESPMenu.Name = false;
    Config.ESPMenu.Distance = false;
    Config.ESPMenu.Count = false;
    Config.ESPMenu.Alert = false;
    Config.ESPMenu.Aimline = false;
    Config.ESPMenu.ShowFov = false;
    Config.ESPMenu.Crosshair = false;
    Config.ESPMenu.Signal = false;
    Config.ESPMenu.Armor = false;
    Config.ESPMenu.GrenadeWarn = false;
    Config.ESPMenu.BRClass = false;
    Config.ESPMenu.Vehicle = false;
    Config.ESPMenu.VehicleHealth = false;
    Config.ExtraMenu.WallHack = false;
    Config.ExtraMenu.RedWallhack = false;
}

inline void PanicCombatTab()
{
    Config.Aim.Aimbot360 = false;
    Config.Aim.AimSilent = false;
    Config.Aim.AimAssistSize = 0.0f;
    Config.Aim.Cross = 0.0f;
    Config.Aim.Target = EAimTarget::Heads;
    Config.Aim.Trigger = EAimTrigger::None;
    Config.Aim.By = EAim::Distance;
    Config.Aim.HitGroup = HitGroupAuto;
    AimSmooth = 1.0f;
    Config.ExtraMenu.A_Fire = false;
    Config.ExtraMenu.A_FireTrigger = 0;
    Config.ExtraMenu.A_FireDelay = 0.0f;
    A_FireReleaseTrigger(nullptr);   // let go of the trigger right away
}

inline void PanicMemoryTab()
{
    Config.ExtraMenu.Hit = false;
    Config.ExtraMenu.HitboxScale = 3.0f;          // back to the struct default
    Config.ExtraMenu.TuneHitboxHeadBand = 0.35f;
    Config.ExtraMenu.Recoil = false;
    Config.ExtraMenu.Spread = false;
    Config.ExtraMenu.Shake = false;
    Config.ExtraMenu.Rpd = false;
    Config.ExtraMenu.Parachute = false;
    Config.ExtraMenu.Flash = false;
    Config.ExtraMenu.Fire = false;
    Config.ExtraMenu.NoSprintFireDelay = false;
    Config.ExtraMenu.Diving = false;
    Config.ExtraMenu.Reload = false;
    Config.ExtraMenu.Scope = false;
    Config.ExtraMenu.Switch = false;
    Config.ExtraMenu.Kinetic = false;
    Config.ExtraMenu.NoCrouch = false;            // its patch is restored by the sync loop
    SnowBsize = 0.0f;
    SlideRange = 0.0f;
    speedHackMultiplier = 1.0f;
    jumpHeightMultiplier = 1.0f;
    Config.ExtraMenu.ReportSpoof = false;
    Config.ExtraMenu.RenameCard = false;
    Config.ExtraMenu.ForbidKickOff = false;
    Config.ExtraMenu.ForbidKickOffOnLogin = false;
}

inline void PanicSkinTab()
{
    // The skin tab keeps its picks in sBool plus the active-effect maps, so
    // clearing them drops every pending selection. Camo that already reached the
    // game data is put back through RestoreCamo().
    sBool.clear();
    activeKillEffects.clear();
    activeBulletTrackEffects.clear();
    activeWeaponFireEffects.clear();
    activeWeaponBrocast.clear();
    activeVehicleSkins.clear();
    activeVehicleSkinsById.clear();
    activeVehicleSkinConfs.clear();
    Config.ExtraMenu.CamoTest = false;
    Config.ExtraMenu.CamoTestMode = 0;
    RestoreCamo();
    Config.ExtraMenu.Blueprints = false;
    Config.ExtraMenu.Attachment = false;
}

inline void PanicMiscTab()
{
    Config.ExtraMenu.ClearTerrain = false;
    Config.ExtraMenu.NoSmoke = false;
    Config.ExtraMenu.WalkUnderWater = false;
    Config.ExtraMenu.CameraPov = false;
    Config.ExtraMenu.CameraPovSize = 0.0f;
    Config.ExtraMenu.Spectatex = false;
    Config.ExtraMenu.UnliAmmo = false;
    Config.ExtraMenu.NoGravity = false;
    Config.ExtraMenu.NoGravityScale = 1.0f;
}

inline void PanicSettingsTab()
{
    // Clear Display is the menu's own overlay preference (hidden by default), so
    // it is deliberately left untouched here.
    Config.ExtraMenu.Grap = false;
    Config.Aim.FpsLevel = false;
    Config.Aim.showFPSLevelSlider = false;
    Config.Aim.FpsLevelUltra = false;
    Config.Aim.showFPSLevelUltraSlider = false;
    Config.ExtraMenu.ResetGuest = false;
    Config.ExtraMenu.ForbidKickOff = false;
    Config.ExtraMenu.ForbidKickOffOnLogin = false;
}

inline void PanicAllTabs()
{
    PanicVisualTab();
    PanicCombatTab();
    PanicMemoryTab();
    PanicSkinTab();
    PanicMiscTab();
    PanicSettingsTab();
}

// The PANIC row that sits above every tab: the left button clears the tab you
// are on, the right one clears everything.
inline void DrawTabPanicBar(int activeTab)
{
    const char *tabName = nullptr;
    void (*panicFn)() = nullptr;
    switch (activeTab) {
        case 1: tabName = "VISUAL";   panicFn = PanicVisualTab;   break;
        case 2: tabName = "COMBAT";   panicFn = PanicCombatTab;   break;
        case 3: tabName = "MEMORY";   panicFn = PanicMemoryTab;   break;
        case 4: tabName = "SKINS";    panicFn = PanicSkinTab;     break;
        case 5: tabName = "MISC";     panicFn = PanicMiscTab;     break;
        case 6: tabName = "SETTINGS"; panicFn = PanicSettingsTab; break;
        default: return;
    }

    char label[64];
    std::snprintf(label, sizeof(label), "TURN OFF ALL (%s)", tabName);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.545f, 0.110f, 0.129f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.678f, 0.141f, 0.165f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.412f, 0.078f, 0.094f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.000f, 0.925f, 0.925f, 1.00f));

    const float panicRowW = ImMax(1.0f, ImGui::GetContentRegionAvail().x);
    if (ImGui::Button(label, ImVec2(panicRowW * 0.55f, 32.0f)) && panicFn != nullptr)
        panicFn();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Panic: turns every feature on this tab off and restores what it patched.");

    ImGui::SameLine(0.0f, 8.0f);

    if (ImGui::Button("PANIC ALL", ImVec2(ImMax(1.0f, ImGui::GetContentRegionAvail().x), 32.0f)))
        PanicAllTabs();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Panic: turns every feature on every tab off.");

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
}

// ---------------------------------------------------------------------------
// Menu sidebar page row.
//
// Ported from the eliwoahzja/imgui reference menu: each category is an icon +
// label row in the left rail with an animated accent pill behind the active
// entry and a smoothly-animated icon/text tint. Replaces the old floating hub
// launcher, whose categories now live here as real tabs inside the window.
// ---------------------------------------------------------------------------
inline bool DrawSidebarPage(const char *id, const char *icon, const char *label,
                            bool selected, const ImVec2 &size)
{
    ImGui::PushID(id);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 bbMax(pos.x + size.x, pos.y + size.y);

    ImGui::InvisibleButton("##page", size);
    const bool hovered = ImGui::IsItemHovered();
    const bool pressed = ImGui::IsItemClicked();
    const ImGuiID key = ImGui::GetItemID();

    static std::unordered_map<ImGuiID, float> s_anim;
    float &a = s_anim[key];
    a = ImLerp(a, selected ? 1.0f : 0.0f, ImGui::GetIO().DeltaTime * 9.0f);

    ImDrawList *dl = ImGui::GetWindowDrawList();
    const ImU32 accent = zenin::T::Accent;

    if (a > 0.004f) {
        dl->AddRectFilled(pos, bbMax,
                          IM_COL32(255, 90, 92, (int)(40.0f * a)), 10.0f);
        dl->AddRectFilled(pos, ImVec2(pos.x + 3.0f, bbMax.y), accent, 10.0f,
                          ImDrawFlags_RoundCornersLeft);
    } else if (hovered) {
        dl->AddRectFilled(pos, bbMax, IM_COL32(255, 255, 255, 12), 10.0f);
    }

    const ImU32 textCol = selected ? IM_COL32(255, 255, 255, 255)
                        : (hovered ? IM_COL32(216, 216, 222, 255)
                                   : IM_COL32(138, 138, 146, 255));
    const ImU32 iconCol = selected ? accent : textCol;

    ImFont *iconFont = custom::shell::GetIconFont();
    if (iconFont) {
        const ImVec2 isz = custom::shell::MeasureText(iconFont, 15.0f, icon);
        dl->AddText(iconFont, 15.0f, ImVec2(pos.x + 16.0f, pos.y + (size.y - isz.y) * 0.5f),
                    iconCol, icon);
    }

    ImFont *labelFont = custom::shell::GetTextFont();
    const float labelSize = 13.5f;
    const ImVec2 lsz = labelFont ? labelFont->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, label)
                                 : ImGui::CalcTextSize(label);
    if (labelFont)
        dl->AddText(labelFont, labelSize,
                    ImVec2(pos.x + 44.0f, pos.y + (size.y - lsz.y) * 0.5f), textCol, label);

    ImGui::PopID();
    return pressed;
}

EGLBoolean (*old_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    eglQuerySurface(dpy, surface, EGL_WIDTH, &g_GlWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_GlHeight);

    if (!g_App)
    {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL;
        io.LogFilename = NULL;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        // Interaction polish: quicker double-click window, tighter drag threshold
        // so small ImGui drags (wheel, slider) feel responsive instead of floaty.
        io.MouseDoubleClickTime = 0.22f;
        io.MouseDragThreshold = 1.4f;
        ImGui_ImplOpenGL3_Init("#version 300 es");

        ImFontConfig icomoon_logo_config;
        icomoon_logo_config.MergeMode = false;
        icomoon_logo_config.PixelSnapH = true;
        icomoon_logo_config.FontDataOwnedByAtlas = false;
        font::icomoon_logo = io.Fonts->AddFontFromMemoryTTF((void*)icomoon_page, sizeof(icomoon_page), 20.f, &icomoon_logo_config);

        ImFontConfig inter_config;
        inter_config.MergeMode = false;
        inter_config.PixelSnapH = true;
        inter_config.FontDataOwnedByAtlas = false;
        font::inter_semibold = io.Fonts->AddFontFromMemoryTTF((void*)inter_semibold, sizeof(inter_semibold), 16.f, &inter_config);

        ImFontConfig page_config;
        page_config.MergeMode = false;
        page_config.PixelSnapH = true;
        page_config.FontDataOwnedByAtlas = false;
        font::icomoon_page = io.Fonts->AddFontFromMemoryTTF((void*)icomoon_page, sizeof(icomoon_page), 18.f, &page_config);

        static const ImWchar icons_ranges[] = { 0xe000, 0xf8ff, 0 };
        ImFontConfig iconsConfig;
        iconsConfig.MergeMode = true;
        iconsConfig.PixelSnapH = true;
        iconsConfig.OversampleH = 2.5f;
        iconsConfig.OversampleV = 2.5f;
        iconsConfig.FontDataOwnedByAtlas = false;
        // ── Lumin fonts (framework/helpers/fonts.cpp) ───────────────────────
        // inter_medium at 11-16px for helper/status text, and the uicons icon
        // font registered over Lumin's private glyph range.
        ImFontConfig lumin_medium_config;
        lumin_medium_config.MergeMode = false;
        lumin_medium_config.PixelSnapH = true;
        lumin_medium_config.FontDataOwnedByAtlas = false;
        font::lumin_medium = io.Fonts->AddFontFromMemoryTTF(
            (void*)lumin::fonts::inter_medium, sizeof(lumin::fonts::inter_medium), 16.f,
            &lumin_medium_config, io.Fonts->GetGlyphRangesDefault());

        static const ImWchar lumin_icon_ranges[] = {
            0x26A0, 0x26A0, 0x26D4, 0x26D4, 0x2714, 0x2714, 0xE70D, 0xE70E,
            0xF1E7, 0xF1E7, 0xF309, 0xF309, 0xF3A2, 0xF3A2, 0xF5F8, 0xF5F8,
            0xF71C, 0xF71C, 0xF87D, 0xF87D, 0xF8E1, 0xF8E1, 0xFB7C, 0xFB7C,
            0xFD32, 0xFD32, 0xFD5F, 0xFD5F, 0xFE19, 0xFE19, 0xFEA0, 0xFEA0,
            0xFEA6, 0xFEA6, 0xFF21, 0xFF21,
            0
        };
        ImFontConfig lumin_icon_config;
        lumin_icon_config.MergeMode = false;
        lumin_icon_config.PixelSnapH = true;
        lumin_icon_config.FontDataOwnedByAtlas = false;
        font::lumin_icon = io.Fonts->AddFontFromMemoryTTF(
            (void*)lumin::fonts::icon_font, sizeof(lumin::fonts::icon_font), 15.f,
            &lumin_icon_config, lumin_icon_ranges);

        F107 = io.Fonts->AddFontFromMemoryCompressedTTF((void*)font_awesome_data1, (int)font_awesome_size1, 25.0f, &iconsConfig, icons_ranges);
        F50 = io.Fonts->AddFontFromMemoryTTF((void *)F50_data, F50_size, 30.0f, NULL, io.Fonts->GetGlyphRangesDefault());
        if (!F107) F107 = font::inter_semibold;
        if (font::inter_semibold) io.FontDefault = font::inter_semibold;
        io.Fonts->Build();
        ImGui_ImplOpenGL3_CreateFontsTexture();

        memset(&Config, 0, sizeof(sConfig));

        Config.sColorsESPPLAYER.LinePLAYER = CREATE_COLOR(0, 255, 0, 255);
        Config.sColorsESPPLAYER.BoxPLAYER = CREATE_COLOR(0, 255, 0, 255);
        Config.sColorsESPPLAYER.NamePLAYER = CREATE_COLOR(0, 255, 0, 255);
        Config.sColorsESPPLAYER.DistancePLAYER = CREATE_COLOR(0, 255, 0, 255);
        Config.sColorsESPPLAYER.HealthPLAYER = CREATE_COLOR(0, 255, 0, 255);
        Config.sColorsESPPLAYER.SkeletonPLAYER = CREATE_COLOR(0, 255, 0, 255);
        Config.sColorsESPBOT.LineBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.BoxBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.NameBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.HealthBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.DistanceBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPBOT.SkeletonBOT = CREATE_COLOR(0, 255, 0, 180);
        Config.sColorsESPOTHERS.PovOTHERS = CREATE_COLOR(0, 255, 0, 180);

        Config.ExtraMenu.NoGravityScale = 1.0f;
        Config.Aim.AimAssistSize = 0.0f;
        Config.Aim.Cross = 45.0f;
        Config.Aim.Target = EAimTarget::Heads;
        Config.Aim.Trigger = EAimTrigger::None;
        Config.Aim.By = EAim::Distance;
        Config.Bline = 2.0f;
        Config.Pline = 2.0f;
        ui_layout::LoadLayout();
        g_App = true;
    }

    ImGuiIO *io = &ImGui::GetIO();
    screenWidth = (float)g_GlWidth;
    screenHeight = (float)g_GlHeight;
    io->DisplaySize = ImVec2((float)g_GlWidth, (float)g_GlHeight);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // Anti Leak: deferred to the game thread (never run from the JNI login
    // callback) so the il2cpp field write can never take the process down.
    ProcessAntiLeak();

    // Triggerbot driver: runs every frame on the game thread instead of relying
    // on Weapon::Tick, which never fired when the ticked instance did not match
    // the currently held weapon.
    A_FireTick();

    // Rename card / report spoof: keep the local player's own profile in sync with
    // the menu values (report spoof is applied last so its decoy identity wins).
    // Defend against the first frames right after login where get_LocalPawn() can
    // still return a half-baked pointer and dereferencing its offsets crashes.
    if (Tools::IsPtrValid(GamePlay::get_LocalPawn())) {
        ApplyRenameCard();
        ApplyReportSpoofIdentity();
    }

    ImDrawList *draw = ImGui::GetBackgroundDrawList();

    DrawESP(ImGui::GetBackgroundDrawList(), screenWidth, screenHeight, get_dpi());
    floating_info::Render(draw, screenWidth, screenHeight);

    // =====================================================================
    // INVISIBLE MAG RESTORE
    // =====================================================================
    if (windowCollapsed)
    {
        const ImVec2 display = ImGui::GetIO().DisplaySize;

        const float restoreWidth  = display.x;
        const float restoreHeight = 80.0f;

        static double hiddenRestoreLastTap = -10.0;
        const double fastDoubleTapWindow = 0.30;

        ImGui::SetNextWindowPos(ImVec2(0.0f, 8.0f),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(restoreWidth, 80.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

        const ImGuiWindowFlags hiddenFlags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoBringToFrontOnFocus;

        if (ImGui::Begin("##astavex_hidden_restore_zone", nullptr, hiddenFlags))
        {
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::InvisibleButton("##astavex_hidden_restore_doubletap",
                                   ImVec2(restoreWidth, restoreHeight));

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                const ImVec2 mouse = ImGui::GetIO().MousePos;
                const bool inRestoreBand = (mouse.y >= 0.0f) && (mouse.y <= restoreHeight)
                    && (mouse.x >= (display.x * 0.5f - 40.0f)) && (mouse.x <= (display.x * 0.5f + 40.0f));

                if (inRestoreBand)
                {
                    const double now = ImGui::GetTime();
                    if ((hiddenRestoreLastTap >= 0.0) && (now - hiddenRestoreLastTap) > fastDoubleTapWindow)
                    {
                        windowCollapsed = false;
                        isMenuVisible = true;
                        g_ShowRadialMenu = true;
                        hiddenRestoreLastTap = -10.0;
                    }
                    else
                    {
                        hiddenRestoreLastTap = now;
                    }
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
        collapseBarRestoreAnim = 0.0f;
        collapseBarWasCollapsed = false;
        collapseBarEnterAnim = 1.0f;
    }
    else
    {
        collapseBarWasCollapsed = false;
        collapseBarEnterAnim = 1.0f;
    }

    if (isMenuVisible && !windowCollapsed)
    {
        if (!g_RuntimeClearDisplayInit) {
            Config.ExtraMenu.ClearDisplay = true;
            g_RuntimeClearDisplayInit = true;
        }

        if (!g_LoginTextLoaded && VM != nullptr)
        {
            // Dedicated transfer buffer: on devices where snprintf's locale
            // handling or the strncpy tail interacts badly these copies crashed
            // right after the login card appeared. memcpy into a stack buffer
            // first, then clamp explicitly.
            if (LoadTextFromFile() && logintext[0] != '\0')
            {
                char staged[sizeof(s)];
                std::memcpy(staged, logintext, sizeof(staged));
                staged[sizeof(staged) - 1] = '\0';
                std::memcpy(s, staged, sizeof(s));
                s[sizeof(s) - 1] = '\0';
                g_LoginTextLoaded = true;
                LOGI("login: prefill loaded (%zu chars)", strlen(s));
            }
        }

        runtime_preview_menu::EnsureTexturesLoaded();
        main_runtime_theme::ApplyThemeState();

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        const ImVec2 viewportCenter = ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (!isLogin)
        {
            // Login is pinned to the middle of the screen and cannot be dragged.
            ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(640, 460), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.0f);

            if (ImGui::Begin(OBFUSCATE("Login Menu"), nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse))
            {
                const ImVec2 pos = ImGui::GetWindowPos();
                ImDrawList* draw = ImGui::GetWindowDrawList();

                // dmalogin palette (framework/settings/colors.h) — login only; menu keeps the ECHO accent
                const ImU32 dmAccent      = IM_COL32(189, 189, 255, 255);   // c->accent
                const ImU32 dmAccentDim   = IM_COL32(156, 156, 255, 255);   // c->g_accent
                const ImU32 dmFrame       = IM_COL32(25, 25, 36, 255);      // c->frame_layout
                const ImU32 dmFrameBorder = IM_COL32(39, 39, 58, 255);      // c->frame_border
                const ImU32 dmTextMut     = IM_COL32(150, 150, 166, 255);      // redesigned: readable muted
                const ImU32 dmWhite       = IM_COL32(255, 255, 255, 255);

                const ImVec2 login_size = ImVec2(640, 460);

                // ══════════════════════════════════════════════════════════════
                // LUMIN LOGIN
                // Rebuilt from the eliwoahzja/Lumin framework (framework/gui.cpp
                // "Login" tab + framework/widgets): a compact vertical stack of
                // brand header, helper copy, license-key field, error row and the
                // accent "Activate" button, all typeset with Lumin's own fonts.
                // ══════════════════════════════════════════════════════════════
                ImFont *mediumFont = font::lumin_medium ? font::lumin_medium : custom::shell::GetTextFont();
                ImFont *titleFont  = custom::shell::GetTitleFont();
                ImFont *luminIcon  = font::lumin_icon;

                auto drawTextL = [&](const char *txt, const ImVec2 &p, ImU32 col, float size) {
                    if (mediumFont) draw->AddText(mediumFont, size, p, col, txt);
                    else draw->AddText(p, col, txt);
                };

                // Lumin's license_key_icon(): a small accent key drawn with the
                // draw list (the framework does the same, no glyph involved).
                // A real key silhouette: ring on the left, horizontal shaft with
                // two teeth. (The previous version drew a diagonal handle, which
                // read as a magnifying glass.)
                auto drawKeyIcon = [&](const ImVec2 &center, ImU32 col, float scale) {
                    const ImVec2 ring(center.x - 6.0f * scale, center.y);
                    draw->AddCircle(ring, 4.6f * scale, col, 28, 1.8f * scale);
                    draw->AddLine(ImVec2(ring.x + 4.6f * scale, center.y),
                                  ImVec2(center.x + 12.0f * scale, center.y), col, 1.8f * scale);
                    draw->AddLine(ImVec2(center.x + 4.0f * scale, center.y),
                                  ImVec2(center.x + 4.0f * scale, center.y + 4.5f * scale), col, 1.8f * scale);
                    draw->AddLine(ImVec2(center.x + 8.5f * scale, center.y),
                                  ImVec2(center.x + 8.5f * scale, center.y + 3.5f * scale), col, 1.8f * scale);
                };

                // ── Card shell (Lumin child fill + glass border) ──────────────
                draw->AddRectFilled(pos, pos + login_size, zenin::T::WindowBg, 12.0f);
                draw->AddRect(pos, pos + login_size, IM_COL32(42, 42, 48, 200), 12.0f, 0, 1.2f);
                draw->AddRectFilled(pos, ImVec2(pos.x + login_size.x, pos.y + 3.0f),
                                    zenin::T::Accent, 12.0f, ImDrawFlags_RoundCornersTop);

                // ── brand_header("License access") ────────────────────────────
                {
                    const float markX = pos.x + 26.0f;
                    const float markY = pos.y + 24.0f;
                    const float markS = 46.0f;
                    const ImVec2 markC(markX + markS * 0.5f, markY + markS * 0.5f);

                    draw->AddCircleFilled(markC, markS * 0.80f, IM_COL32(255, 90, 92, 30), 48);
                    draw->AddRectFilled(ImVec2(markX, markY), ImVec2(markX + markS, markY + markS),
                                        IM_COL32(28, 28, 32, 255), 12.0f);
                    draw->AddRect(ImVec2(markX, markY), ImVec2(markX + markS, markY + markS),
                                  IM_COL32(255, 90, 92, 90), 12.0f, 0, 1.2f);
                    drawKeyIcon(markC, zenin::T::Accent, 1.15f);

                    if (titleFont) {
                        draw->AddText(titleFont, 20.0f, ImVec2(markX + markS + 14.0f, markY + 5.0f),
                                      zenin::T::Text, "ZENIN");
                        drawTextL("LICENSE ACCESS", ImVec2(markX + markS + 15.0f, markY + 31.0f),
                                  zenin::T::TextMut, 11.0f);
                    }
                }

                // ── helper copy ───────────────────────────────────────────────
                drawTextL("Enter your license key below to unlock the menu.",
                          ImVec2(pos.x + 26.0f, pos.y + 94.0f), zenin::T::TextMut, 12.0f);
                drawTextL("The key is checked against the license server.",
                          ImVec2(pos.x + 26.0f, pos.y + 112.0f), IM_COL32(110, 110, 118, 255), 12.0f);

                // ── LICENSE KEY field (Lumin text_field) ──────────────────────
                const float fieldX = pos.x + 26.0f;
                const float fieldW = login_size.x - 52.0f;
                const float fieldY = pos.y + 152.0f;
                const float fieldH = 58.0f;

                drawTextL("LICENSE KEY", ImVec2(fieldX + 2.0f, fieldY - 20.0f), zenin::T::TextMut, 11.0f);
                draw->AddRectFilled(ImVec2(fieldX + 2.0f, fieldY - 5.0f),
                                    ImVec2(fieldX + 46.0f, fieldY - 3.6f), zenin::T::Accent, 1.0f);

                ImGui::SetCursorPos(ImVec2(fieldX - pos.x, fieldY - pos.y));
                static const ImGui::AstralInputStyle loginInputStyle = {
                    ImVec4(25.0f/255.0f, 25.0f/255.0f, 30.0f/255.0f, 1.0f),     // bg
                    ImVec4(30.0f/255.0f, 30.0f/255.0f, 36.0f/255.0f, 1.0f),     // bgHovered
                    ImVec4(32.0f/255.0f, 32.0f/255.0f, 38.0f/255.0f, 1.0f),     // bgActive
                    ImVec4(52.0f/255.0f, 52.0f/255.0f, 60.0f/255.0f, 1.0f),     // border
                    ImVec4(255.0f/255.0f, 90.0f/255.0f, 92.0f/255.0f, 1.0f),    // borderActive (lumin accent)
                    ImVec4(96.0f/255.0f, 96.0f/255.0f, 104.0f/255.0f, 1.0f),    // hint
                    ImVec4(240.0f/255.0f, 240.0f/255.0f, 244.0f/255.0f, 1.0f),  // text
                    ImVec4(255.0f/255.0f, 90.0f/255.0f, 92.0f/255.0f, 1.0f),    // accent
                    10.0f                                                       // rounding
                };
                ImGui::AstralInput("##key_login", s, sizeof(s), ImVec2(fieldW, fieldH), &loginInputStyle);
                bool loginInputClicked = ImGui::IsItemClicked();
                bool loginInputActive = ImGui::IsItemActive();
                bool loginInputHovered = ImGui::IsItemHovered();

                // key glyph + separator inside the field (lumin text_field icon slot)
                {
                    const ImVec2 fieldMin = ImGui::GetItemRectMin();
                    const ImVec2 fieldMax = ImGui::GetItemRectMax();
                    drawKeyIcon(ImVec2(fieldMin.x + 26.0f, fieldMin.y + fieldH * 0.5f),
                                zenin::T::Accent, 0.95f);
                    draw->AddLine(ImVec2(fieldMin.x + 52.0f, fieldMin.y + 10.0f),
                                  ImVec2(fieldMin.x + 52.0f, fieldMax.y - 10.0f),
                                  IM_COL32(58, 58, 66, 220), 1.0f);
                }

                if (loginInputClicked || loginInputActive) showKeyboard = true;

                if (showKeyboard && !loginInputActive && !loginInputHovered && ImGui::IsMouseClicked(0)) {
                    ImGuiIO& io = ImGui::GetIO();
                    float screenHeight = io.DisplaySize.y;
                    float keyboardHeight = screenHeight * 0.60f;
                    if (ImGui::GetMousePos().y > screenHeight - keyboardHeight) showKeyboard = false;
                }

                // ── error row (Lumin license_error_message) ───────────────────
                {
                    const float errorY = pos.y + 220.0f;
                    std::string statusMsg = err;
                    if (err.empty() && !g_licenseErrorMsg.empty())
                        statusMsg = g_licenseErrorMsg;
                    if (!statusMsg.empty() && statusMsg != "OK") {
                        const ImVec2 eMin(fieldX, errorY);
                        draw->AddRectFilled(eMin, ImVec2(eMin.x + fieldW, eMin.y + 26.0f),
                                            IM_COL32(255, 90, 92, 26), 8.0f);
                        // Lumin draws the key badge here; we swap in the uicons
                        // warning glyph from the framework's icon font.
                        if (luminIcon) {
                            const char warn[] = { (char)0xE2, (char)0x9A, (char)0xA0, 0 }; // U+26A0
                            const ImVec2 wsz = luminIcon->CalcTextSizeA(13.0f, FLT_MAX, 0.0f, warn);
                            draw->AddText(luminIcon, 13.0f,
                                          ImVec2(eMin.x + 10.0f, eMin.y + (26.0f - wsz.y) * 0.5f),
                                          zenin::T::Accent, warn);
                        }
                        const std::string msg = statusMsg;
                        drawTextL(msg.c_str(), ImVec2(eMin.x + 30.0f, eMin.y + 5.0f),
                                  IM_COL32(255, 140, 142, 255), 13.0f);
                    } else if (g_loginInFlight) {
                        drawTextL("Please wait, account verification",
                                  ImVec2(fieldX, errorY + 4.0f), zenin::T::TextMut, 13.0f);
                        const float spin = (float)ImGui::GetTime() * 3.0f;
                        const ImVec2 sc(fieldX + fieldW - 16.0f, errorY + 17.0f);
                        draw->PathClear();
                        for (int i = 0; i <= 12; ++i) {
                            const float a = spin + (float)i / 12.0f * 6.28318f;
                            draw->PathLineTo(ImVec2(sc.x + std::cos(a) * 9.0f, sc.y + std::sin(a) * 9.0f));
                        }
                        draw->PathStroke(zenin::T::Accent, 0, 2.0f);
                    }
                }

                // ── primary "Activate" button (Lumin primary_button) ──────────
                auto drawSecondaryButton = [&](const char *label, const ImVec2 &bMin, const ImVec2 &bMax) -> bool {
                    ImGui::SetCursorScreenPos(bMin);
                    const bool pressed = ImGui::InvisibleButton(label, ImVec2(bMax.x - bMin.x, bMax.y - bMin.y));
                    const bool hovered = ImGui::IsItemHovered();
                    draw->AddRectFilled(bMin, bMax,
                                        hovered ? IM_COL32(46, 46, 52, 255) : IM_COL32(32, 32, 36, 255),
                                        10.0f);
                    draw->AddRect(bMin, bMax,
                                  hovered ? zenin::T::Accent : IM_COL32(58, 58, 66, 220), 10.0f, 0, 1.2f);
                    const float fs = 13.0f;
                    const ImVec2 ts = mediumFont ? mediumFont->CalcTextSizeA(fs, FLT_MAX, 0.0f, label)
                                                 : ImGui::CalcTextSize(label);
                    const ImVec2 tp((bMin.x + bMax.x - ts.x) * 0.5f, (bMin.y + bMax.y - ts.y) * 0.5f);
                    drawTextL(label, tp, hovered ? IM_COL32(255, 255, 255, 255) : zenin::T::Text, fs);
                    return pressed;
                };

                {
                    const float btnW = fieldW;
                    const float btnH = 56.0f;
                    const float shake = g_licenseInvalid
                        ? std::sin(g_licenseInvalidTimer * 48.0f) * 5.0f *
                          (1.0f - ImClamp(g_licenseInvalidTimer / 0.8f, 0.0f, 1.0f))
                        : 0.0f;
                    const ImVec2 bMin(fieldX + shake, pos.y + 262.0f);
                    const ImVec2 bMax(bMin.x + btnW, bMin.y + btnH);

                    ImGui::SetCursorScreenPos(bMin);
                    const bool pressed = ImGui::InvisibleButton("##login_activate", ImVec2(btnW, btnH));
                    const bool hovered = ImGui::IsItemHovered();

                    // soft accent glow (Lumin primary_button shadow)
                    draw->AddRectFilled(ImVec2(bMin.x + 10.0f, bMin.y + 9.0f), ImVec2(bMax.x - 10.0f, bMax.y + 9.0f),
                                        IM_COL32(255, 90, 92, 38), 16.0f);
                    const ImU32 slab = g_loginInFlight ? IM_COL32(196, 72, 74, 255)
                                      : (hovered ? IM_COL32(255, 108, 110, 255) : zenin::T::Accent);
                    draw->AddRectFilled(bMin, bMax, slab, 12.0f);

                    const char *label = "ACTIVATE LICENSE";
                    const float fs = 15.0f;
                    const ImVec2 ts = mediumFont ? mediumFont->CalcTextSizeA(fs, FLT_MAX, 0.0f, label)
                                                 : ImGui::CalcTextSize(label);
                    drawTextL(label, ImVec2((bMin.x + bMax.x - ts.x) * 0.5f, (bMin.y + bMax.y - ts.y) * 0.5f),
                              IM_COL32(22, 16, 16, 255), fs);

                    if (pressed) {
                        if (g_loginInFlight) {
                            // already verifying; ignore re-trigger
                        } else if (s[0] == '\0') {
                            LuminFlagInvalid("License key required");
                        } else {
                            g_licenseInvalid = false;
                            g_licenseErrorMsg.clear();
                            StartLoginAttempt(s);
                        }
                    }
                }

                // ── secondary actions ─────────────────────────────────────────
                {
                    const float secW = (fieldW - 12.0f) * 0.5f;
                    const float secY = pos.y + 336.0f;
                    if (drawSecondaryButton("PASTE KEY", ImVec2(fieldX, secY),
                                            ImVec2(fieldX + secW, secY + 44.0f))) {
                        auto key = getClipboard();
                        strncpy(s, key.c_str(), sizeof(s) - 1);
                        s[sizeof(s) - 1] = '\0';
                    }
                    if (drawSecondaryButton("CLEAR", ImVec2(fieldX + secW + 12.0f, secY),
                                            ImVec2(fieldX + fieldW, secY + 44.0f))) {
                        s[0] = '\0';
                        showKeyboard = false;
                    }
                }

                // ── in-flight / result handling ───────────────────────────────
                if (g_loginInFlight) {
                    ImGui::SetCursorPos(ImVec2(26.0f, 392.0f));
                    ImGui::TextColored(ImVec4(0.56f, 0.56f, 0.62f, 1.0f), "Authorizing...");
                }

                // When the in-flight login completes, act on the result.
                const std::string loginResult = TakeLoginResult();
                if (g_loginInFlight && !loginResult.empty()) {
                    g_loginInFlight = false;
                    err = loginResult;
                    if (err == "OK") {
                        showKeyboard = false;
                        strncpy(logintext, s, sizeof(logintext) - 1);
                        logintext[sizeof(logintext) - 1] = '\0';
                        SaveLoginTextToFile(s);
                        g_LoginTextLoaded = true;
                        err.clear();
                        isLogin = true;
                        g_ShowRadialMenu = true;
                        ApplyForbidKickOffOnLogin();
                    }
                }

                // ── footer status line ────────────────────────────────────────
                {
                    draw->AddLine(ImVec2(fieldX, pos.y + 426.0f), ImVec2(fieldX + fieldW, pos.y + 426.0f),
                                  IM_COL32(44, 44, 50, 200), 1.0f);
                    drawTextL("single key / single device - do not share",
                              ImVec2(fieldX, pos.y + 434.0f), IM_COL32(96, 96, 104, 255), 11.0f);
                }

                // Advance the invalid-shake decay timer (Lumin license_invalid).
                if (g_licenseInvalid) {
                    g_licenseInvalidTimer += ImGui::GetIO().DeltaTime;
                    if (g_licenseInvalidTimer >= 0.8f) {
                        g_licenseInvalid = false;
                        g_licenseInvalidTimer = 0.0f;
                        g_licenseErrorMsg.clear();
                    }
                }

                if (showKeyboard) RenderVirtualKeyboard("##VirtualKeyboardLogin", s, sizeof(s), &showKeyboard);
            }
            ImGui::End();
        }
        else
        {
            // ================================================================
            // EXCLUSIVE VIEW MODE 
            // g_ShowRadialMenu == true  → PIZZA menu lang nakikita
            // g_ShowRadialMenu == false → CONTENT box lang nakikita
            // ================================================================
            // The floating hub launcher is gone. Every category is now a tab in
            // the menu window's own sidebar, so the content box is always shown
            // once the login succeeds -- no separate floating GUI to drag.
            {
                // ---------- CONTENT BOX (exclusive) ----------
                static bool loggedContentFirstFrame = false;
                if (!loggedContentFirstFrame) { loggedContentFirstFrame = true; LOGI("ui: content box first frame, tab=%d", activeTab); }
                uncollapseOpenAnim = ImClamp(uncollapseOpenAnim + ImGui::GetIO().DeltaTime * 5.0f, 0.0f, 1.0f);
                float openEase = uncollapseOpenAnim * uncollapseOpenAnim * (3.0f - 2.0f * uncollapseOpenAnim);
                float openAlpha = 0.2f + 0.8f * openEase;
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, openAlpha);

                ImVec2 mainWindowSize = ImVec2(1120.f, 640.f);
                // Leave a margin so the accent edge/border is never clipped by
                // the screen edge, and never go below a usable minimum.
                mainWindowSize.x = ImMin(mainWindowSize.x, ImMax(360.0f, displaySize.x - 16.0f));
                mainWindowSize.y = ImMin(mainWindowSize.y, ImMax(240.0f, displaySize.y - 16.0f));

                // Centered on first open, then free-draggable: grab the header and
                // the dropped position is remembered in ui_layout.ini.
                static ImVec2 menuWindowPos(0.0f, 0.0f);
                static ImVec2 menuPosOnDisk(-99999.0f, -99999.0f);
                static bool menuPosInit = false;
                if (!menuPosInit)
                {
                    menuPosInit = true;
                    const ui_layout::State& layout = ui_layout::Get();
                    const ImVec2 centeredPos((displaySize.x - mainWindowSize.x) * 0.5f,
                                             (displaySize.y - mainWindowSize.y) * 0.5f);
                    menuWindowPos = centeredPos;
                    if (layout.hasMenu)
                    {
                        // Only honour a remembered position when the whole card
                        // still fits on screen; a position written by an older
                        // build (different window size) would otherwise park the
                        // menu half off the display.
                        const bool fits = layout.menuX >= -0.5f && layout.menuY >= -0.5f &&
                                          layout.menuX + mainWindowSize.x <= displaySize.x + 0.5f &&
                                          layout.menuY + mainWindowSize.y <= displaySize.y + 0.5f;
                        if (fits)
                        {
                            menuWindowPos = ImVec2(layout.menuX, layout.menuY);
                            menuPosOnDisk = menuWindowPos;
                        }
                    }
                    menuWindowPos = ModernUI::ClampMenuPos(menuWindowPos, mainWindowSize, displaySize);
                    ImGui::SetNextWindowPos(menuWindowPos, ImGuiCond_Always);
                }
                ImGui::SetNextWindowSize(mainWindowSize, ImGuiCond_Always);
                ImGui::SetNextWindowBgAlpha(0.0f);

                ImGui::Begin("@EthnirNoir", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
                {
                    runtime_preview_menu::StateRefs runtimeState{dark, tabAlpha, tabAdd, page, activeTab, windowCollapsed, isMenuVisible, collapseBarLastActiveTime, collapseBarOpacityAnim, collapseBarPressAnim};
                    {
                        using namespace runtime_preview_menu;
                        main_runtime_theme::ApplyAccentFromHue();

                        ImGuiStyle *runtimeStyle = &ImGui::GetStyle();
                        c::ApplyMainWindowStyle(*runtimeStyle);
                        c::UpdateTheme(runtimeState.dark, menu, ImGui::GetIO().DeltaTime);
                        main_runtime_theme::ApplyThemeState();
                        main_runtime_theme::applyZeninStandardStyle();

                        const ImVec2 runtimeWindowSize = ImGui::GetWindowSize();
                        ImVec2 runtimeWindowPos = ImGui::GetWindowPos();
                        menuWindowPos = runtimeWindowPos;
                        ImDrawList *runtimeDrawList = ImGui::GetWindowDrawList();

                        // Per-frame rescue: if the card is (partly) off-screen --
                        // a stale saved position, a rotation, or a resolution
                        // change -- pull it fully back into view.
                        {
                            const ImVec2 fixed = ModernUI::ClampMenuPos(runtimeWindowPos, runtimeWindowSize, displaySize);
                            if (fixed.x != runtimeWindowPos.x || fixed.y != runtimeWindowPos.y) {
                                menuWindowPos = fixed;
                                ImGui::SetWindowPos(menuWindowPos, ImGuiCond_Always);
                                runtimeWindowPos = fixed;
                            }
                        }

                        // Zenin window skin: dark charcoal card, thin grey
                        // border, red accent top edge (matches the reference UI).
                        runtimeDrawList->AddRectFilled(runtimeWindowPos,
                            ImVec2(runtimeWindowPos.x + runtimeWindowSize.x, runtimeWindowPos.y + runtimeWindowSize.y),
                            zenin::T::WindowBg, zenin::T::RWindow);
                        runtimeDrawList->AddRect(runtimeWindowPos,
                            ImVec2(runtimeWindowPos.x + runtimeWindowSize.x, runtimeWindowPos.y + runtimeWindowSize.y),
                            IM_COL32(38, 38, 42, 230), zenin::T::RWindow, 0, 1.3f);
                        runtimeDrawList->AddRectFilled(runtimeWindowPos,
                            ImVec2(runtimeWindowPos.x + runtimeWindowSize.x, runtimeWindowPos.y + 3.0f),
                            zenin::T::Accent, zenin::T::RWindow, ImDrawFlags_RoundCornersTop);
runtimeDrawList->AddRectFilledMultiColor(runtimeWindowPos, ImVec2(runtimeWindowPos.x + runtimeWindowSize.x, runtimeWindowPos.y + 110.0f), IM_COL32(24, 24, 26, 210), IM_COL32(24, 24, 26, 0), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));

                        // ── Layout ──────────────────────────────────────────────
                        // Ported from the eliwoahzja/imgui reference menu: a
                        // full-width top bar, a left page rail holding every
                        // category as a tab, and the content container on the
                        // right. The old centred floating hub is gone.
                        const float topbarHeight   = 60.0f;
                        const float sidebarWidth   = 178.0f;
                        const float railPad        = 12.0f;
                        const float gap            = 12.0f;
                        const float contentPadding = 12.0f;
                        const float columnGap      = 10.0f;

                        const float cardLeft   = runtimeWindowPos.x;
                        const float cardTop    = runtimeWindowPos.y;
                        const float cardRight  = runtimeWindowPos.x + runtimeWindowSize.x;
                        const float cardBottom = runtimeWindowPos.y + runtimeWindowSize.y;

                        const ImVec2 topbarMin(cardLeft, cardTop);
                        const ImVec2 topbarMax(cardRight, cardTop + topbarHeight);

                        const float sidebarTop   = cardTop + topbarHeight;
                        const float sidebarLeft  = cardLeft + railPad;
                        const float sidebarRight = sidebarLeft + sidebarWidth;

                        const ImVec2 hostMin(sidebarRight + gap, sidebarTop + gap);
                        const ImVec2 hostMax(cardRight - gap, cardBottom - gap);
                        const float containerHeaderH = 30.0f;
                        const ImVec2 contentInnerMin(hostMin.x + contentPadding,
                                                     hostMin.y + containerHeaderH + contentPadding);
                        const ImVec2 contentInnerSize(ImMax(0.0f, (hostMax.x - hostMin.x) - contentPadding * 2.0f),
                                                      ImMax(0.0f, (hostMax.y - hostMin.y) - containerHeaderH - contentPadding * 2.0f));

                        runtimeState.page = ImClamp(runtimeState.page, 1, 6);
                        runtimeState.activeTab = ImClamp(runtimeState.activeTab, 1, 6);

                        // ── Top bar ─────────────────────────────────────────────
                        runtimeDrawList->AddRectFilled(topbarMin, topbarMax, IM_COL32(13, 13, 15, 255),
                                                        zenin::T::RWindow, ImDrawFlags_RoundCornersTop);
                        runtimeDrawList->AddLine(ImVec2(cardLeft, topbarMax.y), ImVec2(cardRight, topbarMax.y),
                                                 zenin::T::Accent, 1.6f);

                        const ImVec2 markCenter(cardLeft + 38.0f, cardTop + topbarHeight * 0.5f);
                        runtimeDrawList->AddCircleFilled(markCenter, 18.0f, IM_COL32(24, 24, 26, 255), 32);
                        runtimeDrawList->AddCircle(markCenter, 18.0f, zenin::T::Accent, 32, 1.4f);
                        {
                            ImFont *iconFont = custom::shell::GetIconFont();
                            const char *logoIcon = ICON_FA_FIRE;
                            const ImVec2 logoSize = custom::shell::MeasureText(iconFont, 17.0f, logoIcon);
                            if (iconFont)
                                runtimeDrawList->AddText(iconFont, 17.0f,
                                    ImVec2(markCenter.x - logoSize.x * 0.5f, markCenter.y - logoSize.y * 0.5f),
                                    zenin::T::Accent, logoIcon);
                        }

                        ImFont *runtimeTitleFont = custom::shell::GetTitleFont();
                        const float runtimeTitleSize = 20.0f;
                        const char *titleA = "ZENIN";
                        const char *titleB = " | ETHNIR NOIR V3";
                        const ImVec2 titleASize = runtimeTitleFont->CalcTextSizeA(runtimeTitleSize, FLT_MAX, 0.0f, titleA);
                        runtimeDrawList->AddText(runtimeTitleFont, runtimeTitleSize,
                            ImVec2(cardLeft + 66.0f, cardTop + 11.0f), IM_COL32(236, 236, 240, 255), titleA);
                        runtimeDrawList->AddText(runtimeTitleFont, runtimeTitleSize,
                            ImVec2(cardLeft + 66.0f + titleASize.x, cardTop + 11.0f), zenin::T::Accent, titleB);

                        static const char *catNames[] = { "VISUAL", "COMBAT", "MEMORY", "SKINS", "MISC", "SETTINGS" };
                        char currentCat[64];
                        snprintf(currentCat, sizeof(currentCat), "current: %s", catNames[runtimeState.activeTab - 1]);
                        runtimeDrawList->AddText(runtimeTitleFont, 10.0f,
                            ImVec2(cardLeft + 67.0f, cardTop + 36.0f), IM_COL32(138, 138, 146, 235), currentCat);

                        // Header actions: BACK / SAVE / HIDE, right aligned before
                        // the round power button.
                        const float hBtnW = 74.0f;
                        const float hBtnH = 34.0f;
                        const float hBtnGap = 6.0f;
                        const float hBtnY = cardTop + (topbarHeight - hBtnH) * 0.5f;
                        const float hGroupW = hBtnW * 3.0f + hBtnGap * 2.0f;
                        const float hGroupStartX = cardRight - 58.0f - hGroupW;

                        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

                        auto headerButton = [&](const char *label, const ImVec2 &pos, const ImVec2 &size, const char *tip) -> bool {
                            ImGui::SetCursorScreenPos(pos);
                            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.141f, 0.141f, 0.149f, 0.95f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.196f, 0.196f, 0.208f, 1.00f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.247f, 0.247f, 0.259f, 1.00f));
                            ImGui::PushStyleColor(ImGuiCol_Border,        ImVec4(0.278f, 0.278f, 0.290f, 1.00f));
                            ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.878f, 0.878f, 0.894f, 1.00f));
                            const bool pressed = ImGui::Button(label, size);
                            if (tip != nullptr && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tip);
                            ImGui::PopStyleColor(5);
                            return pressed;
                        };

                        if (headerButton("<- BACK", ImVec2(hGroupStartX, hBtnY), ImVec2(hBtnW, hBtnH),
                                         "Back to the login screen")) {
                            // There is no floating hub to go back to any more;
                            // BACK returns to the license screen.
                            isLogin = false;
                            showKeyboard = false;
                        }
                        headerButton("SAVE", ImVec2(hGroupStartX + hBtnW + hBtnGap, hBtnY), ImVec2(hBtnW, hBtnH),
                                     "Save configuration");
                        if (headerButton("HIDE", ImVec2(hGroupStartX + (hBtnW + hBtnGap) * 2.0f, hBtnY), ImVec2(hBtnW, hBtnH),
                                         "Hide (double-tap the top middle to restore)")) {
                            windowCollapsed = true;
                            isMenuVisible = false;
                        }

                        ImGui::PopStyleVar(2);

                        // Round power button (closes the menu card).
                        {
                            const ImVec2 closeCenter(cardRight - 32.0f, cardTop + topbarHeight * 0.5f);
                            ImGui::SetCursorScreenPos(ImVec2(closeCenter.x - 16.0f, closeCenter.y - 16.0f));
                            const bool closePressed = ImGui::InvisibleButton("##menu_close", ImVec2(32.0f, 32.0f));
                            const bool closeHovered = ImGui::IsItemHovered();
                            runtimeDrawList->AddCircle(closeCenter, 15.0f,
                                closeHovered ? zenin::T::Accent : IM_COL32(72, 72, 80, 255), 32, 1.6f);
                            ImFont *iconFont = custom::shell::GetIconFont();
                            if (iconFont) {
                                const ImVec2 gsz = custom::shell::MeasureText(iconFont, 13.0f, ICON_FA_POWER_OFF);
                                runtimeDrawList->AddText(iconFont, 13.0f,
                                    ImVec2(closeCenter.x - gsz.x * 0.5f, closeCenter.y - gsz.y * 0.5f),
                                    closeHovered ? zenin::T::Accent : IM_COL32(170, 170, 178, 255), ICON_FA_POWER_OFF);
                            }
                            if (closePressed)
                                runtime_preview_menu::CollapseMenu(runtimeState);
                        }

                        // ── Left page rail: every category is a tab here ────────
                        {
                            static const char *pageIcons[6] = {
                                ICON_FA_EYE, ICON_FA_CROSSHAIRS, ICON_FA_MICROCHIP,
                                ICON_FA_PALETTE, ICON_FA_MAGIC, ICON_FA_COG
                            };
                            static const char *pageLabels[6] = {
                                "VISUAL", "COMBAT", "MEMORY", "SKINS", "MISC", "SETTINGS"
                            };

                            const float rowH = 40.0f;
                            const float rowGap = 6.0f;
                            float ry = sidebarTop + 14.0f;

                            runtimeDrawList->AddText(runtimeTitleFont, 10.0f,
                                ImVec2(sidebarLeft + 6.0f, ry - 4.0f), IM_COL32(110, 110, 118, 255), "MENU");
                            ry += 16.0f;

                            for (int i = 0; i < 6; ++i) {
                                ImGui::SetCursorScreenPos(ImVec2(sidebarLeft, ry));
                                if (DrawSidebarPage(pageLabels[i], pageIcons[i], pageLabels[i],
                                                    runtimeState.activeTab == (i + 1), ImVec2(sidebarWidth, rowH))) {
                                    runtimeState.page = i + 1;
                                }
                                ry += rowH + rowGap;
                            }

                            runtimeDrawList->AddRectFilled(ImVec2(sidebarLeft + 6.0f, cardBottom - 46.0f),
                                                           ImVec2(sidebarRight - 6.0f, cardBottom - 44.0f),
                                                           IM_COL32(48, 48, 54, 200), 1.0f);
                            runtimeDrawList->AddText(runtimeTitleFont, 10.0f,
                                ImVec2(sidebarLeft + 6.0f, cardBottom - 38.0f),
                                IM_COL32(110, 110, 118, 255), "ethnir noir v3");
                        }

                        runtimeState.tabAlpha = ImClamp(runtimeState.tabAlpha + (4.0f * ImGui::GetIO().DeltaTime * (runtimeState.page == runtimeState.activeTab ? 1.0f : -1.0f)), 0.0f, 1.0f);
                        if (runtimeState.tabAlpha == 0.0f && runtimeState.tabAdd == 0.0f) runtimeState.activeTab = runtimeState.page;

                        runtimeDrawList->AddRectFilled(hostMin, hostMax, IM_COL32(24, 24, 27, 245), 12.0f);
                        runtimeDrawList->AddRect(hostMin, hostMax, IM_COL32(40, 40, 46, 190), 12.0f, 0, 1.0f);

                        // Container header: category name + accent underline.
                        {
                            ImFont *sectionFont = custom::shell::GetTitleFont();
                            const char *sectionName = catNames[runtimeState.activeTab - 1];
                            if (sectionFont) {
                                const ImVec2 ssz = sectionFont->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, sectionName);
                                runtimeDrawList->AddText(sectionFont, 15.0f,
                                    ImVec2(hostMin.x + contentPadding, hostMin.y + 8.0f),
                                    IM_COL32(236, 236, 240, 255), sectionName);
                                runtimeDrawList->AddRectFilled(
                                    ImVec2(hostMin.x + contentPadding, hostMin.y + 8.0f + ssz.y + 2.0f),
                                    ImVec2(hostMin.x + contentPadding + ssz.x, hostMin.y + 8.0f + ssz.y + 3.5f),
                                    zenin::T::Accent, 2.0f);
                            }
                        }

                        ImGui::SetCursorScreenPos(contentInnerMin);
                        ImGui::BeginChild("##RuntimeContentHost", contentInnerSize, false, ImGuiWindowFlags_NoBackground);
                        {
                            const bool pushedContentFont = (font::inter_semibold != nullptr);
                            if (pushedContentFont) ImGui::PushFont(font::inter_semibold);
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, runtimeState.tabAlpha * runtimeStyle->Alpha);

                            const ImVec2 contentRegion = ImGui::GetContentRegionAvail();
                            const float childWidth = ImMax(0.0f, (contentRegion.x - columnGap) * 0.5f);

                            // Panic row: clears the current tab (or every tab).
                            DrawTabPanicBar(runtimeState.activeTab);
                            const float childHeight = ImMax(0.0f, ImGui::GetContentRegionAvail().y);

                            if (runtimeState.activeTab == 1)   // VISUAL
                            {
                                const float espGap = 8.0f;
                                const float espAvailW = ImMax(1.0f, contentRegion.x - espGap * 2.0f);
                                const float espSettingsW = ImFloor(espAvailW * 0.40f);
                                const float espOptionsW = ImFloor(espAvailW * 0.32f);
                                const float espPreviewW = ImMax(1.0f, espAvailW - espSettingsW - espOptionsW);

                                custom::BeginGroup();
                                {
                                    const ChildFrame settings = BeginContentChild("ESP SETTINGS##RUNTIME_ESP", ImVec2(espSettingsW, childHeight));
                                    custom::Checkbox("Hide Stream", &HIDEESP);
                                    custom::Checkbox("ESP Line", &Config.ESPMenu.isPlayerLine);
                                    custom::Checkbox("ESP Box", &Config.ESPMenu.Box);
                                    custom::Checkbox("ESP Skeleton", &Config.ESPMenu.Skeleton);
                                    custom::Checkbox("ESP Health", &Config.ESPMenu.Health);
                                    custom::Checkbox("ESP Name", &Config.ESPMenu.Name);
                                    custom::Checkbox("ESP Distance", &Config.ESPMenu.Distance);
                                    custom::Checkbox("ESP Count", &Config.ESPMenu.Count);
                                    custom::Checkbox("360 Alert", &Config.ESPMenu.Alert);
                                    custom::Checkbox("Show AimLine", &Config.ESPMenu.Aimline);
                                    custom::Checkbox("Yellow Wallhack", &Config.ExtraMenu.WallHack);
                                    custom::Checkbox("Red Wallhack", &Config.ExtraMenu.RedWallhack);
                                    ImGui::Dummy(ImVec2(0.0f, 5.0f));
                                    ImGui::TextColored(main_runtime_theme::GetAccentVec4(), "ESP COLORS");
                                    runtime_preview_menu::DrawRuntimeEspColorRow("Player ESP Color", Config.sColorsESPPLAYER.LinePLAYER);
                                    runtime_preview_menu::CopyLinkedEspColors(Config.sColorsESPPLAYER.LinePLAYER, Config.sColorsESPPLAYER.BoxPLAYER, Config.sColorsESPPLAYER.NamePLAYER, Config.sColorsESPPLAYER.HealthPLAYER, Config.sColorsESPPLAYER.DistancePLAYER, Config.sColorsESPPLAYER.SkeletonPLAYER);
                                    runtime_preview_menu::DrawRuntimeEspColorRow("Bot ESP Color", Config.sColorsESPBOT.LineBOT);
                                    runtime_preview_menu::CopyLinkedEspColors(Config.sColorsESPBOT.LineBOT, Config.sColorsESPBOT.BoxBOT, Config.sColorsESPBOT.NameBOT, Config.sColorsESPBOT.HealthBOT, Config.sColorsESPBOT.DistanceBOT, Config.sColorsESPBOT.SkeletonBOT);
                                    EndContentChild(settings);
                                }
                                custom::EndGroup();

                                ImGui::SameLine(0.0f, espGap);

                                custom::BeginGroup();
                                {
                                    const ChildFrame options = BeginContentChild("DISPLAY OPTIONS##RUNTIME_DISPLAY_OPTIONS", ImVec2(espOptionsW, childHeight));
                                    static const char *boxTypes[] = {"Fill", "Outline", "Corner", "3D"};
                                    static const char *linePositions[] = {"Top", "Mid", "Bottom"};
                                    static const char *healthPositions[] = {"Top", "Side"};
                                    static const char *espStyles[] = {"None", "3D Sphere", "Player Signal"};
                                    custom::Combo("Box Type", (int *)&Config.ESPMenu.BoxType, boxTypes, IM_ARRAYSIZE(boxTypes), -1);
                                    custom::Combo("Line Position", (int *)&Config.ESPMenu.Target, linePositions, IM_ARRAYSIZE(linePositions), -1);
                                    custom::Combo("Health Position", (int *)&Config.ESPMenu.HealthPosition, healthPositions, IM_ARRAYSIZE(healthPositions), -1);
                                    custom::Combo("ESP Style", (int *)&Config.ESPMenu.EspStyle, espStyles, IM_ARRAYSIZE(espStyles), -1);
                                    ImGui::Dummy(ImVec2(0.0f, 8.0f));
                                    ImGui::TextColored(main_runtime_theme::GetAccentVec4(), "EXTRA OPTIONS");
                                    ImGui::TextUnformatted("Player ESP Color");
                                    ImGui::TextUnformatted("Bot ESP Color");
                                    EndContentChild(options);
                                }
                                custom::EndGroup();

                                ImGui::SameLine(0.0f, espGap);
                                runtime_preview_menu::DrawRuntimeEspPreviewPanel(ImGui::GetCursorScreenPos(), ImVec2(espPreviewW, childHeight));
                                ImGui::Dummy(ImVec2(espPreviewW, childHeight));
                            }

                            if (runtimeState.activeTab == 2)   // COMBAT
                            {
                                custom::BeginGroup();
                                {
                                    const ChildFrame left = BeginContentChild("AIMBOT", ImVec2(childWidth, childHeight));
                                    custom::Checkbox("Aimbot 360", &Config.Aim.Aimbot360);
                                    custom::Checkbox("Bullet Track", &Config.Aim.AimSilent);
                                    custom::SliderFloat("Aim Assist Size", &Config.Aim.AimAssistSize, 0.0f, 100.0f, "%.0f");
                                    EndContentChild(left);
                                }
                                custom::EndGroup();

                                ImGui::SameLine(0.0f, 10.0f);

                                custom::BeginGroup();
                                {
                                    const ChildFrame right = BeginContentChild("COMBAT OPTIONS", ImVec2(childWidth, childHeight));
                                    static const char *targets[] = {"Head", "Chest", "Body"};
                                    custom::Combo("Location", (int *)&Config.Aim.Target, targets, IM_ARRAYSIZE(targets), -1);
                                    static const char *triggers[] = {"None", "Shooting", "Scoping"};
                                    custom::Combo("Trigger", (int *)&Config.Aim.Trigger, triggers, IM_ARRAYSIZE(triggers), -1);
                                    static const char *targetBy[] = {"Distance", "FOV"};
                                    custom::Combo("Target By", (int *)&Config.Aim.By, targetBy, IM_ARRAYSIZE(targetBy), -1);
                                    custom::SliderFloat("FOV Size", &Config.Aim.Cross, 0.0f, 100.0f, "%.0f");

                                    static const char *hitGroups[] = {"Auto", "Head", "Hand", "Body", "Foot", "Weak Point", "Neck"};
                                    custom::Combo("Hit Group", &Config.Aim.HitGroup, hitGroups, IM_ARRAYSIZE(hitGroups), -1);

                                    // Triggerbot ("A-Fire"). Rewritten so it actually fires:
                                    // the enemy only has to be inside a generous crosshair zone,
                                    // there is no line-of-sight probe and no distance limit, so
                                    // it works against a target at any range on the map.
                                    custom::Checkbox("Triggerbot", &Config.ExtraMenu.A_Fire);
                                    static const char *aFireCriteria[] = {"Crosshair", "Scoping", "Shooting"};
                                    custom::Combo("Triggerbot Mode", &Config.ExtraMenu.A_FireTrigger, aFireCriteria, IM_ARRAYSIZE(aFireCriteria), -1);
                                    if (Config.ExtraMenu.A_Fire) {
                                        custom::SliderFloat("Shot Delay", &Config.ExtraMenu.A_FireDelay, 0.0f, 1.0f, "%.2fs");
                                        ImGui::TextColored(c::text::text, "Fires at any range - no distance limit");
                                    }
                                    EndContentChild(right);
                                }
                                custom::EndGroup();
                            }

                            if (runtimeState.activeTab == 3)   // MEMORY
                            {
                                custom::BeginGroup();
                                {
                                    const ChildFrame left = BeginContentChild("MEMORY HACKS", ImVec2(childWidth, childHeight));
                                    custom::Checkbox("Hitbox", &Config.ExtraMenu.Hit);
                                    if (Config.ExtraMenu.Hit) {
                                        custom::SliderFloat("Hitbox Size", &Config.ExtraMenu.HitboxScale, 0.5f, 25.0f, "%.1fm");
                                        ImGui::TextColored(c::text::text, "Enlarges enemy body + head hitbox");
                                        ImGui::TextUnformatted("Headshots automatic when near head");
                                    }
                                    custom::Checkbox("No Recoil", &Config.ExtraMenu.Recoil);
                                    custom::Checkbox("No Spread", &Config.ExtraMenu.Spread);
                                    custom::Checkbox("No Shake", &Config.ExtraMenu.Shake);
                                    custom::Checkbox("No Overheat", &Config.ExtraMenu.Rpd);
                                    custom::Checkbox("No Parachute", &Config.ExtraMenu.Parachute);
                                    custom::Checkbox("Anti Flashbang", &Config.ExtraMenu.Flash);
                                    custom::Checkbox("Firerate", &Config.ExtraMenu.Fire);
                                    custom::Checkbox("No Sprint-Fire Delay", &Config.ExtraMenu.NoSprintFireDelay);
                                    custom::Checkbox("Fast Dive", &Config.ExtraMenu.Diving);
                                    custom::Checkbox("Fast Reload", &Config.ExtraMenu.Reload);
                                    custom::Checkbox("Fast Scope", &Config.ExtraMenu.Scope);
                                    custom::Checkbox("Quick Switch", &Config.ExtraMenu.Switch);
                                    custom::Checkbox("Weapon Kinetic", &Config.ExtraMenu.Kinetic);
                                    custom::Checkbox("No Crouch", &Config.ExtraMenu.NoCrouch);
                                    EndContentChild(left);
                                }
                                custom::EndGroup();

                                ImGui::SameLine(0.0f, 10.0f);

                                custom::BeginGroup();
                                {
                                    const ChildFrame right = BeginContentChild("MISC FEATURES", ImVec2(childWidth, childHeight));
                                    custom::SliderFloat("Snowboard Speed", &SnowBsize, 0.0f, 100.0f, "%.1f");
                                    custom::SliderFloat("Slide Distance", &SlideRange, 0.0f, 30.0f, "%.1f");
                                    custom::SliderFloat("SpeedHack", &speedHackMultiplier, 0.5f, 2.0f, "%.1fx");
                                    custom::SliderFloat("High Jump", &jumpHeightMultiplier, 0.5f, 5.0f, "%.2fx");

                                    custom::Checkbox("Report Spoof", &Config.ExtraMenu.ReportSpoof);
                                    {
                                        static char reportUidBuf[24] = "";
                                        ImGui::TextUnformatted("Report To User ID");
                                        ImGui::AstralInput("##report_uid", reportUidBuf, sizeof(reportUidBuf), ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 40.0f), nullptr);
                                        if (ImGui::IsItemClicked()) MiscTextFieldFocus("##VirtualKeyboardReportUid", reportUidBuf, sizeof(reportUidBuf));
                                        Config.ExtraMenu.ReportSpoofTargetId = strtoull(reportUidBuf, nullptr, 10);

                                        static char reportNameBuf[32] = "";
                                        ImGui::TextUnformatted("Show As Name (optional)");
                                        ImGui::AstralInput("##report_name", reportNameBuf, sizeof(reportNameBuf), ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 40.0f), nullptr);
                                        if (ImGui::IsItemClicked()) MiscTextFieldFocus("##VirtualKeyboardReportName", reportNameBuf, sizeof(reportNameBuf));
                                        strncpy(Config.ExtraMenu.ReportSpoofName, reportNameBuf, sizeof(Config.ExtraMenu.ReportSpoofName) - 1);
                                        Config.ExtraMenu.ReportSpoofName[sizeof(Config.ExtraMenu.ReportSpoofName) - 1] = '\0';

                                        // One-shot picker: next frame grabs the highlighted enemy's id+name.
                                        if (Config.ExtraMenu.ReportSpoofPickEnemy) {
                                            PickEnemyForSpoofOrRename();
                                        }
                                        if (ImGui::Button("PICK FROM ENEMY", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 30.0f))) {
                                            Config.ExtraMenu.ReportSpoofPickEnemy = true;
                                        }
                                        if (ImGui::IsItemHovered()) {
                                            ImGui::SetTooltip("Grab the highlighted enemy's name+id into the spoof fields.");
                                        }
                                        if (Config.ExtraMenu.ReportSpoofPickedId != 0) {
                                            ImGui::TextColored(c::text::text, "Last picked: %s (id %llu)",
                                                Config.ExtraMenu.ReportSpoofPickedName, (unsigned long long)Config.ExtraMenu.ReportSpoofPickedId);
                                        }
                                    }

                                    custom::Checkbox("Rename Card", &Config.ExtraMenu.RenameCard);
                                    {
                                        static char renameNameBuf[32] = "";
                                        static char renameCardGidBuf[12] = "0";
                                        ImGui::TextUnformatted("Name");
                                        ImGui::AstralInput("##rename_name", renameNameBuf, sizeof(renameNameBuf), ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 40.0f), nullptr);
                                        if (ImGui::IsItemClicked()) MiscTextFieldFocus("##VirtualKeyboardRename", renameNameBuf, sizeof(renameNameBuf));
                                        strncpy(Config.ExtraMenu.RenameCardName, renameNameBuf, sizeof(Config.ExtraMenu.RenameCardName) - 1);
                                        Config.ExtraMenu.RenameCardName[sizeof(Config.ExtraMenu.RenameCardName) - 1] = '\0';

                                        // One-shot picker: next frame grabs the highlighted enemy's name.
                                        if (Config.ExtraMenu.RenameCardPickEnemy) {
                                            PickEnemyForSpoofOrRename();
                                        }
                                        if (ImGui::Button("PICK FROM ENEMY", ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 30.0f))) {
                                            Config.ExtraMenu.RenameCardPickEnemy = true;
                                        }
                                        if (ImGui::IsItemHovered()) {
                                            ImGui::SetTooltip("Grab the highlighted enemy's name into the rename field.");
                                        }

                                        ImGui::TextUnformatted("Name Card GID (0 = none)");
                                        ImGui::AstralInput("##rename_gid", renameCardGidBuf, sizeof(renameCardGidBuf), ImVec2(ImGui::GetContentRegionAvail().x - 10.0f, 40.0f), nullptr);
                                        if (ImGui::IsItemClicked()) MiscTextFieldFocus("##VirtualKeyboardRenameGid", renameCardGidBuf, sizeof(renameCardGidBuf));
                                        Config.ExtraMenu.RenameCardGid = atoi(renameCardGidBuf);
                                        }

                                    custom::Separator_line();
                                    custom::Checkbox("Forbid Kick-Off", &Config.ExtraMenu.ForbidKickOff);
                                    if (Config.ExtraMenu.ForbidKickOff)
                                    {
                                        ImGui::Indent(12.0f);
                                        ImGui::TextColored(c::text::text, "Keep playing when the same account logs in elsewhere");
                                        ImGui::TextUnformatted("instead of being kicked / getting the");
                                        ImGui::TextUnformatted("logged in on a new device popup.");
                                        ImGui::Unindent(12.0f);
                                        custom::Checkbox("Forbid On Login", &Config.ExtraMenu.ForbidKickOffOnLogin);
                                        ImGui::SetItemTooltip("When enabled, the next successful login");
                                        ImGui::SetItemTooltip("activates the game's own forbid-kick-off");
                                        ImGui::SetItemTooltip("so you are not kicked for multi-device.");
                                    }
                                    custom::Separator_line();
                                    EndContentChild(right);
                                }
                                custom::EndGroup();
                            }

                            // Virtual keyboard for the MISC text fields: rendered once
                            // per frame after all windows so it draws on top and stays
                            // open for the focused field instead of flashing.
                            RenderMiscVirtualKeyboard();

                            if (runtimeState.activeTab == 4)   // SKINS
                            {
                                const ChildFrame skinChild = BeginContentChild("SKINS", ImVec2(contentRegion.x, childHeight));
                                RenderSkinCategoryContent(skinSubTab, true);
                                EndContentChild(skinChild);
                            }

                            if (runtimeState.activeTab == 5)   // MISC
                            {
                                const ChildFrame misc = BeginContentChild("MISC", ImVec2(contentRegion.x, childHeight));
                                const misc_tab::LayoutMetrics layout = misc_tab::CalculateLayout(contentRegion.x, runtime_preview_menu::g_activeChangelogTab);

                                {
                                    const ChildFrame changelog = BeginContentChild("CHANGELOG", ImVec2(contentRegion.x, layout.changelogHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                                    const float changelogTabGap = 5.0f;
                                    const float changelogTabH = 40.0f;
                                    const float changelogSafeRight = ImGui::GetStyle().ScrollbarSize + 12.0f;
                                    const float changelogAvailW = ImMax(1.0f, ImGui::GetContentRegionAvail().x - changelogSafeRight);
                                    const float changelogTabW = ImMax(1.0f, ImFloor((changelogAvailW - changelogTabGap * 2.0f) / 3.0f));
                                    for (int i = 0; i < IM_ARRAYSIZE(misc_tab::kChangelogTabs); ++i) {
                                        char buttonId[48] = {};
                                        std::snprintf(buttonId, sizeof(buttonId), "##changelog_%d", i);
                                        if (i > 0) ImGui::SameLine(0.0f, changelogTabGap);
                                        if (misc_tab::DrawChangelogTab(buttonId, misc_tab::kChangelogIcons[i], misc_tab::kChangelogTabs[i], runtime_preview_menu::g_activeChangelogTab == i, ImVec2(changelogTabW, changelogTabH))) {
                                            runtime_preview_menu::g_activeChangelogTab = i;
                                        }
                                    }
                                    misc_tab::ContentGap(12.0f);
                                    const misc_tab::ChangelogSelection activeSection = misc_tab::GetActiveChangelogSelection(runtime_preview_menu::g_activeChangelogTab);
                                    ImFont *activeIconFont = F107 ? F107 : ImGui::GetFont();
                                    ImGui::PushFont(activeIconFont);
                                    ImGui::TextColored(activeSection.color, "%s", activeSection.icon);
                                    ImGui::PopFont();
                                    ImGui::SameLine(0.0f, 8.0f);
                                    ImGui::TextColored(activeSection.color, "%s", activeSection.title);
                                    ImGui::SameLine(0.0f, 10.0f);
                                    ImGui::TextColored(c::text::text, "March 10, 2026");
                                    misc_tab::ContentGap(8.0f);
                                    misc_tab::DrawSectionItems(activeSection.items, activeSection.count);
                                    EndContentChild(changelog);
                                }

                                misc_tab::ContentGap(layout.rowGap);

                                {
                                    const ChildFrame info = BeginContentChild("INFO", ImVec2(contentRegion.x, layout.infoHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                                    misc_tab::DrawInfoSummary();
                                    EndContentChild(info);
                                }

                                misc_tab::ContentGap(layout.rowGap);

                                {
                                    const ChildFrame price = BeginContentChild("PRICELIST", ImVec2(contentRegion.x, layout.priceHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                                    misc_tab::DrawPriceSummary("##runtime_main_misc_prices");
                                    EndContentChild(price);
                                }

                                EndContentChild(misc);
                            }

                            if (runtimeState.activeTab == 6)   // SETTINGS
                            {
                                ImGuiStyle &style = ImGui::GetStyle();
                                const float settingsColumnGap = ImClamp(contentRegion.x * 0.022f, 12.0f, 18.0f);
                                const float settingsRowGap = 6.0f;
                                const float leftChildWidth = ImMax(0.0f, ImFloor((contentRegion.x - settingsColumnGap) * 0.5f));
                                const float rightChildWidth = ImMax(0.0f, contentRegion.x - settingsColumnGap - leftChildWidth);
                                const float topChildHeight = ImMax(0.0f, ImFloor((childHeight - settingsRowGap) * 0.5f));
                                const float bottomChildHeight = ImMax(0.0f, childHeight - settingsRowGap - topChildHeight);
                                const float startX = ImGui::GetCursorPosX();

                                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, settingsRowGap));

                                ImGui::SetCursorPosX(startX);
                                custom::BeginGroup();
                                {
                                    const ChildFrame licenseChild = BeginContentChild("LICENSE INFO", ImVec2(leftChildWidth, topChildHeight));
                                    settings_tab::RenderLicenseCard();
                                    EndContentChild(licenseChild);

                                    const ChildFrame logoChild = BeginContentChild("LOGO SETTINGS", ImVec2(leftChildWidth, bottomChildHeight));
                                    settings_tab::RenderLogoCard();
                                    EndContentChild(logoChild);
                                }
                                custom::EndGroup();

                                ImGui::SameLine(0.0f, settingsColumnGap);

                                custom::BeginGroup();
                                {
                                    const ChildFrame configChild = BeginContentChild("CONFIG MANAGEMENT", ImVec2(rightChildWidth, topChildHeight));
                                    settings_tab::RenderConfigCard();
                                    EndContentChild(configChild);

                                    const ChildFrame enhancementChild = BeginContentChild("ENHANCEMENT", ImVec2(rightChildWidth, bottomChildHeight));
                                    settings_tab::RenderEnhancementCard();
                                    EndContentChild(enhancementChild);
                                }
                                custom::EndGroup();

                                ImGui::PopStyleVar();
                            }

                            ImGui::PopStyleVar();
                            if (pushedContentFont) ImGui::PopFont();
                        }
                        ImGui::EndChild();
                        runtime_preview_menu::ResetPopupFocusWindow();
                        runtime_preview_menu::DrawPopupBackdropFocusLayer(ImGui::GetForegroundDrawList());
                    }

                    if (Config.ExtraMenu.WallHack) Patches.A1.Modify();
                    else Patches.A1.Restore();

                    static bool noCrouchPatched = false;
                    if (Config.ExtraMenu.NoCrouch != noCrouchPatched)
                    {
                        if (Config.ExtraMenu.NoCrouch)
                        {
                            Patches.NoCrouch.Modify();
                            Patches.NoCrouchPawn.Modify();
                            Patches.NoCrouchPlayerPawn.Modify();
                        }
                        else
                        {
                            Patches.NoCrouch.Restore();
                            Patches.NoCrouchPawn.Restore();
                            Patches.NoCrouchPlayerPawn.Restore();
                        }
                        noCrouchPatched = Config.ExtraMenu.NoCrouch;
                    }
                }
                ImGui::End();

                // Persist the container position once the drag is finished.
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                    (menuWindowPos.x != menuPosOnDisk.x || menuWindowPos.y != menuPosOnDisk.y))
                {
                    ui_layout::RememberMenu(menuWindowPos.x, menuWindowPos.y);
                    menuPosOnDisk = menuWindowPos;
                }

                ImGui::PopStyleVar();
            }
        }

        ImGui::PopStyleVar();
    }

    auto Input_get_touchCount = (int (*)())(Class_Input_get_touchCount);
    if (Input_get_touchCount() > 0)
    {
        auto Input_GetTouch = (Touch(*)(uintptr_t, int))(Class_Input_GetTouch);
        auto Input_get_mousePosition = (Vector3(*)(uintptr_t))(Class_Input_get_mousePosition);
        Touch t = Input_GetTouch(Config.ImGuiMenu.thiz, 0);
        Vector3 mp = Input_get_mousePosition(Config.ImGuiMenu.thiz);
        float mx = mp.x;
        float my = (float)get_height() - mp.y;

        switch (t.m_Phase)
        {
        case TouchPhase::Began:
        case TouchPhase::Stationary:
            io->MouseDown[0] = true;
            io->MousePos     = ImVec2(mx, my);
            break;
        case TouchPhase::Moved:
            io->MouseDown[0] = true;
            io->MousePos     = ImVec2(mx, my);
            break;
        case TouchPhase::Ended:
        case TouchPhase::Canceled:
            io->MouseDown[0] = false;
            g_clearMousePos  = true;
            break;
        default:
            break;
        }
    }
    else
    {
        if (g_clearMousePos) {
            io->MousePos    = ImVec2(-FLT_MAX, -FLT_MAX);
            g_clearMousePos = false;
        }
        io->MouseDown[0] = false;
    }

    UpdateCamoOverride();
    ApplyWorldVisualsRuntime();
    InstallBRClassEspConfigHook();

    ImGui::EndFrame();
    ImGui::Render();

    if (HIDEESP && old_eglSwapBuffers != nullptr) {
        EGLDisplay eglDpy = (EGLDisplay)dpy;
        EGLSurface eglSurface = (EGLSurface)surface;
        EGLContext gameCtx = eglGetCurrentContext();
        ZEL_Render(ImGui::GetDrawData(), eglDpy, eglSurface, gameCtx, (android::HideRecAImGui::SwapBuffersFn)old_eglSwapBuffers);
    } else {
        ZEL_Shutdown();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return old_eglSwapBuffers(dpy, surface);
}

size_t hook_strlen(const char *thread)
{
    if (strstr(thread, "eglSwapBuffers")) { }
    return strlen(thread);
}

void Init_Thread2()
{
    InitializeProtection();
}

void Init_SwapHook() { }

void Init_Thread()
{
    while (!m_unity)
    {
        m_unity = Tools::GetBaseAddress("libunity.so");
        sleep(1);
    }
    LOGI("libunity.so: %p", m_unity);
    UpdateAllOffset();

    Patches.A1 = MemoryPatch::createWithHex("libunity.so", 0x8D781DC, "1F 20 03 D5 E0 03 13 AA");
    Patches.NoCrouch = MemoryPatch::createWithHex("libunity.so",0x511F50C,"00 00 80 D2 C0 03 5F D6");
    Patches.NoCrouchPawn = MemoryPatch::createWithHex("libunity.so",0x524D15C,"00 00 80 D2 C0 03 5F D6");
    Patches.NoCrouchPlayerPawn = MemoryPatch::createWithHex("libunity.so",0x54B6820,"00 00 80 D2 C0 03 5F D6");
    Patches.NoWingsuit = MemoryPatch::createWithHex("libunity.so",0x54A12F4,"00 00 80 D2 C0 03 5F D6");

    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0xC9B6F90), (void *)&WeaponFireComponent_Instant_CreateBulletLine, (void **)&oWeaponFireComponent_Instant_CreateBulletLine);
    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0xC9C33A4), (void *)&WeaponFireComponent_Instant_CreateBulletProjectile, (void **)&oWeaponFireComponent_Instant_CreateBulletProjectile);
    InstallBRClassEspConfigHook();
    InitializeAllHooks();

MemoryPatch::createWithHex("libanogs.so", 0x204218, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x20A39C, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x20AE78, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x25FA98, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x2AA880, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x2B63E4, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x36F0AC, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x334310, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x39AE94, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x44BC90, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x491EDC, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x497E64, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x4B68A0, "00 00 80 D2 C0 03 5F D6").Modify();
MemoryPatch::createWithHex("libanogs.so", 0x4B9C10, "00 00 80 D2 C0 03 5F D6").Modify();

    auto swapBuffers = ((uintptr_t)DobbySymbolResolver(OBFUSCATE("libunity.so"), OBFUSCATE("eglSwapBuffers")));
    KittyMemory::ProtectAddr((void *)swapBuffers, sizeof(swapBuffers), PROT_READ | PROT_WRITE | PROT_EXEC);
    xhook_enable_debug(0);
    xhook_register(OBFUSCATE(".*libunity\\.so$"), OBFUSCATE("eglSwapBuffers"), (void*)hook_eglSwapBuffers, (void**)&old_eglSwapBuffers);
    if (xhook_refresh(0) == 0) { xhook_clear(); }
}

__attribute__((constructor))
void native_Init(JNIEnv *env, jclass clazz, jobject mContext) { }

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    jvm = vm;
    VM = vm;
    ZEL_SetVM(vm);
    std::thread(Init_Thread).detach();
    std::thread(Init_Thread2).detach();
    std::thread(Skins_Thread).detach();
    return JNI_VERSION_1_6;
}