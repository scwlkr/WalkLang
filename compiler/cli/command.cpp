#include "cli/command.h"

#include "parse/parser.h"
#include "support/diagnostic.h"
#include "support/source_file.h"
#include "support/version.h"

#include <algorithm>
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

CommandResult parse_only_check(const std::vector<std::string>& args) {
    bool parse_only = false;
    std::string source_path;

    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg == "--parse-only") {
            parse_only = true;
            continue;
        }
        if (arg == "--warnings=off" || arg == "--warnings=default" || arg == "--warnings=error") {
            continue;
        }
        if (arg == "--warnings") {
            if (index + 1 >= args.size() || (args[index + 1] != "off" && args[index + 1] != "default" && args[index + 1] != "error")) {
                return {kUsageExitCode, "", format_simple_error("W0005", "usage: walk-cpp check --parse-only [--warnings=off|default|error] <source.walk>")};
            }
            ++index;
            continue;
        }
        if (!arg.empty() && arg[0] == '-') {
            return {kUsageExitCode, "", format_simple_error("W0005", "usage: walk-cpp check --parse-only [--warnings=off|default|error] <source.walk>")};
        }
        if (!source_path.empty()) {
            return {kUsageExitCode, "", format_simple_error("W0005", "usage: walk-cpp check --parse-only [--warnings=off|default|error] <source.walk>")};
        }
        source_path = arg;
    }

    if (!parse_only) {
        return {kDiagnosticExitCode, "", format_simple_error("W0001", "command \"check\" is not ported in this phase without --parse-only")};
    }
    if (source_path.empty()) {
        return {kUsageExitCode, "", format_simple_error("W0005", "usage: walk-cpp check --parse-only [--warnings=off|default|error] <source.walk>")};
    }

    Result<SourceFile> loaded = SourceFile::load(source_path);
    if (!loaded.ok()) {
        return {kDiagnosticExitCode, "", format_simple_error("W0006", loaded.error())};
    }
    SourceFile source = loaded.take_value();
    parse::ParseResult parsed = parse::parse_source(source);
    if (!parsed.ok()) {
        return {kDiagnosticExitCode, "", parsed.diagnostics.format(&source)};
    }
    return {0, "", ""};
}

}  // namespace

std::vector<CommandInfo> command_table() {
    return {
        {"version", "print compiler version", true},
        {"help", "show this help", true},
        {"check", "parse-only frontend check", true},
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
    output << "\nPhase 4 ports check --parse-only; other language commands remain staged for later phases.\n";
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
