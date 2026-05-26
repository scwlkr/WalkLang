#include "ast/ast.h"
#include "parse/parser.h"
#include "support/source_file.h"

#include <algorithm>
#include <filesystem>
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

void expect_eq_size(const std::string& name, std::size_t got, std::size_t want) {
    if (got != want) {
        fail(name, "want " + std::to_string(want) + " got " + std::to_string(got));
    }
}

walk::parse::ParseResult parse_text(const std::string& text) {
    const walk::SourceFile source = walk::SourceFile::from_text("parser.walk", text);
    return walk::parse::parse_source(source);
}

void expect_file_parses(const std::filesystem::path& path) {
    walk::Result<walk::SourceFile> loaded = walk::SourceFile::load(path.string());
    expect_true("load " + path.string(), loaded.ok(), loaded.error());
    if (!loaded.ok()) {
        return;
    }
    walk::SourceFile source = loaded.take_value();
    walk::parse::ParseResult parsed = walk::parse::parse_source(source);
    expect_true("parse " + path.string(), parsed.ok(), parsed.diagnostics.format(&source));
}

void test_constructs_core_ast_shapes() {
    walk::parse::ParseResult parsed = parse_text(
        "struct: User\n"
        "    name string\n"
        "    age int\n"
        "func: User.is_adult(self User) bool\n"
        "    if: >= self.age 18\n"
        "        return: true\n"
        "    else:\n"
        "        return: false\n"
        "var: users = [User('Walker', 25)]\n"
        "out: users[0].is_adult()\n");
    expect_true("parser core ok", parsed.ok(), parsed.diagnostics.format());
    if (!parsed.ok()) {
        return;
    }
    expect_eq_size("parser top statements", parsed.program->statements.size(), 4);
    expect_true("parser struct kind", parsed.program->statements[0]->kind == walk::ast::StatementKind::StructDecl, "first statement not struct");
    expect_true("parser func kind", parsed.program->statements[1]->kind == walk::ast::StatementKind::FuncDecl, "second statement not func");
    const auto* function = dynamic_cast<walk::ast::FuncDecl*>(parsed.program->statements[1]);
    expect_true("parser method receiver", function != nullptr && function->receiver == "User", "missing method receiver");
}

void test_multiline_prefix_expression() {
    walk::parse::ParseResult parsed = parse_text(
        "var: total =\n"
        "    +:\n"
        "        1\n"
        "        2\n"
        "out: total\n");
    expect_true("parser multiline prefix ok", parsed.ok(), parsed.diagnostics.format());
    if (!parsed.ok()) {
        return;
    }
    const auto* var = dynamic_cast<walk::ast::VarDecl*>(parsed.program->statements[0]);
    expect_true("parser var decl", var != nullptr, "missing var decl");
    expect_true("parser prefix value", var != nullptr && var->value->kind == walk::ast::ExpressionKind::Prefix, "value is not prefix");
}

void test_interpolation_constructs_expression_part() {
    walk::parse::ParseResult parsed = parse_text("out: 'score {+ 1 2}'\n");
    expect_true("parser interpolation ok", parsed.ok(), parsed.diagnostics.format());
    if (!parsed.ok()) {
        return;
    }
    const auto* out = dynamic_cast<walk::ast::Out*>(parsed.program->statements[0]);
    expect_true("parser out stmt", out != nullptr, "missing out statement");
    expect_true("parser interpolation expr", out != nullptr && out->value->kind == walk::ast::ExpressionKind::InterpolatedString, "not interpolated");
}

void test_current_pass_fixtures_parse() {
    std::vector<std::filesystem::path> paths;
    for (const auto& entry : std::filesystem::directory_iterator("tests/pass")) {
        if (entry.path().extension() == ".walk") {
            paths.push_back(entry.path());
        }
    }
    paths.push_back("tools/walktop/src/main.walk");
    paths.push_back("tools/walktop/src/walktop.walk");
    paths.push_back("tools/walktop/tests/main_test.walk");
    std::sort(paths.begin(), paths.end());
    for (const std::filesystem::path& path : paths) {
        expect_file_parses(path);
    }
}

void test_syntax_fail_fixtures_reject() {
    for (const std::filesystem::path& path : {std::filesystem::path("tests/fail/bad_indent.walk"), std::filesystem::path("tests/fail/top_break.walk")}) {
        walk::Result<walk::SourceFile> loaded = walk::SourceFile::load(path.string());
        expect_true("load syntax fail " + path.string(), loaded.ok(), loaded.error());
        if (!loaded.ok()) {
            continue;
        }
        walk::SourceFile source = loaded.take_value();
        walk::parse::ParseResult parsed = walk::parse::parse_source(source);
        expect_true("reject syntax fail " + path.string(), !parsed.ok(), "syntax fail unexpectedly parsed");
        expect_true("syntax diagnostic " + path.string(), parsed.diagnostics.format(&source).find("syntax error") != std::string::npos, "missing syntax error");
    }
}

void test_semantic_fail_fixture_still_parses() {
    expect_file_parses("tests/fail/bad_array.walk");
}

}  // namespace

int run_parser_tests() {
    failures = 0;
    test_constructs_core_ast_shapes();
    test_multiline_prefix_expression();
    test_interpolation_constructs_expression_part();
    test_current_pass_fixtures_parse();
    test_syntax_fail_fixtures_reject();
    test_semantic_fail_fixture_still_parses();
    return failures;
}
