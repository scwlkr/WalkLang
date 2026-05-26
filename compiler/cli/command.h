#pragma once

#include <string>
#include <vector>

namespace walk::cli {

struct CommandInfo {
    std::string name;
    std::string summary;
    bool ported;
};

struct CommandResult {
    int exit_code;
    std::string stdout_text;
    std::string stderr_text;
};

std::vector<CommandInfo> command_table();
std::string help_text();
CommandResult dispatch(const std::vector<std::string>& args);

}  // namespace walk::cli
