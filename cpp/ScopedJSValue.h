#pragma once

#include "QuickJSRuntime.h"
#include "QuickJSPointerValue.h"

namespace qjs {

class ScopedJSValue {
 public:
  explicit ScopedJSValue(JSContext *context, JSValue *value)
      : context_(context), value_(value) {
    assert(value_ != nullptr);
  };

  ~ScopedJSValue() {
    assert(value_ != nullptr);
    JS_FreeValue(context_, *value_);
  }

  // Prevent copying of Scope objects.
  ScopedJSValue(const ScopedJSValue &) = delete;
  ScopedJSValue &operator=(const ScopedJSValue &) = delete;

  JSValue get() const {
    return *value_;
  };

 private:
  JSContext *context_;
  JSValue *value_;
};

class ScopedCString {
 public:
  explicit ScopedCString(JSContext *context, const char *cstring)
      : context_(context), cstring_(cstring) {
    assert(cstring_ != nullptr);
  };

  ~ScopedCString() {
    assert(cstring_ != nullptr);
    JS_FreeCString(context_, cstring_);
  }

  // Prevent copying of Scope objects.
  ScopedCString(const ScopedCString &) = delete;
  ScopedCString &operator=(const ScopedCString &) = delete;

  const char *get() const {
    return cstring_;
  };

 private:
  JSContext *context_;
  const char *cstring_;
};

} // namespace qjs
