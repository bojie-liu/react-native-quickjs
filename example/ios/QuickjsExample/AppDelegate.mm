#import "AppDelegate.h"

#import <React/RCTBundleURLProvider.h>
#import <React/RCTCxxBridgeDelegate.h>
#import <React/RCTJSIExecutorRuntimeInstaller.h>
#import <ReactCommon/RCTTurboModuleManager.h>

#ifndef RCT_USE_HERMES
#if __has_include(<reacthermes/HermesExecutorFactory.h>)
#define RCT_USE_HERMES 1
#else
#define RCT_USE_HERMES 0
#endif
#endif

#if RCT_USE_HERMES
#import <reacthermes/HermesExecutorFactory.h>
//#else
//#import <React/JSCExecutorFactory.h>
#endif
#import <QuickJSExecutorFactory.h>

@interface AppDelegate () <RCTCxxBridgeDelegate>
@end

@implementation AppDelegate

+ (void)initialize {
  extern NSTimeInterval onCreateTimestamp;
  onCreateTimestamp = [[NSDate date] timeIntervalSince1970];
}

- (std::unique_ptr<facebook::react::JSExecutorFactory>)jsExecutorFactoryForBridge:(RCTBridge *)bridge
{
  auto installBindings = facebook::react::RCTJSIExecutorRuntimeInstaller(nullptr);
#if RCT_USE_HERMES
  return std::make_unique<facebook::react::HermesExecutorFactory>(installBindings);
#else
//  return std::make_unique<facebook::react::JSCExecutorFactory>(installBindings);
  auto cacheDir = [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES).firstObject UTF8String];
  return std::make_unique<qjs::QuickJSExecutorFactory>(installBindings, "");
#endif
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
  self.moduleName = @"QuickjsExample";
  // You can add your custom initial props in the dictionary below.
  // They will be passed down to the ViewController used by React Native.
  self.initialProps = @{};

  return [super application:application didFinishLaunchingWithOptions:launchOptions];
}

- (NSURL *)sourceURLForBridge:(RCTBridge *)bridge
{
#if DEBUG
  return [[RCTBundleURLProvider sharedSettings] jsBundleURLForBundleRoot:@"index"];
#else
  return [[NSBundle mainBundle] URLForResource:@"main" withExtension:@"jsbundle"];
#endif
}

/// This method controls whether the `concurrentRoot`feature of React18 is turned on or off.
///
/// @see: https://reactjs.org/blog/2022/03/29/react-v18.html
/// @note: This requires to be rendering on Fabric (i.e. on the New Architecture).
/// @return: `true` if the `concurrentRoot` feature is enabled. Otherwise, it returns `false`.
- (BOOL)concurrentRootEnabled
{
  return true;
}

@end
