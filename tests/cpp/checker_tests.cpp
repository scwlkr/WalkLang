#include "parse/parser.h"
#include "sema/checker.h"
#include "support/source_file.h"

#include <iostream>
#include <map>
#include <memory>
#include <string>

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

walk::sema::CheckResult check_text(const std::string& text) {
    walk::SourceFile source = walk::SourceFile::from_text("checker.walk", text);
    walk::parse::ParseResult parsed = walk::parse::parse_source(source);
    if (!parsed.ok()) {
        walk::sema::CheckResult result;
        result.diagnostics = std::move(parsed.diagnostics);
        return result;
    }
    std::map<std::string, std::unique_ptr<walk::sema::Module>> modules;
    return walk::sema::check_programs(*parsed.program, modules);
}

void test_shadowing_produces_warning() {
    walk::sema::CheckResult result = check_text(
        "var: x = 1\n"
        "if: true\n"
        "    var: x = 2\n"
        "    out: x\n");
    expect_true("shadowing ok", result.ok(), result.diagnostics.format());
    expect_true("shadowing warning", result.warnings.size() == 1, "expected one warning");
    if (result.warnings.size() == 1) {
        expect_true("shadowing text", result.warnings[0].message == "x shadows outer name", "wrong warning");
    }
}

void test_rejects_type_mismatch() {
    walk::sema::CheckResult result = check_text(
        "var: x = 1\n"
        "x = 'one'\n");
    expect_true("type mismatch rejects", !result.ok(), "type mismatch unexpectedly passed");
    expect_true("type mismatch diagnostic", result.diagnostics.format().find("type error: x is int, got string") != std::string::npos, "missing type mismatch");
}

}  // namespace

int run_checker_tests() {
    failures = 0;
    test_shadowing_produces_warning();
    test_rejects_type_mismatch();
    return failures;
}
