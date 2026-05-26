#include "cli/command.h"

#include "codegen/c/c_emitter.h"
#include "ir/lower.h"
#include "parse/parser.h"
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
};

struct CompiledC {
    std::string c_code;
    std::string warning_stderr;
};

struct ProjectConfig {
    std::filesystem::path root;
    std::string entry = "src/main.walk";
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
    return "usage: walk-cpp check [--parse-only] [--warnings=off|default|error] <source.walk>";
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
    return "usage: walk-cpp emit-c [--warnings=off|default|error] <source.walk> -o <output.c>";
}

std::string build_usage() {
    return "usage: walk-cpp build [--mode debug|release] [--warnings=off|default|error] [--cc <cc>] [--cflag <flag>] <source.walk> -o <output>";
}

std::string run_usage() {
    return "usage: walk-cpp run [--mode debug|release] [--warnings=off|default|error] [--cc <cc>] [--cflag <flag>] <source.walk>";
}

std::string test_usage() {
    return "usage: walk-cpp test [--warnings=off|default|error] [--cc <cc>] [--cflag <flag>] <source.walk>";
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

std::optional<RunArgs> parse_run_like_args(const std::vector<std::string>& args, CommandResult& error, const std::string& usage) {
    RunArgs parsed;
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& arg = args[index];
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

std::optional<WarningMode> parse_project_test_args(const std::vector<std::string>& args, CommandResult& error) {
    WarningMode mode = WarningMode::Default;
    const std::string usage = "usage: walk-cpp test [--warnings=off|default|error]";
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

std::string trim(std::string value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (begin >= end) {
        return "";
    }
    return std::string(begin, end);
}

std::string strip_toml_comment(const std::string& value) {
    const std::size_t index = value.find('#');
    if (index == std::string::npos) {
        return value;
    }
    return value.substr(0, index);
}

std::string unquote(std::string value) {
    value = trim(std::move(value));
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

Result<ProjectConfig> load_project_config_from_cwd() {
    std::filesystem::path dir = std::filesystem::current_path();
    for (;;) {
        const std::filesystem::path config_path = dir / "walk.toml";
        if (std::filesystem::is_regular_file(config_path)) {
            ProjectConfig config;
            config.root = dir;
            std::ifstream input(config_path, std::ios::binary);
            if (!input) {
                return Result<ProjectConfig>::failure("could not read " + config_path.string());
            }
            std::string section;
            std::string line;
            while (std::getline(input, line)) {
                line = trim(strip_toml_comment(line));
                if (line.empty()) {
                    continue;
                }
                if (line.front() == '[' && line.back() == ']') {
                    section = trim(line.substr(1, line.size() - 2));
                    continue;
                }
                const std::size_t equals = line.find('=');
                if (equals == std::string::npos) {
                    continue;
                }
                const std::string key = trim(line.substr(0, equals));
                const std::string value = unquote(line.substr(equals + 1));
                if (section.empty() && key == "entry") {
                    config.entry = value;
                }
            }
            return Result<ProjectConfig>::success(std::move(config));
        }
        const std::filesystem::path parent = dir.parent_path();
        if (parent == dir || parent.empty()) {
            return Result<ProjectConfig>::failure("walk.toml not found; run walk init <project-name> or pass a .walk source file");
        }
        dir = parent;
    }
}

std::vector<std::filesystem::path> project_test_files(const ProjectConfig& config) {
    std::vector<std::filesystem::path> files;
    const std::filesystem::path tests_dir = config.root / "tests";
    if (!std::filesystem::is_directory(tests_dir)) {
        return files;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(tests_dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const std::string name = entry.path().filename().string();
        if (name.size() >= std::string("_test.walk").size() && name.substr(name.size() - std::string("_test.walk").size()) == "_test.walk") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

std::vector<std::string> project_search_dirs(const ProjectConfig& config, const std::filesystem::path& source_path) {
    std::vector<std::string> dirs;
    dirs.push_back(source_path.parent_path().string());
    dirs.push_back((config.root / std::filesystem::path(config.entry).parent_path()).string());
    dirs.push_back((config.root / "tests").string());
    std::vector<std::string> result;
    std::set<std::string> seen;
    for (const std::string& dir : dirs) {
        if (!dir.empty() && seen.insert(dir).second) {
            result.push_back(dir);
        }
    }
    return result;
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
    const std::filesystem::path log = temp_dir("walk-cpp-cc") / "cc.log";
    command << " > " << shell_quote(log.string()) << " 2>&1";
    const int code = std::system(command.str().c_str());
    if (code != 0) {
        return Result<void>::failure("native build failed: " + read_text_file(log));
    }
    std::filesystem::remove_all(log.parent_path());
    return Result<void>::success();
}

int run_passthrough(const std::string& executable) {
    return std::system(shell_quote(executable).c_str());
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
    const std::optional<RunArgs> parsed = parse_run_like_args(args, parse_error, run_usage());
    if (!parsed) {
        return parse_error;
    }
    CommandResult compile_error;
    std::optional<CompiledC> compiled = compile_file_to_c(parsed->source_path, false, parsed->warning_mode, compile_error);
    if (!compiled) {
        return compile_error;
    }
    const std::filesystem::path dir = temp_dir("walk-cpp-run");
    const std::string stem = std::filesystem::path(parsed->source_path).stem().empty() ? "program" : std::filesystem::path(parsed->source_path).stem().string();
    const std::filesystem::path exe = dir / stem;
    Result<void> built = build_c(compiled->c_code, (dir / (stem + ".c")).string(), exe.string(), parsed->native);
    if (!built.ok()) {
        std::filesystem::remove_all(dir);
        return {kDiagnosticExitCode, "", format_simple_error("W5003", built.error())};
    }
    if (const int code = run_passthrough(exe.string()); code != 0) {
        std::filesystem::remove_all(dir);
        return {kDiagnosticExitCode, "", compiled->warning_stderr + format_simple_error("W5004", "program failed")};
    }
    std::filesystem::remove_all(dir);
    return {0, "", compiled->warning_stderr};
}

CommandResult test_command(const std::vector<std::string>& args) {
    CommandResult parse_error{0, "", ""};
    if (std::optional<WarningMode> project_mode = parse_project_test_args(args, parse_error)) {
        Result<ProjectConfig> config = load_project_config_from_cwd();
        if (config.ok()) {
            const std::vector<std::filesystem::path> tests = project_test_files(config.value());
            if (tests.empty()) {
                return {0, "ok 0 test files\n", ""};
            }
            std::string warning_stderr;
            for (const std::filesystem::path& test_path : tests) {
                CommandResult compile_error;
                std::optional<CompiledC> compiled =
                    compile_file_to_c(test_path.string(), true, *project_mode, compile_error, project_search_dirs(config.value(), test_path));
                if (!compiled) {
                    return compile_error;
                }
                warning_stderr += compiled->warning_stderr;
                const std::filesystem::path dir = temp_dir("walk-cpp-project-test");
                const std::filesystem::path exe = dir / "tests";
                Result<void> built = build_c(compiled->c_code, (dir / "tests.c").string(), exe.string(), {});
                if (!built.ok()) {
                    std::filesystem::remove_all(dir);
                    return {kDiagnosticExitCode, "", format_simple_error("W5003", built.error())};
                }
                if (const int code = run_passthrough(exe.string()); code != 0) {
                    std::filesystem::remove_all(dir);
                    return {kDiagnosticExitCode, "", warning_stderr + format_simple_error("W5005", "tests failed")};
                }
                std::filesystem::remove_all(dir);
            }
            return {0, "", warning_stderr};
        }
    }
    parse_error = {0, "", ""};
    const std::optional<RunArgs> parsed = parse_run_like_args(args, parse_error, test_usage());
    if (!parsed) {
        return parse_error;
    }
    CommandResult compile_error;
    std::optional<CompiledC> compiled = compile_file_to_c(parsed->source_path, true, parsed->warning_mode, compile_error);
    if (!compiled) {
        return compile_error;
    }
    const std::filesystem::path dir = temp_dir("walk-cpp-test");
    const std::filesystem::path exe = dir / "tests";
    Result<void> built = build_c(compiled->c_code, (dir / "tests.c").string(), exe.string(), parsed->native);
    if (!built.ok()) {
        std::filesystem::remove_all(dir);
        return {kDiagnosticExitCode, "", format_simple_error("W5003", built.error())};
    }
    if (const int code = run_passthrough(exe.string()); code != 0) {
        std::filesystem::remove_all(dir);
        return {kDiagnosticExitCode, "", compiled->warning_stderr + format_simple_error("W5005", "tests failed")};
    }
    std::filesystem::remove_all(dir);
    return {0, "", compiled->warning_stderr};
}

CommandResult parse_only_check(const std::vector<std::string>& args) {
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

    sema::ProgramBundle bundle = sema::load_program_with_modules(parsed->source_path);
    if (!bundle.ok()) {
        return {kDiagnosticExitCode, "", bundle.diagnostics.format(bundle.source.get())};
    }
    sema::CheckResult checked = sema::check_programs(*bundle.parsed.program, bundle.modules);
    if (!checked.ok()) {
        return {kDiagnosticExitCode, "", checked.diagnostics.format(bundle.source.get())};
    }
    CommandResult warning_result = handle_warnings(checked.warnings, parsed->warning_mode);
    if (warning_result.exit_code != 0) {
        return warning_result;
    }
    return {0, "ok\n", warning_result.stderr_text};
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
        {"fmt", "not ported in this phase", false},
        {"clean", "not ported in this phase", false},
        {"init", "not ported in this phase", false},
        {"package", "not ported in this phase", false},
        {"docs", "not ported in this phase", false},
        {"debug-map", "not ported in this phase", false},
        {"lsp", "not ported in this phase", false},
        {"repl", "not ported in this phase", false},
    };
}

std::string help_text() {
    std::ostringstream output;
    output << "WalkLang C++ compiler\n";
    output << "usage: walk-cpp <command> [args]\n\n";
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
    output << "\nPhase 6 ports check, emit-c, build, run, and test; project and tooling commands remain staged for later phases.\n";
    return output.str();
}

CommandResult dispatch(const std::vector<std::string>& args) {
    if (args.empty()) {
        return {kUsageExitCode, "", help_text()};
    }

    const std::string& command_name = args.front();
    if (is_help_alias(command_name)) {
        if (args.size() != 1) {
            return {kUsageExitCode, "", format_simple_error("W0002", "usage: walk-cpp help")};
        }
        return {0, help_text(), ""};
    }
    if (is_version_alias(command_name)) {
        if (args.size() != 1) {
            return {kUsageExitCode, "", format_simple_error("W0003", "usage: walk-cpp version")};
        }
        return {0, std::string(kWalkVersion) + "\n", ""};
    }

    const CommandInfo* command = find_command(command_name);
    if (command == nullptr) {
        return {kUsageExitCode, "", format_simple_error("W0004", "unknown command \"" + command_name + "\"")};
    }
    if (command_name == "check") {
        return parse_only_check(std::vector<std::string>(args.begin() + 1, args.end()));
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
    if (!command->ported) {
        return {kDiagnosticExitCode, "", format_simple_error("W0001", "command \"" + command_name + "\" is not ported in this phase")};
    }

    return {0, "", ""};
}

}  // namespace walk::cli
