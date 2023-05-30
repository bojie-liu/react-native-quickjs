//
//  QuickJSExecutorFactory.m
//  react-native-quickjs
//
//  Created by liubojie on 2023/5/29.
//

#import "QuickJSExecutorFactory.h"

#import <React/RCTLog.h>
#import <memory>
#import <QuickJSRuntimeFactory.h>

#include "jsi/jsi.h"

namespace jsi = facebook::jsi;
namespace react = facebook::react;

namespace qjs {

std::unique_ptr<react::JSExecutor> QuickJSExecutorFactory::createJSExecutor(
    std::shared_ptr<react::ExecutorDelegate> delegate,
    std::shared_ptr<react::MessageQueueThread> jsQueue)
{
  auto installBindings = [runtimeInstaller = runtimeInstaller_](jsi::Runtime &runtime) {
    react::Logger iosLoggingBinder = [](const std::string &message, unsigned int logLevel) {
      _RCTLogJavaScriptInternal(static_cast<RCTLogLevel>(logLevel), [NSString stringWithUTF8String:message.c_str()]);
    };
    react::bindNativeLogger(runtime, iosLoggingBinder);
    // Wrap over the original runtimeInstaller
    if (runtimeInstaller) {
      runtimeInstaller(runtime);
    }
  };
  return folly::make_unique<react::JSIExecutor>(
      createQuickJSRuntime(),
      delegate,
      react::JSIExecutor::defaultTimeoutInvoker,
      std::move(installBindings));
}

} // namespace qjs
