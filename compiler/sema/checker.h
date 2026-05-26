#pragma once

#include "ast/ast.h"
#include "sema/modules.h"
#include "support/diagnostic.h"

#include <map>
#include <string>
#include <vector>

namespace walk::sema {

struct Warning {
    SourceRange range;
    std::string message;
};

struct CheckResult {
    std::vector<Warning> warnings;
    DiagnosticSet diagnostics;

    [[nodiscard]] bool ok() const {
        return !diagnostics.has_errors();
    }
};

struct StructDef {
    SourceRange range;
    std::string name;
    std::vector<ast::StructField> fields;
    std::map<std::string, ast::StructField> field_map;
};

struct MethodDef {
    SourceRange range;
    std::string receiver;
    std::string name;
    ast::Type type;
    ast::FuncDecl* function = nullptr;
};

[[nodiscard]] CheckResult check_programs(ast::Program& program, std::map<std::string, std::unique_ptr<Module>>& modules);
[[nodiscard]] std::map<std::string, StructDef> struct_definitions(ast::Program& program, DiagnosticSet& diagnostics);
[[nodiscard]] std::map<std::string, std::map<std::string, MethodDef>> method_definitions(ast::Program& program, DiagnosticSet& diagnostics);
[[nodiscard]] std::map<std::string, ast::Type> exported_functions(ast::Program& program, DiagnosticSet& diagnostics);
[[nodiscard]] std::map<std::string, ast::FuncDecl*> exported_generic_functions(ast::Program& program);

}  // namespace walk::sema
