#include "cli/command.h"

#include "support/diagnostic.h"
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

}  // namespace

std::vector<CommandInfo> command_table() {
    return {
        {"version", "print compiler version", true},
        {"help", "show this help", true},
        {"check", "not ported in this phase", false},
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
    output << "WalkLang C++ compiler skeleton\n";
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
    output << "\nOnly version and help are ported in Phase 3.\n";
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
    if (!command->ported) {
        return {kDiagnosticExitCode, "", format_simple_error("W0001", "command \"" + command_name + "\" is not ported in this phase")};
    }

    return {0, "", ""};
}

}  // namespace walk::cli
