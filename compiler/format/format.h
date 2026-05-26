#pragma once

#include "support/result.h"

#include <string>

namespace walk::format {

[[nodiscard]] Result<std::string> format_source(const std::string& source, const std::string& filename);

}  // namespace walk::format
