#pragma once

#include "ast/ast.h"

#include <map>
#include <string>
#include <vector>

namespace walk::sema {

[[nodiscard]] bool known_type(const ast::Type& type);
[[nodiscard]] bool type_equal(const ast::Type& left, const ast::Type& right);
[[nodiscard]] bool assignable(const ast::Type& source, const ast::Type& target);
[[nodiscard]] bool comparable(const ast::Type& left, const ast::Type& right);
[[nodiscard]] bool is_numeric(const ast::Type& type);
[[nodiscard]] bool contains_kind(const std::vector<ast::Type>& types, ast::TypeKind kind);
[[nodiscard]] bool native_array_element(const ast::Type& type);
[[nodiscard]] bool interpolatable(const ast::Type& type);
[[nodiscard]] ast::Type nullable_string();
[[nodiscard]] ast::Type substitute_type(const ast::Type& type, const std::map<std::string, ast::Type>& bindings);

}  // namespace walk::sema
