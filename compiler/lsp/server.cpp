#include "lsp/server.h"

#include "docs/api_docs.h"
#include "format/format.h"
#include "parse/parser.h"
#include "sema/checker.h"
#include "support/diagnostic.h"
#include "support/source_file.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <istream>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>

namespace walk::lsp {
namespace {

struct Document {
    std::string uri;
    std::string path;
    std::string text;
};

bool starts_with(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::string json_string(const std::string& value) {
    return "\"" + docs::json_escape(value) + "\"";
}

std::optional<std::string> extract_string(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return std::nullopt;
    }
    const std::size_t colon = json.find(':', key_pos + needle.size());
    if (colon == std::string::npos) {
        return std::nullopt;
    }
    std::size_t pos = colon + 1;
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
        ++pos;
    }
    if (pos >= json.size() || json[pos] != '"') {
        return std::nullopt;
    }
    ++pos;
    std::ostringstream out;
    while (pos < json.size()) {
        const char ch = json[pos++];
        if (ch == '"') {
            return out.str();
        }
        if (ch != '\\' || pos >= json.size()) {
            out << ch;
            continue;
        }
        const char escaped = json[pos++];
        switch (escaped) {
        case 'n':
            out << '\n';
            break;
        case 'r':
            out << '\r';
            break;
        case 't':
            out << '\t';
            break;
        case '"':
        case '\\':
        case '/':
            out << escaped;
            break;
        default:
            out << escaped;
            break;
        }
    }
    return std::nullopt;
}

std::optional<std::string> extract_id(const std::string& json) {
    const std::string needle = "\"id\"";
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return std::nullopt;
    }
    const std::size_t colon = json.find(':', key_pos + needle.size());
    if (colon == std::string::npos) {
        return std::nullopt;
    }
    std::size_t pos = colon + 1;
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos])) != 0) {
        ++pos;
    }
    if (pos >= json.size()) {
        return std::nullopt;
    }
    if (json[pos] == '"') {
        std::size_t end = pos + 1;
        bool escaped = false;
        while (end < json.size()) {
            if (!escaped && json[end] == '"') {
                return json.substr(pos, end - pos + 1);
            }
            escaped = !escaped && json[end] == '\\';
            if (json[end] != '\\') {
                escaped = false;
            }
            ++end;
        }
        return std::nullopt;
    }
    std::size_t end = pos;
    while (end < json.size() && json[end] != ',' && json[end] != '}' && !std::isspace(static_cast<unsigned char>(json[end]))) {
        ++end;
    }
    return json.substr(pos, end - pos);
}

std::string uri_to_path(const std::string& uri) {
    const std::string prefix = "file://";
    if (!starts_with(uri, prefix)) {
        return uri;
    }
    std::string path = uri.substr(prefix.size());
    std::string decoded;
    for (std::size_t index = 0; index < path.size(); ++index) {
        if (path[index] == '%' && index + 2 < path.size()) {
            const std::string hex = path.substr(index + 1, 2);
            char* end = nullptr;
            const long value = std::strtol(hex.c_str(), &end, 16);
            if (end != nullptr && *end == '\0') {
                decoded.push_back(static_cast<char>(value));
                index += 2;
                continue;
            }
        }
        decoded.push_back(path[index]);
    }
    return decoded;
}

std::string range_json(const SourceRange& range, std::size_t fallback_length) {
    std::size_t line = range.start.line > 0 ? range.start.line - 1 : 0;
    std::size_t start = range.start.column > 0 ? range.start.column - 1 : 0;
    std::size_t end = range.end.column > range.start.column ? range.end.column - 1 : start + fallback_length;
    if (end <= start) {
        end = start + 1;
    }
    std::ostringstream out;
    out << "{\"start\":{\"line\":" << line << ",\"character\":" << start << "},\"end\":{\"line\":" << line << ",\"character\":" << end << "}}";
    return out.str();
}

std::string full_document_range_json(const std::string& source) {
    std::size_t line = 0;
    std::size_t character = 0;
    for (const char ch : source) {
        if (ch == '\n') {
            ++line;
            character = 0;
        } else {
            ++character;
        }
    }
    std::ostringstream out;
    out << "{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":" << line << ",\"character\":" << character << "}}";
    return out.str();
}

std::string diagnostic_json(const SourceRange& range, int severity, const std::string& message) {
    std::ostringstream out;
    out << "{\"range\":" << range_json(range, 1) << ",\"severity\":" << severity << ",\"source\":\"walk\",\"message\":" << json_string(message) << "}";
    return out.str();
}

std::string fallback_diagnostic_json(const std::string& path, const std::string& message) {
    SourceRange range;
    range.path = path;
    range.start = {1, 1};
    range.end = {1, 2};
    return diagnostic_json(range, 1, message);
}

std::string read_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

std::optional<std::string> read_lsp_message(std::istream& input) {
    std::string line;
    int content_length = -1;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            break;
        }
        const std::size_t split = line.find(':');
        if (split == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, split);
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (key == "content-length") {
            content_length = std::stoi(line.substr(split + 1));
        }
    }
    if (content_length < 0) {
        return std::nullopt;
    }
    std::string body(static_cast<std::size_t>(content_length), '\0');
    input.read(body.data(), content_length);
    if (input.gcount() != content_length) {
        return std::nullopt;
    }
    return body;
}

void write_payload(std::ostream& output, const std::string& body) {
    output << "Content-Length: " << body.size() << "\r\n\r\n" << body;
    output.flush();
}

std::string response_json(const std::string& id, const std::string& result) {
    return "{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":" + result + "}";
}

std::string notification_json(const std::string& method, const std::string& params) {
    return "{\"jsonrpc\":\"2.0\",\"method\":" + json_string(method) + ",\"params\":" + params + "}";
}

}  // namespace

std::string initialize_result_json() {
    return "{\"capabilities\":{\"textDocumentSync\":{\"openClose\":true,\"change\":1},\"documentFormattingProvider\":true,\"hoverProvider\":true,\"definitionProvider\":true,\"referencesProvider\":true,\"renameProvider\":true,\"completionProvider\":{\"triggerCharacters\":[\".\"]}}}";
}

std::string diagnostics_json(const std::string& path, const std::string& source) {
    const SourceFile source_file = SourceFile::from_text(path, source);
    parse::ParseResult parsed = parse::parse_source(source_file);
    std::vector<std::string> diagnostics;
    if (!parsed.ok()) {
        for (const Diagnostic& diagnostic : parsed.diagnostics.diagnostics()) {
            if (diagnostic.range()) {
                diagnostics.push_back(diagnostic_json(*diagnostic.range(), 1, diagnostic.message()));
            } else {
                diagnostics.push_back(fallback_diagnostic_json(path, diagnostic.message()));
            }
        }
    } else {
        std::map<std::string, std::unique_ptr<sema::Module>> modules;
        sema::CheckResult checked = sema::check_programs(*parsed.program, modules);
        if (!checked.ok()) {
            for (const Diagnostic& diagnostic : checked.diagnostics.diagnostics()) {
                if (diagnostic.range()) {
                    diagnostics.push_back(diagnostic_json(*diagnostic.range(), 1, diagnostic.message()));
                } else {
                    diagnostics.push_back(fallback_diagnostic_json(path, diagnostic.message()));
                }
            }
        }
        for (const sema::Warning& warning : checked.warnings) {
            diagnostics.push_back(diagnostic_json(warning.range, 2, warning.message));
        }
    }
    std::ostringstream out;
    out << "[";
    for (std::size_t index = 0; index < diagnostics.size(); ++index) {
        if (index != 0) {
            out << ",";
        }
        out << diagnostics[index];
    }
    out << "]";
    return out.str();
}

std::string formatting_edits_json(const std::string& path, const std::string& source) {
    Result<std::string> formatted = format::format_source(source, path);
    if (!formatted.ok()) {
        return "[]";
    }
    return "[{\"range\":" + full_document_range_json(source) + ",\"newText\":" + json_string(formatted.value()) + "}]";
}

int serve(std::istream& input, std::ostream& output) {
    std::map<std::string, Document> documents;
    bool shutdown = false;
    for (;;) {
        std::optional<std::string> body = read_lsp_message(input);
        if (!body) {
            return 0;
        }
        const std::optional<std::string> method = extract_string(*body, "method");
        if (!method) {
            continue;
        }
        const std::optional<std::string> id = extract_id(*body);
        if (*method == "exit") {
            return shutdown ? 0 : 1;
        }
        if (*method == "initialize" && id) {
            write_payload(output, response_json(*id, initialize_result_json()));
            continue;
        }
        if (*method == "shutdown" && id) {
            shutdown = true;
            write_payload(output, response_json(*id, "null"));
            continue;
        }
        if (*method == "textDocument/didOpen") {
            const std::optional<std::string> uri = extract_string(*body, "uri");
            const std::optional<std::string> text = extract_string(*body, "text");
            if (uri && text) {
                const std::string path = uri_to_path(*uri);
                documents[*uri] = {*uri, path, *text};
                write_payload(output, notification_json("textDocument/publishDiagnostics", "{\"uri\":" + json_string(*uri) + ",\"diagnostics\":" + diagnostics_json(path, *text) + "}"));
            }
            continue;
        }
        if (*method == "textDocument/didChange") {
            const std::optional<std::string> uri = extract_string(*body, "uri");
            const std::optional<std::string> text = extract_string(*body, "text");
            if (uri && text) {
                Document& doc = documents[*uri];
                doc.uri = *uri;
                doc.path = uri_to_path(*uri);
                doc.text = *text;
                write_payload(output, notification_json("textDocument/publishDiagnostics", "{\"uri\":" + json_string(*uri) + ",\"diagnostics\":" + diagnostics_json(doc.path, doc.text) + "}"));
            }
            continue;
        }
        if (*method == "textDocument/formatting" && id) {
            const std::optional<std::string> uri = extract_string(*body, "uri");
            if (!uri) {
                write_payload(output, response_json(*id, "[]"));
                continue;
            }
            Document doc;
            const auto found = documents.find(*uri);
            if (found != documents.end()) {
                doc = found->second;
            } else {
                doc = {*uri, uri_to_path(*uri), read_file(uri_to_path(*uri))};
            }
            write_payload(output, response_json(*id, formatting_edits_json(doc.path, doc.text)));
            continue;
        }
        if (id) {
            write_payload(output, "{\"jsonrpc\":\"2.0\",\"id\":" + *id + ",\"error\":{\"code\":-32601,\"message\":\"method not found: " + docs::json_escape(*method) + "\"}}");
        }
    }
}

}  // namespace walk::lsp
