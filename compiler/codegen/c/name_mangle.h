#pragma once

#include "ast/ast.h"

#include <string>
#include <vector>

namespace walk::codegen::c {

[[nodiscard]] std::string module_symbol_name(const std::string& module, const std::string& name);
[[nodiscard]] std::string method_symbol_name(const std::string& receiver, const std::string& name);
[[nodiscard]] std::string type_signature(const ast::Type& type);
[[nodiscard]] std::string generic_instance_name(const std::string& callee, const std::vector<ast::Type>& type_args);

}  // namespace walk::codegen::c
