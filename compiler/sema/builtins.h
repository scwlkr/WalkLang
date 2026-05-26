#pragma once

#include "ast/ast.h"

#include <string>
#include <vector>

namespace walk::sema {

struct BuiltinFunction {
    std::string module;
    std::string name;
    std::vector<ast::Type> params;
    ast::Type return_type;
    bool effect = false;
    bool draft = false;

    [[nodiscard]] std::string qualified_name() const;
};

struct BuiltinStruct {
    std::string name;
    std::vector<ast::StructField> fields;
    bool draft = false;
};

[[nodiscard]] const BuiltinFunction* lookup_builtin(const std::string& module, const std::string& name);
[[nodiscard]] const BuiltinFunction* lookup_qualified_builtin(const std::string& qualified);
[[nodiscard]] bool is_builtin_module(const std::string& name);
[[nodiscard]] const std::vector<BuiltinStruct>& builtin_structs();

}  // namespace walk::sema
