#include "format/format.h"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect_eq(const std::string& name, const std::string& got, const std::string& want) {
    if (got != want) {
        std::cerr << "FAIL " << name << ": want <" << want << "> got <" << got << ">\n";
        ++failures;
    }
}

void test_formatter_normalizes_current_surface() {
    const walk::Result<std::string> formatted = walk::format::format_source(
        "struct:User\n  name string\n  age int\nvar:user=User('Walker',25)\nout:user.is_adult()\n",
        "main.walk");
    if (!formatted.ok()) {
        std::cerr << "FAIL formatter result: " << formatted.error() << "\n";
        ++failures;
        return;
    }
    expect_eq(
        "formatter current surface",
        formatted.value(),
        "struct: User\n    name string\n    age int\nvar: user = User('Walker', 25)\nout: user.is_adult()\n");
}

void test_formatter_preserves_interpolation_and_effects() {
    const walk::Result<std::string> formatted = walk::format::format_source(
        "out:'value {+ 1 2}'\ndo:io.write('Loading')\ndefer:do term.reset()\n",
        "main.walk");
    if (!formatted.ok()) {
        std::cerr << "FAIL formatter interpolation: " << formatted.error() << "\n";
        ++failures;
        return;
    }
    expect_eq(
        "formatter interpolation effects",
        formatted.value(),
        "out: 'value {+ 1 2}'\ndo: io.write('Loading')\ndefer: do term.reset()\n");
}

void test_formatter_distinguishes_quoted_delimiters() {
    const auto formatted = walk::format::format_source(
        "out:')'\nvar:signs=['+',')','(',',',']']\nout:identity('.')\n", "quoted.walk");
    if (!formatted.ok()) {
        std::cerr << "FAIL formatter quoted syntax: " << formatted.error() << "\n";
        ++failures;
        return;
    }
    expect_eq("formatter quoted syntax", formatted.value(),
        "out: ')'\nvar: signs = ['+', ')', '(', ',', ']']\nout: identity('.')\n");
    const auto twice = walk::format::format_source(formatted.value(), "quoted.walk");
    if (!twice.ok()) { ++failures; return; }
    expect_eq("formatter quoted syntax idempotence", twice.value(), formatted.value());
}

}  // namespace

int run_format_tests() {
    failures = 0;
    test_formatter_normalizes_current_surface();
    test_formatter_preserves_interpolation_and_effects();
    test_formatter_distinguishes_quoted_delimiters();
    return failures;
}
