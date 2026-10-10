#!/bin/bash
# Syntax check harness for the JNI sources (NDK r26d clang, aarch64 API 21).
# Usage: ./check.sh <source.cpp|source.h> ...
set -o pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
JNI="$ROOT/src/main/jni"
TC="$ROOT/.toolchain/android-ndk-r26d/toolchains/llvm/prebuilt/linux-x86_64"
ABI=arm64-v8a
INC="-I$JNI -I$JNI/Includes -I$JNI/ImGui -I$JNI/curl/curl-android-$ABI/include -I$JNI/curl/openssl-android-$ABI/include -I$JNI/libzip -I$JNI/foxcheats -I$JNI/foxcheats/includes -I$JNI/SDK/Xhook -I$JNI/IL2CppSDKGenerator/Dobby -I$JNI/IL2CppSDKGenerator"
"$TC/bin/clang++" --target=aarch64-linux-android21 --sysroot="$TC/sysroot" \
  -std=c++17 -fms-extensions -w -fsyntax-only $INC "$@"
echo "EXIT=$?"
