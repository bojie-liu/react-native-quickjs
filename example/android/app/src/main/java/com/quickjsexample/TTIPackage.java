//*** licence placeholder ***//

package com.quickjsexample;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import androidx.annotation.NonNull;

import com.facebook.common.logging.FLog;
import com.facebook.react.ReactPackage;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.module.annotations.ReactModule;
import com.facebook.react.uimanager.ViewManager;

public class TTIPackage implements ReactPackage {

   @ReactModule(name = TTIModule.NAME)
   public class TTIModule extends ReactContextBaseJavaModule {
      public static final String NAME = "TTIModule";

      public TTIModule(ReactApplicationContext reactContext) {
         super(reactContext);
      }

      @NonNull
      @Override
      public String getName() {
         return NAME;
      }

      @ReactMethod
      public void componentDidMount(Double timestamp) {
         if (TTIRecorder.mComponentDidMount == 0) {
            TTIRecorder.mComponentDidMount = (long) (timestamp - TTIRecorder.mOnCreateTimestamp);
            TTIRecorder.printTTI();
         }
      }
   }

   @NonNull
   @Override
   public List<NativeModule> createNativeModules(
       @NonNull ReactApplicationContext reactApplicationContext) {
      return Collections.singletonList(new TTIModule(reactApplicationContext));
   }

   @NonNull
   @Override
   public List<ViewManager> createViewManagers(
       @NonNull ReactApplicationContext reactApplicationContext) {
      return new ArrayList<>();
   }
}
