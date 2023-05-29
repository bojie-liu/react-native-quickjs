#include "QuickJSInstrumentation.h"

#include "QuickJSRuntime.h"
namespace qjs {
QuickJSInstrumentation::QuickJSInstrumentation(QuickJSRuntime *runtime) : runtime_(runtime) {}

std::string QuickJSInstrumentation::getRecordedGCStats() {
  return "";
}

std::unordered_map<std::string, int64_t> QuickJSInstrumentation::getHeapInfo(bool) {
  if (runtime_) {
    return runtime_->getHeapInfo();
  } else {
    return {};
  }
}

void QuickJSInstrumentation::collectGarbage(std::string cause) {
  JS_RunGC(runtime_->getJSRuntime());
}

void QuickJSInstrumentation::createSnapshotToFile(const std::string &) {
}

void QuickJSInstrumentation::createSnapshotToStream(std::ostream &) {
}

void QuickJSInstrumentation::writeBasicBlockProfileTraceToFile(
    const std::string &) const {
}

void QuickJSInstrumentation::dumpProfilerSymbolsToFile(const std::string &) const {
}
} // namespace qjs
