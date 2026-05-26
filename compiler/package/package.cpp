#include "package/package.h"

#include "support/toml_like.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace walk::package {
namespace {

namespace toml = walk::support::toml_like;

class Sha256 {
public:
    void update(const std::string& text) {
        update(reinterpret_cast<const unsigned char*>(text.data()), text.size());
    }

    void update_byte(unsigned char byte) {
        update(&byte, 1);
    }

    std::string hex_digest() {
        const std::uint64_t bit_count = bit_count_;
        update_byte(0x80);
        while (buffer_size_ != 56) {
            if (buffer_size_ == 64) {
                transform();
            }
            update_byte(0x00);
        }
        for (int shift = 56; shift >= 0; shift -= 8) {
            update_byte(static_cast<unsigned char>((bit_count >> shift) & 0xffU));
        }

        std::ostringstream output;
        output << std::hex << std::setfill('0');
        for (const std::uint32_t word : state_) {
            output << std::setw(8) << word;
        }
        return output.str();
    }

private:
    void update(const unsigned char* data, std::size_t size) {
        bit_count_ += static_cast<std::uint64_t>(size) * 8U;
        for (std::size_t index = 0; index < size; ++index) {
            buffer_[buffer_size_++] = data[index];
            if (buffer_size_ == 64) {
                transform();
            }
        }
    }

    static std::uint32_t rotr(std::uint32_t value, int bits) {
        return (value >> bits) | (value << (32 - bits));
    }

    void transform() {
        static constexpr std::array<std::uint32_t, 64> k = {
            0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
            0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
            0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
            0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
            0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
            0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
            0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
            0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
        };

        std::array<std::uint32_t, 64> w{};
        for (std::size_t index = 0; index < 16; ++index) {
            w[index] = (static_cast<std::uint32_t>(buffer_[index * 4]) << 24U) |
                (static_cast<std::uint32_t>(buffer_[index * 4 + 1]) << 16U) |
                (static_cast<std::uint32_t>(buffer_[index * 4 + 2]) << 8U) |
                static_cast<std::uint32_t>(buffer_[index * 4 + 3]);
        }
        for (std::size_t index = 16; index < 64; ++index) {
            const std::uint32_t s0 = rotr(w[index - 15], 7) ^ rotr(w[index - 15], 18) ^ (w[index - 15] >> 3U);
            const std::uint32_t s1 = rotr(w[index - 2], 17) ^ rotr(w[index - 2], 19) ^ (w[index - 2] >> 10U);
            w[index] = w[index - 16] + s0 + w[index - 7] + s1;
        }

        std::uint32_t a = state_[0];
        std::uint32_t b = state_[1];
        std::uint32_t c = state_[2];
        std::uint32_t d = state_[3];
        std::uint32_t e = state_[4];
        std::uint32_t f = state_[5];
        std::uint32_t g = state_[6];
        std::uint32_t h = state_[7];

        for (std::size_t index = 0; index < 64; ++index) {
            const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const std::uint32_t ch = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = h + s1 + ch + k[index] + w[index];
            const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + maj;
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
        buffer_size_ = 0;
    }

    std::array<std::uint32_t, 8> state_{
        0x6a09e667U,
        0xbb67ae85U,
        0x3c6ef372U,
        0xa54ff53aU,
        0x510e527fU,
        0x9b05688cU,
        0x1f83d9abU,
        0x5be0cd19U,
    };
    std::array<unsigned char, 64> buffer_{};
    std::size_t buffer_size_ = 0;
    std::uint64_t bit_count_ = 0;
};

bool is_ascii_letter(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

bool is_ascii_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

std::vector<LockEntry> sorted_entries(const std::map<std::string, LockEntry>& entries) {
    std::vector<LockEntry> result;
    result.reserve(entries.size());
    for (const auto& [_, entry] : entries) {
        result.push_back(entry);
    }
    return result;
}

bool should_skip_package_path(const std::filesystem::directory_entry& entry) {
    const std::string name = entry.path().filename().string();
    if (entry.is_directory()) {
        return name == "build" || name == ".walk" || name == ".git";
    }
    return name == "walk.lock" || (name.size() >= 4 && name.substr(name.size() - 4) == ".tmp");
}

Result<void> copy_package_tree(const std::filesystem::path& source_root, const std::filesystem::path& destination_root) {
    try {
        std::filesystem::create_directories(destination_root);
        for (auto it = std::filesystem::recursive_directory_iterator(source_root); it != std::filesystem::recursive_directory_iterator(); ++it) {
            const std::filesystem::directory_entry& entry = *it;
            if (should_skip_package_path(entry)) {
                if (entry.is_directory()) {
                    it.disable_recursion_pending();
                }
                continue;
            }
            const std::filesystem::path rel = std::filesystem::relative(entry.path(), source_root);
            const std::filesystem::path target = destination_root / rel;
            if (entry.is_directory()) {
                std::filesystem::create_directories(target);
                continue;
            }
            if (!entry.is_regular_file()) {
                continue;
            }
            std::filesystem::create_directories(target.parent_path());
            std::filesystem::copy_file(entry.path(), target, std::filesystem::copy_options::overwrite_existing);
        }
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<void>::failure(error.what());
    }
    return Result<void>::success();
}

Result<void> resolve_dependency(
    const project::ProjectConfig& config,
    const std::filesystem::path& registry_root,
    const project::Dependency& dependency,
    std::map<std::string, LockEntry>& entries) {
    const auto existing = entries.find(dependency.name);
    if (existing != entries.end()) {
        if (existing->second.version != dependency.version) {
            return Result<void>::failure("package error: dependency " + dependency.name + " is required at both " + existing->second.version + " and " + dependency.version);
        }
        return Result<void>::success();
    }

    const std::filesystem::path source_root = registry_root / dependency.name / dependency.version;
    std::error_code stat_error;
    const bool exists = std::filesystem::exists(source_root, stat_error);
    if (stat_error) {
        return Result<void>::failure(stat_error.message());
    }
    if (!exists) {
        return Result<void>::failure("package error: dependency " + dependency.name + "@" + dependency.version + " not found in registry " + registry_root.string());
    }
    if (!std::filesystem::is_directory(source_root)) {
        return Result<void>::failure("package error: dependency " + dependency.name + "@" + dependency.version + " is not a directory");
    }

    Result<project::ProjectConfig> dependency_config = project::load_project_config_at(source_root);
    if (!dependency_config.ok()) {
        return Result<void>::failure(dependency_config.error());
    }
    if (dependency_config.value().name != dependency.name || dependency_config.value().version != dependency.version) {
        return Result<void>::failure(
            "package error: registry package " + dependency.name + "@" + dependency.version + " has manifest " + dependency_config.value().name + "@" + dependency_config.value().version);
    }

    const std::filesystem::path cache_root = package_cache_root(config.root, dependency);
    try {
        std::filesystem::remove_all(cache_root);
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<void>::failure(error.what());
    }
    Result<void> copied = copy_package_tree(source_root, cache_root);
    if (!copied.ok()) {
        return copied;
    }
    Result<std::string> checksum = package_checksum(cache_root);
    if (!checksum.ok()) {
        return Result<void>::failure(checksum.error());
    }
    entries[dependency.name] = LockEntry{dependency.name, dependency.version, "sha256:" + checksum.value()};

    for (const project::Dependency& child : dependency_config.value().dependencies) {
        Result<void> child_resolved = resolve_dependency(config, registry_root, child, entries);
        if (!child_resolved.ok()) {
            return child_resolved;
        }
    }
    return Result<void>::success();
}

bool path_inside(const std::filesystem::path& root, const std::filesystem::path& path) {
    std::error_code error;
    const std::filesystem::path abs_root = std::filesystem::absolute(root, error).lexically_normal();
    if (error) {
        return false;
    }
    const std::filesystem::path abs_path = std::filesystem::absolute(path, error).lexically_normal();
    if (error) {
        return false;
    }
    const std::filesystem::path rel = std::filesystem::relative(abs_path, abs_root, error);
    if (error) {
        return false;
    }
    const std::string generic = rel.generic_string();
    return generic == "." || (generic.rfind("../", 0) != 0 && !rel.is_absolute());
}

std::string initial_package_config(const std::string& name) {
    return "name = " + toml::quote_string(name) + "\nversion = \"0.1.0\"\nentry = \"src/main.walk\"\n\n[build]\noutput = " +
        toml::quote_string((std::filesystem::path("build") / name).generic_string()) + "\nmode = \"debug\"\n\n[dependencies]\n";
}

std::string initial_package_readme(const std::string& name) {
    return "# " + name + "\n\nA WalkLang package.\n";
}

std::string initial_package_main_source(const std::string& name) {
    return "imp: " + name + ".core\n\nout: " + name + ".core.double(3)\n\n";
}

std::string initial_package_module_source() {
    return "func: double(x int) int\n    return: * x 2\n\nexp: double\n\n";
}

std::string initial_package_test_source(const std::string& name) {
    return "imp: " + name + ".core\n\ntest: 'double works'\n    assert: == " + name + ".core.double(3) 6\n\n";
}

Result<void> write_text_file(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return Result<void>::failure("could not write " + path.string());
    }
    output << text;
    return Result<void>::success();
}

}  // namespace

bool valid_package_name(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    for (std::size_t index = 0; index < name.size(); ++index) {
        const char ch = name[index];
        if (index == 0) {
            if (!is_ascii_letter(ch) && ch != '_') {
                return false;
            }
            continue;
        }
        if (is_ascii_letter(ch) || is_ascii_digit(ch) || ch == '_') {
            continue;
        }
        return false;
    }
    return true;
}

bool reserved_package_collection_root(const std::string& name) {
    return name == "std" || name == "math" || name == "string" || name == "array" || name == "time" || name == "random" || name == "testing" ||
        name == "io" || name == "parse" || name == "process" || name == "file" || name == "dir" || name == "path" || name == "json" ||
        name == "term" || name == "http" || name == "html";
}

bool valid_semver(const std::string& version) {
    std::size_t start = 0;
    int parts = 0;
    for (;;) {
        const std::size_t dot = version.find('.', start);
        const std::string part = version.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
        if (part.empty() || !std::all_of(part.begin(), part.end(), [](char ch) { return is_ascii_digit(ch); })) {
            return false;
        }
        ++parts;
        if (dot == std::string::npos) {
            break;
        }
        start = dot + 1;
    }
    return parts == 3;
}

Result<std::filesystem::path> init_package(const std::filesystem::path& raw_package_path) {
    const std::filesystem::path package_path = raw_package_path.lexically_normal();
    const std::string name = package_path.filename().string();
    if (!valid_package_name(name)) {
        return Result<std::filesystem::path>::failure("package name \"" + name + "\" may contain only letters, numbers, and underscore, and must start with a letter or underscore");
    }
    if (name == "std") {
        return Result<std::filesystem::path>::failure("package error: package name \"std\" is reserved for a built-in collection root");
    }
    std::error_code exists_error;
    if (std::filesystem::exists(package_path, exists_error)) {
        return Result<std::filesystem::path>::failure("package already exists: " + package_path.string());
    }
    if (exists_error) {
        return Result<std::filesystem::path>::failure("package check failed: " + exists_error.message());
    }

    try {
        std::filesystem::create_directories(package_path / "src" / name);
        std::filesystem::create_directories(package_path / "tests");
        std::filesystem::create_directories(package_path / "build");
        const std::vector<std::pair<std::filesystem::path, std::string>> files = {
            {"walk.toml", initial_package_config(name)},
            {"README.md", initial_package_readme(name)},
            {"src/main.walk", initial_package_main_source(name)},
            {std::filesystem::path("src") / name / "core.walk", initial_package_module_source()},
            {"tests/main_test.walk", initial_package_test_source(name)},
        };
        for (const auto& [rel, contents] : files) {
            Result<void> written = write_text_file(package_path / rel, contents);
            if (!written.ok()) {
                return Result<std::filesystem::path>::failure(written.error());
            }
        }
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<std::filesystem::path>::failure(error.what());
    }
    return Result<std::filesystem::path>::success(package_path);
}

Result<std::vector<LockEntry>> resolve_dependencies(const project::ProjectConfig& config, const std::filesystem::path& registry_root) {
    std::map<std::string, LockEntry> entries;
    for (const project::Dependency& dependency : config.dependencies) {
        Result<void> resolved = resolve_dependency(config, registry_root, dependency, entries);
        if (!resolved.ok()) {
            return Result<std::vector<LockEntry>>::failure(resolved.error());
        }
    }
    return Result<std::vector<LockEntry>>::success(sorted_entries(entries));
}

Result<std::map<std::string, LockEntry>> read_lock_file(const std::filesystem::path& root) {
    const std::filesystem::path path = root / "walk.lock";
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return Result<std::map<std::string, LockEntry>>::failure("package error: dependencies are not locked; run walk package resolve <registry-dir>");
    }

    std::map<std::string, LockEntry> entries;
    LockEntry current;
    bool in_package = false;
    auto flush = [&](int line) -> Result<void> {
        if (!in_package) {
            return Result<void>::success();
        }
        if (current.name.empty() || current.version.empty() || current.checksum.empty()) {
            return Result<void>::failure(path.string() + ":" + std::to_string(line) + ": incomplete package lock entry");
        }
        if (!valid_package_name(current.name)) {
            return Result<void>::failure(path.string() + ":" + std::to_string(line) + ": invalid package name \"" + current.name + "\"");
        }
        if (!valid_semver(current.version)) {
            return Result<void>::failure(path.string() + ":" + std::to_string(line) + ": invalid package version \"" + current.version + "\"");
        }
        if (current.checksum.rfind("sha256:", 0) != 0) {
            return Result<void>::failure(path.string() + ":" + std::to_string(line) + ": invalid package checksum");
        }
        if (entries.find(current.name) != entries.end()) {
            return Result<void>::failure(path.string() + ":" + std::to_string(line) + ": duplicate package \"" + current.name + "\"");
        }
        entries[current.name] = current;
        current = LockEntry{};
        return Result<void>::success();
    };

    std::string line;
    int line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        line = toml::trim(toml::strip_comment(line));
        if (line.empty()) {
            continue;
        }
        if (line == "[[package]]") {
            Result<void> flushed = flush(line_number);
            if (!flushed.ok()) {
                return Result<std::map<std::string, LockEntry>>::failure(flushed.error());
            }
            in_package = true;
            continue;
        }
        if (!in_package) {
            return Result<std::map<std::string, LockEntry>>::failure(path.string() + ":" + std::to_string(line_number) + ": expected [[package]]");
        }
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos) {
            return Result<std::map<std::string, LockEntry>>::failure(path.string() + ":" + std::to_string(line_number) + ": expected key = value");
        }
        const std::string key = toml::trim(line.substr(0, equals));
        Result<std::string> value = toml::parse_string(toml::trim(line.substr(equals + 1)), path.string(), line_number);
        if (!value.ok()) {
            return Result<std::map<std::string, LockEntry>>::failure(value.error());
        }
        if (key == "name") {
            current.name = value.take_value();
        } else if (key == "version") {
            current.version = value.take_value();
        } else if (key == "checksum") {
            current.checksum = value.take_value();
        } else {
            return Result<std::map<std::string, LockEntry>>::failure(path.string() + ":" + std::to_string(line_number) + ": unknown package lock key \"" + key + "\"");
        }
    }
    Result<void> flushed = flush(line_number + 1);
    if (!flushed.ok()) {
        return Result<std::map<std::string, LockEntry>>::failure(flushed.error());
    }
    return Result<std::map<std::string, LockEntry>>::success(std::move(entries));
}

Result<void> write_lock_file(const std::filesystem::path& root, const std::vector<LockEntry>& entries) {
    std::ostringstream output;
    output << "# Generated by walk package resolve. Do not edit.\n";
    for (const LockEntry& entry : entries) {
        output << "\n[[package]]\n";
        output << "name = " << toml::quote_string(entry.name) << "\n";
        output << "version = " << toml::quote_string(entry.version) << "\n";
        output << "checksum = " << toml::quote_string(entry.checksum) << "\n";
    }
    std::ofstream file(root / "walk.lock", std::ios::binary);
    if (!file) {
        return Result<void>::failure("could not write " + (root / "walk.lock").string());
    }
    file << output.str();
    return Result<void>::success();
}

Result<std::vector<std::filesystem::path>> package_search_dirs(const project::ProjectConfig& config) {
    if (config.dependencies.empty()) {
        return Result<std::vector<std::filesystem::path>>::success({});
    }
    Result<std::map<std::string, LockEntry>> entries = read_lock_file(config.root);
    if (!entries.ok()) {
        return Result<std::vector<std::filesystem::path>>::failure(entries.error());
    }
    for (const project::Dependency& dependency : config.dependencies) {
        const auto entry = entries.value().find(dependency.name);
        if (entry == entries.value().end()) {
            return Result<std::vector<std::filesystem::path>>::failure(
                "package error: dependency " + dependency.name + "@" + dependency.version + " is not locked; run walk package resolve <registry-dir>");
        }
        if (entry->second.version != dependency.version) {
            return Result<std::vector<std::filesystem::path>>::failure(
                "package error: dependency " + dependency.name + "@" + dependency.version + " does not match walk.lock " + entry->second.version + "; run walk package resolve <registry-dir>");
        }
    }

    std::vector<std::filesystem::path> dirs;
    for (const LockEntry& entry : sorted_entries(entries.value())) {
        const std::filesystem::path cache_root = package_cache_root(config.root, project::Dependency{entry.name, entry.version});
        Result<std::string> checksum = package_checksum(cache_root);
        if (!checksum.ok()) {
            return Result<std::vector<std::filesystem::path>>::failure(
                "package error: dependency " + entry.name + "@" + entry.version + " cache is unavailable: " + checksum.error());
        }
        if (entry.checksum != "sha256:" + checksum.value()) {
            return Result<std::vector<std::filesystem::path>>::failure(
                "package error: dependency " + entry.name + "@" + entry.version + " cache does not match walk.lock; run walk package resolve <registry-dir>");
        }
        dirs.push_back(cache_root / "src");
    }
    return Result<std::vector<std::filesystem::path>>::success(std::move(dirs));
}

Result<void> validate_publish_manifest(const project::ProjectConfig& config) {
    if (!valid_package_name(config.name)) {
        return Result<void>::failure("package error: package name \"" + config.name + "\" may contain only letters, numbers, and underscore, and must start with a letter or underscore");
    }
    if (reserved_package_collection_root(config.name)) {
        return Result<void>::failure("package error: package name \"" + config.name + "\" is reserved for a built-in collection root");
    }
    if (!valid_semver(config.version)) {
        return Result<void>::failure("package error: package version \"" + config.version + "\" must be MAJOR.MINOR.PATCH");
    }
    std::ifstream readme(config.root / "README.md", std::ios::binary);
    if (!readme) {
        return Result<void>::failure("package error: README.md is required before publish");
    }
    std::ostringstream contents;
    contents << readme.rdbuf();
    if (toml::trim(contents.str()).empty()) {
        return Result<void>::failure("package error: README.md is required before publish");
    }
    return Result<void>::success();
}

Result<std::filesystem::path> publish_package(const project::ProjectConfig& config, const std::filesystem::path& registry_root) {
    const std::filesystem::path destination = registry_root / config.name / config.version;
    if (std::filesystem::exists(destination)) {
        return Result<std::filesystem::path>::failure("package error: " + config.name + "@" + config.version + " already exists in registry");
    }
    const std::filesystem::path tmp = destination.string() + ".tmp";
    if (path_inside(config.root, tmp)) {
        return Result<std::filesystem::path>::failure("package error: registry destination must not be inside the package being published");
    }
    try {
        std::filesystem::remove_all(tmp);
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<std::filesystem::path>::failure(error.what());
    }
    Result<void> copied = copy_package_tree(config.root, tmp);
    if (!copied.ok()) {
        std::filesystem::remove_all(tmp);
        return Result<std::filesystem::path>::failure(copied.error());
    }
    try {
        std::filesystem::create_directories(destination.parent_path());
        std::filesystem::rename(tmp, destination);
    } catch (const std::filesystem::filesystem_error& error) {
        std::filesystem::remove_all(tmp);
        return Result<std::filesystem::path>::failure(error.what());
    }
    return Result<std::filesystem::path>::success(destination);
}

std::filesystem::path package_cache_root(const std::filesystem::path& root, const project::Dependency& dependency) {
    return root / ".walk" / "packages" / dependency.name / dependency.version;
}

Result<std::string> package_checksum(const std::filesystem::path& root) {
    if (!std::filesystem::is_directory(root)) {
        return Result<std::string>::failure(root.string() + " is not a directory");
    }
    std::vector<std::string> files;
    try {
        for (auto it = std::filesystem::recursive_directory_iterator(root); it != std::filesystem::recursive_directory_iterator(); ++it) {
            const std::filesystem::directory_entry& entry = *it;
            if (should_skip_package_path(entry)) {
                if (entry.is_directory()) {
                    it.disable_recursion_pending();
                }
                continue;
            }
            if (!entry.is_regular_file()) {
                continue;
            }
            files.push_back(std::filesystem::relative(entry.path(), root).generic_string());
        }
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<std::string>::failure(error.what());
    }
    std::sort(files.begin(), files.end());
    Sha256 hash;
    for (const std::string& rel : files) {
        hash.update(rel);
        hash.update_byte(0);
        std::ifstream input(root / std::filesystem::path(rel), std::ios::binary);
        if (!input) {
            return Result<std::string>::failure("could not read " + (root / std::filesystem::path(rel)).string());
        }
        std::ostringstream contents;
        contents << input.rdbuf();
        hash.update(contents.str());
        hash.update_byte(0);
    }
    return Result<std::string>::success(hash.hex_digest());
}

}  // namespace walk::package
