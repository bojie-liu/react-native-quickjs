#pragma once

#include <fstream>
#include <mutex>
#include <unordered_map>

#include "jsi/jsi.h"
#include "quickjs.h"

namespace jsi = facebook::jsi;

namespace qjs {

class QuickJSInstrumentation;
class QuickJSPointerValue;

struct CodeCacheItem {
  enum Result {
    UNINITIALIZED,
    INITIALIZED,
    REQUEST_UPDATE,
    UPDATED
  };

  std::unique_ptr<uint8_t> data = nullptr;
  size_t size = 0;
  Result result = UNINITIALIZED;
};

class QuickJSRuntime : public jsi::Runtime {
 public:
  QuickJSRuntime();
  ~QuickJSRuntime();

  std::unordered_map<std::string, int64_t> getHeapInfo();

 private:
  void checkAndThrowException(JSContext *context) const;
  void loadCodeCache(CodeCacheItem &codeCacheItem, const std::string& url);
  void updateCodeCache(CodeCacheItem &codeCacheItem, const std::string& url);


  //
  // jsi::Runtime implementations
  //
 public:
  jsi::Value evaluateJavaScript(
      const std::shared_ptr<const jsi::Buffer> &buffer,
      const std::string &sourceURL) override;

  std::shared_ptr<const jsi::PreparedJavaScript> prepareJavaScript(
      const std::shared_ptr<const jsi::Buffer> &buffer,
      std::string sourceURL) override;
  jsi::Value evaluatePreparedJavaScript(
      const std::shared_ptr<const jsi::PreparedJavaScript> &js) override;

  jsi::Object global() override;
  std::string description() override;
  bool isInspectable() override;

  jsi::Instrumentation& instrumentation() override;

 protected:
  PointerValue *cloneSymbol(const Runtime::PointerValue *pv) override;
  PointerValue* cloneBigInt(const Runtime::PointerValue* pv) override;
  PointerValue *cloneString(const Runtime::PointerValue *pv) override;
  PointerValue *cloneObject(const Runtime::PointerValue *pv) override;
  PointerValue *clonePropNameID(const Runtime::PointerValue *pv) override;

  bool bigintIsInt64(const jsi::BigInt&) override;
  bool bigintIsUint64(const jsi::BigInt&) override;
  uint64_t truncate(const jsi::BigInt&) override;

  bool hasNativeState(const jsi::Object&) override;
  std::shared_ptr<jsi::NativeState> getNativeState(const jsi::Object&) override;
  void setNativeState(
      const jsi::Object&,
      std::shared_ptr<jsi::NativeState> state) override;

  bool drainMicrotasks(int maxMicrotasksHint) override;

  jsi::PropNameID createPropNameIDFromSymbol(const jsi::Symbol& sym) override;
  jsi::PropNameID createPropNameIDFromAscii(const char *str, size_t length)
      override;
  jsi::PropNameID createPropNameIDFromUtf8(const uint8_t *utf8, size_t length)
      override;
  jsi::PropNameID createPropNameIDFromString(const jsi::String &str) override;
  std::string utf8(const jsi::PropNameID &) override;
  bool compare(const jsi::PropNameID &, const jsi::PropNameID &) override;

  jsi::BigInt createBigIntFromInt64(int64_t) override;
  jsi::BigInt createBigIntFromUint64(uint64_t) override;
  jsi::String bigintToString(const jsi::BigInt&, int) override;
  std::string symbolToString(const jsi::Symbol &) override;

  jsi::String createStringFromAscii(const char *str, size_t length) override;
  jsi::String createStringFromUtf8(const uint8_t *utf8, size_t length) override;
  std::string utf8(const jsi::String &) override;

  jsi::Object createObject() override;
  jsi::Object createObject(
      std::shared_ptr<jsi::HostObject> hostObject) override;
  std::shared_ptr<jsi::HostObject> getHostObject(const jsi::Object &) override;
  jsi::HostFunctionType &getHostFunction(const jsi::Function &) override;

  jsi::Value getProperty(const jsi::Object &, const jsi::PropNameID &name)
      override;
  jsi::Value getProperty(const jsi::Object &, const jsi::String &name) override;
  bool hasProperty(const jsi::Object &, const jsi::PropNameID &name) override;
  bool hasProperty(const jsi::Object &, const jsi::String &name) override;
  void setPropertyValue(
      jsi::Object &,
      const jsi::PropNameID &name,
      const jsi::Value &value) override;
  void setPropertyValue(jsi::Object &,
      const jsi::String &name,
      const jsi::Value &value) override;

  bool isArray(const jsi::Object &) const override;
  bool isArrayBuffer(const jsi::Object &) const override;
  bool isFunction(const jsi::Object &) const override;
  bool isHostObject(const jsi::Object &) const override;
  bool isHostFunction(const jsi::Function &) const override;
  jsi::Array getPropertyNames(const jsi::Object &) override;

  jsi::WeakObject createWeakObject(const jsi::Object &) override;
  jsi::Value lockWeakObject(jsi::WeakObject &) override;

  jsi::Array createArray(size_t length) override;
    jsi::ArrayBuffer createArrayBuffer(
      std::shared_ptr<jsi::MutableBuffer> buffer) override;
  size_t size(const jsi::Array &) override;
  size_t size(const jsi::ArrayBuffer &) override;
  uint8_t *data(const jsi::ArrayBuffer &) override;
  jsi::Value getValueAtIndex(const jsi::Array &, size_t i) override;
  void setValueAtIndexImpl(jsi::Array &, size_t i, const jsi::Value &value)
      override;

  jsi::Function createFunctionFromHostFunction(
      const jsi::PropNameID &name,
      unsigned int paramCount,
      jsi::HostFunctionType func) override;
  jsi::Value call(
      const jsi::Function &,
      const jsi::Value &jsThis,
      const jsi::Value *args,
      size_t count) override;
  jsi::Value callAsConstructor(
      const jsi::Function &,
      const jsi::Value *args,
      size_t count) override;
    bool strictEquals(const jsi::BigInt& a, const jsi::BigInt& b) const override;

  bool strictEquals(const jsi::Symbol &a, const jsi::Symbol &b) const override;
  bool strictEquals(const jsi::String &a, const jsi::String &b) const override;
  bool strictEquals(const jsi::Object &a, const jsi::Object &b) const override;

  bool instanceOf(const jsi::Object &o, const jsi::Function &f) override;

 private:
  friend class QuickJSPointerValue;
  friend class JSIValueConverter;

 public:
  JSRuntime *getJSRuntime() const {
    return runtime_;
  };

  JSContext *getJSContext() const {
    return context_;
  };

 private:
  JSRuntime *runtime_;
  JSContext *context_;

  std::unique_ptr<QuickJSInstrumentation> instrumentation_;
};

} // namespace qjs
