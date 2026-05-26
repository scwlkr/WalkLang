#pragma once

#include "ast/ast.h"
#include "parse/parser.h"
#include "support/diagnostic.h"
#include "support/source_file.h"

#include <map>
#include <memory>
#include <string>

namespace walk::sema {

struct Module {
    std::string name;
    std::string path;
    std::unique_ptr<SourceFile> source;
    parse::ParseResult parsed;
    std::map<std::string, ast::Type> exports;
    std::map<std::string, ast::FuncDecl*> generic_exports;
};

struct ProgramBundle {
    std::unique_ptr<SourceFile> source;
    parse::ParseResult parsed;
    std::map<std::string, std::unique_ptr<Module>> modules;
    DiagnosticSet diagnostics;

    [[nodiscard]] bool ok() const;
};

[[nodiscard]] ProgramBundle load_program_with_modules(const std::string& source_path);

}  // namespace walk::sema
