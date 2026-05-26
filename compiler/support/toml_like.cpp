#include "support/toml_like.h"

#include <cctype>
#include <sstream>
#include <string>

namespace walk::support::toml_like {

std::string trim(std::string value) {
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }
    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return value.substr(begin, end - begin);
}

std::string strip_comment(const std::string& line) {
    bool in_string = false;
    bool escaped = false;
    for (std::size_t index = 0; index < line.size(); ++index) {
        const char ch = line[index];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (ch == '\\' && in_string) {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            in_string = !in_string;
            continue;
        }
        if (ch == '#' && !in_string) {
            return line.substr(0, index);
        }
    }
    return line;
}

Result<std::string> parse_string(const std::string& value, const std::string& filename, int line) {
    if (value.size() < 2 || value.front() != '"' || value.back() != '"') {
        return Result<std::string>::failure(filename + ":" + std::to_string(line) + ": expected quoted string");
    }
    std::ostringstream output;
    bool escaped = false;
    for (std::size_t index = 1; index + 1 < value.size(); ++index) {
        const char ch = value[index];
        if (escaped) {
            switch (ch) {
            case 'n':
                output << '\n';
                break;
            case 't':
                output << '\t';
                break;
            case '\\':
            case '"':
                output << ch;
                break;
            default:
                output << ch;
                break;
            }
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        output << ch;
    }
    if (escaped) {
        return Result<std::string>::failure(filename + ":" + std::to_string(line) + ": expected quoted string");
    }
    return Result<std::string>::success(output.str());
}

Result<bool> parse_bool(const std::string& value, const std::string& filename, int line) {
    if (value == "true") {
        return Result<bool>::success(true);
    }
    if (value == "false") {
        return Result<bool>::success(false);
    }
    return Result<bool>::failure(filename + ":" + std::to_string(line) + ": expected true or false");
}

std::string quote_string(const std::string& value) {
    std::ostringstream output;
    output << '"';
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            output << "\\\\";
            break;
        case '"':
            output << "\\\"";
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
    output << '"';
    return output.str();
}

}  // namespace walk::support::toml_like
