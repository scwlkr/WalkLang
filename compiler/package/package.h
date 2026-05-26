#pragma once

#include "project/project.h"
#include "support/result.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace walk::package {

struct LockEntry {
    std::string name;
    std::string version;
    std::string checksum;
};

[[nodiscard]] bool valid_package_name(const std::string& name);
[[nodiscard]] bool reserved_package_collection_root(const std::string& name);
[[nodiscard]] bool valid_semver(const std::string& version);
[[nodiscard]] Result<std::filesystem::path> init_package(const std::filesystem::path& package_path);
[[nodiscard]] Result<std::vector<LockEntry>> resolve_dependencies(const project::ProjectConfig& config, const std::filesystem::path& registry_root);
[[nodiscard]] Result<std::map<std::string, LockEntry>> read_lock_file(const std::filesystem::path& root);
[[nodiscard]] Result<void> write_lock_file(const std::filesystem::path& root, const std::vector<LockEntry>& entries);
[[nodiscard]] Result<std::vector<std::filesystem::path>> package_search_dirs(const project::ProjectConfig& config);
[[nodiscard]] Result<void> validate_publish_manifest(const project::ProjectConfig& config);
[[nodiscard]] Result<std::filesystem::path> publish_package(const project::ProjectConfig& config, const std::filesystem::path& registry_root);
[[nodiscard]] std::filesystem::path package_cache_root(const std::filesystem::path& root, const project::Dependency& dependency);
[[nodiscard]] Result<std::string> package_checksum(const std::filesystem::path& root);

}  // namespace walk::package
