#include "codegen/c/c_emitter.h"
#include "ir/lower.h"
#include "parse/parser.h"
#include "sema/checker.h"
#include "sema/modules.h"
#include "support/source_file.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
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

std::string shell_quote(const std::filesystem::path& path) {
    std::string value = path.string();
    std::string out = "'";
    for (const char ch : value) {
        if (ch == '\'') {
            out += "'\\''";
        } else {
            out += ch;
        }
    }
    out += "'";
    return out;
}

walk::ir::LoweredProgram lower_source(const std::string& source) {
    walk::SourceFile file = walk::SourceFile::from_text("main.walk", source);
    walk::parse::ParseResult parsed = walk::parse::parse_source(file);
    if (!parsed.ok()) {
        fail("parse source", parsed.diagnostics.format(&file));
        return {};
    }
    std::map<std::string, std::unique_ptr<walk::sema::Module>> modules;
    walk::sema::CheckResult checked = walk::sema::check_programs(*parsed.program, modules);
    if (!checked.ok()) {
        fail("check source", checked.diagnostics.format(&file));
        return {};
    }
    return walk::ir::lower_program(*parsed.program, modules);
}

std::string emit_source(const std::string& source, bool tests_only = false) {
    walk::ir::LoweredProgram lowered = lower_source(source);
    walk::Result<std::string> emitted = walk::codegen::c::emit_c(lowered, {tests_only});
    if (!emitted.ok()) {
        fail("emit source", emitted.error());
        return "";
    }
    return emitted.take_value();
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

void build_generated_c(const std::string& name, const std::string& c_code, const std::filesystem::path& c_path, const std::filesystem::path& exe_path) {
    {
        std::ofstream output(c_path, std::ios::binary);
        output << c_code;
    }
    const std::filesystem::path runtime_dir = std::filesystem::current_path() / "runtime";
    const std::string command = "cc " + shell_quote(c_path) + " " + shell_quote(runtime_dir / "walk_runtime.c") + " " +
        shell_quote(runtime_dir / "platform" / "walk_platform_posix.c") + " -I " + shell_quote(runtime_dir) + " -o " + shell_quote(exe_path) + " -lm";
    const int code = std::system(command.c_str());
    if (code != 0) {
        fail(name, "cc failed");
    }
}

void test_ir_lowering_keeps_typed_program() {
    walk::ir::LoweredProgram lowered = lower_source("var: x = + 1 2\nout: x\n");
    expect_true("lowered statements", lowered.program.statements.size() == 2, "wrong statement count");
    if (lowered.program.statements.empty()) {
        return;
    }
    const auto* var = dynamic_cast<const walk::ir::VarDecl*>(lowered.program.statements[0].get());
    expect_true("lowered var", var != nullptr, "first statement is not var");
    if (var != nullptr) {
        expect_eq("lowered var name", var->name, "x");
        expect_true("lowered var type", var->value->type.kind == walk::ast::TypeKind::Int, "value is not typed int");
    }
}

void test_emit_c_for_variables_and_output() {
    const std::string c_code = emit_source("var: x = + 1 2\nconst: ok = true\nout: x\nout: ok\n");
    for (const std::string& want : {
             "#include \"walk_runtime.h\"",
             "/* source: main.walk:1:1 */",
             "WalkInt x = (1 + 2);",
             "const WalkBool ok = true;",
             "walk_rt_print_int((WalkInt)(x));",
             "walk_rt_print_bool((WalkBool)(ok));",
         }) {
        expect_true("generated C contains " + want, c_code.find(want) != std::string::npos, "missing snippet\n" + c_code);
    }
}

void test_generated_c_builds_and_runs() {
    if (std::system("command -v cc >/dev/null 2>&1") != 0) {
        return;
    }
    const std::string c_code = emit_source("var: x = + 1 2\nout: x\nout: 'ok'\nout: / 5 2\n");
    const std::filesystem::path dir = std::filesystem::path("build") / "cpp-tests" / "emitter-run";
    std::filesystem::create_directories(dir);
    const std::filesystem::path c_path = dir / "main.c";
    const std::filesystem::path exe_path = dir / "main";
    const std::filesystem::path out_path = dir / "main.out";
    build_generated_c("generated C build", c_code, c_path, exe_path);
    const int code = std::system((shell_quote(exe_path) + " > " + shell_quote(out_path)).c_str());
    if (code != 0) {
        fail("generated C run", "program failed");
        return;
    }
    expect_eq("generated C output", read_file(out_path), "3\nok\n2.5\n");
}

void test_test_runner_program_builds_and_runs() {
    if (std::system("command -v cc >/dev/null 2>&1") != 0) {
        return;
    }
    const std::string c_code = emit_source(
        "imp: string\n"
        "test: 'strings work'\n"
        "    assert: == string.len('walk') 4\n",
        true);
    const std::filesystem::path dir = std::filesystem::path("build") / "cpp-tests" / "emitter-test";
    std::filesystem::create_directories(dir);
    const std::filesystem::path c_path = dir / "tests.c";
    const std::filesystem::path exe_path = dir / "tests";
    const std::filesystem::path out_path = dir / "tests.out";
    build_generated_c("test runner build", c_code, c_path, exe_path);
    const int code = std::system((shell_quote(exe_path) + " > " + shell_quote(out_path)).c_str());
    if (code != 0) {
        fail("test runner run", "program failed");
        return;
    }
    expect_eq("test runner output", read_file(out_path), "test: strings work\nok 1 tests\n");
}

}  // namespace

int run_emitter_tests() {
    test_ir_lowering_keeps_typed_program();
    test_emit_c_for_variables_and_output();
    test_generated_c_builds_and_runs();
    test_test_runner_program_builds_and_runs();
    return failures;
}
