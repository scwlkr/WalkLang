#pragma once

#include "support/result.h"

#include <string>

namespace walk::support::toml_like {

[[nodiscard]] std::string trim(std::string value);
[[nodiscard]] std::string strip_comment(const std::string& line);
[[nodiscard]] Result<std::string> parse_string(const std::string& value, const std::string& filename, int line);
[[nodiscard]] Result<bool> parse_bool(const std::string& value, const std::string& filename, int line);
[[nodiscard]] std::string quote_string(const std::string& value);

}  // namespace walk::support::toml_like
