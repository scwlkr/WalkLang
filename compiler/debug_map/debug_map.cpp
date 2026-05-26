#include "debug_map/debug_map.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace walk::debug_map {
namespace {

std::string slash_path(const std::string& path) {
    return std::filesystem::path(path).generic_string();
}

std::string absolute_slash_path(const std::string& path) {
    std::error_code error;
    const std::filesystem::path abs = std::filesystem::absolute(path, error).lexically_normal();
    if (error) {
        return slash_path(path);
    }
    return abs.generic_string();
}

Result<void> write_text(const std::string& path, const std::string& text) {
    try {
        const std::filesystem::path target(path);
        if (!target.parent_path().empty()) {
            std::filesystem::create_directories(target.parent_path());
        }
        std::ofstream output(target, std::ios::binary);
        if (!output) {
            return Result<void>::failure("could not write " + path);
        }
        output << text;
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<void>::failure(error.what());
    }
    return Result<void>::success();
}

}  // namespace

std::string render_debug_map_json(const std::string& source_path, const docs::ToolingAnalysis& analysis) {
    const std::vector<docs::ToolingSymbol> symbols = docs::sorted_symbols(analysis);
    std::ostringstream out;
    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"source\": \"" << docs::json_escape(absolute_slash_path(source_path)) << "\",\n";
    out << "  \"symbols\": [\n";
    for (std::size_t index = 0; index < symbols.size(); ++index) {
        const docs::ToolingSymbol& symbol = symbols[index];
        out << "    {\n";
        out << "      \"name\": \"" << docs::json_escape(symbol.name) << "\",\n";
        out << "      \"kind\": \"" << docs::json_escape(symbol.kind) << "\",\n";
        if (!symbol.detail.empty()) {
            out << "      \"detail\": \"" << docs::json_escape(symbol.detail) << "\",\n";
        }
        out << "      \"file\": \"" << docs::json_escape(slash_path(symbol.range.path)) << "\",\n";
        out << "      \"line\": " << symbol.range.start.line << ",\n";
        out << "      \"column\": " << symbol.range.start.column << "\n";
        out << "    }" << (index + 1 == symbols.size() ? "\n" : ",\n");
    }
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

Result<void> write_debug_map(const std::string& output_path, const std::string& source_path, const docs::ToolingAnalysis& analysis) {
    return write_text(output_path, render_debug_map_json(source_path, analysis));
}

}  // namespace walk::debug_map
