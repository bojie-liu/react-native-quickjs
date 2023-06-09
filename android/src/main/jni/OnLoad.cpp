#include "QuickJSExecutorFactory.h"
#include <fbjni/fbjni.h>
#include <folly/Memory.h>
#include <glog/logging.h>
#include <jni.h>
#include <react/jni/JReactMarker.h>
#include <react/jni/JRuntimeExecutor.h>
#include <react/jni/JSLogging.h>
#include <react/jni/JavaScriptExecutorHolder.h>

namespace jni = facebook::jni;
namespace jsi = facebook::jsi;
namespace react = facebook::react;

namespace qjs {

static void installBindings(jsi::Runtime &runtime) {
  react::Logger androidLogger =
      static_cast<void (*)(const std::string &, unsigned int)>(
          &react::reactAndroidLoggingHook);
  react::bindNativeLogger(runtime, androidLogger);
}

class QuickJSExecutorHolder
    : public jni::HybridClass<QuickJSExecutorHolder, react::JavaScriptExecutorHolder> {
 public:
  static constexpr auto kJavaDescriptor =
      "Lcom/quickjs/QuickJSExecutor;";

  static jni::local_ref<jhybriddata> initHybrid(
      jni::alias_ref<jclass>, const std::string &codeCacheDir) {
    react::JReactMarker::setLogPerfMarkerIfNeeded();
    return makeCxxInstance(folly::make_unique<QuickJSExecutorFactory>(
        installBindings,
        react::JSIExecutor::defaultTimeoutInvoker,
        codeCacheDir));
  }

  static void registerNatives() {
    registerHybrid({
        makeNativeMethod("initHybrid", QuickJSExecutorHolder::initHybrid),
    });
  }

 private:
  friend HybridBase;
  using HybridBase::HybridBase;
};

} // namespace qjs

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
  return facebook::jni::initialize(
      vm, [] { qjs::QuickJSExecutorHolder::registerNatives(); });
}
