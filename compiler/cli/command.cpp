#include "cli/command.h"

#include "codegen/c/c_emitter.h"
#include "debug_map/debug_map.h"
#include "docs/api_docs.h"
#include "docs/sitegen.h"
#include "ir/lower.h"
#include "package/package.h"
#include "parse/parser.h"
#include "project/project.h"
#include "repl/repl.h"
#include "sema/checker.h"
#include "sema/modules.h"
#include "support/diagnostic.h"
#include "support/source_file.h"
#include "support/version.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace walk::cli {
namespace {

constexpr int kUsageExitCode = 2;
constexpr int kDiagnosticExitCode = 1;

bool is_help_alias(const std::string& command) {
    return command == "help" || command == "--help" || command == "-h";
}

bool is_version_alias(const std::string& command) {
    return command == "version" || command == "--version";
}

const CommandInfo* find_command(const std::string& name) {
    static const std::vector<CommandInfo> commands = command_table();
    const auto found = std::find_if(commands.begin(), commands.end(), [&](const CommandInfo& command) {
        return command.name == name;
    });
    if (found == commands.end()) {
        return nullptr;
    }
    return &(*found);
}

std::string format_simple_error(std::string code, std::string message) {
    const Diagnostic diagnostic(DiagnosticSeverity::Error, std::move(code), std::move(message));
    return format_diagnostic(diagnostic) + "\n";
}

enum class WarningMode {
    Off,
    Default,
    Error,
};

struct CheckArgs {
    bool parse_only = false;
    WarningMode warning_mode = WarningMode::Default;
    std::string source_path;
};

struct NativeOptions {
    std::string cc;
    bool release = false;
    bool mode_set = false;
    std::vector<std::string> c_flags;
};

struct EmitArgs {
    WarningMode warning_mode = WarningMode::Default;
    std::string source_path;
    std::string output_path;
};

struct BuildArgs {
    WarningMode warning_mode = WarningMode::Default;
    std::string source_path;
    std::string output_path;
    std::string c_output_path;
    NativeOptions native;
};

struct RunArgs {
    WarningMode warning_mode = WarningMode::Default;
    std::string source_path;
    NativeOptions native;
    std::vector<std::string> program_args;
};

struct DocsArgs {
    std::string source_path;
    std::string output_path;
    std::string format = "markdown";
    bool strict = false;
};

struct DebugMapArgs {
    std::string source_path;
    std::string output_path;
};

struct SitegenArgs {
    std::string docs_dir = "docs";
    std::string public_dir = "public";
};

struct ProjectBuildArgs {
    WarningMode warning_mode = WarningMode::Default;
    NativeOptions native;
};

struct CompiledC {
    std::string c_code;
    std::string warning_stderr;
};

std::optional<WarningMode> parse_warning_value(const std::string& value) {
    if (value == "off") {
        return WarningMode::Off;
    }
    if (value == "default") {
        return WarningMode::Default;
    }
    if (value == "error") {
        return WarningMode::Error;
    }
    return std::nullopt;
}

bool parse_warning_arg(const std::vector<std::string>& args, std::size_t& index, WarningMode& mode, CommandResult& error, const std::string& usage) {
    const std::string& arg = args[index];
    if (arg == "--warnings") {
        if (index + 1 >= args.size()) {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return true;
        }
        const std::optional<WarningMode> parsed = parse_warning_value(args[index + 1]);
        if (!parsed) {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return true;
        }
        mode = *parsed;
        ++index;
        return true;
    }
    if (arg.rfind("--warnings=", 0) == 0) {
        const std::optional<WarningMode> parsed = parse_warning_value(arg.substr(std::string("--warnings=").size()));
        if (!parsed) {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return true;
        }
        mode = *parsed;
        return true;
    }
    return false;
}

std::string check_usage() {
    return "usage: walk check [--parse-only] [--warnings=off|default|error] <source.walk>";
}

std::optional<CheckArgs> parse_check_args(const std::vector<std::string>& args, CommandResult& error) {
    CheckArgs parsed;
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg == "--parse-only") {
            parsed.parse_only = true;
            continue;
        }
        if (arg == "--warnings") {
            if (index + 1 >= args.size()) {
                error = {kUsageExitCode, "", format_simple_error("W0005", check_usage())};
                return std::nullopt;
            }
            const std::optional<WarningMode> mode = parse_warning_value(args[index + 1]);
            if (!mode) {
                error = {kUsageExitCode, "", format_simple_error("W0005", check_usage())};
                return std::nullopt;
            }
            parsed.warning_mode = *mode;
            ++index;
            continue;
        }
        if (arg.rfind("--warnings=", 0) == 0) {
            const std::optional<WarningMode> mode = parse_warning_value(arg.substr(std::string("--warnings=").size()));
            if (!mode) {
                error = {kUsageExitCode, "", format_simple_error("W0005", check_usage())};
                return std::nullopt;
            }
            parsed.warning_mode = *mode;
            continue;
        }
        if (!arg.empty() && arg[0] == '-') {
            error = {kUsageExitCode, "", format_simple_error("W0005", check_usage())};
            return std::nullopt;
        }
        if (!parsed.source_path.empty()) {
            error = {kUsageExitCode, "", format_simple_error("W0005", check_usage())};
            return std::nullopt;
        }
        parsed.source_path = arg;
    }
    if (parsed.source_path.empty()) {
        error = {kUsageExitCode, "", format_simple_error("W0005", check_usage())};
        return std::nullopt;
    }
    return parsed;
}

std::string emit_usage() {
    return "usage: walk emit-c [--warnings=off|default|error] <source.walk> -o <output.c>";
}

std::string build_usage() {
    return "usage: walk build [--mode debug|release] [--warnings=off|default|error] [--cc <cc>] [--cflag <flag>] <source.walk> -o <output>";
}

std::string run_usage() {
    return "usage: walk run [--mode debug|release] [--warnings=off|default|error] [--cc <cc>] [--cflag <flag>] <source.walk> [-- <program args>]";
}

std::string test_usage() {
    return "usage: walk test [--warnings=off|default|error] [--cc <cc>] [--cflag <flag>] <source.walk>";
}

std::optional<bool> parse_build_mode_value(const std::string& value) {
    if (value == "debug") {
        return false;
    }
    if (value == "release") {
        return true;
    }
    return std::nullopt;
}

bool set_build_mode(NativeOptions& options, bool release) {
    if (options.mode_set && options.release != release) {
        return false;
    }
    options.mode_set = true;
    options.release = release;
    return true;
}

std::optional<EmitArgs> parse_emit_args(const std::vector<std::string>& args, CommandResult& error) {
    EmitArgs parsed;
    const std::string usage = emit_usage();
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg == "-o") {
            if (index + 1 >= args.size()) {
                error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
                return std::nullopt;
            }
            parsed.output_path = args[index + 1];
            ++index;
            continue;
        }
        if (parse_warning_arg(args, index, parsed.warning_mode, error, usage)) {
            if (error.exit_code != 0) {
                return std::nullopt;
            }
            continue;
        }
        if (!arg.empty() && arg[0] == '-') {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return std::nullopt;
        }
        if (!parsed.source_path.empty()) {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return std::nullopt;
        }
        parsed.source_path = arg;
    }
    if (parsed.source_path.empty() || parsed.output_path.empty()) {
        error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
        return std::nullopt;
    }
    return parsed;
}

bool parse_native_arg(const std::vector<std::string>& args, std::size_t& index, NativeOptions& native, CommandResult& error, const std::string& usage) {
    const std::string& arg = args[index];
    if (arg == "--release" || arg == "--debug") {
        if (!set_build_mode(native, arg == "--release")) {
            error = {kUsageExitCode, "", format_simple_error("W0005", "build mode conflict: choose one of debug or release")};
        }
        return true;
    }
    if (arg == "--mode") {
        if (index + 1 >= args.size()) {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return true;
        }
        const std::optional<bool> release = parse_build_mode_value(args[index + 1]);
        if (!release || !set_build_mode(native, *release)) {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return true;
        }
        ++index;
        return true;
    }
    if (arg.rfind("--mode=", 0) == 0) {
        const std::optional<bool> release = parse_build_mode_value(arg.substr(std::string("--mode=").size()));
        if (!release || !set_build_mode(native, *release)) {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return true;
        }
        return true;
    }
    if (arg == "--cc") {
        if (index + 1 >= args.size()) {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return true;
        }
        native.cc = args[index + 1];
        ++index;
        return true;
    }
    if (arg == "--cflag") {
        if (index + 1 >= args.size()) {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return true;
        }
        native.c_flags.push_back(args[index + 1]);
        ++index;
        return true;
    }
    return false;
}

std::optional<BuildArgs> parse_build_args(const std::vector<std::string>& args, CommandResult& error) {
    BuildArgs parsed;
    const std::string usage = build_usage();
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg == "-o") {
            if (index + 1 >= args.size()) {
                error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
                return std::nullopt;
            }
            parsed.output_path = args[index + 1];
            ++index;
            continue;
        }
        if (arg == "--emit-c") {
            if (index + 1 >= args.size()) {
                error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
                return std::nullopt;
            }
            parsed.c_output_path = args[index + 1];
            ++index;
            continue;
        }
        if (parse_warning_arg(args, index, parsed.warning_mode, error, usage) || parse_native_arg(args, index, parsed.native, error, usage)) {
            if (error.exit_code != 0) {
                return std::nullopt;
            }
            continue;
        }
        if (!arg.empty() && arg[0] == '-') {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return std::nullopt;
        }
        if (!parsed.source_path.empty()) {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return std::nullopt;
        }
        parsed.source_path = arg;
    }
    if (parsed.source_path.empty() || parsed.output_path.empty()) {
        error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
        return std::nullopt;
    }
    return parsed;
}

std::optional<RunArgs> parse_run_like_args(const std::vector<std::string>& args, CommandResult& error, const std::string& usage, bool allow_program_args = false) {
    RunArgs parsed;
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (allow_program_args && arg == "--") {
            if (parsed.source_path.empty()) {
                error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
                return std::nullopt;
            }
            parsed.program_args.insert(parsed.program_args.end(), args.begin() + static_cast<std::ptrdiff_t>(index + 1), args.end());
            break;
        }
        if (parse_warning_arg(args, index, parsed.warning_mode, error, usage) || parse_native_arg(args, index, parsed.native, error, usage)) {
            if (error.exit_code != 0) {
                return std::nullopt;
            }
            continue;
        }
        if (!arg.empty() && arg[0] == '-') {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return std::nullopt;
        }
        if (!parsed.source_path.empty()) {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return std::nullopt;
        }
        parsed.source_path = arg;
    }
    if (parsed.source_path.empty()) {
        error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
        return std::nullopt;
    }
    return parsed;
}

std::optional<DocsArgs> parse_docs_args(const std::vector<std::string>& args, CommandResult& error) {
    DocsArgs parsed;
    const std::string usage = "usage: walk docs [--strict] [--format markdown|json] [-o output] [source.walk]";
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg == "-o" || arg == "--output") {
            if (index + 1 >= args.size()) {
                error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
                return std::nullopt;
            }
            parsed.output_path = args[index + 1];
            ++index;
            continue;
        }
        if (arg == "--format") {
            if (index + 1 >= args.size()) {
                error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
                return std::nullopt;
            }
            parsed.format = args[index + 1];
            if (parsed.format != "markdown" && parsed.format != "json") {
                error = {kUsageExitCode, "", format_simple_error("W0005", "docs format must be markdown or json")};
                return std::nullopt;
            }
            ++index;
            continue;
        }
        if (arg == "--strict") {
            parsed.strict = true;
            continue;
        }
        if (!arg.empty() && arg[0] == '-') {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return std::nullopt;
        }
        if (!parsed.source_path.empty()) {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return std::nullopt;
        }
        parsed.source_path = arg;
    }
    return parsed;
}

std::optional<DebugMapArgs> parse_debug_map_args(const std::vector<std::string>& args, CommandResult& error) {
    DebugMapArgs parsed;
    const std::string usage = "usage: walk debug-map [-o output.json] [source.walk]";
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg == "-o" || arg == "--output") {
            if (index + 1 >= args.size()) {
                error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
                return std::nullopt;
            }
            parsed.output_path = args[index + 1];
            ++index;
            continue;
        }
        if (!arg.empty() && arg[0] == '-') {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return std::nullopt;
        }
        if (!parsed.source_path.empty()) {
            error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
            return std::nullopt;
        }
        parsed.source_path = arg;
    }
    return parsed;
}

std::optional<SitegenArgs> parse_sitegen_args(const std::vector<std::string>& args, CommandResult& error) {
    SitegenArgs parsed;
    const std::string usage = "usage: walk sitegen [-docs docs-dir] [-public public-dir]";
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg == "-docs" || arg == "--docs") {
            if (index + 1 >= args.size()) {
                error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
                return std::nullopt;
            }
            parsed.docs_dir = args[index + 1];
            ++index;
            continue;
        }
        if (arg == "-public" || arg == "--public") {
            if (index + 1 >= args.size()) {
                error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
                return std::nullopt;
            }
            parsed.public_dir = args[index + 1];
            ++index;
            continue;
        }
        error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
        return std::nullopt;
    }
    return parsed;
}

bool use_project_build(const std::vector<std::string>& args) {
    if (args.empty()) {
        return true;
    }
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg == "--release" || arg == "--debug" || arg.rfind("--mode=", 0) == 0 || arg.rfind("--warnings=", 0) == 0) {
            continue;
        }
        if (arg == "--cc" || arg == "--cflag" || arg == "--warnings" || arg == "--mode") {
            ++index;
            if (index >= args.size()) {
                return false;
            }
            continue;
        }
        return false;
    }
    return true;
}

bool use_project_check_like(const std::vector<std::string>& args) {
    if (args.empty()) {
        return true;
    }
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg.rfind("--warnings=", 0) == 0) {
            continue;
        }
        if (arg == "--warnings") {
            ++index;
            if (index >= args.size()) {
                return false;
            }
            continue;
        }
        return false;
    }
    return true;
}

std::optional<ProjectBuildArgs> parse_project_build_args(const std::vector<std::string>& args, CommandResult& error) {
    ProjectBuildArgs parsed;
    const std::string usage = "usage: walk build [--mode debug|release] [--warnings=off|default|error] [--cc <cc>] [--cflag <flag>]";
    for (std::size_t index = 0; index < args.size(); ++index) {
        if (parse_warning_arg(args, index, parsed.warning_mode, error, usage) || parse_native_arg(args, index, parsed.native, error, usage)) {
            if (error.exit_code != 0) {
                return std::nullopt;
            }
            continue;
        }
        error = {kUsageExitCode, "", format_simple_error("W0005", usage)};
        return std::nullopt;
    }
    return parsed;
}

std::optional<WarningMode> parse_project_check_args(const std::vector<std::string>& args, CommandResult& error, const std::string& command) {
    WarningMode mode = WarningMode::Default;
    const std::string usage = "usage: walk " + command + " [--warnings=off|default|error]";
    for (std::size_t index = 0; index < args.size(); ++index) {
        if (parse_warning_arg(args, index, mode, error, usage)) {
            if (error.exit_code != 0) {
                return std::nullopt;
            }
            continue;
        }
        return std::nullopt;
    }
    return mode;
}

CommandResult handle_warnings(const std::vector<sema::Warning>& warnings, WarningMode mode) {
    if (mode == WarningMode::Off || warnings.empty()) {
        return {0, "", ""};
    }
    std::ostringstream stderr_text;
    for (const sema::Warning& warning : warnings) {
        stderr_text << format_diagnostic(Diagnostic(DiagnosticSeverity::Warning, "", warning.message, warning.range)) << "\n";
    }
    if (mode == WarningMode::Error) {
        stderr_text << "warnings-as-errors: " << warnings.size() << " warning(s)\n";
        return {kDiagnosticExitCode, "", stderr_text.str()};
    }
    return {0, "", stderr_text.str()};
}

std::optional<CompiledC> compile_file_to_c(
    const std::string& source_path,
    bool tests_only,
    WarningMode warning_mode,
    CommandResult& error,
    const std::vector<std::string>& search_dirs = {}) {
    sema::ProgramBundle bundle = sema::load_program_with_modules_and_search_dirs(source_path, search_dirs);
    if (!bundle.ok()) {
        error = {kDiagnosticExitCode, "", bundle.diagnostics.format(bundle.source.get())};
        return std::nullopt;
    }
    sema::CheckResult checked = sema::check_programs(*bundle.parsed.program, bundle.modules);
    if (!checked.ok()) {
        error = {kDiagnosticExitCode, "", checked.diagnostics.format(bundle.source.get())};
        return std::nullopt;
    }
    CommandResult warning_result = handle_warnings(checked.warnings, warning_mode);
    if (warning_result.exit_code != 0) {
        error = warning_result;
        return std::nullopt;
    }
    ir::LoweredProgram lowered = ir::lower_program(*bundle.parsed.program, bundle.modules);
    Result<std::string> emitted = codegen::c::emit_c(lowered, {tests_only});
    if (!emitted.ok()) {
        error = {kDiagnosticExitCode, "", format_simple_error("W5001", emitted.error())};
        return std::nullopt;
    }
    return CompiledC{emitted.take_value(), warning_result.stderr_text};
}

std::optional<std::string> check_file_with_search_dirs(
    const std::string& source_path,
    WarningMode warning_mode,
    CommandResult& error,
    const std::vector<std::string>& search_dirs = {}) {
    sema::ProgramBundle bundle = sema::load_program_with_modules_and_search_dirs(source_path, search_dirs);
    if (!bundle.ok()) {
        error = {kDiagnosticExitCode, "", bundle.diagnostics.format(bundle.source.get())};
        return std::nullopt;
    }
    sema::CheckResult checked = sema::check_programs(*bundle.parsed.program, bundle.modules);
    if (!checked.ok()) {
        error = {kDiagnosticExitCode, "", checked.diagnostics.format(bundle.source.get())};
        return std::nullopt;
    }
    CommandResult warning_result = handle_warnings(checked.warnings, warning_mode);
    if (warning_result.exit_code != 0) {
        error = warning_result;
        return std::nullopt;
    }
    return warning_result.stderr_text;
}

std::vector<std::string> path_strings(const std::vector<std::filesystem::path>& paths) {
    std::vector<std::string> result;
    std::set<std::string> seen;
    for (const std::filesystem::path& path : paths) {
        if (path.empty()) {
            continue;
        }
        const std::string text = path.string();
        if (seen.insert(text).second) {
            result.push_back(text);
        }
    }
    return result;
}

Result<std::vector<std::string>> project_search_dirs_with_packages(const project::ProjectConfig& config, const std::filesystem::path& source_path) {
    std::vector<std::filesystem::path> dirs = project::local_search_dirs(config, source_path);
    Result<std::vector<std::filesystem::path>> package_dirs = package::package_search_dirs(config);
    if (!package_dirs.ok()) {
        return Result<std::vector<std::string>>::failure(package_dirs.error());
    }
    dirs.insert(dirs.end(), package_dirs.value().begin(), package_dirs.value().end());
    return Result<std::vector<std::string>>::success(path_strings(dirs));
}

Result<std::filesystem::path> source_path_or_project_entry(const std::string& source_path) {
    if (!source_path.empty()) {
        return Result<std::filesystem::path>::success(std::filesystem::absolute(source_path).lexically_normal());
    }
    Result<project::ProjectConfig> config = project::load_project_config_from_cwd();
    if (!config.ok()) {
        return Result<std::filesystem::path>::failure(config.error());
    }
    Result<std::filesystem::path> entry = project::project_path(config.value(), config.value().entry);
    if (!entry.ok()) {
        return entry;
    }
    return Result<std::filesystem::path>::success(std::filesystem::absolute(entry.value()).lexically_normal());
}

std::vector<std::string> tooling_search_dirs_for_source(const std::filesystem::path& source_path) {
    Result<project::ProjectConfig> config = project::load_project_config_from_cwd();
    if (!config.ok()) {
        return {};
    }
    Result<std::vector<std::string>> dirs = project_search_dirs_with_packages(config.value(), source_path);
    if (!dirs.ok()) {
        return {};
    }
    return dirs.take_value();
}

std::string shell_quote(const std::string& value) {
    std::string out = "'";
    for (const char ch : value) {
        if (ch == '\'') {
            out += "'\\''";
        } else {
            out += ch;
        }
    }
    out += "'";
    return out;
}

void ensure_parent_dir(const std::string& path) {
    const std::filesystem::path parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

Result<void> write_file(const std::string& path, const std::string& text) {
    try {
        ensure_parent_dir(path);
        std::ofstream output(path, std::ios::binary);
        if (!output) {
            return Result<void>::failure("could not write " + path);
        }
        output << text;
        return Result<void>::success();
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<void>::failure(error.what());
    }
}

bool is_runtime_dir(const std::filesystem::path& dir) {
    return std::filesystem::is_regular_file(dir / "walk_runtime.h") && std::filesystem::is_regular_file(dir / "walk_runtime.c");
}

std::optional<std::filesystem::path> find_runtime_dir_from(std::filesystem::path start) {
    start = std::filesystem::absolute(std::move(start)).lexically_normal();
    for (;;) {
        if (is_runtime_dir(start)) {
            return start;
        }
        const std::filesystem::path candidate = start / "runtime";
        if (is_runtime_dir(candidate)) {
            return candidate;
        }
        const std::filesystem::path parent = start.parent_path();
        if (parent == start || parent.empty()) {
            return std::nullopt;
        }
        start = parent;
    }
}

Result<std::filesystem::path> find_runtime_dir() {
    if (const char* env = std::getenv("WALK_RUNTIME_DIR")) {
        const std::filesystem::path dir(env);
        if (is_runtime_dir(dir)) {
            return Result<std::filesystem::path>::success(std::filesystem::absolute(dir).lexically_normal());
        }
        return Result<std::filesystem::path>::failure("WALK_RUNTIME_DIR does not contain walk_runtime.c: " + std::string(env));
    }
    const std::optional<std::filesystem::path> found = find_runtime_dir_from(std::filesystem::current_path());
    if (found) {
        return Result<std::filesystem::path>::success(*found);
    }
    if (const char* home = std::getenv("HOME")) {
        const std::filesystem::path default_install = std::filesystem::path(home) / ".local" / "lib" / "walk" / "runtime";
        if (is_runtime_dir(default_install)) {
            return Result<std::filesystem::path>::success(std::filesystem::absolute(default_install).lexically_normal());
        }
    }
    return Result<std::filesystem::path>::failure("walk runtime not found; run from the repo or set WALK_RUNTIME_DIR");
}

std::string platform_source_name() {
#if defined(_WIN32)
    return "walk_platform_windows.c";
#else
    return "walk_platform_posix.c";
#endif
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

std::filesystem::path temp_dir(const std::string& prefix) {
    const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path dir = std::filesystem::temp_directory_path() / (prefix + "-" + std::to_string(tick));
    std::filesystem::create_directories(dir);
    return dir;
}

std::vector<std::string> native_build_args(const std::string& c_path, const std::string& output_path, const NativeOptions& options, const std::filesystem::path& runtime_dir) {
    std::vector<std::string> args = {
        c_path,
        (runtime_dir / "walk_runtime.c").string(),
        (runtime_dir / "platform" / platform_source_name()).string(),
        "-I",
        runtime_dir.string(),
        "-o",
        output_path,
    };
    if (options.release) {
        args.push_back("-O3");
        args.push_back("-DNDEBUG");
    } else {
        args.push_back("-g");
        args.push_back("-O0");
    }
    args.insert(args.end(), options.c_flags.begin(), options.c_flags.end());
    args.push_back("-lm");
    return args;
}

Result<void> build_c(const std::string& c_code, const std::string& c_path, const std::string& output_path, const NativeOptions& options) {
    Result<void> written = write_file(c_path, c_code);
    if (!written.ok()) {
        return written;
    }
    try {
        ensure_parent_dir(output_path);
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<void>::failure(error.what());
    }
    std::string cc = options.cc;
    if (cc.empty()) {
        if (const char* env = std::getenv("WALK_CC")) {
            cc = env;
        }
    }
    if (cc.empty()) {
        cc = "cc";
    }
    Result<std::filesystem::path> runtime_dir = find_runtime_dir();
    if (!runtime_dir.ok()) {
        return Result<void>::failure(runtime_dir.error());
    }
    std::vector<std::string> argv = native_build_args(c_path, output_path, options, runtime_dir.value());
    std::ostringstream command;
    command << shell_quote(cc);
    for (const std::string& arg : argv) {
        command << " " << shell_quote(arg);
    }
    const std::filesystem::path log = temp_dir("walk-cc") / "cc.log";
    command << " > " << shell_quote(log.string()) << " 2>&1";
    const int code = std::system(command.str().c_str());
    if (code != 0) {
        return Result<void>::failure("native build failed: " + read_text_file(log));
    }
    std::filesystem::remove_all(log.parent_path());
    return Result<void>::success();
}

int run_passthrough(const std::string& executable, const std::vector<std::string>& args = {}) {
    std::ostringstream command;
    command << shell_quote(executable);
    for (const std::string& arg : args) {
        command << " " << shell_quote(arg);
    }
    return std::system(command.str().c_str());
}

std::string process_failure_message(const std::string& label, int status) {
    if (status == -1) {
        return label + " failed: process launch failed";
    }
#if defined(_WIN32)
    return label + " failed with status " + std::to_string(status);
#else
    const auto signal_name = [](int signal) -> std::string {
        switch (signal) {
#if defined(SIGABRT)
        case SIGABRT:
            return "SIGABRT";
#endif
#if defined(SIGALRM)
        case SIGALRM:
            return "SIGALRM";
#endif
#if defined(SIGBUS)
        case SIGBUS:
            return "SIGBUS";
#endif
#if defined(SIGFPE)
        case SIGFPE:
            return "SIGFPE";
#endif
#if defined(SIGILL)
        case SIGILL:
            return "SIGILL";
#endif
#if defined(SIGKILL)
        case SIGKILL:
            return "SIGKILL";
#endif
#if defined(SIGSEGV)
        case SIGSEGV:
            return "SIGSEGV";
#endif
#if defined(SIGTERM)
        case SIGTERM:
            return "SIGTERM";
#endif
        default:
            return "";
        }
    };
    if (WIFEXITED(status)) {
        return label + " failed with exit status " + std::to_string(WEXITSTATUS(status));
    }
    if (WIFSIGNALED(status)) {
        const int signal = WTERMSIG(status);
        const std::string name = signal_name(signal);
        if (!name.empty()) {
            return label + " terminated by signal " + std::to_string(signal) + " (" + name + ")";
        }
        return label + " terminated by signal " + std::to_string(signal);
    }
    return label + " failed with status " + std::to_string(status);
#endif
}

int run_capture(const std::string& executable, const std::filesystem::path& stdout_path, const std::filesystem::path& stderr_path) {
    const std::string command = shell_quote(executable) + " > " + shell_quote(stdout_path.string()) + " 2> " + shell_quote(stderr_path.string());
    return std::system(command.c_str());
}

CommandResult init_command(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return {kUsageExitCode, "", format_simple_error("W0005", "usage: walk init <project-name>")};
    }
    Result<std::filesystem::path> created = project::init_project(args[0]);
    if (!created.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W7001", created.error())};
    }
    return {0, created.value().string() + "\n", ""};
}

CommandResult clean_command(const std::vector<std::string>& args) {
    if (!args.empty()) {
        return {kUsageExitCode, "", format_simple_error("W0005", "usage: walk clean")};
    }
    Result<project::ProjectConfig> config = project::load_project_config_from_cwd();
    if (!config.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W7002", config.error())};
    }
    Result<void> cleaned = project::clean_project(config.value());
    if (!cleaned.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W7003", cleaned.error())};
    }
    return {0, "clean\n", ""};
}

CommandResult project_check_command(const std::vector<std::string>& args) {
    CommandResult parse_error{0, "", ""};
    const std::optional<WarningMode> warning_mode = parse_project_check_args(args, parse_error, "check");
    if (!warning_mode) {
        return parse_error;
    }
    Result<project::ProjectConfig> config = project::load_project_config_from_cwd();
    if (!config.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W7002", config.error())};
    }
    std::string stderr_text;
    for (const std::string& warning : config.value().warnings) {
        stderr_text += warning + "\n";
    }
    Result<std::filesystem::path> entry_path = project::project_path(config.value(), config.value().entry);
    if (!entry_path.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W7004", entry_path.error())};
    }
    std::vector<std::filesystem::path> sources{entry_path.value()};
    const std::vector<std::filesystem::path> tests = project::test_files(config.value());
    sources.insert(sources.end(), tests.begin(), tests.end());
    for (const std::filesystem::path& source : sources) {
        Result<std::vector<std::string>> search_dirs = project_search_dirs_with_packages(config.value(), source);
        if (!search_dirs.ok()) {
            return {kDiagnosticExitCode, "", stderr_text + format_simple_error("W7005", search_dirs.error())};
        }
        CommandResult check_error;
        std::optional<std::string> warnings = check_file_with_search_dirs(source.string(), *warning_mode, check_error, search_dirs.value());
        if (!warnings) {
            check_error.stderr_text = stderr_text + check_error.stderr_text;
            return check_error;
        }
        stderr_text += *warnings;
    }
    return {0, "ok\n", stderr_text};
}

CommandResult project_build_command(const std::vector<std::string>& args) {
    CommandResult parse_error{0, "", ""};
    const std::optional<ProjectBuildArgs> parsed = parse_project_build_args(args, parse_error);
    if (!parsed) {
        return parse_error;
    }
    Result<project::ProjectConfig> config = project::load_project_config_from_cwd();
    if (!config.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W7002", config.error())};
    }
    std::string stderr_text;
    for (const std::string& warning : config.value().warnings) {
        stderr_text += warning + "\n";
    }
    Result<std::filesystem::path> entry_path = project::project_path(config.value(), config.value().entry);
    if (!entry_path.ok()) {
        return {kDiagnosticExitCode, "", stderr_text + format_simple_error("W7004", entry_path.error())};
    }
    Result<std::filesystem::path> output_path = project::project_path(config.value(), config.value().build.output);
    if (!output_path.ok()) {
        return {kDiagnosticExitCode, "", stderr_text + format_simple_error("W7004", output_path.error())};
    }
    NativeOptions native = parsed->native;
    if (!native.mode_set) {
        native.release = config.value().build.release;
    }
    Result<std::vector<std::string>> search_dirs = project_search_dirs_with_packages(config.value(), entry_path.value());
    if (!search_dirs.ok()) {
        return {kDiagnosticExitCode, "", stderr_text + format_simple_error("W7005", search_dirs.error())};
    }
    CommandResult compile_error;
    std::optional<CompiledC> compiled = compile_file_to_c(entry_path.value().string(), false, parsed->warning_mode, compile_error, search_dirs.value());
    if (!compiled) {
        compile_error.stderr_text = stderr_text + compile_error.stderr_text;
        return compile_error;
    }
    stderr_text += compiled->warning_stderr;
    Result<void> built = build_c(compiled->c_code, output_path.value().string() + ".c", output_path.value().string(), native);
    if (!built.ok()) {
        return {kDiagnosticExitCode, "", stderr_text + format_simple_error("W5003", built.error())};
    }
    return {0, output_path.value().string() + "\n", stderr_text};
}

CommandResult project_test_command(const std::vector<std::string>& args) {
    CommandResult parse_error{0, "", ""};
    const std::optional<WarningMode> warning_mode = parse_project_check_args(args, parse_error, "test");
    if (!warning_mode) {
        return parse_error;
    }
    Result<project::ProjectConfig> config = project::load_project_config_from_cwd();
    if (!config.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W7002", config.error())};
    }
    const std::vector<std::filesystem::path> tests = project::test_files(config.value());
    if (tests.empty()) {
        return {0, "ok 0 test files\n", ""};
    }
    std::string warning_stderr;
    for (const std::filesystem::path& test_path : tests) {
        Result<std::vector<std::string>> search_dirs = project_search_dirs_with_packages(config.value(), test_path);
        if (!search_dirs.ok()) {
            return {kDiagnosticExitCode, "", warning_stderr + format_simple_error("W7005", search_dirs.error())};
        }
        CommandResult compile_error;
        std::optional<CompiledC> compiled = compile_file_to_c(test_path.string(), true, *warning_mode, compile_error, search_dirs.value());
        if (!compiled) {
            compile_error.stderr_text = warning_stderr + compile_error.stderr_text;
            return compile_error;
        }
        warning_stderr += compiled->warning_stderr;
        const std::filesystem::path dir = temp_dir("walk-project-test");
        const std::filesystem::path exe = dir / "tests";
        Result<void> built = build_c(compiled->c_code, (dir / "tests.c").string(), exe.string(), {});
        if (!built.ok()) {
            std::filesystem::remove_all(dir);
            return {kDiagnosticExitCode, "", warning_stderr + format_simple_error("W5003", built.error())};
        }
        if (const int code = run_passthrough(exe.string()); code != 0) {
            std::filesystem::remove_all(dir);
            return {kDiagnosticExitCode, "", warning_stderr + format_simple_error("W5005", process_failure_message("tests", code))};
        }
        std::filesystem::remove_all(dir);
    }
    return {0, "", warning_stderr};
}

CommandResult fmt_command(const std::vector<std::string>& args) {
    bool write = false;
    std::string source_path;
    for (const std::string& arg : args) {
        if (arg == "-w") {
            write = true;
            continue;
        }
        if (!source_path.empty()) {
            return {kUsageExitCode, "", format_simple_error("W0005", "usage: walk fmt [-w] <source.walk>")};
        }
        source_path = arg;
    }
    if (source_path.empty()) {
        Result<project::ProjectConfig> config = project::load_project_config_from_cwd();
        if (!config.ok()) {
            return {kDiagnosticExitCode, "", format_simple_error("W7002", config.error())};
        }
        std::ostringstream stdout_text;
        for (const std::filesystem::path& path : project::format_files(config.value())) {
            const std::string source = read_text_file(path);
            Result<std::string> formatted = project::format_source(source, path.string());
            if (!formatted.ok()) {
                return {kDiagnosticExitCode, "", formatted.error()};
            }
            if (source != formatted.value()) {
                Result<void> written = write_file(path.string(), formatted.value());
                if (!written.ok()) {
                    return {kDiagnosticExitCode, "", format_simple_error("W7006", written.error())};
                }
            }
            std::error_code rel_error;
            const std::filesystem::path rel = std::filesystem::relative(path, config.value().root, rel_error);
            stdout_text << (rel_error ? path.string() : rel.string()) << "\n";
        }
        return {0, stdout_text.str(), ""};
    }

    const std::string source = read_text_file(source_path);
    Result<std::string> formatted = project::format_source(source, source_path);
    if (!formatted.ok()) {
        return {kDiagnosticExitCode, "", formatted.error()};
    }
    if (write) {
        Result<void> written = write_file(source_path, formatted.value());
        if (!written.ok()) {
            return {kDiagnosticExitCode, "", format_simple_error("W7006", written.error())};
        }
        return {0, "", ""};
    }
    return {0, formatted.value(), ""};
}

CommandResult package_command(const std::vector<std::string>& args) {
    if (args.empty()) {
        return {kUsageExitCode, "", format_simple_error("W0005", "usage: walk package <init|resolve|publish>")};
    }
    const std::string& subcommand = args.front();
    const std::vector<std::string> rest(args.begin() + 1, args.end());
    if (subcommand == "init") {
        if (rest.size() != 1) {
            return {kUsageExitCode, "", format_simple_error("W0005", "usage: walk package init <package-name>")};
        }
        Result<std::filesystem::path> created = package::init_package(rest[0]);
        if (!created.ok()) {
            return {kDiagnosticExitCode, "", format_simple_error("W7101", created.error())};
        }
        return {0, created.value().string() + "\n", ""};
    }
    if (subcommand == "resolve") {
        if (rest.size() != 1) {
            return {kUsageExitCode, "", format_simple_error("W0005", "usage: walk package resolve <registry-dir>")};
        }
        Result<project::ProjectConfig> config = project::load_project_config_from_cwd();
        if (!config.ok()) {
            return {kDiagnosticExitCode, "", format_simple_error("W7002", config.error())};
        }
        if (config.value().dependencies.empty()) {
            return {0, "ok 0 dependencies\n", ""};
        }
        Result<std::vector<package::LockEntry>> entries = package::resolve_dependencies(config.value(), rest[0]);
        if (!entries.ok()) {
            return {kDiagnosticExitCode, "", format_simple_error("W7102", entries.error())};
        }
        Result<void> written = package::write_lock_file(config.value().root, entries.value());
        if (!written.ok()) {
            return {kDiagnosticExitCode, "", format_simple_error("W7103", written.error())};
        }
        return {0, "resolved " + std::to_string(entries.value().size()) + " package(s)\n", ""};
    }
    if (subcommand == "publish") {
        if (rest.size() != 1) {
            return {kUsageExitCode, "", format_simple_error("W0005", "usage: walk package publish <registry-dir>")};
        }
        Result<project::ProjectConfig> config = project::load_project_config_from_cwd();
        if (!config.ok()) {
            return {kDiagnosticExitCode, "", format_simple_error("W7002", config.error())};
        }
        Result<void> manifest = package::validate_publish_manifest(config.value());
        if (!manifest.ok()) {
            return {kDiagnosticExitCode, "", format_simple_error("W7104", manifest.error())};
        }
        CommandResult checked = project_check_command({"--warnings=error"});
        if (checked.exit_code != 0) {
            checked.stderr_text = format_simple_error("W7105", "package check failed: " + checked.stderr_text);
            return checked;
        }
        CommandResult tested = project_test_command({"--warnings=error"});
        if (tested.exit_code != 0) {
            tested.stderr_text = checked.stderr_text + format_simple_error("W7106", "package tests failed: " + tested.stderr_text);
            return tested;
        }
        Result<std::filesystem::path> destination = package::publish_package(config.value(), rest[0]);
        if (!destination.ok()) {
            return {kDiagnosticExitCode, checked.stdout_text + tested.stdout_text, checked.stderr_text + tested.stderr_text + format_simple_error("W7107", destination.error())};
        }
        return {0, checked.stdout_text + tested.stdout_text + destination.value().string() + "\n", checked.stderr_text + tested.stderr_text};
    }
    return {kUsageExitCode, "", format_simple_error("W0005", "unknown package command \"" + subcommand + "\"")};
}

CommandResult emit_c_command(const std::vector<std::string>& args) {
    CommandResult parse_error{0, "", ""};
    const std::optional<EmitArgs> parsed = parse_emit_args(args, parse_error);
    if (!parsed) {
        return parse_error;
    }
    CommandResult compile_error;
    std::optional<CompiledC> compiled = compile_file_to_c(parsed->source_path, false, parsed->warning_mode, compile_error);
    if (!compiled) {
        return compile_error;
    }
    Result<void> written = write_file(parsed->output_path, compiled->c_code);
    if (!written.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W5002", written.error())};
    }
    return {0, parsed->output_path + "\n", compiled->warning_stderr};
}

CommandResult build_command(const std::vector<std::string>& args) {
    if (use_project_build(args)) {
        return project_build_command(args);
    }
    CommandResult parse_error{0, "", ""};
    const std::optional<BuildArgs> parsed = parse_build_args(args, parse_error);
    if (!parsed) {
        return parse_error;
    }
    CommandResult compile_error;
    std::optional<CompiledC> compiled = compile_file_to_c(parsed->source_path, false, parsed->warning_mode, compile_error);
    if (!compiled) {
        return compile_error;
    }
    std::string c_path = parsed->c_output_path;
    if (c_path.empty()) {
        c_path = parsed->output_path + ".c";
    }
    Result<void> built = build_c(compiled->c_code, c_path, parsed->output_path, parsed->native);
    if (!built.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W5003", built.error())};
    }
    return {0, parsed->output_path + "\n", compiled->warning_stderr};
}

CommandResult run_command(const std::vector<std::string>& args) {
    CommandResult parse_error{0, "", ""};
    const std::optional<RunArgs> parsed = parse_run_like_args(args, parse_error, run_usage(), true);
    if (!parsed) {
        return parse_error;
    }
    CommandResult compile_error;
    std::optional<CompiledC> compiled = compile_file_to_c(parsed->source_path, false, parsed->warning_mode, compile_error);
    if (!compiled) {
        return compile_error;
    }
    const std::filesystem::path dir = temp_dir("walk-run");
    const std::string stem = std::filesystem::path(parsed->source_path).stem().empty() ? "program" : std::filesystem::path(parsed->source_path).stem().string();
    const std::filesystem::path exe = dir / stem;
    Result<void> built = build_c(compiled->c_code, (dir / (stem + ".c")).string(), exe.string(), parsed->native);
    if (!built.ok()) {
        std::filesystem::remove_all(dir);
        return {kDiagnosticExitCode, "", format_simple_error("W5003", built.error())};
    }
    if (const int code = run_passthrough(exe.string(), parsed->program_args); code != 0) {
        std::filesystem::remove_all(dir);
        return {kDiagnosticExitCode, "", compiled->warning_stderr + format_simple_error("W5004", process_failure_message("program", code))};
    }
    std::filesystem::remove_all(dir);
    return {0, "", compiled->warning_stderr};
}

CommandResult test_command(const std::vector<std::string>& args) {
    if (use_project_check_like(args)) {
        return project_test_command(args);
    }
    CommandResult parse_error{0, "", ""};
    const std::optional<RunArgs> parsed = parse_run_like_args(args, parse_error, test_usage());
    if (!parsed) {
        return parse_error;
    }
    CommandResult compile_error;
    std::optional<CompiledC> compiled = compile_file_to_c(parsed->source_path, true, parsed->warning_mode, compile_error);
    if (!compiled) {
        return compile_error;
    }
    const std::filesystem::path dir = temp_dir("walk-test");
    const std::filesystem::path exe = dir / "tests";
    Result<void> built = build_c(compiled->c_code, (dir / "tests.c").string(), exe.string(), parsed->native);
    if (!built.ok()) {
        std::filesystem::remove_all(dir);
        return {kDiagnosticExitCode, "", format_simple_error("W5003", built.error())};
    }
    if (const int code = run_passthrough(exe.string()); code != 0) {
        std::filesystem::remove_all(dir);
        return {kDiagnosticExitCode, "", compiled->warning_stderr + format_simple_error("W5005", process_failure_message("tests", code))};
    }
    std::filesystem::remove_all(dir);
    return {0, "", compiled->warning_stderr};
}

CommandResult check_command(const std::vector<std::string>& args) {
    if (use_project_check_like(args)) {
        return project_check_command(args);
    }
    CommandResult parse_error{0, "", ""};
    const std::optional<CheckArgs> parsed = parse_check_args(args, parse_error);
    if (!parsed) {
        return parse_error;
    }

    if (parsed->parse_only) {
        Result<SourceFile> loaded = SourceFile::load(parsed->source_path);
        if (!loaded.ok()) {
            return {kDiagnosticExitCode, "", format_simple_error("W0006", loaded.error())};
        }
        SourceFile source = loaded.take_value();
        parse::ParseResult parsed_source = parse::parse_source(source);
        if (!parsed_source.ok()) {
            return {kDiagnosticExitCode, "", parsed_source.diagnostics.format(&source)};
        }
        return {0, "", ""};
    }

    CommandResult check_error;
    std::optional<std::string> warnings = check_file_with_search_dirs(parsed->source_path, parsed->warning_mode, check_error);
    if (!warnings) {
        return check_error;
    }
    return {0, "ok\n", *warnings};
}

CommandResult docs_command(const std::vector<std::string>& args) {
    CommandResult parse_error{0, "", ""};
    const std::optional<DocsArgs> parsed = parse_docs_args(args, parse_error);
    if (!parsed) {
        return parse_error;
    }
    Result<std::filesystem::path> source_path = source_path_or_project_entry(parsed->source_path);
    if (!source_path.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W8001", source_path.error())};
    }
    Result<docs::ToolingAnalysis> analysis = docs::analyze_file(source_path.value().string(), tooling_search_dirs_for_source(source_path.value()));
    if (!analysis.ok()) {
        return {kDiagnosticExitCode, "", analysis.error()};
    }
    docs::DocsIndex index = docs::generate_index(source_path.value().string(), analysis.value());
    if (parsed->strict) {
        Result<void> valid = docs::validate_index(index);
        if (!valid.ok()) {
            return {kDiagnosticExitCode, "", format_simple_error("W8002", valid.error())};
        }
    }
    Result<std::string> rendered = docs::render_index(index, parsed->format);
    if (!rendered.ok()) {
        return {kUsageExitCode, "", format_simple_error("W0005", rendered.error())};
    }
    if (parsed->output_path.empty()) {
        return {0, rendered.value(), ""};
    }
    Result<void> written = write_file(parsed->output_path, rendered.value());
    if (!written.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W8003", written.error())};
    }
    return {0, "", ""};
}

CommandResult debug_map_command(const std::vector<std::string>& args) {
    CommandResult parse_error{0, "", ""};
    const std::optional<DebugMapArgs> parsed = parse_debug_map_args(args, parse_error);
    if (!parsed) {
        return parse_error;
    }
    Result<std::filesystem::path> source_path = source_path_or_project_entry(parsed->source_path);
    if (!source_path.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W8101", source_path.error())};
    }
    Result<docs::ToolingAnalysis> analysis = docs::analyze_file(source_path.value().string(), tooling_search_dirs_for_source(source_path.value()));
    if (!analysis.ok()) {
        return {kDiagnosticExitCode, "", analysis.error()};
    }
    const std::string rendered = debug_map::render_debug_map_json(source_path.value().string(), analysis.value());
    if (parsed->output_path.empty()) {
        return {0, rendered, ""};
    }
    Result<void> written = write_file(parsed->output_path, rendered);
    if (!written.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W8102", written.error())};
    }
    return {0, "", ""};
}

CommandResult sitegen_command(const std::vector<std::string>& args) {
    CommandResult parse_error{0, "", ""};
    const std::optional<SitegenArgs> parsed = parse_sitegen_args(args, parse_error);
    if (!parsed) {
        return parse_error;
    }
    Result<void> built = docs::build_site(parsed->docs_dir, parsed->public_dir);
    if (!built.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W8201", built.error())};
    }
    return {0, "", ""};
}

}  // namespace

std::vector<CommandInfo> command_table() {
    return {
        {"version", "print compiler version", true},
        {"help", "show this help", true},
        {"check", "semantic check", true},
        {"emit-c", "emit deterministic C", true},
        {"run", "compile and run a source file", true},
        {"build", "compile a native executable", true},
        {"test", "compile and run source tests", true},
        {"fmt", "format WalkLang source", true},
        {"clean", "remove project build outputs", true},
        {"init", "create a WalkLang project", true},
        {"package", "manage local packages", true},
        {"docs", "generate API reference docs", true},
        {"debug-map", "emit source symbol map JSON", true},
        {"lsp", "run the stdio language server", true},
        {"repl", "start the expression REPL", true},
        {"sitegen", "generate the static docs site", true},
    };
}

std::string help_text() {
    std::ostringstream output;
    output << "WalkLang C++ compiler\n";
    output << "usage: walk <command> [args]\n\n";
    output << "commands:\n";
    for (const CommandInfo& command : command_table()) {
        output << "  " << command.name;
        if (command.name.size() < 10) {
            output << std::string(10 - command.name.size(), ' ');
        } else {
            output << " ";
        }
        output << command.summary << "\n";
    }
    output << "\nPhase 9 ports formatter, docs, debug-map, LSP, REPL, and static docs-site generation to the C++ toolchain.\n";
    return output.str();
}

CommandResult dispatch(const std::vector<std::string>& args) {
    if (args.empty()) {
        return {kUsageExitCode, "", help_text()};
    }

    const std::string& command_name = args.front();
    if (is_help_alias(command_name)) {
        if (args.size() != 1) {
            return {kUsageExitCode, "", format_simple_error("W0002", "usage: walk help")};
        }
        return {0, help_text(), ""};
    }
    if (is_version_alias(command_name)) {
        if (args.size() != 1) {
            return {kUsageExitCode, "", format_simple_error("W0003", "usage: walk version")};
        }
        return {0, std::string(kWalkVersion) + "\n", ""};
    }
    if (std::filesystem::path(command_name).extension() == ".walk") {
        return run_command(args);
    }

    const CommandInfo* command = find_command(command_name);
    if (command == nullptr) {
        return {kUsageExitCode, "", format_simple_error("W0004", "unknown command \"" + command_name + "\"")};
    }
    if (command_name == "check") {
        return check_command(std::vector<std::string>(args.begin() + 1, args.end()));
    }
    if (command_name == "emit-c") {
        return emit_c_command(std::vector<std::string>(args.begin() + 1, args.end()));
    }
    if (command_name == "build") {
        return build_command(std::vector<std::string>(args.begin() + 1, args.end()));
    }
    if (command_name == "run") {
        return run_command(std::vector<std::string>(args.begin() + 1, args.end()));
    }
    if (command_name == "test") {
        return test_command(std::vector<std::string>(args.begin() + 1, args.end()));
    }
    if (command_name == "fmt") {
        return fmt_command(std::vector<std::string>(args.begin() + 1, args.end()));
    }
    if (command_name == "clean") {
        return clean_command(std::vector<std::string>(args.begin() + 1, args.end()));
    }
    if (command_name == "init") {
        return init_command(std::vector<std::string>(args.begin() + 1, args.end()));
    }
    if (command_name == "package") {
        return package_command(std::vector<std::string>(args.begin() + 1, args.end()));
    }
    if (command_name == "docs") {
        return docs_command(std::vector<std::string>(args.begin() + 1, args.end()));
    }
    if (command_name == "debug-map") {
        return debug_map_command(std::vector<std::string>(args.begin() + 1, args.end()));
    }
    if (command_name == "sitegen") {
        return sitegen_command(std::vector<std::string>(args.begin() + 1, args.end()));
    }
    if (command_name == "lsp") {
        if (args.size() != 1) {
            return {kUsageExitCode, "", format_simple_error("W0005", "usage: walk lsp")};
        }
        return {0, "", ""};
    }
    if (command_name == "repl") {
        if (args.size() != 1) {
            return {kUsageExitCode, "", format_simple_error("W0005", "usage: walk repl")};
        }
        return {0, "", ""};
    }
    if (!command->ported) {
        return {kDiagnosticExitCode, "", format_simple_error("W0001", "command \"" + command_name + "\" is not ported in this phase")};
    }

    return {0, "", ""};
}

int run_repl(std::istream& input, std::ostream& output, std::ostream& error) {
    std::string line;
    for (;;) {
        output << "walk> ";
        output.flush();
        if (!std::getline(input, line)) {
            return input.eof() ? 0 : 1;
        }
        const std::string trimmed = [&] {
            const std::size_t start = line.find_first_not_of(" \t\r\n");
            if (start == std::string::npos) {
                return std::string();
            }
            const std::size_t end = line.find_last_not_of(" \t\r\n");
            return line.substr(start, end - start + 1);
        }();
        if (trimmed.empty()) {
            continue;
        }
        if (trimmed == ":quit" || trimmed == ":exit") {
            return 0;
        }

        const std::filesystem::path dir = temp_dir("walk-repl");
        const std::filesystem::path source_path = dir / "repl.walk";
        const std::filesystem::path exe_path = dir / "repl";
        const std::filesystem::path stdout_path = dir / "stdout.txt";
        const std::filesystem::path stderr_path = dir / "stderr.txt";
        Result<void> source_written = write_file(source_path.string(), repl::source_for_expression(trimmed));
        if (!source_written.ok()) {
            error << format_simple_error("W8301", source_written.error());
            std::filesystem::remove_all(dir);
            continue;
        }
        CommandResult compile_error;
        std::optional<CompiledC> compiled = compile_file_to_c(source_path.string(), false, WarningMode::Default, compile_error);
        if (!compiled) {
            error << compile_error.stderr_text;
            std::filesystem::remove_all(dir);
            continue;
        }
        Result<void> built = build_c(compiled->c_code, (dir / "repl.c").string(), exe_path.string(), {});
        if (!built.ok()) {
            error << format_simple_error("W5003", built.error());
            std::filesystem::remove_all(dir);
            continue;
        }
        if (run_capture(exe_path.string(), stdout_path, stderr_path) != 0) {
            error << read_text_file(stderr_path);
            std::filesystem::remove_all(dir);
            continue;
        }
        output << read_text_file(stdout_path);
        std::filesystem::remove_all(dir);
    }
}

}  // namespace walk::cli
