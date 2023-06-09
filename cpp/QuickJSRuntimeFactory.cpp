#include "QuickJSRuntimeFactory.h"

#include "QuickJSRuntime.h"
#include <memory>

namespace qjs {

std::unique_ptr<jsi::Runtime> createQuickJSRuntime(const std::string &codeCacheDir) {
  return std::make_unique<QuickJSRuntime>(codeCacheDir);
}

} // namespace qjs
