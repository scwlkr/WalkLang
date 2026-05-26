#include "format/format.h"

#include "lex/lexer.h"
#include "support/source_file.h"

#include <sstream>
#include <string>
#include <vector>

namespace walk::format {
namespace {

std::string escape_walk_string(const std::string& value) {
    std::ostringstream output;
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            output << "\\\\";
            break;
        case '\'':
            output << "\\'";
            break;
        case '\n':
            output << "\\n";
            break;
        case '\t':
            output << "\\t";
            break;
        default:
            output << ch;
            break;
        }
    }
    return output.str();
}

bool needs_space(const std::string& previous, const std::string& current) {
    if (current == ")" || current == "]" || current == "," || current == ":" || current == "." || current == "?") {
        return false;
    }
    if (previous == "=" || current == "=") {
        return true;
    }
    if (current == "(" || current == "[") {
        return false;
    }
    if (previous == "(" || previous == "[" || previous == ".") {
        return false;
    }
    if (previous == "," || previous == ":") {
        return true;
    }
    return true;
}

std::string format_tokens(const std::vector<lex::Token>& tokens) {
    std::ostringstream output;
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        if (index > 0 && needs_space(tokens[index - 1].value, tokens[index].value)) {
            output << ' ';
        }
        if (tokens[index].kind == lex::TokenKind::String) {
            output << "'" << escape_walk_string(tokens[index].value) << "'";
        } else {
            output << tokens[index].value;
        }
    }
    return output.str();
}

}  // namespace

Result<std::string> format_source(const std::string& source, const std::string& filename) {
    const SourceFile source_file = SourceFile::from_text(filename, source);
    lex::LexResult lexed = lex::lex_source(source_file);
    if (!lexed.ok()) {
        return Result<std::string>::failure(lexed.diagnostics.format(&source_file));
    }
    std::ostringstream output;
    std::vector<std::size_t> indent_stack{0};
    for (const lex::Line& line : lexed.lines) {
        while (indent_stack.size() > 1 && line.indent < indent_stack.back()) {
            indent_stack.pop_back();
        }
        if (line.indent > indent_stack.back()) {
            indent_stack.push_back(line.indent);
        }
        output << std::string((indent_stack.size() - 1) * 4, ' ');
        output << format_tokens(line.tokens);
        output << '\n';
    }
    return Result<std::string>::success(output.str());
}

}  // namespace walk::format
