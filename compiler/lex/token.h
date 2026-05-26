#pragma once

#include "support/source_file.h"

#include <cstddef>
#include <string>
#include <vector>

namespace walk::lex {

enum class TokenKind {
    Name,
    Number,
    String,
    Symbol,
};

struct Token {
    TokenKind kind;
    std::string value;
    SourceRange range;
};

struct Line {
    std::size_t indent = 0;
    std::vector<Token> tokens;
    SourceRange range;
};

[[nodiscard]] std::string token_kind_name(TokenKind kind);

}  // namespace walk::lex
