# react-native-quickjs

An lightweight javascript engine for RN. 

## Run the example

```sh
cd example && yarn
yarn android
yarn ios
```

## Run with existing project

**Only tested for React Native >= 0.71.8. Please create an issue if it does not work for other version. Will fix it ASAP.**

1. Install dependency

```sh
yarn add react-native-quickjs
```

##### For Android

2. Override "getJavaScriptExecutorFactory" to use QuickJS as JS runtime

``` diff
diff --git a/android/app/src/main/java/com/awesomeproject/MainApplication.java b/android/app/src/main/java/com/awesomeproject/MainApplication.java
index 105e48c..b309fea 100644
--- a/android/app/src/main/java/com/awesomeproject/MainApplication.java
+++ b/android/app/src/main/java/com/awesomeproject/MainApplication.java
@@ -5,11 +5,16 @@ import com.facebook.react.PackageList;
 import com.facebook.react.ReactApplication;
 import com.facebook.react.ReactNativeHost;
 import com.facebook.react.ReactPackage;
+import com.facebook.react.bridge.JavaScriptExecutorFactory;
 import com.facebook.react.defaults.DefaultNewArchitectureEntryPoint;
 import com.facebook.react.defaults.DefaultReactNativeHost;
 import com.facebook.soloader.SoLoader;
+import com.quickjs.QuickJSExecutorFactory;
+
 import java.util.List;
 
+import androidx.annotation.Nullable;
+
 public class MainApplication extends Application implements ReactApplication {
 
   private final ReactNativeHost mReactNativeHost =
@@ -42,6 +47,13 @@ public class MainApplication extends Application implements ReactApplication {
         protected Boolean isHermesEnabled() {
           return BuildConfig.IS_HERMES_ENABLED;
         }
+
+        @Nullable
+        @Override
+        protected JavaScriptExecutorFactory getJavaScriptExecutorFactory() {
+          // Pass empty string to disable code cache.
+          return new QuickJSExecutorFactory(getApplication().getCacheDir().getAbsolutePath() + "/qjs");
+        }
       };
 
   @Override

```

3. Disable Hermes and its bundling procedure

``` diff
--- a/android/gradle.properties
+++ b/android/gradle.properties
@@ -41,4 +41,4 @@ newArchEnabled=false

 # Use this property to enable or disable the Hermes JS engine.
 # If set to false, you will be using JSC instead.
-hermesEnabled=true
+hermesEnabled=false
```

4. (Optional) Exclude unused libraries to reduce APK size

``` diff
--- a/android/app/build.gradle
+++ b/android/app/build.gradle
@@ -161,11 +161,18 @@ android {
             }
         }
     }
+
+    packagingOptions {
+        // Make sure libjsc.so does not packed in APK
+        exclude "**/libjsc.so"
+    }
 }
```

5. Run your application
    
    a. For Debug.
    ``` sh
    yarn android
    ```
    b. For Release. Run this command in the project root.
    ``` sh
    cd android && ./gradlew installRelease
    ```

##### For iOS

2. Disable Hermes and its bundling procedure

``` sh
USE_HERMES=0 pod install
```

3. Use "QuickJSExecutorFactory" in your application

``` diff
diff --git a/ios/AwesomeProject/AppDelegate.mm b/ios/AwesomeProject/AppDelegate.mm
index 029aa44..2f579c3 100644
--- a/ios/AwesomeProject/AppDelegate.mm
+++ b/ios/AwesomeProject/AppDelegate.mm
@@ -2,6 +2,27 @@
 
 #import <React/RCTBundleURLProvider.h>
 
+#import <React/RCTCxxBridgeDelegate.h>
+#import <React/RCTJSIExecutorRuntimeInstaller.h>
+#import <ReactCommon/RCTTurboModuleManager.h>
+#ifndef RCT_USE_HERMES
+#if __has_include(<reacthermes/HermesExecutorFactory.h>)
+#define RCT_USE_HERMES 1
+#else
+#define RCT_USE_HERMES 0
+#endif
+#endif
+
+#if RCT_USE_HERMES
+#import <reacthermes/HermesExecutorFactory.h>
+//#else
+//#import <React/JSCExecutorFactory.h>
+#endif
+#import <QuickJSExecutorFactory.h>
+
+@interface AppDelegate () <RCTCxxBridgeDelegate>
+@end
+
 @implementation AppDelegate
 
 - (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
@@ -14,6 +35,20 @@
   return [super application:application didFinishLaunchingWithOptions:launchOptions];
 }
 
+#pragma mark - RCTCxxBridgeDelegate
+
+- (std::unique_ptr<facebook::react::JSExecutorFactory>)jsExecutorFactoryForBridge:(RCTBridge *)bridge
+{
+  auto installBindings = facebook::react::RCTJSIExecutorRuntimeInstaller(nullptr);
+#if RCT_USE_HERMES
+  return std::make_unique<facebook::react::HermesExecutorFactory>(installBindings);
+#else
+//  return std::make_unique<facebook::react::JSCExecutorFactory>(installBindings);
+  auto cacheDir = [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES).firstObject UTF8String];
+  return std::make_unique<qjs::QuickJSExecutorFactory>(installBindings, ""); // pass empty string to disable code cache
+#endif
+}
+
 - (NSURL *)sourceURLForBridge:(RCTBridge *)bridge
 {
 #if DEBUG
```

5. Run your application
    
    a. For Debug. Just run it in XCode.

    b. For Release. Run this command in the project root.
    ``` sh
    npx react-native run-ios --mode Release
    ```

## Performance

1. [Example performance](./docs/DemoPerformance.md)
2. Real world bundle performance
   1. We tested QuickJS/V8/JSC with an online bundle size of 1.43M.
   2. On Android, we enabled code cache for both QuickJS and V8. For TTI, QuickJS is 5-20% slower than V8. For PSS memory, QuickJS is 40-(-5)% lower than V8.
   3. On iOS, TTI using QuickJS is 15% slower than JSC without code cache. With code cache, QuickJS is 15% faster than JSC and 50% lower than JSC in footprint memory usage.

## ESx Compatibility

As listed on official QuickJS [website](https://bellard.org/quickjs/). QuickJS passed 82% of ECMA-262 tests. Meanwhile V8 passed 86% and JSC passed 85% in 2022. If internationalization tests which accounts for nearly 3% are excluded, QuickJS is fairly close to V8 and JSC. You can checkout [https://test262.report/](https://test262.report/) for failed cases just in case.

## Why would I choose QuickJS for React Native

IMHO:

1. On Android, QuickJS is slower than V8 in most of my tests. Although it has advantages in memory and binary size which is important on some resource-limited devices. Also QuickJS is benefit from its startup time for some simple bundles.
2. On iOS, QuickJS simply better than JSC with code cache and worst than JSC without code cache.
3. QuickJS is easy to customize when it compared with V8 or Hermes. Like adding some high performance builtin functions or customized classes. It depends on your business.
4. Currently QuickJS have no inspector. But it seems some open source projects have made it worked. Anyway it needs extra works to support inspector in the future.

## Contributing

See the [contributing guide](CONTRIBUTING.md) to learn how to contribute to the repository and the development workflow.

Pull requests are always welcome.

## License

MIT

---

Made with [create-react-native-library](https://github.com/callstack/react-native-builder-bob)
