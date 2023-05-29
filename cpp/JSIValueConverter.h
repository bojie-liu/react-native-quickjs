#pragma once

#include "QuickJSRuntime.h"
#include "jsi/jsi.h"

namespace qjs {

class JSIValueConverter {
 private:
  JSIValueConverter() = delete;
  ~JSIValueConverter() = delete;
  JSIValueConverter(JSIValueConverter &&) = delete;

 public:
  static jsi::Value ToJSIValue(
      const QuickJSRuntime &runtime,
      const JSValueConst &value);

  static JSValue ToJSValue(
      const QuickJSRuntime &runtime,
      const jsi::Value &value);

  static JSValue ToJSString(
      const QuickJSRuntime &runtime,
      const jsi::String &string);

  static JSValue ToJSString(
      const QuickJSRuntime &runtime,
      const jsi::PropNameID &propName);

  static JSValue ToJSSymbol(
      const QuickJSRuntime &runtime,
      const jsi::Symbol &symbol);

  static JSValue ToJSObject(
      const QuickJSRuntime &runtime,
      const jsi::Object &object);

  static JSValue ToJSBigInt(
      const QuickJSRuntime &runtime,
      const jsi::BigInt &bigInt);

  static JSValue ToJSArray(
      const QuickJSRuntime &runtime,
      const jsi::Array &array);

  static JSValue ToJSFunction(
      const QuickJSRuntime &runtime,
      const jsi::Function &function);

  static jsi::PropNameID ToJSIPropNameID(
      const QuickJSRuntime &runtime,
      const JSAtom &property);

  static std::string ToSTLString(JSContext *ctx, JSAtom atom);

  static std::string ToSTLString(
      JSContext *context,
      JSValueConst &string);
};

} // namespace qjs
