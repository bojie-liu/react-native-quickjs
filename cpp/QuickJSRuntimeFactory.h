#pragma once

#include <memory.h>

#include <jsi/jsi.h>

namespace jsi = facebook::jsi;

namespace qjs {

std::unique_ptr<jsi::Runtime> createQuickJSRuntime(const std::string &codeCacheDir);

} // namespace qjs
