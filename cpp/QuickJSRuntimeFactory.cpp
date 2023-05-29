#include "QuickJSRuntimeFactory.h"

#include "QuickJSRuntime.h"
#include <memory>

namespace qjs {

std::unique_ptr<jsi::Runtime> createQuickJSRuntime() {
  return std::make_unique<QuickJSRuntime>();
}

} // namespace qjs
