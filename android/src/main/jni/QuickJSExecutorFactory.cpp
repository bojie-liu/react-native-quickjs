#include "QuickJSExecutorFactory.h"

#include <thread>

#include "QuickJSRuntime.h"
#include "QuickJSRuntimeFactory.h"
#include "cxxreact/MessageQueueThread.h"
#include "cxxreact/SystraceSection.h"

namespace qjs {

namespace {

std::unique_ptr<jsi::Runtime> makeQuickJSRuntimeSystraced(std::shared_ptr<react::ExecutorDelegate>
        delegate, const std::string &codeCacheDir) {
  react::SystraceSection s("QuickJSExecutorFactory::makeQuickJSRuntimeSystraced");
  return createQuickJSRuntime(codeCacheDir);
}

} // namespace

std::unique_ptr<react::JSExecutor> QuickJSExecutorFactory::createJSExecutor(
    std::shared_ptr<react::ExecutorDelegate> delegate,
    std::shared_ptr<react::MessageQueueThread> jsQueue) {
  std::unique_ptr<jsi::Runtime> quickJSRuntime = makeQuickJSRuntimeSystraced(delegate, codeCacheDir_);

  // Add js engine information to Error.prototype so in error reporting we
  // can send this information.
  auto errorPrototype = quickJSRuntime->global()
                            .getPropertyAsObject(*quickJSRuntime, "Error")
                            .getPropertyAsObject(*quickJSRuntime, "prototype");
  errorPrototype.setProperty(*quickJSRuntime, "jsEngine", "quickjs");

  return std::make_unique<QuickJSExecutor>(
      std::move(quickJSRuntime),
      delegate,
      jsQueue,
      timeoutInvoker_,
      runtimeInstaller_);
}

QuickJSExecutor::QuickJSExecutor(
    std::shared_ptr<jsi::Runtime> runtime,
    std::shared_ptr<react::ExecutorDelegate> delegate,
    std::shared_ptr<react::MessageQueueThread> jsQueue,
    const react::JSIScopedTimeoutInvoker &timeoutInvoker,
    RuntimeInstaller runtimeInstaller)
    : JSIExecutor(runtime, delegate, timeoutInvoker, runtimeInstaller) {}

} // namespace qjs
