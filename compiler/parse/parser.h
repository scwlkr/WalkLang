#pragma once

#include "ast/arena.h"
#include "ast/ast.h"
#include "lex/token.h"
#include "support/diagnostic.h"
#include "support/source_file.h"

#include <memory>
#include <vector>

namespace walk::parse {

struct ParseResult {
    ast::Arena arena;
    std::unique_ptr<ast::Program> program;
    DiagnosticSet diagnostics;

    [[nodiscard]] bool ok() const {
        return program != nullptr && !diagnostics.has_errors();
    }
};

[[nodiscard]] ParseResult parse_source(const SourceFile& source);
[[nodiscard]] ParseResult parse_lines(const SourceFile& source, const std::vector<lex::Line>& lines);

}  // namespace walk::parse
