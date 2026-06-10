#include "sema/modules.h"

#include "sema/builtins.h"

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace walk::sema {
namespace {

constexpr const char* kModuleDiagnostic = "W3001";

void add_module_error(DiagnosticSet& diagnostics, const SourceRange& range, std::string message) {
    diagnostics.add(Diagnostic(DiagnosticSeverity::Error, kModuleDiagnostic, std::move(message), range));
}

std::vector<ast::Import*> imports(ast::Program& program) {
    std::vector<ast::Import*> result;
    for (ast::Statement* statement : program.statements) {
        if (auto* import = dynamic_cast<ast::Import*>(statement)) {
            result.push_back(import);
        }
    }
    return result;
}

bool validate_module_surface(ast::Program& program, DiagnosticSet& diagnostics) {
    for (ast::Statement* statement : program.statements) {
        if (dynamic_cast<ast::Import*>(statement) != nullptr || dynamic_cast<ast::FuncDecl*>(statement) != nullptr ||
            dynamic_cast<ast::StructDecl*>(statement) != nullptr || dynamic_cast<ast::TestDecl*>(statement) != nullptr ||
            dynamic_cast<ast::Export*>(statement) != nullptr) {
            continue;
        }
        if (dynamic_cast<ast::Defer*>(statement) != nullptr) {
            add_module_error(diagnostics, statement->range, "module error: top-level defer is only valid in an entry or test source");
            return false;
        }
        add_module_error(diagnostics, statement->range, "module error: modules may contain only imp, struct, func, test, and exp at top level");
        return false;
    }
    return true;
}

std::vector<std::filesystem::path> module_file_candidates(const std::string& module) {
    std::string nested = module;
    for (char& ch : nested) {
        if (ch == '.') {
            ch = static_cast<char>(std::filesystem::path::preferred_separator);
        }
    }
    nested += ".walk";
    const std::string literal = module + ".walk";
    if (nested == literal) {
        return {nested};
    }
    return {nested, literal};
}

std::optional<std::filesystem::path> find_module_path(const std::string& module, const std::filesystem::path& base_dir) {
    for (const std::filesystem::path& candidate : module_file_candidates(module)) {
        std::filesystem::path path = base_dir / candidate;
        std::error_code error;
        if (std::filesystem::is_regular_file(path, error)) {
            return path;
        }
    }
    return std::nullopt;
}

std::vector<std::filesystem::path> append_search_dir(std::vector<std::filesystem::path> existing, const std::vector<std::string>& dirs) {
    std::set<std::string> seen;
    std::vector<std::filesystem::path> result;
    result.reserve(existing.size() + dirs.size());
    auto add = [&](std::filesystem::path dir) {
        if (dir.empty()) {
            return;
        }
        std::filesystem::path clean = std::filesystem::absolute(std::move(dir)).lexically_normal();
        std::string key = clean.string();
        if (seen.count(key) != 0) {
            return;
        }
        seen.insert(std::move(key));
        result.push_back(std::move(clean));
    };
    for (std::filesystem::path& dir : existing) {
        add(std::move(dir));
    }
    for (const std::string& dir : dirs) {
        add(dir);
    }
    return result;
}

class Loader {
public:
    explicit Loader(std::vector<std::string> search_dirs) : search_dirs_(append_search_dir({}, search_dirs)) {}

    bool load_imports(ast::Program& program, const std::filesystem::path& base_dir, ProgramBundle& bundle) {
        for (ast::Import* import : imports(program)) {
            if (is_builtin_module(import->module)) {
                continue;
            }
            if (loading_.count(import->module) != 0) {
                add_module_error(bundle.diagnostics, import->range, "module error: import cycle includes " + import->module);
                return false;
            }
            if (bundle.modules.find(import->module) != bundle.modules.end()) {
                continue;
            }
            const std::optional<std::filesystem::path> module_path = find_module_path_in_search(import->module, base_dir);
            if (!module_path) {
                add_module_error(bundle.diagnostics, import->range, "module error: module " + import->module + " not found at " + (base_dir / (import->module + ".walk")).string());
                return false;
            }
            Result<SourceFile> loaded = SourceFile::load(module_path->string());
            if (!loaded.ok()) {
                add_module_error(bundle.diagnostics, import->range, "module error: module " + import->module + " could not be read at " + module_path->string());
                return false;
            }
            auto module = std::make_unique<Module>();
            module->name = import->module;
            module->path = module_path->string();
            module->source = std::make_unique<SourceFile>(loaded.take_value());
            module->parsed = parse::parse_source(*module->source);
            if (!module->parsed.ok()) {
                bundle.diagnostics = std::move(module->parsed.diagnostics);
                return false;
            }
            if (!validate_module_surface(*module->parsed.program, bundle.diagnostics)) {
                return false;
            }
            ast::Program* module_program = module->parsed.program.get();
            bundle.modules.emplace(import->module, std::move(module));
            loading_.insert(import->module);
            if (!load_imports(*module_program, module_path->parent_path(), bundle)) {
                return false;
            }
            loading_.erase(import->module);
        }
        return true;
    }

private:
    std::optional<std::filesystem::path> find_module_path_in_search(const std::string& module, const std::filesystem::path& base_dir) const {
        std::vector<std::filesystem::path> dirs = append_search_dir({base_dir}, {});
        dirs.insert(dirs.end(), search_dirs_.begin(), search_dirs_.end());
        for (const std::filesystem::path& dir : dirs) {
            if (std::optional<std::filesystem::path> found = find_module_path(module, dir)) {
                return found;
            }
        }
        return std::nullopt;
    }

    std::set<std::string> loading_;
    std::vector<std::filesystem::path> search_dirs_;
};

}  // namespace

bool ProgramBundle::ok() const {
    return source != nullptr && parsed.ok() && !diagnostics.has_errors();
}

ProgramBundle load_program_with_modules_and_search_dirs(const std::string& source_path, const std::vector<std::string>& search_dirs) {
    ProgramBundle bundle;
    Result<SourceFile> loaded = SourceFile::load(source_path);
    if (!loaded.ok()) {
        bundle.diagnostics.add(Diagnostic(DiagnosticSeverity::Error, "W3000", "source read failed: " + source_path));
        return bundle;
    }
    bundle.source = std::make_unique<SourceFile>(loaded.take_value());
    bundle.parsed = parse::parse_source(*bundle.source);
    if (!bundle.parsed.ok()) {
        bundle.diagnostics = bundle.parsed.diagnostics;
        return bundle;
    }
    Loader loader(search_dirs);
    loader.load_imports(*bundle.parsed.program, std::filesystem::path(source_path).parent_path(), bundle);
    return bundle;
}

ProgramBundle load_program_with_modules(const std::string& source_path) {
    return load_program_with_modules_and_search_dirs(source_path, {});
}

}  // namespace walk::sema
