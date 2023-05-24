#include <jni.h>
#include "react-native-quickjs.h"

extern "C"
JNIEXPORT jint JNICALL
Java_com_quickjs_QuickjsModule_nativeMultiply(JNIEnv *env, jclass type, jdouble a, jdouble b) {
    return quickjs::multiply(a, b);
}
