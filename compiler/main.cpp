#include "cli/command.h"
#include "lsp/server.h"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    std::vector<std::string> args;
    args.reserve(argc > 1 ? static_cast<std::size_t>(argc - 1) : 0);
    for (int index = 1; index < argc; ++index) {
        args.emplace_back(argv[index]);
    }

    if (args.size() == 1 && args[0] == "lsp") {
        return walk::lsp::serve(std::cin, std::cout);
    }
    if (args.size() == 1 && args[0] == "repl") {
        return walk::cli::run_repl(std::cin, std::cout, std::cerr);
    }

    const walk::cli::CommandResult result = walk::cli::dispatch(args);
    std::cout << result.stdout_text;
    std::cerr << result.stderr_text;
    return result.exit_code;
}
