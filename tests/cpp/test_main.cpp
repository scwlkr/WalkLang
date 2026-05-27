#include "cli/command.h"
#include "support/diagnostic.h"
#include "support/source_file.h"
#include "support/version.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void fail(const std::string& name, const std::string& message) {
    std::cerr << "FAIL " << name << ": " << message << "\n";
    ++failures;
}

void expect_true(const std::string& name, bool value, const std::string& message) {
    if (!value) {
        fail(name, message);
    }
}

void expect_eq(const std::string& name, const std::string& got, const std::string& want) {
    if (got != want) {
        fail(name, "want <" + want + "> got <" + got + ">");
    }
}

void expect_eq_int(const std::string& name, int got, int want) {
    if (got != want) {
        fail(name, "want " + std::to_string(want) + " got " + std::to_string(got));
    }
}

void test_version_command() {
    const walk::cli::CommandResult result = walk::cli::dispatch({"version"});
    expect_eq_int("version exit", result.exit_code, 0);
    expect_eq("version stdout", result.stdout_text, std::string(walk::kWalkVersion) + "\n");
    expect_eq("version stderr", result.stderr_text, "");
}

void test_help_command_lists_phase_commands() {
    const walk::cli::CommandResult result = walk::cli::dispatch({"help"});
    expect_eq_int("help exit", result.exit_code, 0);
    expect_true("help header", result.stdout_text.find("WalkLang C++ compiler") != std::string::npos, "missing compiler header");
    for (const char* command : {"version", "help", "check", "emit-c", "run", "build", "test", "fmt", "clean", "init", "package", "docs", "debug-map", "lsp", "repl"}) {
        expect_true(std::string("help command ") + command, result.stdout_text.find(command) != std::string::npos, "missing command");
    }
    expect_eq("help stderr", result.stderr_text, "");
}

void test_unported_command_is_diagnostic_not_delegation() {
    const walk::cli::CommandResult result = walk::cli::dispatch({"check", "examples/hello.walk"});
    expect_eq_int("check exit", result.exit_code, 0);
    expect_eq("check stdout", result.stdout_text, "ok\n");
    expect_eq("check stderr", result.stderr_text, "");
}

void test_check_warnings_error_mode() {
    const std::filesystem::path dir = std::filesystem::path("build") / "cpp-tests";
    std::filesystem::create_directories(dir);
    const std::filesystem::path path = dir / "warning.walk";
    {
        std::ofstream output(path, std::ios::binary);
        output << "var: x = 1\n";
        output << "if: true\n";
        output << "    var: x = 2\n";
        output << "    out: x\n";
    }

    const walk::cli::CommandResult result = walk::cli::dispatch({"check", "--warnings=error", path.string()});
    expect_eq_int("warnings error exit", result.exit_code, 1);
    expect_eq("warnings error stdout", result.stdout_text, "");
    expect_true("warnings error stderr", result.stderr_text.find("warning: x shadows outer name") != std::string::npos, "missing warning");
    expect_true("warnings error summary", result.stderr_text.find("warnings-as-errors: 1 warning(s)") != std::string::npos, "missing warning summary");
}

void test_unknown_command_is_usage_error() {
    const walk::cli::CommandResult result = walk::cli::dispatch({"unknown"});
    expect_eq_int("unknown exit", result.exit_code, 2);
    expect_eq("unknown stdout", result.stdout_text, "");
    expect_eq("unknown stderr", result.stderr_text, "error[W0004]: unknown command \"unknown\"\n");
}

void test_run_command_passes_program_args_after_delimiter() {
    const std::filesystem::path dir = std::filesystem::path("build") / "cpp-tests";
    std::filesystem::create_directories(dir);
    const std::filesystem::path path = dir / "run_args.walk";
    {
        std::ofstream output(path, std::ios::binary);
        output << "imp: process\n";
        output << "var: args = process.args()\n";
        output << "if: != process.arg_count() 2\n";
        output << "    do: process.exit(1)\n";
        output << "if: != args[0] 'alpha'\n";
        output << "    do: process.exit(2)\n";
        output << "if: != args[1] 'two words'\n";
        output << "    do: process.exit(3)\n";
    }

    const walk::cli::CommandResult result = walk::cli::dispatch({"run", "--warnings=error", path.string(), "--", "alpha", "two words"});
    expect_eq_int("run args exit", result.exit_code, 0);
    expect_eq("run args stdout", result.stdout_text, "");
    expect_eq("run args stderr", result.stderr_text, "");
}

void test_source_file_loading_and_positions() {
    const std::filesystem::path dir = std::filesystem::path("build") / "cpp-tests";
    std::filesystem::create_directories(dir);
    const std::filesystem::path path = dir / "source_file.walk";
    {
        std::ofstream output(path, std::ios::binary);
        output << "one\n";
        output << "two three\n";
    }

    walk::Result<walk::SourceFile> loaded = walk::SourceFile::load(path.string());
    expect_true("source load ok", loaded.ok(), loaded.error());
    if (!loaded.ok()) {
        return;
    }
    const walk::SourceFile source = loaded.take_value();
    expect_eq("source text", std::string(source.text()), "one\ntwo three\n");
    expect_eq("source line", source.line_text(2), "two three");
    const walk::SourcePosition position = source.position_for_offset(5);
    expect_eq_int("source line position", static_cast<int>(position.line), 2);
    expect_eq_int("source column position", static_cast<int>(position.column), 2);
}

void test_diagnostic_formatting_with_source() {
    const walk::SourceFile source = walk::SourceFile::from_text("main.walk", "one\ntwo three\n");
    const walk::SourceRange range = source.range_for_offsets(4, 7);
    const walk::Diagnostic diagnostic(walk::DiagnosticSeverity::Error, "W1234", "sample message", range);
    expect_eq(
        "diagnostic source format",
        walk::format_diagnostic(diagnostic, &source),
        "main.walk:2:1: error[W1234]: sample message\n\ntwo three\n^");
}

void test_diagnostic_set_sorts_deterministically() {
    const walk::SourceFile source = walk::SourceFile::from_text("main.walk", "first\nsecond\n");
    walk::DiagnosticSet diagnostics;
    diagnostics.add(walk::Diagnostic(walk::DiagnosticSeverity::Warning, "W2000", "second", source.range_for_offsets(6, 12)));
    diagnostics.add(walk::Diagnostic(walk::DiagnosticSeverity::Error, "W1000", "first", source.range_for_offsets(0, 5)));
    diagnostics.sort();
    expect_eq(
        "diagnostic set format",
        diagnostics.format(&source),
        "main.walk:1:1: error[W1000]: first\n\nfirst\n^\nmain.walk:2:1: warning: second\n\nsecond\n^\n");
}

}  // namespace

int run_lexer_tests();
int run_parser_tests();
int run_checker_tests();
int run_module_tests();
int run_emitter_tests();
int run_project_tests();
int run_package_tests();
int run_format_tests();
int run_docs_tests();
int run_lsp_tests();
int run_repl_tests();

int main() {
    test_version_command();
    test_help_command_lists_phase_commands();
    test_unported_command_is_diagnostic_not_delegation();
    test_check_warnings_error_mode();
    test_unknown_command_is_usage_error();
    test_run_command_passes_program_args_after_delimiter();
    test_source_file_loading_and_positions();
    test_diagnostic_formatting_with_source();
    test_diagnostic_set_sorts_deterministically();
    failures += run_lexer_tests();
    failures += run_parser_tests();
    failures += run_checker_tests();
    failures += run_module_tests();
    failures += run_emitter_tests();
    failures += run_project_tests();
    failures += run_package_tests();
    failures += run_format_tests();
    failures += run_docs_tests();
    failures += run_lsp_tests();
    failures += run_repl_tests();

    if (failures != 0) {
        std::cerr << failures << " C++ compiler test(s) failed\n";
        return 1;
    }
    std::cout << "C++ compiler tests passed\n";
    return 0;
}
