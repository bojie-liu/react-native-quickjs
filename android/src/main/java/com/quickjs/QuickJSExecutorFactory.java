/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 * <p>
 * This source code is licensed under the MIT license found in the LICENSE file in the root
 * directory of this source tree.
 */
package com.quickjs;

import java.io.File;

import com.facebook.common.file.FileUtils;
import com.facebook.react.bridge.JavaScriptExecutor;
import com.facebook.react.bridge.JavaScriptExecutorFactory;

public class QuickJSExecutorFactory implements JavaScriptExecutorFactory {

  private static final String TAG = "QuickJS";
  private String mCodeCacheDir;

  public QuickJSExecutorFactory(final String codeCacheDir) {
    mCodeCacheDir = codeCacheDir;
  }

  @Override
  public JavaScriptExecutor create() {
    try {
      FileUtils.mkdirs(new File(mCodeCacheDir));
    } catch (Exception e) {
      e.printStackTrace();
      // Empty string to avoid using code cache.
      mCodeCacheDir = "";
    }
    return new QuickJSExecutor(mCodeCacheDir);
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
