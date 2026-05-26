#include "sema/checker.h"
#include "sema/modules.h"

#include <filesystem>
#include <fstream>
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

void write_file(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
}

void test_module_export_check() {
    const std::filesystem::path dir = std::filesystem::path("build") / "cpp-tests" / "modules";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    write_file(dir / "math_extra.walk", "func: square(x int) int\n    return: * x x\nexp: square\n");
    write_file(dir / "main.walk", "imp: math_extra\nout: math_extra.square(4)\n");

    walk::sema::ProgramBundle bundle = walk::sema::load_program_with_modules((dir / "main.walk").string());
    expect_true("module load ok", bundle.ok(), bundle.diagnostics.format(bundle.source.get()));
    if (!bundle.ok()) {
        return;
    }
    walk::sema::CheckResult checked = walk::sema::check_programs(*bundle.parsed.program, bundle.modules);
    expect_true("module check ok", checked.ok(), checked.diagnostics.format(bundle.source.get()));
}

}  // namespace

int run_module_tests() {
    failures = 0;
    test_module_export_check();
    return failures;
}
