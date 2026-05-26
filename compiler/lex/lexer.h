#pragma once

#include "lex/token.h"
#include "support/diagnostic.h"
#include "support/source_file.h"

#include <vector>

namespace walk::lex {

struct LexResult {
    std::vector<Line> lines;
    DiagnosticSet diagnostics;

    [[nodiscard]] bool ok() const {
        return !diagnostics.has_errors();
    }
};

[[nodiscard]] LexResult lex_source(const SourceFile& source);

}  // namespace walk::lex
