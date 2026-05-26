#include "cli/command.h"

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

void test_package_lifecycle() {
    if (!has_cc()) {
        return;
    }
    const std::filesystem::path root = std::filesystem::absolute(std::filesystem::path("build") / "cpp-tests" / "package-lifecycle");
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    const std::filesystem::path registry = root / "registry";
    const std::filesystem::path package_dir = root / "geometry";

    expect_exit("package init", walk::cli::dispatch({"package", "init", package_dir.string()}), 0);
    expect_true("package module exists", std::filesystem::is_regular_file(package_dir / "src" / "geometry" / "core.walk"), "missing package module");
    {
        WorkingDir cwd(package_dir);
        expect_exit("package publish", walk::cli::dispatch({"package", "publish", registry.string()}), 0);
    }
    expect_true("published package exists", std::filesystem::is_regular_file(registry / "geometry" / "0.1.0" / "walk.toml"), "missing published package");

    const std::filesystem::path app_dir = root / "shape_app";
    expect_exit("app init", walk::cli::dispatch({"init", app_dir.string()}), 0);
    write_file(
        app_dir / "walk.toml",
        "name = \"shape_app\"\nversion = \"0.1.0\"\nentry = \"src/main.walk\"\n\n[build]\noutput = \"build/shape_app\"\nrelease = false\n\n[dependencies]\ngeometry = \"0.1.0\"\n");
    write_file(app_dir / "src" / "main.walk", "imp: geometry.core\n\nout: geometry.core.double(4)\n\n");
    write_file(app_dir / "tests" / "main_test.walk", "imp: geometry.core\n\ntest: 'package import works'\n    assert: == geometry.core.double(4) 8\n\n");

    {
        WorkingDir cwd(app_dir);
        walk::cli::CommandResult unlocked = walk::cli::dispatch({"check", "--warnings=error"});
        expect_true("unlocked dependency fails", unlocked.exit_code != 0, "expected unlocked dependency failure");
        expect_true("unlocked dependency diagnostic", unlocked.stderr_text.find("dependencies are not locked") != std::string::npos, unlocked.stderr_text);

        expect_exit("package resolve", walk::cli::dispatch({"package", "resolve", registry.string()}), 0);
        const std::string lock = read_file(app_dir / "walk.lock");
        expect_true("lock package name", lock.find("name = \"geometry\"") != std::string::npos, lock);
        expect_true("lock package version", lock.find("version = \"0.1.0\"") != std::string::npos, lock);
        expect_true("lock package checksum", lock.find("checksum = \"sha256:") != std::string::npos, lock);

        expect_exit("package app check", walk::cli::dispatch({"check", "--warnings=error"}), 0);
        expect_exit("package app test", walk::cli::dispatch({"test", "--warnings=error"}), 0);
        expect_exit("package app build", walk::cli::dispatch({"build"}), 0);
        const std::filesystem::path out = root / "shape_app.out";
        if (std::system((shell_quote(app_dir / "build" / "shape_app") + " > " + shell_quote(out)).c_str()) != 0) {
            fail("package app executable", "run failed");
        } else {
            expect_eq("package app output", read_file(out), "8\n");
        }

        write_file(app_dir / ".walk" / "packages" / "geometry" / "0.1.0" / "src" / "geometry" / "core.walk", "func: double(x int) int\n    return: + x 100\n\nexp: double\n");
        walk::cli::CommandResult corrupted = walk::cli::dispatch({"check", "--warnings=error"});
        expect_true("corrupt cache fails", corrupted.exit_code != 0, "expected package cache checksum mismatch");
        expect_true("corrupt cache diagnostic", corrupted.stderr_text.find("cache does not match walk.lock") != std::string::npos, corrupted.stderr_text);
    }
}

}  // namespace

int run_package_tests() {
    test_package_lifecycle();
    return failures;
}
