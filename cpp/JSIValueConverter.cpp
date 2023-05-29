#include "JSIValueConverter.h"

#include "QuickJSPointerValue.h"
#include "ScopedJSValue.h"

namespace qjs {

// static
jsi::Value JSIValueConverter::ToJSIValue(
    const QuickJSRuntime &runtime,
    const JSValueConst &value) {
  auto jsRuntime = runtime.getJSRuntime();
  auto jsContext = runtime.getJSContext();
  if (JS_IsUndefined(value)) {
    return jsi::Value::undefined();
  }
  if (JS_IsNull(value)) {
    return jsi::Value::null();
  }
  if (JS_IsBool(value)) {
    return jsi::Value(JS_VALUE_GET_BOOL(value));
  }
  if (JS_IsNumber(value)) {
    if (JS_TAG_IS_FLOAT64(JS_VALUE_GET_TAG(value))) {
      return jsi::Value(JS_VALUE_GET_FLOAT64(value));
    } else {
      return jsi::Value(JS_VALUE_GET_INT(value));
    }
  }
  if (JS_IsString(value)) {
    return QuickJSRuntime::make<jsi::String>(new QuickJSPointerValue(jsRuntime, jsContext, value));
  }
  if (JS_IsSymbol(value)) {
    return QuickJSRuntime::make<jsi::Symbol>(new QuickJSPointerValue(jsRuntime, jsContext, value));
  }
  if (JS_IsObject(value)) {
    return QuickJSRuntime::make<jsi::Object>(new QuickJSPointerValue(jsRuntime, jsContext, value));
  }
  if (JS_IsBigInt(jsContext, value) || JS_IsBigDecimal(value) || JS_IsBigFloat(value)) {
    return QuickJSRuntime::make<jsi::BigInt>(new QuickJSPointerValue(jsRuntime, jsContext, value));
  }

  return jsi::Value::undefined();
}

JSValue JSIValueConverter::ToJSValue(
    const QuickJSRuntime &constRuntime,
    const jsi::Value &value) {
  QuickJSRuntime &runtime = const_cast<QuickJSRuntime &>(constRuntime);
  auto context = const_cast<JSContext *>(runtime.getJSContext());
  if (value.isUndefined()) {
    return JS_UNDEFINED;
  } else if (value.isNull()) {
    return JS_NULL;
  } else if (value.isBool()) {
    return JS_NewBool(context, value.getBool());
  } else if (value.isNumber()) {
    return JS_NewFloat64(context, value.getNumber());
  } else if (value.isString()) {
    auto str = value.getString(runtime).utf8(runtime);
    return JS_NewStringLen(context, str.c_str(), str.size());
  } else if (value.isObject()) {
    return ToJSObject(runtime, value.getObject(runtime));
  } else if (value.isBigInt()) {
    return ToJSBigInt(runtime, value.getBigInt(runtime));
  } else {
    // What are you?
    std::abort();
  }
}

JSValue JSIValueConverter::ToJSString(
    const QuickJSRuntime &runtime,
    const jsi::String &string) {
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(runtime.getPointerValue(string));
  auto jsObject = quickJSPointerValue->Get(runtime.getJSContext());
  assert(JS_IsString(jsObject));
  return jsObject;
}

JSValue JSIValueConverter::ToJSString(
    const QuickJSRuntime &runtime,
    const jsi::PropNameID &propName) {
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(runtime.getPointerValue(propName));
  auto jsObject = quickJSPointerValue->Get(runtime.getJSContext());
  assert(JS_IsString(jsObject));
  return jsObject;
}

JSValue JSIValueConverter::ToJSSymbol(
    const QuickJSRuntime &runtime,
    const jsi::Symbol &symbol) {
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(runtime.getPointerValue(symbol));
  auto jsObject = quickJSPointerValue->Get(runtime.getJSContext());
  assert(JS_IsSymbol(jsObject));
  return jsObject;
}

JSValue JSIValueConverter::ToJSObject(
    const QuickJSRuntime &runtime,
    const jsi::Object &object) {
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(runtime.getPointerValue(object));
  auto jsObject = quickJSPointerValue->Get(runtime.getJSContext());
  assert(JS_IsObject(jsObject));
  return jsObject;
}

JSValue JSIValueConverter::ToJSBigInt(
    const QuickJSRuntime &runtime,
    const jsi::BigInt &bigInt) {
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(runtime.getPointerValue(bigInt));
  auto jsBigInt = quickJSPointerValue->Get(runtime.getJSContext());
  assert(JS_IsBigInt(runtime.getJSContext(), jsBigInt));
  return jsBigInt;
}

JSValue JSIValueConverter::ToJSArray(
    const QuickJSRuntime &runtime,
    const jsi::Array &array) {
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(runtime.getPointerValue(array));
  auto jsObject = quickJSPointerValue->Get(runtime.getJSContext());
  assert(JS_IsObject(jsObject));
  return jsObject;
}

JSValue JSIValueConverter::ToJSFunction(
    const QuickJSRuntime &runtime,
    const jsi::Function &function) {
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(runtime.getPointerValue(function));
  auto jsObject = quickJSPointerValue->Get(runtime.getJSContext());
  assert(JS_IsFunction(runtime.getJSContext(), jsObject));
  return jsObject;
}

jsi::PropNameID JSIValueConverter::ToJSIPropNameID(
    const QuickJSRuntime &runtime,
    const JSAtom &property) {
  JSValue jsValue = JS_AtomToValue(runtime.getJSContext(), property);
  ScopedJSValue scopeJsValue(runtime.getJSContext(), &jsValue);
  return runtime.make<jsi::PropNameID>(
      new QuickJSPointerValue(runtime.getJSRuntime(), runtime.getJSContext(), jsValue));
}

std::string JSIValueConverter::ToSTLString(
    JSContext *ctx, JSAtom atom) {
  const char *str = JS_AtomToCString(ctx, atom);
  ScopedCString scopedCString(ctx, str);
  if (str) {
    return std::string(str);
  }
  return {};
}

// static
std::string JSIValueConverter::ToSTLString(
    JSContext *context,
    JSValueConst &string) {
  assert(JS_IsString(string));
  const char *str = JS_ToCString(context, string);
  ScopedCString scopedCString(context, str);
  return std::string(str);
}

} // namespace qjs
