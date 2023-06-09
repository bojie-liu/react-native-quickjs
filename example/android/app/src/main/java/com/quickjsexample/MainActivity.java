package com.quickjsexample;

import android.os.Bundle;

import com.facebook.react.ReactActivity;
import com.facebook.react.ReactActivityDelegate;
import com.facebook.react.bridge.ReactMarker;
import com.facebook.react.bridge.ReactMarkerConstants;
import com.facebook.react.defaults.DefaultNewArchitectureEntryPoint;
import com.facebook.react.defaults.DefaultReactActivityDelegate;

public class MainActivity extends ReactActivity {

  /**
   * Returns the name of the main component registered from JavaScript. This is used to schedule
   * rendering of the component.
   */
  @Override
  protected String getMainComponentName() {
    return "QuickjsExample";
  }

  /**
   * Returns the instance of the {@link ReactActivityDelegate}. Here we use a util class {@link
   * DefaultReactActivityDelegate} which allows you to easily enable Fabric and Concurrent React
   * (aka React 18) with two boolean flags.
   */
  @Override
  protected ReactActivityDelegate createReactActivityDelegate() {
    return new DefaultReactActivityDelegate(
        this,
        getMainComponentName(),
        // If you opted-in for the New Architecture, we enable the Fabric Renderer.
        DefaultNewArchitectureEntryPoint.getFabricEnabled(), // fabricEnabled
        // If you opted-in for the New Architecture, we enable Concurrent React (i.e. React 18).
        DefaultNewArchitectureEntryPoint.getConcurrentReactEnabled() // concurrentRootEnabled
        );
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    TTIRecorder.mOnCreateTimestamp = System.currentTimeMillis();
    super.onCreate(savedInstanceState);
    ReactMarker.addListener((reactMarkerConstants, s, i) -> {
      if (reactMarkerConstants == ReactMarkerConstants.GET_REACT_INSTANCE_MANAGER_START) {
        // Start timestamp in react-native-v8
      } else if (reactMarkerConstants == ReactMarkerConstants.CONTENT_APPEARED) {
        TTIRecorder.mTTI = System.currentTimeMillis() - TTIRecorder.mOnCreateTimestamp;
        TTIRecorder.printTTI();
      }
    });
  }
}
