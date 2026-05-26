#pragma once

#include "docs/api_docs.h"
#include "support/result.h"

#include <string>

namespace walk::debug_map {

[[nodiscard]] std::string render_debug_map_json(const std::string& source_path, const docs::ToolingAnalysis& analysis);
[[nodiscard]] Result<void> write_debug_map(const std::string& output_path, const std::string& source_path, const docs::ToolingAnalysis& analysis);

}  // namespace walk::debug_map
