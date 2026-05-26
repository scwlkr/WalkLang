#include "cli/command.h"
#include "docs/sitegen.h"

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

void expect_contains(const std::string& name, const std::string& text, const std::string& needle) {
    if (text.find(needle) == std::string::npos) {
        fail(name, "missing " + needle + " in " + text);
    }
}

void write_file(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void test_docs_and_debug_map_commands() {
    const std::filesystem::path dir = std::filesystem::path("build") / "cpp-tests" / "docs";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    const std::filesystem::path source_path = dir / "main.walk";
    write_file(source_path, "/// Summary: Doubles an integer.\n"
                            "/// Params:\n"
                            "/// - x: value to multiply\n"
                            "/// Returns: the doubled value.\n"
                            "/// Example:\n"
                            "/// ```walk\n"
                            "/// out: double(4)\n"
                            "/// ```\n"
                            "/// Since: current\n"
                            "func: double(x int) int\n"
                            "    return: * x 2\n\n"
                            "/// Summary: Exposes double from this module.\n"
                            "/// Example:\n"
                            "/// ```walk\n"
                            "/// exp: double\n"
                            "/// ```\n"
                            "/// Since: current\n"
                            "exp: double\n\n"
                            "out: double(4)\n");

    const std::filesystem::path docs_path = dir / "api.md";
    const walk::cli::CommandResult docs = walk::cli::dispatch({"docs", "--strict", "-o", docs_path.string(), source_path.string()});
    if (docs.exit_code != 0) {
        fail("docs command", docs.stderr_text);
        return;
    }
    const std::string markdown = read_file(docs_path);
    expect_contains("docs markdown header", markdown, "# WalkLang API");
    expect_contains("docs markdown function", markdown, "func double(x int) int");
    expect_contains("docs markdown summary", markdown, "Doubles an integer.");

    const std::filesystem::path json_path = dir / "api.json";
    const walk::cli::CommandResult json = walk::cli::dispatch({"docs", "--strict", "--format", "json", "-o", json_path.string(), source_path.string()});
    if (json.exit_code != 0) {
        fail("docs json command", json.stderr_text);
        return;
    }
    const std::string payload = read_file(json_path);
    expect_contains("docs json version", payload, "\"version\": 1");
    expect_contains("docs json symbol", payload, "\"name\": \"double\"");

    const std::filesystem::path debug_path = dir / "debug.json";
    const walk::cli::CommandResult debug = walk::cli::dispatch({"debug-map", "-o", debug_path.string(), source_path.string()});
    if (debug.exit_code != 0) {
        fail("debug-map command", debug.stderr_text);
        return;
    }
    const std::string debug_payload = read_file(debug_path);
    expect_contains("debug map function", debug_payload, "\"name\": \"double\"");
    expect_contains("debug map kind", debug_payload, "\"kind\": \"function\"");
}

void test_site_generator_writes_core_outputs() {
    const std::filesystem::path out = std::filesystem::path("build") / "cpp-tests" / "site";
    std::filesystem::remove_all(out);
    const walk::Result<void> built = walk::docs::build_site("docs", out.string());
    if (!built.ok()) {
        fail("site generator", built.error());
        return;
    }
    const std::vector<std::filesystem::path> expected_paths = {out / "index.html", out / "docs" / "index.html", out / "docs" / "reference" / "api.html"};
    for (const std::filesystem::path& path : expected_paths) {
        if (!std::filesystem::is_regular_file(path)) {
            fail("site output", "missing " + path.string());
        }
    }
    const std::string home = read_file(out / "index.html");
    expect_contains("site static shortcuts", home, "doc-shortcuts");
    if (home.find("<script") != std::string::npos) {
        fail("site scripts", "site output should not include JavaScript");
    }
    if (std::filesystem::exists(out / "assets" / "site.js") || std::filesystem::exists(out / "docs" / "search.json")) {
        fail("site assets", "site output should be static HTML/CSS without JavaScript search assets");
    }
}

}  // namespace

int run_docs_tests() {
    failures = 0;
    test_docs_and_debug_map_commands();
    test_site_generator_writes_core_outputs();
    return failures;
}
