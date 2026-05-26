#pragma once

#include "ast/ast.h"
#include "sema/checker.h"
#include "sema/modules.h"
#include "support/result.h"
#include "support/source_file.h"

#include <map>
#include <string>
#include <vector>

namespace walk::docs {

struct ToolingSymbol {
    std::string name;
    std::string kind;
    std::string detail;
    SourceRange range;
};

struct ToolingAnalysis {
    sema::ProgramBundle bundle;
    std::vector<sema::Warning> warnings;
    std::map<std::string, std::string> sources;
};

struct DocsParam {
    std::string name;
    std::string description;
};

struct DocsSymbol {
    std::string kind;
    std::string name;
    std::string path;
    std::string signature;
    std::string summary;
    std::vector<DocsParam> params;
    std::string returns;
    std::vector<std::string> examples;
    std::string since;
};

struct DocsIndex {
    int version = 1;
    std::string source;
    std::vector<DocsSymbol> symbols;
};

[[nodiscard]] Result<ToolingAnalysis> analyze_file(const std::string& source_path, const std::vector<std::string>& search_dirs = {});
[[nodiscard]] std::vector<ToolingSymbol> sorted_symbols(const ToolingAnalysis& analysis);
[[nodiscard]] DocsIndex generate_index(const std::string& source_path, const ToolingAnalysis& analysis);
[[nodiscard]] Result<std::string> render_index(const DocsIndex& index, const std::string& format);
[[nodiscard]] Result<void> validate_index(const DocsIndex& index);
[[nodiscard]] std::string display_path(const std::string& path);
[[nodiscard]] std::string json_escape(const std::string& value);

}  // namespace walk::docs
