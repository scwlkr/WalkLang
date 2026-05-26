#include "cli/command.h"

#include "parse/parser.h"
#include "sema/checker.h"
#include "sema/modules.h"
#include "support/diagnostic.h"
#include "support/source_file.h"
#include "support/version.h"

#include <algorithm>
#include <optional>
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

CommandResult parse_only_check(const std::vector<std::string>& args) {
    CommandResult parse_error;
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
        {"emit-c", "not ported in this phase", false},
        {"run", "not ported in this phase", false},
        {"build", "not ported in this phase", false},
        {"test", "not ported in this phase", false},
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
    output << "\nPhase 5 ports check; other language commands remain staged for later phases.\n";
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
    if (!command->ported) {
        return {kDiagnosticExitCode, "", format_simple_error("W0001", "command \"" + command_name + "\" is not ported in this phase")};
    }

    return {0, "", ""};
}

}  // namespace walk::cli
