cmake_minimum_required(VERSION 3.4.1)
project(react-native-quickjs)
set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 14)

set(PACKAGE_NAME "quickjsexecutor")

# Folly compile flags
include("${REACT_NATIVE_DIR}/ReactAndroid/cmake-utils/folly-flags.cmake")
add_compile_options(${folly_FLAGS})

# QuickJS compile flags
add_compile_options(
        -DCONFIG_VERSION="\"1\""
        -D_GNU_SOURCE
        -Wno-unused-variable
        -DCONFIG_CC="gcc"
        -DLOG_TAG=\"ReactNative\"
        -DENABLE_HASH_CHECK=${ENABLE_HASH_CHECK}
        -DCONFIG_BIGNUM)

file(GLOB quickjs_jni_SRC CONFIGURE_DEPENDS ./src/main/jni/*.cpp)
file(GLOB quickjs_SRC CONFIGURE_DEPENDS ../cpp/*.cpp ../cpp/engine/*.c)
add_library(${PACKAGE_NAME}
            SHARED
            ${quickjs_jni_SRC}
            ${quickjs_SRC}
)

target_include_directories(
  ${PACKAGE_NAME}
  PRIVATE
  "${REACT_NATIVE_DIR}/ReactCommon/jsiexecutor"
)

# Specifies a path to native header files.
include_directories(
        ./src/main/jni/
        ../cpp
        ../cpp/engine
)

# find libraries
find_package(ReactAndroid REQUIRED CONFIG)

find_package(fbjni REQUIRED CONFIG)

# reactnative_internal_static
file(GLOB rn_jsi_react_SRC  "${REACT_NATIVE_DIR}/ReactCommon/jsiexecutor/jsireact/*.cpp")
file(GLOB react_marker_SRC  "${REACT_NATIVE_DIR}/ReactCommon/reactperflogger/reactperflogger/*.cpp")

add_library(
  reactnative_internal_static
  STATIC
  "${rn_jsi_react_SRC}"
  "${react_marker_SRC}"
  "${REACT_NATIVE_DIR}/ReactCommon/cxxreact/JSExecutor.cpp"
)

target_include_directories(
  reactnative_internal_static
  PRIVATE
  "${REACT_NATIVE_DIR}/ReactCommon/jsiexecutor"
  "${REACT_NATIVE_DIR}/ReactCommon/reactperflogger"
)

target_link_libraries(
  reactnative_internal_static
  ReactAndroid::folly_runtime
  ReactAndroid::glog
  ReactAndroid::jsi
  ReactAndroid::reactnativejni
)

# link to shared libraries

target_link_libraries(
  ${PACKAGE_NAME}
  log
  android
  fbjni::fbjni
  ReactAndroid::folly_runtime
  ReactAndroid::glog
  ReactAndroid::jsi
  ReactAndroid::jsinspector
  ReactAndroid::reactnativejni
  ReactAndroid::runtimeexecutor
  reactnative_internal_static
)

# target_compile_options(${PACKAGE_NAME} PUBLIC -fsanitize=address -fno-omit-frame-pointer)
# set_target_properties(${PACKAGE_NAME} PROPERTIES LINK_FLAGS -fsanitize=address)