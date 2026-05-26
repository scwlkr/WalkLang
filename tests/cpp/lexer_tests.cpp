#include "lex/lexer.h"

#include <iostream>
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

void expect_eq(const std::string& name, const std::string& got, const std::string& want) {
    if (got != want) {
        fail(name, "want <" + want + "> got <" + got + ">");
    }
}

void expect_eq_size(const std::string& name, std::size_t got, std::size_t want) {
    if (got != want) {
        fail(name, "want " + std::to_string(want) + " got " + std::to_string(got));
    }
}

void test_lexes_commands_comments_and_strings() {
    const walk::SourceFile source = walk::SourceFile::from_text(
        "lexer.walk",
        "imp: math # comment\n"
        "var: text = 'value # not comment'\n"
        "out: 'score {+ 1 2}' /// trailing\n");
    const walk::lex::LexResult result = walk::lex::lex_source(source);
    expect_true("lexer ok", result.ok(), result.diagnostics.format(&source));
    if (!result.ok()) {
        return;
    }
    expect_eq_size("lexer line count", result.lines.size(), 3);
    expect_eq("lexer first token", result.lines[0].tokens[0].value, "imp");
    expect_eq("lexer import name", result.lines[0].tokens[2].value, "math");
    expect_eq("lexer string keeps comment marker", result.lines[1].tokens[4].value, "value # not comment");
    expect_eq("lexer interpolation string", result.lines[2].tokens[2].value, "score {+ 1 2}");
}

void test_lexes_indentation_and_negative_numbers() {
    const walk::SourceFile source = walk::SourceFile::from_text(
        "lexer.walk",
        "while: < count 2\n"
        "    var: next = -1\n"
        "    out: - count 1\n");
    const walk::lex::LexResult result = walk::lex::lex_source(source);
    expect_true("lexer indent ok", result.ok(), result.diagnostics.format(&source));
    if (!result.ok()) {
        return;
    }
    expect_eq_size("lexer indent line count", result.lines.size(), 3);
    expect_eq_size("lexer child indent", result.lines[1].indent, 4);
    expect_eq("lexer negative literal", result.lines[1].tokens[4].value, "-1");
    expect_eq("lexer prefix minus", result.lines[2].tokens[2].value, "-");
}

void test_rejects_tabs() {
    const walk::SourceFile source = walk::SourceFile::from_text("tabs.walk", "if: true\n\tout: 'bad'\n");
    const walk::lex::LexResult result = walk::lex::lex_source(source);
    expect_true("lexer tabs fail", !result.ok(), "tabbed source unexpectedly passed");
    expect_true("lexer tab diagnostic", result.diagnostics.format(&source).find("tabs are invalid in indentation") != std::string::npos, "missing tab diagnostic");
}

}  // namespace

int run_lexer_tests() {
    failures = 0;
    test_lexes_commands_comments_and_strings();
    test_lexes_indentation_and_negative_numbers();
    test_rejects_tabs();
    return failures;
}
