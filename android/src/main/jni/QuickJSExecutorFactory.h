#pragma once

#include <jsireact/JSIExecutor.h>

namespace jsi = facebook::jsi;
namespace react = facebook::react;

namespace qjs {

class QuickJSExecutorFactory : public react::JSExecutorFactory {
 public:
  explicit QuickJSExecutorFactory(
      react::JSIExecutor::RuntimeInstaller runtimeInstaller,
      const react::JSIScopedTimeoutInvoker &timeoutInvoker)
      : runtimeInstaller_(runtimeInstaller),
        timeoutInvoker_(timeoutInvoker) {}

  std::unique_ptr<react::JSExecutor> createJSExecutor(
      std::shared_ptr<react::ExecutorDelegate> delegate,
      std::shared_ptr<react::MessageQueueThread> jsQueue) override;

 private:
  react::JSIExecutor::RuntimeInstaller runtimeInstaller_;
  react::JSIScopedTimeoutInvoker timeoutInvoker_;
};

class QuickJSExecutor : public react::JSIExecutor {
 public:
  QuickJSExecutor(
      std::shared_ptr<jsi::Runtime> runtime,
      std::shared_ptr<react::ExecutorDelegate> delegate,
      std::shared_ptr<react::MessageQueueThread> jsQueue,
      const react::JSIScopedTimeoutInvoker &timeoutInvoker,
      RuntimeInstaller runtimeInstaller);

 private:
  react::JSIScopedTimeoutInvoker timeoutInvoker_;
};

} // namespace qjs
