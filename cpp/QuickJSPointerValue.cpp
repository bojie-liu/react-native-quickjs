#include "QuickJSPointerValue.h"

namespace qjs {

QuickJSPointerValue::QuickJSPointerValue(JSRuntime *runtime, JSContext *context, JSValue value)
    : runtime_(runtime), value_(JS_DupValue(context, value)) {
}

QuickJSPointerValue::~QuickJSPointerValue() {
  JS_FreeValueRT(runtime_, value_);
}

JSValue QuickJSPointerValue::Get(JSContext *context) const {
  return JS_DupValue(context, value_);
}

void QuickJSPointerValue::invalidate() {
  delete this;
}

} // namespace qjs
