#include "docs/api_docs.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace walk::docs {
namespace {

std::string read_file_if_present(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return "";
    }
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

std::string slash_path(const std::filesystem::path& path) {
    return path.generic_string();
}

std::string trim(const std::string& text) {
    const std::size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

bool starts_with(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    if (!text.empty() && text.back() == '\n') {
        lines.emplace_back();
    }
    return lines;
}

std::vector<std::string> doc_comment_before(const std::string& source, std::size_t one_based_line) {
    if (source.empty() || one_based_line <= 1) {
        return {};
    }
    const std::vector<std::string> lines = split_lines(source);
    std::size_t index = one_based_line - 2;
    std::vector<std::string> block;
    for (;;) {
        if (index >= lines.size()) {
            break;
        }
        const std::string trimmed = trim(lines[index]);
        if (!starts_with(trimmed, "///")) {
            break;
        }
        block.push_back(trim(trimmed.substr(3)));
        if (index == 0) {
            break;
        }
        --index;
    }
    std::reverse(block.begin(), block.end());
    return block;
}

struct DocsComment {
    std::string summary;
    std::vector<DocsParam> params;
    std::string returns;
    std::vector<std::string> examples;
    std::string since;
};

DocsComment parse_docs_comment(const std::vector<std::string>& lines) {
    DocsComment doc;
    std::vector<std::string> example_lines;
    std::string mode;
    auto flush_example = [&] {
        if (example_lines.empty()) {
            return;
        }
        std::ostringstream joined;
        for (std::size_t index = 0; index < example_lines.size(); ++index) {
            if (index != 0) {
                joined << '\n';
            }
            joined << example_lines[index];
        }
        doc.examples.push_back(trim(joined.str()));
        example_lines.clear();
    };

    for (const std::string& line : lines) {
        const std::string trimmed = trim(line);
        if (starts_with(trimmed, "Summary:")) {
            flush_example();
            mode.clear();
            doc.summary = trim(trimmed.substr(std::string("Summary:").size()));
        } else if (starts_with(trimmed, "Params:")) {
            flush_example();
            mode = "params";
        } else if (mode == "params" && starts_with(trimmed, "-")) {
            const std::string param = trim(trimmed.substr(1));
            const std::size_t split = param.find(':');
            if (split != std::string::npos) {
                doc.params.push_back({trim(param.substr(0, split)), trim(param.substr(split + 1))});
            }
        } else if (starts_with(trimmed, "Returns:")) {
            flush_example();
            mode.clear();
            doc.returns = trim(trimmed.substr(std::string("Returns:").size()));
        } else if (starts_with(trimmed, "Example:")) {
            flush_example();
            mode = "example";
            const std::string rest = trim(trimmed.substr(std::string("Example:").size()));
            if (!rest.empty()) {
                example_lines.push_back(rest);
            }
        } else if (starts_with(trimmed, "Since:")) {
            flush_example();
            mode.clear();
            doc.since = trim(trimmed.substr(std::string("Since:").size()));
        } else if (mode == "example") {
            if (!starts_with(trimmed, "```")) {
                example_lines.push_back(line);
            }
        }
    }
    flush_example();
    return doc;
}

std::string join_strings(const std::vector<std::string>& values, const std::string& sep) {
    std::ostringstream out;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            out << sep;
        }
        out << values[index];
    }
    return out.str();
}

std::string function_doc_name(const ast::FuncDecl& decl) {
    if (!decl.receiver.empty()) {
        return decl.receiver + "." + decl.name;
    }
    return decl.name;
}

std::string function_signature(const ast::FuncDecl& decl) {
    std::string name = function_doc_name(decl);
    if (!decl.type_params.empty()) {
        name += "[" + join_strings(decl.type_params, ", ") + "]";
    }
    std::vector<std::string> params;
    for (const ast::Param& param : decl.params) {
        params.push_back(param.name + " " + param.type.to_string());
    }
    return "func " + name + "(" + join_strings(params, ", ") + ") " + decl.return_type.to_string();
}

std::string struct_signature(const ast::StructDecl& decl) {
    std::vector<std::string> fields;
    for (const ast::StructField& field : decl.fields) {
        fields.push_back(field.name + " " + field.type.to_string());
    }
    return "struct " + decl.name + " { " + join_strings(fields, ", ") + " }";
}

std::string var_kind(const ast::VarDecl& decl) {
    return decl.mutable_binding ? "variable" : "constant";
}

std::string var_detail(const ast::VarDecl& decl) {
    ast::Type type_name = decl.annotation;
    if (type_name.kind == ast::TypeKind::Invalid && decl.value != nullptr) {
        type_name = decl.value->type;
    }
    if (type_name.kind == ast::TypeKind::Invalid) {
        return decl.name;
    }
    return std::string(decl.mutable_binding ? "var " : "const ") + decl.name + " " + type_name.to_string();
}

void collect_statement_symbols(const std::string& filename, const std::vector<ast::Statement*>& statements, std::vector<ToolingSymbol>& symbols) {
    for (ast::Statement* statement : statements) {
        if (auto* import = dynamic_cast<ast::Import*>(statement)) {
            symbols.push_back({import->module, "module", "module " + import->module, import->range});
        } else if (auto* function = dynamic_cast<ast::FuncDecl*>(statement)) {
            std::string name = function->name;
            std::string kind = "function";
            if (!function->receiver.empty()) {
                name = function->receiver + "." + function->name;
                kind = "method";
            }
            SourceRange range = function->range;
            range.path = filename;
            symbols.push_back({name, kind, function_signature(*function), range});
            for (const ast::Param& param : function->params) {
                symbols.push_back({param.name, "parameter", param.name + " " + param.type.to_string(), range});
            }
            collect_statement_symbols(filename, function->body, symbols);
        } else if (auto* structure = dynamic_cast<ast::StructDecl*>(statement)) {
            SourceRange range = structure->range;
            range.path = filename;
            symbols.push_back({structure->name, "struct", struct_signature(*structure), range});
            for (const ast::StructField& field : structure->fields) {
                SourceRange field_range = field.range;
                field_range.path = filename;
                symbols.push_back({field.name, "field", field.name + " " + field.type.to_string(), field_range});
            }
        } else if (auto* var = dynamic_cast<ast::VarDecl*>(statement)) {
            SourceRange range = var->range;
            range.path = filename;
            symbols.push_back({var->name, var_kind(*var), var_detail(*var), range});
        } else if (auto* test = dynamic_cast<ast::TestDecl*>(statement)) {
            SourceRange range = test->range;
            range.path = filename;
            symbols.push_back({test->name, "test", "test " + test->name, range});
            collect_statement_symbols(filename, test->body, symbols);
        } else if (auto* if_stmt = dynamic_cast<ast::If*>(statement)) {
            collect_statement_symbols(filename, if_stmt->then_block, symbols);
            collect_statement_symbols(filename, if_stmt->else_block, symbols);
        } else if (auto* while_stmt = dynamic_cast<ast::While*>(statement)) {
            collect_statement_symbols(filename, while_stmt->body, symbols);
        } else if (auto* repeat = dynamic_cast<ast::Repeat*>(statement)) {
            collect_statement_symbols(filename, repeat->body, symbols);
        } else if (auto* for_stmt = dynamic_cast<ast::For*>(statement)) {
            SourceRange range = for_stmt->range;
            range.path = filename;
            symbols.push_back({for_stmt->name, "variable", for_stmt->name, range});
            collect_statement_symbols(filename, for_stmt->body, symbols);
        }
    }
}

std::vector<std::pair<std::string, ast::Program*>> sorted_documents(const ToolingAnalysis& analysis) {
    std::vector<std::pair<std::string, ast::Program*>> documents;
    if (analysis.bundle.parsed.program != nullptr && analysis.bundle.source != nullptr) {
        documents.push_back({analysis.bundle.source->path(), analysis.bundle.parsed.program.get()});
    }
    for (const auto& item : analysis.bundle.modules) {
        if (item.second->parsed.program != nullptr) {
            documents.push_back({item.second->path, item.second->parsed.program.get()});
        }
    }
    std::sort(documents.begin(), documents.end(), [](const auto& left, const auto& right) {
        return left.first < right.first;
    });
    return documents;
}

DocsSymbol docs_symbol_for_decl(const std::string& kind, const std::string& name, const std::string& path, const std::string& signature, const std::string& source, const SourceRange& range) {
    const DocsComment doc = parse_docs_comment(doc_comment_before(source, range.start.line));
    return {kind, name, slash_path(path), signature, doc.summary, doc.params, doc.returns, doc.examples, doc.since};
}

std::vector<std::string> index_paths(const DocsIndex& index) {
    std::vector<std::string> paths;
    for (const DocsSymbol& symbol : index.symbols) {
        if (std::find(paths.begin(), paths.end(), symbol.path) == paths.end()) {
            paths.push_back(symbol.path);
        }
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

std::vector<DocsSymbol> symbols_for_path_and_kind(const DocsIndex& index, const std::string& path, const std::string& kind) {
    std::vector<DocsSymbol> symbols;
    for (const DocsSymbol& symbol : index.symbols) {
        if (symbol.path == path && symbol.kind == kind) {
            symbols.push_back(symbol);
        }
    }
    std::sort(symbols.begin(), symbols.end(), [](const DocsSymbol& left, const DocsSymbol& right) {
        return left.name < right.name;
    });
    return symbols;
}

std::string section_title(const std::string& kind) {
    if (kind == "struct") {
        return "Structs";
    }
    if (kind == "function") {
        return "Functions";
    }
    if (kind == "export") {
        return "Exports";
    }
    return kind;
}

bool signature_has_params(const std::string& signature) {
    const std::size_t start = signature.find('(');
    const std::size_t end = signature.find(')');
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return false;
    }
    return !trim(signature.substr(start + 1, end - start - 1)).empty();
}

}  // namespace

std::string json_escape(const std::string& value) {
    std::ostringstream out;
    for (const unsigned char ch : value) {
        switch (ch) {
        case '\\':
            out << "\\\\";
            break;
        case '"':
            out << "\\\"";
            break;
        case '&':
            out << "\\u0026";
            break;
        case '<':
            out << "\\u003c";
            break;
        case '>':
            out << "\\u003e";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            if (ch < 0x20) {
                out << "\\u00";
                const char* hex = "0123456789abcdef";
                out << hex[(ch >> 4) & 0x0f] << hex[ch & 0x0f];
            } else {
                out << static_cast<char>(ch);
            }
            break;
        }
    }
    return out.str();
}

Result<ToolingAnalysis> analyze_file(const std::string& source_path, const std::vector<std::string>& search_dirs) {
    ToolingAnalysis analysis;
    analysis.bundle = sema::load_program_with_modules_and_search_dirs(source_path, search_dirs);
    if (!analysis.bundle.ok()) {
        return Result<ToolingAnalysis>::failure(analysis.bundle.diagnostics.format(analysis.bundle.source.get()));
    }
    sema::CheckResult checked = sema::check_programs(*analysis.bundle.parsed.program, analysis.bundle.modules);
    if (!checked.ok()) {
        return Result<ToolingAnalysis>::failure(checked.diagnostics.format(analysis.bundle.source.get()));
    }
    analysis.warnings = std::move(checked.warnings);
    if (analysis.bundle.source != nullptr) {
        analysis.sources[analysis.bundle.source->path()] = std::string(analysis.bundle.source->text());
    }
    for (const auto& item : analysis.bundle.modules) {
        if (item.second->source != nullptr) {
            analysis.sources[item.second->path] = std::string(item.second->source->text());
        } else {
            analysis.sources[item.second->path] = read_file_if_present(item.second->path);
        }
    }
    return Result<ToolingAnalysis>::success(std::move(analysis));
}

std::vector<ToolingSymbol> sorted_symbols(const ToolingAnalysis& analysis) {
    std::vector<ToolingSymbol> symbols;
    for (const auto& document : sorted_documents(analysis)) {
        collect_statement_symbols(document.first, document.second->statements, symbols);
    }
    std::sort(symbols.begin(), symbols.end(), [](const ToolingSymbol& left, const ToolingSymbol& right) {
        if (left.name == right.name) {
            return left.range.path < right.range.path;
        }
        return left.name < right.name;
    });
    return symbols;
}

std::string display_path(const std::string& path) {
    std::error_code error;
    const std::filesystem::path abs = std::filesystem::absolute(path, error).lexically_normal();
    if (error) {
        return slash_path(path);
    }
    const std::filesystem::path cwd = std::filesystem::current_path(error);
    if (error) {
        return slash_path(abs);
    }
    const std::filesystem::path rel = abs.lexically_relative(cwd);
    const std::string generic = rel.generic_string();
    if (generic.empty() || generic == "." || generic == ".." || generic.rfind("../", 0) == 0) {
        return slash_path(abs);
    }
    return generic;
}

DocsIndex generate_index(const std::string& source_path, const ToolingAnalysis& analysis) {
    DocsIndex index;
    index.source = display_path(source_path);
    for (const auto& document : sorted_documents(analysis)) {
        const std::string& path = document.first;
        const ast::Program* program = document.second;
        const auto source_found = analysis.sources.find(path);
        const std::string source = source_found == analysis.sources.end() ? "" : source_found->second;
        for (ast::Statement* statement : program->statements) {
            if (auto* structure = dynamic_cast<ast::StructDecl*>(statement)) {
                index.symbols.push_back(docs_symbol_for_decl("struct", structure->name, display_path(path), struct_signature(*structure), source, structure->range));
            } else if (auto* function = dynamic_cast<ast::FuncDecl*>(statement)) {
                index.symbols.push_back(docs_symbol_for_decl("function", function_doc_name(*function), display_path(path), function_signature(*function), source, function->range));
            } else if (auto* export_stmt = dynamic_cast<ast::Export*>(statement)) {
                index.symbols.push_back(docs_symbol_for_decl("export", export_stmt->name, display_path(path), "exp " + export_stmt->name, source, export_stmt->range));
            }
        }
    }
    return index;
}

Result<void> validate_index(const DocsIndex& index) {
    std::vector<std::string> missing;
    for (const DocsSymbol& symbol : index.symbols) {
        const std::string label = symbol.kind + " " + symbol.name;
        if (symbol.summary.empty()) {
            missing.push_back(label + " missing Summary");
        }
        if (signature_has_params(symbol.signature) && symbol.params.empty()) {
            missing.push_back(label + " missing Params");
        }
        if (symbol.kind == "function" && symbol.signature.size() >= 5 && symbol.signature.substr(symbol.signature.size() - 5) != " void" && symbol.returns.empty()) {
            missing.push_back(label + " missing Returns");
        }
        if (symbol.examples.empty()) {
            missing.push_back(label + " missing Example");
        }
        if (symbol.since.empty()) {
            missing.push_back(label + " missing Since");
        }
    }
    if (!missing.empty()) {
        return Result<void>::failure("docs strict check failed: " + join_strings(missing, "; "));
    }
    return Result<void>::success();
}

Result<std::string> render_index(const DocsIndex& index, const std::string& format) {
    if (format == "json") {
        std::ostringstream out;
        out << "{\n";
        out << "  \"version\": " << index.version << ",\n";
        out << "  \"source\": \"" << json_escape(index.source) << "\",\n";
        out << "  \"symbols\": [\n";
        for (std::size_t i = 0; i < index.symbols.size(); ++i) {
            const DocsSymbol& symbol = index.symbols[i];
            out << "    {\n";
            out << "      \"kind\": \"" << json_escape(symbol.kind) << "\",\n";
            out << "      \"name\": \"" << json_escape(symbol.name) << "\",\n";
            out << "      \"path\": \"" << json_escape(symbol.path) << "\",\n";
            out << "      \"signature\": \"" << json_escape(symbol.signature) << "\",\n";
            out << "      \"summary\": \"" << json_escape(symbol.summary) << "\",\n";
            if (symbol.params.empty()) {
                out << "      \"params\": [],\n";
            } else {
                out << "      \"params\": [\n";
                for (std::size_t p = 0; p < symbol.params.size(); ++p) {
                    out << "        {\n";
                    out << "          \"name\": \"" << json_escape(symbol.params[p].name) << "\",\n";
                    out << "          \"description\": \"" << json_escape(symbol.params[p].description) << "\"\n";
                    out << "        }" << (p + 1 == symbol.params.size() ? "\n" : ",\n");
                }
                out << "      ],\n";
            }
            out << "      \"returns\": \"" << json_escape(symbol.returns) << "\",\n";
            if (symbol.examples.empty()) {
                out << "      \"examples\": [],\n";
            } else {
                out << "      \"examples\": [\n";
                for (std::size_t e = 0; e < symbol.examples.size(); ++e) {
                    out << "        \"" << json_escape(symbol.examples[e]) << "\"" << (e + 1 == symbol.examples.size() ? "\n" : ",\n");
                }
                out << "      ],\n";
            }
            out << "      \"since\": \"" << json_escape(symbol.since) << "\"\n";
            out << "    }" << (i + 1 == index.symbols.size() ? "\n" : ",\n");
        }
        out << "  ]\n";
        out << "}\n";
        return Result<std::string>::success(out.str());
    }
    if (!format.empty() && format != "markdown") {
        return Result<std::string>::failure("docs format must be markdown or json");
    }

    std::ostringstream out;
    out << "# WalkLang API\n\n";
    out << "Source: `" << index.source << "`\n\n";
    for (const std::string& path : index_paths(index)) {
        out << "## " << path << "\n\n";
        for (const char* kind : {"struct", "function", "export"}) {
            const std::vector<DocsSymbol> symbols = symbols_for_path_and_kind(index, path, kind);
            if (symbols.empty()) {
                continue;
            }
            out << "### " << section_title(kind) << "\n\n";
            for (const DocsSymbol& symbol : symbols) {
                out << "#### `" << symbol.signature << "`\n\n";
                if (!symbol.summary.empty()) {
                    out << symbol.summary << "\n\n";
                }
                if (!symbol.params.empty()) {
                    out << "Params:\n\n";
                    for (const DocsParam& param : symbol.params) {
                        out << "- `" << param.name << "`: " << param.description << "\n";
                    }
                    out << "\n";
                }
                if (!symbol.returns.empty()) {
                    out << "Returns: " << symbol.returns << "\n\n";
                }
                for (const std::string& example : symbol.examples) {
                    out << "Example:\n\n";
                    out << "```walk\n";
                    out << trim(example) << "\n";
                    out << "```\n\n";
                }
                if (!symbol.since.empty()) {
                    out << "Since: `" << symbol.since << "`\n\n";
                }
            }
        }
    }
    return Result<std::string>::success(out.str());
}

}  // namespace walk::docs
