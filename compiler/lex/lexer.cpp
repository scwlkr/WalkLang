#include "lex/lexer.h"

#include <sstream>
#include <string>
#include <string_view>

namespace walk::lex {
namespace {

constexpr const char* kSyntaxDiagnostic = "W1001";

bool is_ascii_space(char ch) {
    return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t' || ch == '\v' || ch == '\f';
}

bool is_ascii_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

bool is_ascii_letter(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

bool is_name_start(char ch) {
    return is_ascii_letter(ch) || ch == '_';
}

bool is_name_continue(char ch) {
    return is_name_start(ch) || is_ascii_digit(ch);
}

bool is_symbol(char ch) {
    return std::string_view("()[],:=+-*/^><?.").find(ch) != std::string_view::npos;
}

std::string quoted_char(char ch) {
    switch (ch) {
    case '\n':
        return "'\\n'";
    case '\r':
        return "'\\r'";
    case '\t':
        return "'\\t'";
    default:
        return "'" + std::string(1, ch) + "'";
    }
}

void add_error(DiagnosticSet& diagnostics, const SourceFile& source, std::size_t start, std::size_t end, std::string message) {
    diagnostics.add(Diagnostic(DiagnosticSeverity::Error, kSyntaxDiagnostic, std::move(message), source.range_for_offsets(start, end)));
}

std::string strip_comment(std::string_view line) {
    bool in_string = false;
    bool escaped = false;
    for (std::size_t index = 0; index < line.size(); ++index) {
        const char ch = line[index];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '\'') {
                in_string = false;
            }
            continue;
        }
        if (ch == '\'') {
            in_string = true;
            continue;
        }
        if (ch == '/' && index + 2 < line.size() && line.substr(index, 3) == "///") {
            return std::string(line.substr(0, index));
        }
        if (ch == '#') {
            return std::string(line.substr(0, index));
        }
    }
    return std::string(line);
}

bool is_blank(std::string_view text) {
    for (const char ch : text) {
        if (!is_ascii_space(ch)) {
            return false;
        }
    }
    return true;
}

bool starts_number(std::string_view text, std::size_t index) {
    if (is_ascii_digit(text[index])) {
        return true;
    }
    if (text[index] != '-' || index + 1 >= text.size() || !is_ascii_digit(text[index + 1])) {
        return false;
    }
    if (index == 0) {
        return true;
    }
    const char previous = text[index - 1];
    return is_ascii_space(previous) || std::string_view("([,=:").find(previous) != std::string_view::npos;
}

std::pair<std::string, std::size_t> read_number(std::string_view text, std::size_t index) {
    const std::size_t start = index;
    if (text[index] == '-') {
        ++index;
    }
    bool seen_dot = false;
    while (index < text.size()) {
        if (text[index] == '.' && !seen_dot) {
            seen_dot = true;
            ++index;
            continue;
        }
        if (!is_ascii_digit(text[index])) {
            break;
        }
        ++index;
    }
    return {std::string(text.substr(start, index - start)), index};
}

std::pair<std::string, std::size_t> read_name(std::string_view text, std::size_t index) {
    const std::size_t start = index;
    while (index < text.size() && is_name_continue(text[index])) {
        ++index;
    }
    return {std::string(text.substr(start, index - start)), index};
}

struct StringRead {
    std::string value;
    std::size_t next = 0;
    bool ok = false;
};

StringRead read_string(std::string_view text, std::size_t index, DiagnosticSet& diagnostics, const SourceFile& source, std::size_t base_offset) {
    const std::size_t quote_index = index;
    ++index;
    std::ostringstream output;
    bool escaped = false;
    int interpolation_depth = 0;
    bool in_interpolation_string = false;
    bool interpolation_escaped = false;

    while (index < text.size()) {
        const char ch = text[index];
        if (interpolation_depth > 0) {
            output << ch;
            if (in_interpolation_string) {
                if (interpolation_escaped) {
                    interpolation_escaped = false;
                } else if (ch == '\\') {
                    interpolation_escaped = true;
                } else if (ch == '\'') {
                    in_interpolation_string = false;
                }
                ++index;
                continue;
            }
            switch (ch) {
            case '\'':
                in_interpolation_string = true;
                break;
            case '{':
                ++interpolation_depth;
                break;
            case '}':
                --interpolation_depth;
                break;
            default:
                break;
            }
        } else if (escaped) {
            switch (ch) {
            case 'n':
                output << '\n';
                break;
            case 't':
                output << '\t';
                break;
            case '\\':
            case '\'':
                output << ch;
                break;
            default:
                output << ch;
                break;
            }
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else if (ch == '\'') {
            return {output.str(), index + 1, true};
        } else if (ch == '{' && index + 1 < text.size() && text[index + 1] == '{') {
            output << "{{";
            index += 2;
            continue;
        } else if (ch == '{') {
            interpolation_depth = 1;
            output << ch;
        } else {
            output << ch;
        }
        ++index;
    }

    add_error(diagnostics, source, base_offset + quote_index, base_offset + quote_index + 1, "syntax error: unterminated string");
    return {};
}

std::vector<Token> tokenize_line(std::string_view text, const SourceFile& source, std::size_t base_offset, DiagnosticSet& diagnostics) {
    std::vector<Token> tokens;
    for (std::size_t index = 0; index < text.size();) {
        const char ch = text[index];
        if (is_ascii_space(ch)) {
            ++index;
            continue;
        }

        const std::size_t token_start = index;
        if (ch == '\'') {
            const StringRead string = read_string(text, index, diagnostics, source, base_offset);
            if (!string.ok) {
                return {};
            }
            tokens.push_back({TokenKind::String, string.value, source.range_for_offsets(base_offset + token_start, base_offset + string.next)});
            index = string.next;
            continue;
        }

        if (starts_number(text, index)) {
            const auto [value, next] = read_number(text, index);
            tokens.push_back({TokenKind::Number, value, source.range_for_offsets(base_offset + token_start, base_offset + next)});
            index = next;
            continue;
        }

        if (index + 1 < text.size()) {
            const std::string_view two = text.substr(index, 2);
            if (two == ">=" || two == "<=" || two == "==" || two == "!=") {
                tokens.push_back({TokenKind::Symbol, std::string(two), source.range_for_offsets(base_offset + token_start, base_offset + token_start + 2)});
                index += 2;
                continue;
            }
        }

        if (is_symbol(ch)) {
            tokens.push_back({TokenKind::Symbol, std::string(1, ch), source.range_for_offsets(base_offset + token_start, base_offset + token_start + 1)});
            ++index;
            continue;
        }

        if (is_name_start(ch)) {
            const auto [value, next] = read_name(text, index);
            tokens.push_back({TokenKind::Name, value, source.range_for_offsets(base_offset + token_start, base_offset + next)});
            index = next;
            continue;
        }

        add_error(diagnostics, source, base_offset + token_start, base_offset + token_start + 1, "syntax error: unexpected character " + quoted_char(ch));
        return {};
    }
    return tokens;
}

}  // namespace

std::string token_kind_name(TokenKind kind) {
    switch (kind) {
    case TokenKind::Name:
        return "NAME";
    case TokenKind::Number:
        return "NUMBER";
    case TokenKind::String:
        return "STRING";
    case TokenKind::Symbol:
        return "SYMBOL";
    }
    return "UNKNOWN";
}

LexResult lex_source(const SourceFile& source) {
    LexResult result;
    const std::string text(source.text());

    std::size_t line_start = 0;
    while (line_start <= text.size()) {
        std::size_t line_end = text.find('\n', line_start);
        if (line_end == std::string::npos) {
            line_end = text.size();
        }
        const std::string_view raw(text.data() + line_start, line_end - line_start);

        const std::size_t tab = raw.find('\t');
        if (tab != std::string_view::npos) {
            add_error(result.diagnostics, source, line_start + tab, line_start + tab + 1, "syntax error: tabs are invalid in indentation");
            return result;
        }

        const std::string body = strip_comment(raw);
        if (!is_blank(body)) {
            std::size_t indent = 0;
            while (indent < body.size() && body[indent] == ' ') {
                ++indent;
            }
            std::vector<Token> tokens = tokenize_line(std::string_view(body).substr(indent), source, line_start + indent, result.diagnostics);
            if (result.diagnostics.has_errors()) {
                return result;
            }
            if (!tokens.empty()) {
                result.lines.push_back({indent, std::move(tokens), source.range_for_offsets(line_start + indent, line_start + indent + 1)});
            }
        }

        if (line_end == text.size()) {
            break;
        }
        line_start = line_end + 1;
    }

    return result;
}

}  // namespace walk::lex
