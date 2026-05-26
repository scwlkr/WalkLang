#pragma once

#include "support/result.h"

#include <string>

namespace walk::docs {

[[nodiscard]] Result<void> build_site(const std::string& docs_dir, const std::string& public_dir);

}  // namespace walk::docs
