#include "cli/command.h"
#include "repl/repl.h"

#include <iostream>
#include <sstream>
#include <string>

namespace {

int failures = 0;

void expect_contains(const std::string& name, const std::string& text, const std::string& needle) {
    if (text.find(needle) == std::string::npos) {
        std::cerr << "FAIL " << name << ": missing " << needle << " in " << text << "\n";
        ++failures;
    }
}

void test_repl_source_wraps_expression() {
    const std::string source = walk::repl::source_for_expression("+ 1 2");
    expect_contains("repl imports", source, "imp: math\n");
    expect_contains("repl out", source, "out: + 1 2\n");
}

void test_repl_runs_expression_until_quit() {
    std::istringstream input("+ 1 2\n:quit\n");
    std::ostringstream output;
    std::ostringstream error;
    const int code = walk::cli::run_repl(input, output, error);
    if (code != 0) {
        std::cerr << "FAIL repl exit: " << code << " " << error.str() << "\n";
        ++failures;
        return;
    }
    expect_contains("repl prompt", output.str(), "walk> ");
    expect_contains("repl output", output.str(), "3\n");
}

}  // namespace

int run_repl_tests() {
    failures = 0;
    test_repl_source_wraps_expression();
    test_repl_runs_expression_until_quit();
    return failures;
}
