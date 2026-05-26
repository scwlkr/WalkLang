#pragma once

#include <string>

namespace walk::repl {

[[nodiscard]] std::string source_for_expression(const std::string& expression);

}  // namespace walk::repl
