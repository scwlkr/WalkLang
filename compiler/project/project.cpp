#include "project/project.h"

#include "lex/lexer.h"
#include "support/source_file.h"
#include "support/toml_like.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace walk::project {
namespace {

namespace toml = walk::support::toml_like;

bool is_ascii_letter(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

bool is_ascii_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

bool valid_package_name_like(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    for (std::size_t index = 0; index < name.size(); ++index) {
        const char ch = name[index];
        if (index == 0) {
            if (!is_ascii_letter(ch) && ch != '_') {
                return false;
            }
            continue;
        }
        if (is_ascii_letter(ch) || is_ascii_digit(ch) || ch == '_') {
            continue;
        }
        return false;
    }
    return true;
}

bool valid_semver(const std::string& version) {
    std::size_t start = 0;
    int parts = 0;
    for (;;) {
        const std::size_t dot = version.find('.', start);
        const std::string part = version.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
        if (part.empty() || !std::all_of(part.begin(), part.end(), [](char ch) { return is_ascii_digit(ch); })) {
            return false;
        }
        ++parts;
        if (dot == std::string::npos) {
            break;
        }
        start = dot + 1;
    }
    return parts == 3;
}

Result<bool> parse_build_mode(const std::string& mode) {
    if (mode == "debug") {
        return Result<bool>::success(false);
    }
    if (mode == "release") {
        return Result<bool>::success(true);
    }
    return Result<bool>::failure("unknown build mode \"" + mode + "\"");
}

Result<void> assign_config_value(ProjectConfig& config, const std::string& section, const std::string& key, const std::string& value, const std::string& filename, int line) {
    if (section.empty()) {
        if (key == "name") {
            Result<std::string> parsed = toml::parse_string(value, filename, line);
            if (!parsed.ok()) {
                return Result<void>::failure(parsed.error());
            }
            if (!valid_project_name(parsed.value())) {
                return Result<void>::failure(filename + ":" + std::to_string(line) + ": project name \"" + parsed.value() + "\" may contain only letters, numbers, underscore, and dash");
            }
            config.name = parsed.take_value();
            return Result<void>::success();
        }
        if (key == "version") {
            Result<std::string> parsed = toml::parse_string(value, filename, line);
            if (!parsed.ok()) {
                return Result<void>::failure(parsed.error());
            }
            config.version = parsed.take_value();
            return Result<void>::success();
        }
        if (key == "entry") {
            Result<std::string> parsed = toml::parse_string(value, filename, line);
            if (!parsed.ok()) {
                return Result<void>::failure(parsed.error());
            }
            config.entry = parsed.take_value();
            return Result<void>::success();
        }
        return Result<void>::failure(filename + ":" + std::to_string(line) + ": unknown project key \"" + key + "\"");
    }
    if (section == "build") {
        if (key == "output") {
            Result<std::string> parsed = toml::parse_string(value, filename, line);
            if (!parsed.ok()) {
                return Result<void>::failure(parsed.error());
            }
            config.build.output = parsed.take_value();
            return Result<void>::success();
        }
        if (key == "release") {
            Result<bool> parsed = toml::parse_bool(value, filename, line);
            if (!parsed.ok()) {
                return Result<void>::failure(parsed.error());
            }
            config.build.release = parsed.value();
            config.build.release_set = true;
            if (config.build.mode_set) {
                config.warnings.push_back(filename + ":" + std::to_string(line) + ": warning: [build].mode overrides [build].release");
                config.build.release = config.build.mode == "release";
            }
            return Result<void>::success();
        }
        if (key == "mode") {
            Result<std::string> parsed = toml::parse_string(value, filename, line);
            if (!parsed.ok()) {
                return Result<void>::failure(parsed.error());
            }
            Result<bool> release = parse_build_mode(parsed.value());
            if (!release.ok()) {
                return Result<void>::failure(filename + ":" + std::to_string(line) + ": [build].mode: " + release.error());
            }
            config.build.mode = parsed.take_value();
            config.build.mode_set = true;
            config.build.release = release.value();
            if (config.build.release_set) {
                config.warnings.push_back(filename + ":" + std::to_string(line) + ": warning: [build].mode overrides [build].release");
            }
            return Result<void>::success();
        }
        return Result<void>::failure(filename + ":" + std::to_string(line) + ": unknown build key \"" + key + "\"");
    }
    if (section == "dependencies") {
        if (!valid_package_name_like(key)) {
            return Result<void>::failure(filename + ":" + std::to_string(line) + ": dependency name \"" + key + "\" may contain only letters, numbers, and underscore");
        }
        for (const Dependency& dependency : config.dependencies) {
            if (dependency.name == key) {
                return Result<void>::failure(filename + ":" + std::to_string(line) + ": dependency \"" + key + "\" is already defined");
            }
        }
        Result<std::string> parsed = toml::parse_string(value, filename, line);
        if (!parsed.ok()) {
            return Result<void>::failure(parsed.error());
        }
        if (!valid_semver(parsed.value())) {
            return Result<void>::failure(filename + ":" + std::to_string(line) + ": dependency \"" + key + "\" version must be MAJOR.MINOR.PATCH");
        }
        config.dependencies.push_back({key, parsed.take_value()});
        return Result<void>::success();
    }
    return Result<void>::failure(filename + ":" + std::to_string(line) + ": unknown section [" + section + "]");
}

std::string initial_project_config(const std::string& name) {
    return "name = " + toml::quote_string(name) + "\nversion = \"0.1.0\"\nentry = \"src/main.walk\"\n\n[build]\noutput = " +
        toml::quote_string((std::filesystem::path("build") / name).generic_string()) + "\nmode = \"debug\"\n";
}

std::string initial_main_source() {
    return "imp: math_extra\n\nout: math_extra.cube(3)\n\n";
}

std::string initial_module_source() {
    return "func: cube(x int) int\n    return: * x x x\n\nexp: cube\n\n";
}

std::string initial_test_source() {
    return "imp: math_extra\n\ntest: 'cube works'\n    assert: == math_extra.cube(3) 27\n\n";
}

Result<void> write_text_file(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return Result<void>::failure("could not write " + path.string());
    }
    output << text;
    return Result<void>::success();
}

bool has_suffix(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

void append_unique(std::vector<std::filesystem::path>& dirs, const std::filesystem::path& dir) {
    if (dir.empty()) {
        return;
    }
    const std::filesystem::path clean = std::filesystem::absolute(dir).lexically_normal();
    if (std::find(dirs.begin(), dirs.end(), clean) == dirs.end()) {
        dirs.push_back(clean);
    }
}

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

bool valid_project_name(const std::string& name) {
    if (name.empty() || name == "." || name == "..") {
        return false;
    }
    for (const char ch : name) {
        if (is_ascii_letter(ch) || is_ascii_digit(ch) || ch == '_' || ch == '-') {
            continue;
        }
        return false;
    }
    return true;
}

Result<ProjectConfig> parse_project_config(const std::string& contents, const std::string& filename) {
    ProjectConfig config;
    std::string section;
    std::istringstream input(contents);
    std::string line;
    int line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        line = toml::trim(toml::strip_comment(line));
        if (line.empty()) {
            continue;
        }
        if (line.front() == '[' && line.back() == ']') {
            section = toml::trim(line.substr(1, line.size() - 2));
            if (section != "build" && section != "dependencies") {
                return Result<ProjectConfig>::failure(filename + ":" + std::to_string(line_number) + ": unknown section [" + section + "]");
            }
            continue;
        }
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos) {
            return Result<ProjectConfig>::failure(filename + ":" + std::to_string(line_number) + ": expected key = value");
        }
        const std::string key = toml::trim(line.substr(0, equals));
        const std::string value = toml::trim(line.substr(equals + 1));
        Result<void> assigned = assign_config_value(config, section, key, value, filename, line_number);
        if (!assigned.ok()) {
            return Result<ProjectConfig>::failure(assigned.error());
        }
    }
    if (config.name.empty()) {
        return Result<ProjectConfig>::failure(filename + ": name is required");
    }
    if (config.build.output.empty()) {
        config.build.output = (std::filesystem::path("build") / config.name).generic_string();
    }
    std::sort(config.dependencies.begin(), config.dependencies.end(), [](const Dependency& left, const Dependency& right) {
        return left.name < right.name;
    });
    return Result<ProjectConfig>::success(std::move(config));
}

Result<ProjectConfig> load_project_config_from_cwd() {
    std::filesystem::path dir = std::filesystem::current_path();
    for (;;) {
        const std::filesystem::path config_path = dir / "walk.toml";
        if (std::filesystem::is_regular_file(config_path)) {
            Result<ProjectConfig> config = load_project_config_at(dir);
            if (!config.ok()) {
                return config;
            }
            return config;
        }
        const std::filesystem::path parent = dir.parent_path();
        if (parent == dir || parent.empty()) {
            return Result<ProjectConfig>::failure("walk.toml not found; run walk init <project-name> or pass a .walk source file");
        }
        dir = parent;
    }
}

Result<ProjectConfig> load_project_config_at(const std::filesystem::path& root) {
    const std::filesystem::path config_path = root / "walk.toml";
    std::ifstream input(config_path, std::ios::binary);
    if (!input) {
        return Result<ProjectConfig>::failure("package manifest read failed: " + config_path.string());
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    Result<ProjectConfig> parsed = parse_project_config(contents.str(), config_path.string());
    if (!parsed.ok()) {
        return parsed;
    }
    ProjectConfig config = parsed.take_value();
    config.root = root;
    return Result<ProjectConfig>::success(std::move(config));
}

Result<std::filesystem::path> project_path(const ProjectConfig& config, const std::string& rel) {
    const std::filesystem::path path(rel);
    if (path.is_absolute()) {
        return Result<std::filesystem::path>::failure("project path must be relative: " + rel);
    }
    const std::filesystem::path clean = path.lexically_normal();
    const std::string generic = clean.generic_string();
    if (generic == "." || generic == ".." || generic.rfind("../", 0) == 0) {
        return Result<std::filesystem::path>::failure("project path escapes project root: " + rel);
    }
    return Result<std::filesystem::path>::success(config.root / clean);
}

std::vector<std::filesystem::path> local_search_dirs(const ProjectConfig& config, const std::filesystem::path& source_path) {
    std::vector<std::filesystem::path> dirs;
    append_unique(dirs, source_path.parent_path());
    Result<std::filesystem::path> entry = project_path(config, config.entry);
    if (entry.ok()) {
        append_unique(dirs, entry.value().parent_path());
    }
    return dirs;
}

std::vector<std::filesystem::path> test_files(const ProjectConfig& config) {
    std::vector<std::filesystem::path> files;
    const std::filesystem::path tests_dir = config.root / "tests";
    if (!std::filesystem::is_directory(tests_dir)) {
        return files;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(tests_dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (has_suffix(entry.path().filename().string(), "_test.walk")) {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

std::vector<std::filesystem::path> format_files(const ProjectConfig& config) {
    std::vector<std::filesystem::path> dirs;
    Result<std::filesystem::path> entry = project_path(config, config.entry);
    if (entry.ok()) {
        append_unique(dirs, entry.value().parent_path());
    }
    append_unique(dirs, config.root / "tests");

    std::vector<std::filesystem::path> files;
    for (const std::filesystem::path& dir : dirs) {
        if (!std::filesystem::is_directory(dir)) {
            continue;
        }
        for (const auto& entry_item : std::filesystem::recursive_directory_iterator(dir)) {
            if (entry_item.is_regular_file() && entry_item.path().extension() == ".walk") {
                files.push_back(entry_item.path());
            }
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

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

Result<std::filesystem::path> init_project(const std::filesystem::path& raw_project_path) {
    const std::filesystem::path clean_path = raw_project_path.lexically_normal();
    const std::string name = clean_path.filename().string();
    if (!valid_project_name(name)) {
        return Result<std::filesystem::path>::failure("project name \"" + name + "\" may contain only letters, numbers, underscore, and dash");
    }
    std::error_code exists_error;
    if (std::filesystem::exists(clean_path, exists_error)) {
        return Result<std::filesystem::path>::failure("project already exists: " + clean_path.string());
    }
    if (exists_error) {
        return Result<std::filesystem::path>::failure("project check failed: " + exists_error.message());
    }
    try {
        std::filesystem::create_directories(clean_path / "src");
        std::filesystem::create_directories(clean_path / "tests");
        std::filesystem::create_directories(clean_path / "build");
        for (const auto& file : {
                 std::pair<std::filesystem::path, std::string>{"walk.toml", initial_project_config(name)},
                 {"src/main.walk", initial_main_source()},
                 {"src/math_extra.walk", initial_module_source()},
                 {"tests/main_test.walk", initial_test_source()},
             }) {
            Result<void> written = write_text_file(clean_path / file.first, file.second);
            if (!written.ok()) {
                return Result<std::filesystem::path>::failure(written.error());
            }
        }
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<std::filesystem::path>::failure(error.what());
    }
    return Result<std::filesystem::path>::success(clean_path);
}

Result<void> clean_project(const ProjectConfig& config) {
    try {
        if (first_path_part(config.build.output) == "build") {
            std::filesystem::remove_all(config.root / "build");
            return Result<void>::success();
        }
        Result<std::filesystem::path> output_path = project_path(config, config.build.output);
        if (!output_path.ok()) {
            return Result<void>::failure(output_path.error());
        }
        std::filesystem::remove(output_path.value());
        std::filesystem::remove(output_path.value().string() + ".c");
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<void>::failure(error.what());
    }
    return Result<void>::success();
}

std::string first_path_part(const std::string& path) {
    std::filesystem::path clean = std::filesystem::path(path).lexically_normal();
    for (const auto& part : clean) {
        return part.string();
    }
    return "";
}

}  // namespace walk::project
