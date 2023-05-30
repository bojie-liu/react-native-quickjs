//
//  QuickJSExecutorFactory.h
//  Pods
//
//  Created by liubojie on 2023/5/29.
//

#ifndef QuickJSExecutorFactory_h
#define QuickJSExecutorFactory_h

#include <jsireact/JSIExecutor.h>

namespace qjs {

class QuickJSExecutorFactory : public facebook::react::JSExecutorFactory {
public:
  explicit QuickJSExecutorFactory(
                                  facebook::react::JSIExecutor::RuntimeInstaller runtimeInstaller)
  : runtimeInstaller_(std::move(runtimeInstaller)) {}
  
  std::unique_ptr<facebook::react::JSExecutor> createJSExecutor(
                                                                std::shared_ptr<facebook::react::ExecutorDelegate> delegate,
                                                                std::shared_ptr<facebook::react::MessageQueueThread> jsQueue) override;
  
private:
  facebook::react::JSIExecutor::RuntimeInstaller runtimeInstaller_;
};
}

#endif /* QuickJSExecutorFactory_h */
