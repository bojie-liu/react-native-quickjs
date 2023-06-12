#!/bin/sh

# For app
#/Users/liubojie/env/android-ndk-r19c/toolchains/aarch64-linux-android-4.9/prebuilt/darwin-x86_64/bin/aarch64-linux-android-addr2line -C -f -e ./ReactAndroid/build/react-ndk/exported/arm64-v8a/libreactnativejni.so $@

# For RNTester
 /Users/liubojie/env/android-ndk-r19c/toolchains/aarch64-linux-android-4.9/prebuilt/darwin-x86_64/bin/aarch64-linux-android-addr2line -C -f -e android/build/intermediates/cmake/debug/obj/arm64-v8a/libquickjsexecutor.so $@

