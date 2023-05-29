/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 * <p>
 * This source code is licensed under the MIT license found in the LICENSE file in the root
 * directory of this source tree.
 */
package com.quickjs;

import java.util.TimeZone;

import com.facebook.react.bridge.JavaScriptExecutor;
import com.facebook.react.bridge.JavaScriptExecutorFactory;

public class QuickJSExecutorFactory implements JavaScriptExecutorFactory {

  private static final String TAG = "QuickJS";

  public QuickJSExecutorFactory() {
  }

  @Override
  public JavaScriptExecutor create() {
    return new QuickJSExecutor();
  }

  @Override
  public void startSamplingProfiler() {
  }

  @Override
  public void stopSamplingProfiler(String filename) {
  }

  @Override
  public String toString() {
    return "QuickJSExecutor+QuickJSRuntime";
  }
}
