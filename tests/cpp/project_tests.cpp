#include "cli/command.h"
#include "project/project.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
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

void expect_exit(const std::string& name, const walk::cli::CommandResult& result, int want) {
    if (result.exit_code != want) {
        fail(name, "want exit " + std::to_string(want) + " got " + std::to_string(result.exit_code) + " stderr <" + result.stderr_text + ">");
    }
}

std::string shell_quote(const std::filesystem::path& path) {
    const std::string value = path.string();
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

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

void write_file(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
}

class WorkingDir {
public:
    explicit WorkingDir(std::filesystem::path next) : previous_(std::filesystem::current_path()) {
        std::filesystem::current_path(std::move(next));
    }

    ~WorkingDir() {
        std::filesystem::current_path(previous_);
    }

private:
    std::filesystem::path previous_;
};

bool has_cc() {
    return std::system("command -v cc >/dev/null 2>&1") == 0;
}

void test_project_config_validation() {
    walk::Result<walk::project::ProjectConfig> parsed = walk::project::parse_project_config("name = \"bad name\"\nentry = \"src/main.walk\"\n", "walk.toml");
    expect_true("project invalid name fails", !parsed.ok(), "expected invalid project name");
    if (!parsed.ok()) {
        expect_eq("project invalid name diagnostic", parsed.error(), "walk.toml:1: project name \"bad name\" may contain only letters, numbers, underscore, and dash");
    }

    walk::Result<walk::project::ProjectConfig> dependency =
        walk::project::parse_project_config("name = \"shape_app\"\n\n[dependencies]\ngeometry = \"latest\"\n", "walk.toml");
    expect_true("project invalid dependency fails", !dependency.ok(), "expected invalid dependency version");
    if (!dependency.ok()) {
        expect_eq("project invalid dependency diagnostic", dependency.error(), "walk.toml:4: dependency \"geometry\" version must be MAJOR.MINOR.PATCH");
    }
}

void test_project_lifecycle() {
    if (!has_cc()) {
        return;
    }
    const std::filesystem::path root = std::filesystem::absolute(std::filesystem::path("build") / "cpp-tests" / "project-lifecycle");
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    const std::filesystem::path project_dir = root / "hello";

    walk::cli::CommandResult init = walk::cli::dispatch({"init", project_dir.string()});
    expect_exit("project init", init, 0);
    for (const char* rel : {"walk.toml", "src/main.walk", "src/math_extra.walk", "tests/main_test.walk"}) {
        expect_true(std::string("project file ") + rel, std::filesystem::is_regular_file(project_dir / rel), "missing file");
    }

    {
        WorkingDir cwd(project_dir);
        expect_exit("project check", walk::cli::dispatch({"check", "--warnings=error"}), 0);
        expect_exit("project build", walk::cli::dispatch({"build"}), 0);
        const std::filesystem::path out = root / "hello.out";
        if (std::system((shell_quote(project_dir / "build" / "hello") + " > " + shell_quote(out)).c_str()) != 0) {
            fail("project executable", "run failed");
        } else {
            expect_eq("project output", read_file(out), "27\n");
        }
        expect_exit("project test", walk::cli::dispatch({"test", "--warnings=error"}), 0);
        write_file(project_dir / "src" / "messy.walk", "out:+ 1 2\n");
        expect_exit("project fmt", walk::cli::dispatch({"fmt"}), 0);
        expect_eq("project formatted source", read_file(project_dir / "src" / "messy.walk"), "out: + 1 2\n");
        expect_exit("project clean", walk::cli::dispatch({"clean"}), 0);
        expect_true("project build removed", !std::filesystem::exists(project_dir / "build"), "build directory should be removed");
    }
}

void test_single_file_fmt() {
    const std::filesystem::path root = std::filesystem::absolute(std::filesystem::path("build") / "cpp-tests" / "project-format");
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    const std::filesystem::path source = root / "messy.walk";
    write_file(source, "if:true\n  out:'inside'\nout:'outside'\n");
    walk::cli::CommandResult result = walk::cli::dispatch({"fmt", source.string()});
    expect_exit("single file fmt", result, 0);
    expect_eq("single file fmt output", result.stdout_text, "if: true\n    out: 'inside'\nout: 'outside'\n");
}

}  // namespace

int run_project_tests() {
    test_project_config_validation();
    test_project_lifecycle();
    test_single_file_fmt();
    return failures;
}
