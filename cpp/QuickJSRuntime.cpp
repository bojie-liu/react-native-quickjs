#include "QuickJSRuntime.h"

#include <string.h>

#include <glog/logging.h>
#include <set>
#include <sstream>
#include "cutils.h"
#include "QuickJSPointerValue.h"
#include "ScopedJSValue.h"
#include "JSIValueConverter.h"
#include "HostProxy.h"
#include <jsi/jsilib.h>

#include "QuickJSInstrumentation.h"

#include <folly/FileUtil.h>
#include <glog/logging.h>

namespace qjs {

static void js_dump_obj(JSContext *ctx, JSValueConst val) {
  const char *str;

  str = JS_ToCString(ctx, val);
  if (str) {
    LOG(ERROR) << "QuickJS exception " << str;
    JS_FreeCString(ctx, str);
  } else {
    LOG(ERROR) << "QuickJS exception ";
  }
}

static void js_std_dump_error1(JSContext *ctx, JSValueConst exception_val) {
  JSValue val;
  BOOL is_error;

  is_error = JS_IsError(ctx, exception_val);
  js_dump_obj(ctx, exception_val);
  if (is_error) {
    val = JS_GetPropertyStr(ctx, exception_val, "stack");
    if (!JS_IsUndefined(val)) {
      js_dump_obj(ctx, val);
    }
    JS_FreeValue(ctx, val);
  }
}

__maybe_unused void js_std_dump_error(JSContext *ctx) {
  JSValue exception_val;

  exception_val = JS_GetException(ctx);
  js_std_dump_error1(ctx, exception_val);
  JS_FreeValue(ctx, exception_val);
}

QuickJSRuntime::QuickJSRuntime() {
  runtime_ = JS_NewRuntime();
  JS_SetMaxStackSize(runtime_, 1024 * 1024 * 1024);
  context_ = JS_NewContext(runtime_);
  if (context_ == nullptr) {
    JS_FreeRuntime(runtime_);
  }

  JS_SetRuntimeInfo(runtime_, "RNQuickJS");
  JS_SetCanBlock(runtime_, true);

  instrumentation_ = std::make_unique<QuickJSInstrumentation>(this);
}

QuickJSRuntime::~QuickJSRuntime() {
  for (;;) {
    JSContext *ctx1;
    int ret = JS_ExecutePendingJob(JS_GetRuntime(context_), &ctx1);
    if (ret < 0) {
      checkAndThrowException(context_);
    } else if (ret == 0) {
      break;
    }
  }

  JS_FreeContext(context_);
  JS_FreeRuntime(runtime_);
}

std::unordered_map<std::string, int64_t> QuickJSRuntime::getHeapInfo() {
  JSMemoryUsage memoryUsage;
  JS_ComputeMemoryUsage(runtime_, &memoryUsage);
  return {{"malloc_size", memoryUsage.malloc_size},
      {"memory_used_size", memoryUsage.memory_used_size},
      {"malloc_count", memoryUsage.malloc_count},
      {"memory_used_count", memoryUsage.memory_used_count},
      {"atom_size", memoryUsage.atom_size},
      {"str_size", memoryUsage.str_size},
      {"obj_size", memoryUsage.obj_size},
      {"prop_size", memoryUsage.prop_size},
      {"shape_size", memoryUsage.shape_size},
      {"js_func_size", memoryUsage.js_func_size},
      {"js_func_code_size", memoryUsage.js_func_code_size},
      {"js_func_pc2line_size", memoryUsage.js_func_pc2line_size},
      {"c_func_count", memoryUsage.c_func_count},
      {"array_count", memoryUsage.array_count},
      {"fast_array_count", memoryUsage.fast_array_count},
      {"fast_array_elements", memoryUsage.fast_array_elements},
      {"binary_object_size", memoryUsage.binary_object_size}};
}

void QuickJSRuntime::checkAndThrowException(JSContext *context) const {
  JSValue exceptionValue = JS_GetException(context);
  if (!JS_IsNull(exceptionValue)) {
    ScopedJSValue scopedJsValue(context, &exceptionValue);
    throw jsi::JSError(const_cast<QuickJSRuntime &>(*this),
                       JSIValueConverter::ToJSIValue(*this, exceptionValue));
  }
}

void QuickJSRuntime::loadCodeCache(CodeCacheItem &codeCacheItem, const std::string &url) {
  std::string codeCachePath = "/data/data/com.facebook.react.uiapp/files/codecache/" + url;
  std::vector<uint8_t> buffer;
  if (folly::readFile(codeCachePath.c_str(), buffer)) {
    auto *data = new uint8_t[buffer.size()];
    memcpy(data, buffer.data(), buffer.size());
    codeCacheItem.data = std::unique_ptr<uint8_t>(data);
    codeCacheItem.size = buffer.size();
    codeCacheItem.result = CodeCacheItem::INITIALIZED;
  }
}

void QuickJSRuntime::updateCodeCache(CodeCacheItem &codeCacheItem, const std::string &url) {
  std::string codeCachePath = "/data/data/com.facebook.react.uiapp/files/codecache/" + url;
  std::vector<uint8_t> buffer(codeCacheItem.data.get(), codeCacheItem.data.get() + codeCacheItem
      .size);
  if (folly::writeFile(buffer, codeCachePath.c_str())) {
    codeCacheItem.result = CodeCacheItem::UPDATED;
  }
}

//
// jsi::Runtime implementations
//
double performanceNow() {
  auto time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
      time.time_since_epoch())
      .count();

  constexpr double NANOSECONDS_IN_MILLISECOND = 1000000.0;
  return duration / NANOSECONDS_IN_MILLISECOND;
}

jsi::Value QuickJSRuntime::evaluateJavaScript(
    const std::shared_ptr<const jsi::Buffer> &buffer,
    const std::string &sourceURL) {
  bool enableCodeCache = true;
  JSValue retValue;
  ScopedJSValue scopedJsValue(context_, &retValue);

  if (enableCodeCache) {
    CodeCacheItem codeCacheItem;
    loadCodeCache(codeCacheItem, sourceURL);
    bool hasCodeCache = (codeCacheItem.result == CodeCacheItem::INITIALIZED);
    JSValue func, cachedFunc;
    if (hasCodeCache) {
      func = JS_ReadObject(
          context_,
          codeCacheItem.data.get(),
          codeCacheItem.size,
          JS_READ_OBJ_BYTECODE);
    } else {
      func = JS_Eval(
          context_,
          (const char *) buffer->data(),
          buffer->size(),
          sourceURL.c_str(),
          JS_EVAL_TYPE_GLOBAL | JS_EVAL_FLAG_COMPILE_ONLY);
      cachedFunc = JS_DupValue(context_, func);
    }
    checkAndThrowException(context_);

    JSValue retValue = JS_EvalFunction(context_, func);
    ScopedJSValue scopedResult(context_, &retValue);

    checkAndThrowException(context_);

    for (;;) {
      JSContext *ctx1;
      int ret = JS_ExecutePendingJob(JS_GetRuntime(context_), &ctx1);
      if (ret < 0) {
        checkAndThrowException(context_);
      } else if (ret == 0) {
        break;
      }
    }

    if (!hasCodeCache) {
      size_t size;
      uint8_t *buf =
          JS_WriteObject(context_, &size, cachedFunc, JS_WRITE_OBJ_BYTECODE);
      ScopedJSValue scopedCachedFunc(context_, &cachedFunc);
      if (buf && size != 0) {
        codeCacheItem.data = std::unique_ptr<uint8_t>(buf);
        codeCacheItem.size = size;
        codeCacheItem.result = CodeCacheItem::REQUEST_UPDATE;
        updateCodeCache(codeCacheItem, sourceURL);
      } else {
        throw std::logic_error("no code cache");
      }
    }
  } else {
    retValue = JS_Eval(
        context_,
        (const char *) buffer->data(),
        buffer->size(),
        sourceURL.c_str(),
        JS_EVAL_TYPE_GLOBAL);

    checkAndThrowException(context_);
    for (;;) {
      JSContext *ctx1;
      int ret = JS_ExecutePendingJob(JS_GetRuntime(context_), &ctx1);
      if (ret < 0) {
        checkAndThrowException(context_);
      } else if (ret == 0) {
        break;
      }
    }
  }
  return JSIValueConverter::ToJSIValue(*this, retValue);
}

std::shared_ptr<const jsi::PreparedJavaScript> QuickJSRuntime::prepareJavaScript(
    const std::shared_ptr<const jsi::Buffer> &buffer,
    std::string sourceURL) {
  return std::make_shared<jsi::SourceJavaScriptPreparation>(
      buffer, std::move(sourceURL));
}

jsi::Value QuickJSRuntime::evaluatePreparedJavaScript(
    const std::shared_ptr<const jsi::PreparedJavaScript> &js) {
  assert(
      dynamic_cast<const jsi::SourceJavaScriptPreparation *>(js.get()) &&
          "preparedJavaScript must be a SourceJavaScriptPreparation");
  auto sourceJs =
      std::static_pointer_cast<const jsi::SourceJavaScriptPreparation>(js);
  return evaluateJavaScript(sourceJs, sourceJs->sourceURL());
}

jsi::Object QuickJSRuntime::global() {
  // TRACE_SCOPE("QuickJSRuntime", "misc");
  JSValue global = JS_GetGlobalObject(context_);
  ScopedJSValue scopedJsValue(context_, &global);
  return make<jsi::Object>(
      new QuickJSPointerValue(runtime_, context_, global));
}

std::string QuickJSRuntime::description() {
  std::ostringstream ss;
  ss << "<QuickJSRuntime@" << this << ">";
  return ss.str();
}

bool QuickJSRuntime::isInspectable() {
  return false;
}

jsi::Instrumentation &QuickJSRuntime::instrumentation() {
  return *instrumentation_;
}

// These clone methods are shallow clone
jsi::Runtime::PointerValue *QuickJSRuntime::cloneSymbol(
    const Runtime::PointerValue *pv) {
  // TRACE_SCOPE("QuickJSRuntime", "string");
  if (!pv) {
    return nullptr;
  }
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(pv);
  JSValue jsValue = quickJSPointerValue->Get(context_);
  assert(JS_IsSymbol(jsValue));
  ScopedJSValue scopedJsValue(context_, &jsValue);

  return new QuickJSPointerValue(runtime_, context_, jsValue);
}

jsi::Runtime::PointerValue *QuickJSRuntime::cloneBigInt(
    const Runtime::PointerValue *pv) {
  // TRACE_SCOPE("QuickJSRuntime", "string");
  if (!pv) {
    return nullptr;
  }
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(pv);
  JSValue jsValue = quickJSPointerValue->Get(context_);
  assert(JS_IsBigInt(context_, jsValue));
  ScopedJSValue scopedJsValue(context_, &jsValue);

  return new QuickJSPointerValue(runtime_, context_, jsValue);
}

jsi::Runtime::PointerValue *QuickJSRuntime::cloneString(
    const Runtime::PointerValue *pv) {
  // TRACE_SCOPE("QuickJSRuntime", "string");
  if (!pv) {
    return nullptr;
  }

  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(pv);
  JSValue jsValue = quickJSPointerValue->Get(context_);
  assert(JS_IsString(jsValue));
  ScopedJSValue scopedJsValue(context_, &jsValue);

  return new QuickJSPointerValue(runtime_, context_, jsValue);
}

jsi::Runtime::PointerValue *QuickJSRuntime::cloneObject(
    const Runtime::PointerValue *pv) {
  // TRACE_SCOPE("QuickJSRuntime", "object");
  if (!pv) {
    return nullptr;
  }
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(pv);
  JSValue jsValue = quickJSPointerValue->Get(context_);
  assert(JS_IsObject(jsValue));
  ScopedJSValue scopedJsValue(context_, &jsValue);

  return new QuickJSPointerValue(runtime_, context_, jsValue);
}

jsi::Runtime::PointerValue *QuickJSRuntime::clonePropNameID(
    const Runtime::PointerValue *pv) {
  // TRACE_SCOPE("QuickJSRuntime", "string");
  return cloneString(pv);
}

bool QuickJSRuntime::bigintIsInt64(const jsi::BigInt &bigInt) {
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(getPointerValue(bigInt));
  JSValue jsValue = quickJSPointerValue->Get(context_);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  auto result = JS_IsNumber(jsValue) || JS_IsBigInt(context_, jsValue);
  checkAndThrowException(context_);
  return result;
}

bool QuickJSRuntime::bigintIsUint64(const jsi::BigInt &bigInt) {
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(getPointerValue(bigInt));
  JSValue jsValue = quickJSPointerValue->Get(context_);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  auto result = JS_IsNumber(jsValue) || JS_IsBigInt(context_, jsValue);
  checkAndThrowException(context_);
  return result;
}

uint64_t QuickJSRuntime::truncate(const jsi::BigInt &bigInt) {
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(getPointerValue(bigInt));
  JSValue jsValue = quickJSPointerValue->Get(context_);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  uint64_t result;
  JS_ToIndex(context_, &result, jsValue);
  checkAndThrowException(context_);
  return result;
}

bool QuickJSRuntime::hasNativeState(const jsi::Object &object) {
  JSValue jsValue = JSIValueConverter::ToJSObject(*this, object);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  auto opaqueData = reinterpret_cast<OpaqueData *>(JS_GetOpaqueUnsafe(jsValue));
  assert(opaqueData);
  return opaqueData->nativeState_ != nullptr;
}

std::shared_ptr<jsi::NativeState> QuickJSRuntime::getNativeState(const jsi::Object &object) {
  JSValue jsValue = JSIValueConverter::ToJSObject(*this, object);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  auto opaqueData = reinterpret_cast<OpaqueData *>(JS_GetOpaqueUnsafe(jsValue));
  assert(opaqueData);
  return opaqueData->nativeState_;
}

void QuickJSRuntime::setNativeState(
    const jsi::Object &object,
    std::shared_ptr<jsi::NativeState> state) {
  JSValue jsValue = JSIValueConverter::ToJSObject(*this, object);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  auto opaqueData = reinterpret_cast<OpaqueData *>(JS_GetOpaqueUnsafe(jsValue));
  assert(opaqueData);
  opaqueData->nativeState_ = state;
}

bool QuickJSRuntime::drainMicrotasks(int maxMicrotasksHint) {
  int taskNum = 0;
  int ret;
  for (;;) {
    if (taskNum < maxMicrotasksHint || maxMicrotasksHint == -1) {
      JSContext *ctx1;
      ret = JS_ExecutePendingJob(JS_GetRuntime(context_), &ctx1);
      taskNum++;
      if (ret < 0) {
        checkAndThrowException(context_);
      } else if (ret == 0) {
        return true;
      }
    } else {
      break;
    }
  }
  return false;
}

jsi::PropNameID QuickJSRuntime::createPropNameIDFromAscii(
    const char *str,
    size_t length) {
  // TRACE_SCOPE("QuickJSRuntime", "string");
  JSValue jsValue = JS_NewStringLen(context_, str, length);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  QuickJSPointerValue *value = new QuickJSPointerValue(runtime_, context_, jsValue);
  return make<jsi::PropNameID>(value);
}

jsi::PropNameID QuickJSRuntime::createPropNameIDFromUtf8(
    const uint8_t *utf8,
    size_t length) {
  // TRACE_SCOPE("QuickJSRuntime", "string");
  JSValue jsValue = JS_NewStringLen(context_, (const char *) utf8, length);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  QuickJSPointerValue *value = new QuickJSPointerValue(runtime_, context_, jsValue);
  return make<jsi::PropNameID>(value);
}

jsi::PropNameID QuickJSRuntime::createPropNameIDFromString(const jsi::String &str) {
  // TRACE_SCOPE("QuickJSRuntime", "string");
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(getPointerValue(str));
  JSValue jsValue = quickJSPointerValue->Get(context_);
  ScopedJSValue scopedJsValue(context_, &jsValue);

  assert(JS_IsString(jsValue));

  auto utf8 = JS_ToCString(context_, jsValue);
  auto result = createPropNameIDFromUtf8(
      reinterpret_cast<const uint8_t *>(utf8), strlen(utf8));
  JS_FreeCString(context_, utf8);
  return result;
}

std::string QuickJSRuntime::utf8(const jsi::PropNameID &sym) {
  // TRACE_SCOPE("QuickJSRuntime", "string");
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(getPointerValue(sym));
  JSValue jsValue = quickJSPointerValue->Get(context_);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  return JSIValueConverter::ToSTLString(context_, jsValue);
}

bool QuickJSRuntime::compare(const jsi::PropNameID &a, const jsi::PropNameID &b) {
  // TRACE_SCOPE("QuickJSRuntime", "misc");
  const QuickJSPointerValue *quickJSPointValueA =
      static_cast<const QuickJSPointerValue *>(getPointerValue(a));
  JSValue jsValueA = quickJSPointValueA->Get(context_);
  const QuickJSPointerValue *quickJSPointValueB =
      static_cast<const QuickJSPointerValue *>(getPointerValue(b));
  JSValue jsValueB = quickJSPointValueB->Get(context_);

  ScopedJSValue scopedJsValueA(context_, &jsValueA);
  ScopedJSValue scopedJsValueB(context_, &jsValueB);
  auto result = JS_IsStrictEqual(context_, jsValueA, jsValueB);
  checkAndThrowException(context_);
  return result;
}

std::string QuickJSRuntime::symbolToString(const jsi::Symbol &symbol) {
  // TRACE_SCOPE("QuickJSRuntime", "string");
  return jsi::Value(*this, symbol).toString(*this).utf8(*this);
}

jsi::String QuickJSRuntime::createStringFromAscii(const char *str, size_t length) {
  // TRACE_SCOPE("QuickJSRuntime", "string");
  JSValue jsValue = JS_NewStringLen(context_, str, length);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  QuickJSPointerValue *value = new QuickJSPointerValue(runtime_, context_, jsValue);
  return make<jsi::String>(value);
}

jsi::String QuickJSRuntime::createStringFromUtf8(const uint8_t *str, size_t length) {
  // TRACE_SCOPE("QuickJSRuntime", "string");
  JSValue jsValue = JS_NewStringLen(context_, (const char *) str, length);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  QuickJSPointerValue *value = new QuickJSPointerValue(runtime_, context_, jsValue);
  return make<jsi::String>(value);
}

std::string QuickJSRuntime::utf8(const jsi::String &str) {
  // TRACE_SCOPE("QuickJSRuntime", "string");
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(getPointerValue(str));
  JSValue jsValue = quickJSPointerValue->Get(context_);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  assert(JS_IsString(jsValue));

  return JSIValueConverter::ToSTLString(context_, jsValue);
}

jsi::Object QuickJSRuntime::createObject() {
  // TRACE_SCOPE("QuickJSRuntime", "object");
  JSValue jsValue = JS_NewObject(context_);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  return make<jsi::Object>(new QuickJSPointerValue(runtime_, context_, jsValue));
}

jsi::Object QuickJSRuntime::createObject(
    std::shared_ptr<jsi::HostObject> hostObject) {
  // TRACE_SCOPE("QuickJSRuntime", "object");

  HostObjectProxy *hostObjectProxy =
      new HostObjectProxy(*this, hostObject);

  JSClassID jsClassID = HostObjectProxy::GetClassID();

  // create prototype
  JSValue proto = JS_GetClassProtoOrNull(context_, jsClassID);
  if (JS_IsNull(proto)) {
    proto = JS_NewObject(context_);

    // JS_NewClassID(&js_point_class_id);
    JS_NewClass(
        JS_GetRuntime(context_), jsClassID, &HostObjectProxy::kJSClassDef);
    JS_SetPropertyFunctionList(
        context_, proto, HostObjectProxy::kTemplateInterceptor, 1);

    // JSValue constructor;
    /* create the Point class */
    // constructor = JS_NewCFunction2(context_, js_point_ctor, "Point", 2,
    // JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    // JS_SetConstructor(context_, constructor, proto);
    JS_SetClassProto(context_, jsClassID, proto);
    JS_DupValue(context_, proto);
  }
  JSValue object =
      JS_NewObjectProtoClass(context_, proto, jsClassID);
  ScopedJSValue scopedJsValue(context_, &object);
  ScopedJSValue scopedJsProto(context_, &proto);

  JS_SetOpaque(object, hostObjectProxy->GetOpaqueData());

  return make<jsi::Object>(new QuickJSPointerValue(runtime_, context_, object));
}

std::shared_ptr<jsi::HostObject> QuickJSRuntime::getHostObject(
    const jsi::Object &object) {
  // TRACE_SCOPE("QuickJSRuntime", "object");
  assert(isHostObject(object));

  // We are guarenteed at this point to have isHostObject(obj) == true
  // so the internal data should be HostObjectMetadata
  JSValue jsValue = JSIValueConverter::ToJSObject(*this, object);
  ScopedJSValue scopedJsValue(context_, &jsValue);
  auto opaqueData = reinterpret_cast<OpaqueData *>(JS_GetOpaque(jsValue,
                                                                HostObjectProxy::GetClassID()));
  assert(opaqueData);
  auto hostObjectProxy = reinterpret_cast<HostObjectProxy *>(opaqueData->hostData_);
  return hostObjectProxy->GetHostObject();
}

jsi::HostFunctionType &QuickJSRuntime::getHostFunction(
    const jsi::Function &function) {
  assert(isHostFunction(function));

  // We know that isHostFunction(function) is true here, so its safe to proceed
  const QuickJSPointerValue *quickJSPointerValue =
      static_cast<const QuickJSPointerValue *>(getPointerValue(function));
  auto jsFunction = quickJSPointerValue->Get(context_);
  ScopedJSValue scopedJsValue(context_, &jsFunction);
  assert(JS_IsFunction(context_, jsFunction));

  auto opaqueData = reinterpret_cast<OpaqueData *>(JS_GetOpaque(jsFunction,
                                                                HostObjectProxy::GetClassID()));
  assert(opaqueData);
  auto hostFunctionProxy = reinterpret_cast<HostFunctionProxy *>(opaqueData->hostData_);
  return hostFunctionProxy->GetHostFunction();
}

jsi::Value QuickJSRuntime::getProperty(
    const jsi::Object &object,
    const jsi::PropNameID &name) {
  // TRACE_SCOPE("QuickJSRuntime", "object");
  auto jsValue = JSIValueConverter::ToJSObject(*this, object);
  auto jsName = name.utf8(*this);
  auto prop = JS_GetPropertyStr(context_, jsValue, jsName.c_str());
  ScopedJSValue scopeValue(context_, &jsValue);
  ScopedJSValue scopeProp(context_, &prop);

  checkAndThrowException(context_);

  return JSIValueConverter::ToJSIValue(*this, prop);
}

jsi::Value QuickJSRuntime::getProperty(
    const jsi::Object &object,
    const jsi::String &name) {
  // TRACE_SCOPE("QuickJSRuntime", "object");
  auto jsValue = JSIValueConverter::ToJSObject(*this, object);
  auto jsName = name.utf8(*this);
  auto prop = JS_GetPropertyStr(context_, jsValue, jsName.c_str());
  ScopedJSValue scopeValue(context_, &jsValue);
  ScopedJSValue scopeProp(context_, &prop);

  checkAndThrowException(context_);

  return JSIValueConverter::ToJSIValue(*this, prop);
}

bool QuickJSRuntime::hasProperty(
    const jsi::Object &object,
    const jsi::PropNameID &name) {
  // TRACE_SCOPE("QuickJSRuntime", "object");
  auto jsValue = JSIValueConverter::ToJSObject(*this, object);
  ScopedJSValue scopeValue(context_, &jsValue);
  auto jsName = JS_NewAtom(context_, name.utf8(*this).c_str());

  return JS_HasProperty(context_, jsValue, jsName) == TRUE;
}

bool QuickJSRuntime::hasProperty(
    const jsi::Object &object,
    const jsi::String &name) {
  // TRACE_SCOPE("QuickJSRuntime", "object");
  auto jsValue = JSIValueConverter::ToJSObject(*this, object);
  ScopedJSValue scopeValue(context_, &jsValue);
  auto jsName = JS_NewAtom(context_, name.utf8(*this).c_str());

  return JS_HasProperty(context_, jsValue, jsName) == TRUE;
}

void QuickJSRuntime::setPropertyValue(
    jsi::Object &object,
    const jsi::PropNameID &name,
    const jsi::Value &value) {
  // TRACE_SCOPE("QuickJSRuntime", "object");
  auto jsValue = JSIValueConverter::ToJSObject(*this, object);
  auto jsProperty = JSIValueConverter::ToJSValue(*this, value);
  auto jsName = name.utf8(*this);
  ScopedJSValue scopeValue(context_, &jsValue);

  // DO NOT FREE jsProperty
  JS_SetPropertyStr(context_, jsValue, jsName.c_str(), jsProperty);
  checkAndThrowException(context_);
}

void QuickJSRuntime::setPropertyValue(
    jsi::Object &object,
    const jsi::String &name,
    const jsi::Value &value) {
  // TRACE_SCOPE("QuickJSRuntime", "object");
  auto jsValue = JSIValueConverter::ToJSObject(*this, object);
  auto jsProperty = JSIValueConverter::ToJSValue(*this, value);
  auto jsName = name.utf8(*this);
  ScopedJSValue scopeValue(context_, &jsValue);

  // DO NOT FREE jsProperty
  JS_SetPropertyStr(context_, jsValue, jsName.c_str(), jsProperty);
  checkAndThrowException(context_);
}

bool QuickJSRuntime::isArray(const jsi::Object &object) const {
  // TRACE_SCOPE("QuickJSRuntime", "array");
  auto jsValue = JSIValueConverter::ToJSObject(*this, object);
  ScopedJSValue scopeValue(context_, &jsValue);

  return JS_IsArray(context_, jsValue) == TRUE;
}

bool QuickJSRuntime::isArrayBuffer(const jsi::Object &object) const {
  // TRACE_SCOPE("QuickJSRuntime", "array");
  auto jsValue = JSIValueConverter::ToJSObject(*this, object);
  ScopedJSValue scopeValue(context_, &jsValue);

  return JS_IsArrayBuffer(context_, jsValue) == TRUE;
}

bool QuickJSRuntime::isFunction(const jsi::Object &object) const {
  // TRACE_SCOPE("QuickJSRuntime", "object");
  auto jsValue = JSIValueConverter::ToJSObject(*this, object);
  ScopedJSValue scopeValue(context_, &jsValue);

  return JS_IsFunction(context_, jsValue) == TRUE;
}

bool QuickJSRuntime::isHostObject(const jsi::Object &object) const {
  // TRACE_SCOPE("QuickJSRuntime", "object");
  auto jsValue = JSIValueConverter::ToJSObject(*this, object);
  ScopedJSValue scopeValue(context_, &jsValue);

  return JS_GetOpaque(jsValue, HostObjectProxy::GetClassID()) != nullptr;
}

bool QuickJSRuntime::isHostFunction(const jsi::Function &function) const {
  // TRACE_SCOPE("QuickJSRuntime", "object");
  auto jsValue = JSIValueConverter::ToJSFunction(*this, function);
  ScopedJSValue scopeValue(context_, &jsValue);

  return JS_GetOpaque(jsValue, HostObjectProxy::GetClassID()) != nullptr;
}

jsi::Array QuickJSRuntime::getPropertyNames(const jsi::Object &object) {
  // TRACE_SCOPE("QuickJSRuntime", "object");
  auto jsValue = JSIValueConverter::ToJSObject(*this, object);
  ScopedJSValue scopeValue(context_, &jsValue);

  JSPropertyEnum *names;
  uint32_t size;
  int ret = JS_GetOwnPropertyNames(context_,
                                   &names,
                                   &size,
                                   jsValue,
                                   JS_GPN_ENUM_ONLY | JS_GPN_STRING_MASK);
  checkAndThrowException(context_);

  auto result = JS_NewArray(context_);
  ScopedJSValue scopeResult(context_, &result);
  for (int i = 0; i < size; i++) {
    JSValue prop = JS_AtomToValue(context_, names[i].atom);
    // DO NOT FREE prop
    JS_SetPropertyUint32(context_, result, i, prop);
    checkAndThrowException(context_);
  }

  JS_FreeEnumArray(context_, names, size);

  return make<jsi::Object>(new QuickJSPointerValue(runtime_, context_, result)).getArray(*this);
}

jsi::WeakObject QuickJSRuntime::createWeakObject(const jsi::Object &weakObject) {
  throw std::logic_error("Not implemented");
}

jsi::Value QuickJSRuntime::lockWeakObject(jsi::WeakObject &weakObject) {
  throw std::logic_error("Not implemented");
}

jsi::Array QuickJSRuntime::createArray(size_t length) {
  // TRACE_SCOPE("QuickJSRuntime", "array");
  auto result = JS_NewArray(context_);
  ScopedJSValue scopeResult(context_, &result);

  return make<jsi::Object>(new QuickJSPointerValue(runtime_, context_, result)).getArray(*this);
}

size_t QuickJSRuntime::size(const jsi::Array &array) {
  // TRACE_SCOPE("QuickJSRuntime", "array");
  auto jsValue = JSIValueConverter::ToJSArray(*this, array);
  ScopedJSValue scopeValue(context_, &jsValue);

  auto lengthValue = JS_GetArrayLength(context_, jsValue);

  checkAndThrowException(context_);

  return JS_VALUE_GET_INT(lengthValue);
}

size_t QuickJSRuntime::size(const jsi::ArrayBuffer &arrayBuffer) {
  throw std::logic_error("Not implemented");
}

uint8_t *QuickJSRuntime::data(const jsi::ArrayBuffer &arrayBuffer) {
  throw std::logic_error("Not implemented");
}

jsi::Value QuickJSRuntime::getValueAtIndex(const jsi::Array &array, size_t i) {
  // TRACE_SCOPE("QuickJSRuntime", "array");
  auto jsValue = JSIValueConverter::ToJSArray(*this, array);
  ScopedJSValue scopeValue(context_, &jsValue);

  JSValue property = JS_GetPropertyUint32(context_, jsValue, i);
  ScopedJSValue scopeProperty(context_, &property);

  checkAndThrowException(context_);
  return JSIValueConverter::ToJSIValue(*this, property);
}

void QuickJSRuntime::setValueAtIndexImpl(
    jsi::Array &array,
    size_t i,
    const jsi::Value &value) {
  // TRACE_SCOPE("QuickJSRuntime", "array");
  auto jsValue = JSIValueConverter::ToJSArray(*this, array);
  ScopedJSValue scopeValue(context_, &jsValue);

  auto property = JSIValueConverter::ToJSValue(*this, value);
  // DO NOT FREE property
  if (JS_SetPropertyUint32(context_, jsValue, i, property) != TRUE) {
    checkAndThrowException(context_);
  }
}

jsi::Function QuickJSRuntime::createFunctionFromHostFunction(
    const jsi::PropNameID &name,
    unsigned int paramCount,
    jsi::HostFunctionType func) {
  // TRACE_SCOPE("QuickJSRuntime", "object");

  HostFunctionProxy *hostFunctionProxy =
      new HostFunctionProxy(*this, std::move(func));

  JSClassID jsClassID = HostFunctionProxy::GetClassID();

  // create prototype
  JSValue proto = JS_GetClassProtoOrNull(context_, jsClassID);
  if (JS_IsNull(proto)) {
    proto = JS_NewObject(context_);

    // JS_NewClassID(&js_point_class_id);
    JS_NewClass(
        JS_GetRuntime(context_), jsClassID, &HostFunctionProxy::kJSClassDef);

    // JSValue constructor;
    /* create the Point class */
    // constructor = JS_NewCFunction2(context_, js_point_ctor, "Point", 2,
    // JS_CFUNC_constructor, 0);
    /* set proto.constructor and ctor.prototype */
    // JS_SetConstructor(context_, constructor, proto);
    JS_SetClassProto(context_, jsClassID, proto);
    JS_DupValue(context_, proto);
  }

  JSValue object =
      JS_NewObjectProtoClass(context_, proto, jsClassID);
  ScopedJSValue scopedJsValue(context_, &object);
  ScopedJSValue scopedJsProto(context_, &proto);

  JS_SetOpaque(object, hostFunctionProxy->GetOpaqueData());

  return make<jsi::Object>(new QuickJSPointerValue(runtime_, context_, object))
      .getFunction(*this);
}

jsi::Value QuickJSRuntime::call(
    const jsi::Function &function,
    const jsi::Value &jsThis,
    const jsi::Value *args,
    size_t count) {

  auto jsFunction = JSIValueConverter::ToJSFunction(*this, function);
  ScopedJSValue scopedJsFunction(context_, &jsFunction);

  auto jsObject = jsThis.isUndefined() ? JS_GetGlobalObject(context_) :
      JSIValueConverter::ToJSValue(*this, jsThis);
  ScopedJSValue scopedJsObject(context_, &jsObject);

  JSValue *argv = new JSValue[count];
  for (size_t i = 0; i < count; i++) {
    argv[i] = JSIValueConverter::ToJSValue(*this, args[i]);
  }

  auto result = JS_Call(context_, jsFunction, jsObject,
                        count, argv);
  ScopedJSValue scopeResult(context_, &result);

  for (size_t i = 0; i < count; i++) {
    JS_FreeValue(context_, argv[i]);
  }

  checkAndThrowException(context_);

  return JSIValueConverter::ToJSIValue(*this, result);
}

jsi::Value QuickJSRuntime::callAsConstructor(
    const jsi::Function &function,
    const jsi::Value *args,
    size_t count) {

  auto jsFunction = JSIValueConverter::ToJSFunction(*this, function);
  ScopedJSValue scopedJsFunction(context_, &jsFunction);

  JSValue *argv = new JSValue[count];
  for (size_t i = 0; i < count; i++) {
    argv[i] = JSIValueConverter::ToJSValue(*this, args[i]);
  }

  auto result = JS_CallConstructor(context_, jsFunction, count, argv);
  ScopedJSValue scopeResult(context_, &result);

  for (size_t i = 0; i < count; i++) {
    JS_FreeValue(context_, argv[i]);
  }

  checkAndThrowException(context_);

  return JSIValueConverter::ToJSIValue(*this, result);
}

bool QuickJSRuntime::strictEquals(const jsi::Symbol &a, const jsi::Symbol &b) const {
  // TRACE_SCOPE("QuickJSRuntime", "misc");
  JSValue jsSymbolA = JSIValueConverter::ToJSSymbol(*this, a);
  ScopedJSValue scopedJsSymbolA(context_, &jsSymbolA);
  JSValue jsSymbolB = JSIValueConverter::ToJSSymbol(*this, b);
  ScopedJSValue scopedJsSymbolB(context_, &jsSymbolB);

  bool result = (JS_IsStrictEqual(context_, jsSymbolA, jsSymbolB) == TRUE);

  checkAndThrowException(context_);

  return result;
}

bool QuickJSRuntime::strictEquals(const jsi::String &a, const jsi::String &b) const {
  // TRACE_SCOPE("QuickJSRuntime", "misc");
  JSValue jsStringA = JSIValueConverter::ToJSString(*this, a);
  ScopedJSValue scopedJsStringA(context_, &jsStringA);
  JSValue jsStringB = JSIValueConverter::ToJSString(*this, b);
  ScopedJSValue scopedJsStringB(context_, &jsStringB);

  bool result = (JS_IsStrictEqual(context_, jsStringA, jsStringB) == TRUE);

  checkAndThrowException(context_);

  return result;
}

bool QuickJSRuntime::strictEquals(const jsi::Object &a, const jsi::Object &b) const {
  // TRACE_SCOPE("QuickJSRuntime", "misc");
  JSValue jsObjectA = JSIValueConverter::ToJSObject(*this, a);
  ScopedJSValue scopedJsObjectA(context_, &jsObjectA);
  JSValue jsObjectB = JSIValueConverter::ToJSObject(*this, b);
  ScopedJSValue scopedJsObjectB(context_, &jsObjectB);

  bool result = (JS_IsStrictEqual(context_, jsObjectA, jsObjectB) == TRUE);

  checkAndThrowException(context_);

  return result;
}

bool QuickJSRuntime::instanceOf(const jsi::Object &o, const jsi::Function &f) {
  // TRACE_SCOPE("QuickJSRuntime", "misc");
  JSValue jsObjectA = JSIValueConverter::ToJSObject(*this, o);
  ScopedJSValue scopedJsObjectA(context_, &jsObjectA);
  JSValue jsObjectB = JSIValueConverter::ToJSFunction(*this, f);
  ScopedJSValue scopedJsObjectB(context_, &jsObjectB);

  bool result = (JS_IsInstanceOf(context_, jsObjectA, jsObjectB) == TRUE);

  checkAndThrowException(context_);

  return result;
}

jsi::PropNameID QuickJSRuntime::createPropNameIDFromSymbol(const jsi::Symbol &sym) {
  auto str = sym.toString(*this);
  return createPropNameIDFromAscii(str.c_str(), str.length());
}

jsi::ArrayBuffer QuickJSRuntime::createArrayBuffer(
    std::shared_ptr<jsi::MutableBuffer> buffer) {
  throw std::logic_error("Not implemented");
}

bool QuickJSRuntime::strictEquals(const jsi::BigInt &a, const jsi::BigInt &b) const {
  const QuickJSPointerValue *pointerA =
      static_cast<const QuickJSPointerValue *>(getPointerValue(a));
  const QuickJSPointerValue *pointerB =
      static_cast<const QuickJSPointerValue *>(getPointerValue(b));
  JSValue valueA = pointerA->Get(context_);
  JSValue valueB = pointerB->Get(context_);
  ScopedJSValue scopedJsValueA(context_, &valueA);
  ScopedJSValue scopedJsValueB(context_, &valueB);

  return JS_IsStrictEqual(context_, valueA, valueB);
}

jsi::BigInt QuickJSRuntime::createBigIntFromInt64(int64_t v) {
  return make<jsi::BigInt>(
      new QuickJSPointerValue(runtime_, context_, JS_NewBigInt64(context_, v)));
}

jsi::BigInt QuickJSRuntime::createBigIntFromUint64(uint64_t v) {
  return make<jsi::BigInt>(
      new QuickJSPointerValue(runtime_, context_, JS_NewBigUint64(context_, v)));
}

jsi::String QuickJSRuntime::bigintToString(const jsi::BigInt &value, int radix) {
  const QuickJSPointerValue *jsValuePointer =
      static_cast<const QuickJSPointerValue *>(getPointerValue(value));
  JSValue jsValue = jsValuePointer->Get(context_);
  ScopedJSValue scopedJsValue(context_, &jsValue);

  JSValue toStringFunc = JS_GetPropertyStr(context_, jsValue, "toString");
  ScopedJSValue scopedJsFunction(context_, &toStringFunc);

  JSValue radixValue = JS_NewInt32(context_, radix);
  ScopedJSValue scopedJsRadix(context_, &radixValue);

  JSValue ret = JS_Call(context_, toStringFunc, jsValue, 1, &radixValue);
  assert(JS_IsString(ret));

  return make<jsi::String>(
      new QuickJSPointerValue(runtime_, context_, ret));
}
} // namespace qjs
