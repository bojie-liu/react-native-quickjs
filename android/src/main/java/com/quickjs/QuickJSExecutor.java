/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 * <p>
 * This source code is licensed under the MIT license found in the LICENSE
 * file in the root directory of this source tree.
 */
package com.quickjs;

import com.facebook.jni.HybridData;
import com.facebook.react.bridge.JavaScriptExecutor;
import com.facebook.soloader.SoLoader;

public class QuickJSExecutor extends JavaScriptExecutor {

  static {
    SoLoader.loadLibrary("quickjsexecutor");
  }

  QuickJSExecutor(final String codeCacheDir) {
    super(initHybrid(codeCacheDir));
  }

  @Override
  public String getName() {
    return "QuickJSExecutor";
  }

  private static native HybridData initHybrid(final String codeCacheDir);
}
