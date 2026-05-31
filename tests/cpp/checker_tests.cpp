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

void test_accepts_new_string_helpers() {
    walk::sema::CheckResult result = check_text(
        "imp: string\n"
        "var: lowered = string.lower('Hi')\n"
        "var: parts = string.split('a b', ' ')\n"
        "var: replaced = string.replace('abc', 'b', 'x')\n"
        "var: sliced = string.slice('walklang', 4, 99)\n"
        "var: prefixed = string.prefix('walklang', 4)\n"
        "out: lowered\n"
        "out: parts[1]\n"
        "out: replaced\n"
        "out: sliced\n"
        "out: prefixed\n");
    expect_true("new string helpers ok", result.ok(), result.diagnostics.format());
}

void test_accepts_string_array_maps() {
    walk::sema::CheckResult result = check_text(
        "imp: array\n"
        "imp: map\n"
        "var: table map[string]array[string] = []\n"
        "table = map.push(table, 'of the', 'people')\n"
        "table = map.push(table, 'of the', 'walk')\n"
        "var: values = table['of the']\n"
        "var: keys = map.keys(table)\n"
        "out: map.has(table, 'of the')\n"
        "out: array.len(values)\n"
        "out: map.get(table, 'of the')[1]\n"
        "out: keys[0]\n");
    expect_true("string array maps ok", result.ok(), result.diagnostics.format());
}

void test_accepts_numeric_ml_helpers() {
    walk::sema::CheckResult result = check_text(
        "imp: math\n"
        "imp: random\n"
        "var: e = math.exp(0)\n"
        "var: l = math.log(1)\n"
        "var: r = math.remainder(35, 11)\n"
        "var: f = random.float(0, 1)\n"
        "out: e\n"
        "out: l\n"
        "out: r\n"
        "out: f\n");
    expect_true("numeric ml helpers ok", result.ok(), result.diagnostics.format());
}

void test_rejects_unsupported_map_key_type() {
    walk::sema::CheckResult result = check_text(
        "imp: map\n"
        "var: table map[int]array[string] = []\n"
        "out: map.has(table, 1)\n");
    expect_true("unsupported map key rejects", !result.ok(), "unsupported map key unexpectedly passed");
    expect_true("unsupported map key diagnostic", result.diagnostics.format().find("type error: map key type must be string") != std::string::npos, "missing map key diagnostic");
}

void test_rejects_invalid_numeric_ml_helper_args() {
    walk::sema::CheckResult exp_result = check_text(
        "imp: math\n"
        "out: math.exp('x')\n");
    expect_true("bad math exp rejects", !exp_result.ok(), "bad math exp unexpectedly passed");
    expect_true("bad math exp diagnostic", exp_result.diagnostics.format().find("type error: math.exp needs numeric arg, got string") != std::string::npos, "missing math.exp diagnostic");

    walk::sema::CheckResult log_result = check_text(
        "imp: math\n"
        "out: math.log('x')\n");
    expect_true("bad math log rejects", !log_result.ok(), "bad math log unexpectedly passed");
    expect_true("bad math log diagnostic", log_result.diagnostics.format().find("type error: math.log needs numeric arg, got string") != std::string::npos, "missing math.log diagnostic");

    walk::sema::CheckResult remainder_result = check_text(
        "imp: math\n"
        "out: math.remainder(9.5, 2)\n");
    expect_true("bad math remainder rejects", !remainder_result.ok(), "bad math remainder unexpectedly passed");
    expect_true(
        "bad math remainder diagnostic",
        remainder_result.diagnostics.format().find("type error: math.remainder args must be int, got float") != std::string::npos,
        "missing math.remainder diagnostic");

    walk::sema::CheckResult float_result = check_text(
        "imp: random\n"
        "out: random.float('low', 1)\n");
    expect_true("bad random float rejects", !float_result.ok(), "bad random float unexpectedly passed");
    expect_true("bad random float diagnostic", float_result.diagnostics.format().find("type error: random.float args must be numeric, got string") != std::string::npos, "missing random.float diagnostic");
}

}  // namespace

int run_checker_tests() {
    failures = 0;
    test_shadowing_produces_warning();
    test_rejects_type_mismatch();
    test_accepts_new_string_helpers();
    test_accepts_string_array_maps();
    test_accepts_numeric_ml_helpers();
    test_rejects_unsupported_map_key_type();
    test_rejects_invalid_numeric_ml_helper_args();
    return failures;
}
