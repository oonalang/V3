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

std::vector<sRegion> trapRegions;
uintptr_t address = 0;
std::string md5(std::string s);
uintptr_t g_il2cpp;
static bool isMenuVisible = true;

// ================= PIZZA MENU  =================
static bool g_ShowRadialMenu = true;

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

    int RenderCategoryWheel(const ImVec2& viewportCenter)
    {
        const float windowSize  = 460.0f;
        const float outerRadius = 195.0f;
        const float hubRadius   = 55.0f;
        constexpr float kPi = 3.14159265358979323846f;
        const int sliceCount = 6;
        const char* labels[sliceCount] = { "VISUAL", "COMBAT", "MEMORY", "SKINS", "MISC", "SETTINGS" };

        ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
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

            const char* hub = "JAREDAX";
            ImVec2 hs = ImGui::CalcTextSize(hub);
            dl->AddText(ImVec2(c.x - hs.x * 0.5f, c.y - hs.y * 0.5f), IM_COL32(255, 255, 255, 245), hub);

            if (hoverSlice >= 0 && ImGui::IsItemClicked(ImGuiMouseButton_Left))
                selected = hoverSlice + 1;
        }
        ImGui::End();
        ImGui::PopStyleVar(3);
        return selected;
    }
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
        io.MouseDoubleClickTime = 0.3f;
        io.MouseDragThreshold = 2.f;
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
        g_App = true;
    }

    ImGuiIO *io = &ImGui::GetIO();
    screenWidth = (float)g_GlWidth;
    screenHeight = (float)g_GlHeight;
    io->DisplaySize = ImVec2((float)g_GlWidth, (float)g_GlHeight);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

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
            if (LoadTextFromFile() && logintext[0] != '\0')
            {
                strncpy(s, logintext, sizeof(s) - 1);
                s[sizeof(s) - 1] = '\0';
                g_LoginTextLoaded = true;
            }
        }

        runtime_preview_menu::EnsureTexturesLoaded();
        main_runtime_theme::ApplyThemeState();

        ImVec2 viewportCenter = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 40.0f);
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (!isLogin)
        {
            ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(640, 460), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.0f);

            if (ImGui::Begin(OBFUSCATE("Login Menu"), nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
            {
                const ImVec2 pos = ImGui::GetWindowPos();
                ImDrawList* draw = ImGui::GetWindowDrawList();

                ModernUI::RenderMenuEdgeLightning(draw, draw, pos, ImVec2(pos.x + 640.0f, pos.y + 460.0f));

                const ImVec2 login_size = ImVec2(640, 460);
                const float outerRounding = 10.0f;
                const float innerInset = 10.0f;
                const float innerRounding = 12.0f;
                const float outerGlowHeight = login_size.y * 0.42f;
                const ImVec2 innerMin = pos + ImVec2(innerInset, innerInset);
                const ImVec2 innerMax = pos + login_size - ImVec2(innerInset, innerInset);
                draw->AddRectFilled(pos, pos + login_size, IM_COL32(0, 0, 0, 110), outerRounding);
                draw->AddRectFilledMultiColor(pos, ImVec2(pos.x + login_size.x, pos.y + outerGlowHeight), main_runtime_theme::GetAccentTintU32(0.22f, 0.10f), main_runtime_theme::GetAccentTintU32(0.16f, 0.05f), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));
                draw->AddRectFilled(innerMin, innerMax, IM_COL32(0, 0, 0, 188), innerRounding);
                draw->AddRectFilledMultiColor(innerMin, ImVec2(innerMax.x, innerMin.y + (innerMax.y - innerMin.y) * 0.44f), main_runtime_theme::GetAccentTintU32(0.22f, 0.11f), main_runtime_theme::GetAccentTintU32(0.16f, 0.06f), IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));

                auto drawLoginButton = [&](const char* label, float y, float width, bool primary) -> bool {
                    const float buttonX = (640.0f - width) * 0.5f;
                    ImGui::SetCursorPos(ImVec2(buttonX, y));
                    if (F50) ImGui::PushFont(F50);
                    const bool pressed = ImGui::InvisibleButton(label, ImVec2(width, 50.0f));
                    const bool hovered = ImGui::IsItemHovered();
                    ImFont* labelFont = ImGui::GetFont();
                    const float labelSize = ImGui::GetFontSize();
                    if (F50) ImGui::PopFont();

                    const ImVec2 buttonMin = ImGui::GetItemRectMin();
                    const ImVec2 buttonMax = ImGui::GetItemRectMax();
                    const bool useAccent = primary ? !hovered : hovered;
                    if (useAccent) {
                        draw->AddRectFilledMultiColor(buttonMin, buttonMax, main_runtime_theme::GetAccentU32(1.0f),
                            main_runtime_theme::GetAccentTintU32(0.82f), main_runtime_theme::GetAccentTintU32(0.82f),
                            main_runtime_theme::GetAccentU32(1.0f), 8.0f);
                    } else {
                        draw->AddRectFilled(buttonMin, buttonMax, ImGui::GetColorU32(c::button::background), 8.0f);
                        draw->AddRect(buttonMin, buttonMax, ImGui::GetColorU32(c::button::outline), 8.0f, 0, 1.0f);
                    }

                    const float labelSizeUse = labelSize > 0.0f ? labelSize : 16.0f;
                    const ImVec2 labelTextSize = (labelFont != nullptr)
                        ? labelFont->CalcTextSizeA(labelSizeUse, FLT_MAX, 0.0f, label)
                        : ImGui::CalcTextSize(label);
                    const ImVec2 labelTextPos = ImVec2((buttonMin.x + buttonMax.x - labelTextSize.x) * 0.5f,
                        (buttonMin.y + buttonMax.y - labelTextSize.y) * 0.5f);
                    const ImU32 labelColor = useAccent ? IM_COL32(10, 10, 14, 255) : ImGui::GetColorU32(c::text::text_active);
                    if (labelFont != nullptr) draw->AddText(labelFont, labelSizeUse, labelTextPos, labelColor, label);
                    else draw->AddText(labelTextPos, labelColor, label);

                    return pressed;
                };

                {
                    const char* titleText = "LOG IN";
                    const float titleSize = 34.0f;
                    const float titleX = pos.x + 80.0f;
                    const float titleY = pos.y + 56.0f;
                    if (F50) draw->AddText(F50, titleSize, ImVec2(titleX, titleY), ImGui::GetColorU32(c::text::text_active), titleText);
                    else draw->AddText(ImVec2(titleX, titleY), ImGui::GetColorU32(c::text::text_active), titleText);
                }

                {
                    const char* helperLine1 = "Authorize through your license key where";
                    const char* helperLine2 = "your subscription is located.";
                    const float helperX = pos.x + 30.0f;
                    draw->AddText(ImVec2(helperX, pos.y + 106.0f), ImGui::GetColorU32(c::text::text), helperLine1);
                    draw->AddText(ImVec2(helperX, pos.y + 128.0f), ImGui::GetColorU32(c::text::text), helperLine2);
                }

                {
                    const char* orLabel = "OR";
                    const float sepLeft = pos.x + 80.0f;
                    const float sepRight = pos.x + 520.0f;
                    const float sepMidY = pos.y + 254.0f;
                    const ImVec2 orSize = ImGui::CalcTextSize(orLabel);
                    const float sepMidX = (sepLeft + sepRight) * 0.5f;
                    const float sepGap = orSize.x * 0.5f + 14.0f;
                    const ImU32 sepColor = ImGui::GetColorU32(c::text::text);
                    draw->AddLine(ImVec2(sepLeft, sepMidY), ImVec2(sepMidX - sepGap, sepMidY), sepColor, 1.0f);
                    draw->AddLine(ImVec2(sepMidX + sepGap, sepMidY), ImVec2(sepRight, sepMidY), sepColor, 1.0f);
                    draw->AddText(ImVec2(sepMidX - orSize.x * 0.5f, sepMidY - orSize.y * 0.5f), sepColor, orLabel);
                }

                const float inputWidth = 440.0f;
                const float inputHeight = 56.0f;
                const float inputX = (login_size.x - inputWidth) * 0.5f;
                ImGui::SetCursorPos(ImVec2(inputX, 314.0f));
                ImGui::AstralInput("##key_login", s, sizeof(s), ImVec2(inputWidth, inputHeight));
                bool loginInputClicked = ImGui::IsItemClicked();
                bool loginInputActive = ImGui::IsItemActive();
                bool loginInputHovered = ImGui::IsItemHovered();

                if (loginInputClicked || loginInputActive) showKeyboard = true;

                if (showKeyboard && !loginInputActive && !loginInputHovered && ImGui::IsMouseClicked(0)) {
                    ImGuiIO& io = ImGui::GetIO();
                    float screenHeight = io.DisplaySize.y;
                    float keyboardHeight = screenHeight * 0.60f;
                    if (ImGui::GetMousePos().y > screenHeight - keyboardHeight) showKeyboard = false;
                }

                if (drawLoginButton("PASTE", 176.0f, 480.0f, false)) {
                    auto key = getClipboard();
                    strncpy(s, key.c_str(), sizeof(s) - 1);
                    s[sizeof(s) - 1] = '\0';
                }

                if (drawLoginButton("LOG IN", 360.0f, 480.0f, true)) {
                    err = Login(s);
                    if (err == "OK") {
                        showKeyboard = false;
                        strncpy(logintext, s, sizeof(logintext) - 1);
                        logintext[sizeof(logintext) - 1] = '\0';
                        SaveLoginTextToFile(s);
                        g_LoginTextLoaded = true;
                        err.clear();
                        // WALANG LOADING — deretso sa pizza menu
                        isLogin = true;
                        g_ShowRadialMenu = true;
                    }
                }
                if (!err.empty() && err != "OK") {
                    ImGui::SetCursorPos(ImVec2(30, 458.0f));
                    ImGui::TextColored(ImColor(255, 90, 90, 255), "Error: %s", err.c_str());
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
            if (g_ShowRadialMenu)
            {
                // ---------- PIZZA MENU (exclusive) ----------
                const int picked = ModernUI::RenderCategoryWheel(viewportCenter);
                if (picked >= 1 && picked <= 6) {
                    page = picked;
                    activeTab = picked;
                    g_ShowRadialMenu = false;
                }
            }
            else
            {
                // ---------- CONTENT BOX (exclusive) ----------
                uncollapseOpenAnim = ImClamp(uncollapseOpenAnim + ImGui::GetIO().DeltaTime * 5.0f, 0.0f, 1.0f);
                float openEase = uncollapseOpenAnim * uncollapseOpenAnim * (3.0f - 2.0f * uncollapseOpenAnim);
                float openAlpha = 0.2f + 0.8f * openEase;
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, openAlpha);

                ImVec2 mainWindowSize = ImVec2(1120.f, 640.f);
                mainWindowSize.x = ImMin(mainWindowSize.x, displaySize.x);
                mainWindowSize.y = ImMin(mainWindowSize.y, displaySize.y);
                ImGui::SetNextWindowPos(viewportCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize(mainWindowSize, ImGuiCond_Always);
                ImGui::SetNextWindowBgAlpha(0.0f);

                ImGui::Begin("@ThaxxylHax", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);
                {
                    runtime_preview_menu::StateRefs runtimeState{dark, tabAlpha, tabAdd, page, activeTab, windowCollapsed, isMenuVisible, collapseBarLastActiveTime, collapseBarOpacityAnim, collapseBarPressAnim};
                    {
                        using namespace runtime_preview_menu;
                        main_runtime_theme::ApplyAccentFromHue();

                        ImGuiStyle *runtimeStyle = &ImGui::GetStyle();
                        c::ApplyMainWindowStyle(*runtimeStyle);
                        c::UpdateTheme(runtimeState.dark, menu, ImGui::GetIO().DeltaTime);
                        main_runtime_theme::ApplyThemeState();

                        const ImVec2 runtimeWindowSize = ImGui::GetWindowSize();
                        const ImVec2 runtimeWindowPos = ImGui::GetWindowPos();
                        ImDrawList *runtimeDrawList = ImGui::GetWindowDrawList();

                        runtimeDrawList->AddRectFilled(runtimeWindowPos, ImVec2(runtimeWindowPos.x + runtimeWindowSize.x, runtimeWindowPos.y + runtimeWindowSize.y), IM_COL32(6, 6, 6, 255), 5.0f);
                        runtimeDrawList->AddRect(runtimeWindowPos, ImVec2(runtimeWindowPos.x + runtimeWindowSize.x, runtimeWindowPos.y + runtimeWindowSize.y), IM_COL32(45, 45, 45, 120), 5.0f, 0, 1.0f);
                        runtimeDrawList->AddRect(runtimeWindowPos + ImVec2(1.0f, 1.0f), ImVec2(runtimeWindowPos.x + runtimeWindowSize.x - 1.0f, runtimeWindowPos.y + runtimeWindowSize.y - 1.0f), IM_COL32(10, 10, 10, 120), 4.0f, 0, 1.0f);

                        const float outerPad = 14.0f;
                        const float layoutGap = 8.0f;
                        const float contentPadding = 10.0f;
                        const float columnGap = 10.0f;
                        const float headerHeight = 64.0f;
                        const float closeHeaderWidth = 72.0f;

                        // WALANG SIDEBAR — full width
                        const float contentLeft  = runtimeWindowPos.x + outerPad;
                        const float contentRight = runtimeWindowPos.x + runtimeWindowSize.x - outerPad;
                        const float headerTop = runtimeWindowPos.y + outerPad;
                        const float headerWidth = contentRight - contentLeft;
                        const float headerMainWidth = ImMax(0.0f, headerWidth - closeHeaderWidth - layoutGap);
                        const ImVec2 headerMin(contentLeft, headerTop);
                        const ImVec2 headerMax(contentLeft + headerMainWidth, headerTop + headerHeight);
                        const ImVec2 closeCardMin(headerMax.x + layoutGap, headerTop);
                        const ImVec2 closeCardSize(closeHeaderWidth, headerHeight);

                        const ImVec2 hostMin(contentLeft, headerTop + headerHeight + layoutGap);
                        const ImVec2 hostMax(contentRight, runtimeWindowPos.y + runtimeWindowSize.y - outerPad);
                        const ImVec2 contentInnerMin(hostMin.x + contentPadding, hostMin.y + contentPadding);
                        const ImVec2 contentInnerSize(ImMax(0.0f, (hostMax.x - hostMin.x) - contentPadding * 2.0f), ImMax(0.0f, (hostMax.y - hostMin.y) - contentPadding * 2.0f));

                        runtimeState.page = ImClamp(runtimeState.page, 1, 6);
                        runtimeState.activeTab = ImClamp(runtimeState.activeTab, 1, 6);

                        runtimeDrawList->AddRectFilled(headerMin, headerMax, IM_COL32(16, 16, 16, 255), 4.0f);
                        runtimeDrawList->AddRect(headerMin, headerMax, IM_COL32(22, 22, 22, 255), 4.0f, 0, 1.0f);

                        const ImVec2 flameCenter(headerMin.x + 28.0f, headerMin.y + headerHeight * 0.5f);
                        runtimeDrawList->AddCircleFilled(flameCenter, 18.0f, IM_COL32(24, 24, 24, 255), 28);
                        {
                            ImFont *iconFont = custom::shell::GetIconFont();
                            const char *logoIcon = ICON_FA_FIRE;
                            const ImVec2 logoSize = custom::shell::MeasureText(iconFont, 17.0f, logoIcon);
                            runtimeDrawList->AddText(iconFont, 17.0f, ImVec2(flameCenter.x - logoSize.x * 0.5f, flameCenter.y - logoSize.y * 0.5f), main_runtime_theme::GetAccentU32(), logoIcon);
                        }

                        ImFont *runtimeTitleFont = custom::shell::GetTitleFont();
                        const float runtimeTitleSize = 22.0f;
                        const char *titleA = "JAREDAX CONTAINER";
                        const char *titleB = " V3";
                        const ImVec2 titleASize = runtimeTitleFont->CalcTextSizeA(runtimeTitleSize, FLT_MAX, 0.0f, titleA);
                        runtimeDrawList->AddText(runtimeTitleFont, runtimeTitleSize, ImVec2(headerMin.x + 56.0f, headerMin.y + 11.0f), IM_COL32(235, 235, 235, 255), titleA);
                        runtimeDrawList->AddText(runtimeTitleFont, runtimeTitleSize, ImVec2(headerMin.x + 56.0f + titleASize.x, headerMin.y + 11.0f), main_runtime_theme::GetAccentU32(), titleB);

                        static const char* catNames[] = { "VISUAL", "COMBAT", "MEMORY", "SKINS", "MISC", "SETTINGS" };
                        char currentCat[64];
                        snprintf(currentCat, sizeof(currentCat), "Current: %s", catNames[runtimeState.activeTab - 1]);
                        const ImVec2 subtitlePos(headerMin.x + 57.0f, headerMin.y + 38.0f);
                        runtimeDrawList->AddText(runtimeTitleFont, 10.0f, subtitlePos, IM_COL32(142, 142, 148, 235), currentCat);

                        // ============================================
                        // HEADER BUTTONS — BACK / SAVE / HIDE
                        // ============================================
                        const float hBtnW = 76.0f;
                        const float hBtnH = 34.0f;
                        const float hBtnGap = 5.0f;
                        const float hBtnY = headerTop + 15.0f;

                        const float hGroupW = hBtnW * 3.0f + hBtnGap * 2.0f;
                        const float hGroupStartX = headerMax.x - hGroupW - 4.0f;

                        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

                        // --- BACK TO PIZZA ---
                        ImGui::SetCursorScreenPos(ImVec2(hGroupStartX, hBtnY));
                        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.094f, 0.094f, 0.094f, 0.95f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.122f, 0.122f, 0.122f, 1.00f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.176f, 0.176f, 0.176f, 1.00f));
                        ImGui::PushStyleColor(ImGuiCol_Border,        ImVec4(0.141f, 0.141f, 0.141f, 1.00f));
                        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.804f, 0.804f, 0.824f, 1.00f));
                        if (ImGui::Button("<- BACK", ImVec2(hBtnW, hBtnH))) {
                            g_ShowRadialMenu = true;
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Back to Pizza Menu");
                        ImGui::PopStyleColor(5);

                        ImGui::SameLine(0.0f, hBtnGap);

                        // --- SAVE ---
                        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.094f, 0.094f, 0.094f, 0.95f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.122f, 0.122f, 0.122f, 1.00f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.176f, 0.176f, 0.176f, 1.00f));
                        ImGui::PushStyleColor(ImGuiCol_Border,        ImVec4(0.141f, 0.141f, 0.141f, 1.00f));
                        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.804f, 0.804f, 0.824f, 1.00f));
                        if (ImGui::Button("SAVE", ImVec2(hBtnW, hBtnH))) {
                            SaveConfiguration("astavex_config");
                            SaveConfig();
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save Configuration");
                        ImGui::PopStyleColor(5);

                        ImGui::SameLine(0.0f, hBtnGap);

                        // --- HIDE ---
                        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.094f, 0.094f, 0.094f, 0.95f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.122f, 0.122f, 0.122f, 1.00f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.176f, 0.176f, 0.176f, 1.00f));
                        ImGui::PushStyleColor(ImGuiCol_Border,        ImVec4(0.141f, 0.141f, 0.141f, 1.00f));
                        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.804f, 0.804f, 0.824f, 1.00f));
                        if (ImGui::Button("HIDE", ImVec2(hBtnW, hBtnH))) {
                            windowCollapsed = true;
                            isMenuVisible = false;
                            g_ShowRadialMenu = false;
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Hide (double-tap top middle to restore)");
                        ImGui::PopStyleColor(5);

                        ImGui::PopStyleVar(2);

                        if (custom::shell::DrawCloseCard(closeCardMin, closeCardSize)) {
                            runtime_preview_menu::CollapseMenu(runtimeState);
                        }

                        runtimeState.tabAlpha = ImClamp(runtimeState.tabAlpha + (4.0f * ImGui::GetIO().DeltaTime * (runtimeState.page == runtimeState.activeTab ? 1.0f : -1.0f)), 0.0f, 1.0f);
                        if (runtimeState.tabAlpha == 0.0f && runtimeState.tabAdd == 0.0f) runtimeState.activeTab = runtimeState.page;

                        runtimeDrawList->AddRectFilled(hostMin, hostMax, IM_COL32(6, 6, 6, 255), 4.0f);

                        ImGui::SetCursorScreenPos(contentInnerMin);
                        ImGui::BeginChild("##RuntimeContentHost", contentInnerSize, false, ImGuiWindowFlags_NoBackground);
                        {
                            const bool pushedContentFont = (font::inter_semibold != nullptr);
                            if (pushedContentFont) ImGui::PushFont(font::inter_semibold);
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, runtimeState.tabAlpha * runtimeStyle->Alpha);

                            const ImVec2 contentRegion = ImGui::GetContentRegionAvail();
                            const float childHeight = ImMax(0.0f, contentRegion.y);
                            const float childWidth = ImMax(0.0f, (contentRegion.x - columnGap) * 0.5f);

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
                                    custom::SliderFloat("Hitbox Size", &Config.ExtraMenu.HitboxScale, 0.5f, 25.0f, "%.1fm");
                                    custom::Checkbox("No Recoil", &Config.ExtraMenu.Recoil);
                                    custom::Checkbox("No Spread", &Config.ExtraMenu.Spread);
                                    custom::Checkbox("No Shake", &Config.ExtraMenu.Shake);
                                    custom::Checkbox("No Overheat", &Config.ExtraMenu.Rpd);
                                    custom::Checkbox("No Parachute", &Config.ExtraMenu.Parachute);
                                    custom::Checkbox("Anti Flashbang", &Config.ExtraMenu.Flash);
                                    custom::Checkbox("Firerate", &Config.ExtraMenu.Fire);
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
                                    EndContentChild(right);
                                }
                                custom::EndGroup();
                            }

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