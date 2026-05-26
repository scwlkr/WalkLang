#pragma once

#include "support/result.h"

#include <filesystem>
#include <string>
#include <vector>

namespace walk::project {

struct Dependency {
    std::string name;
    std::string version;
};

struct BuildConfig {
    std::string output;
    bool release = false;
    bool release_set = false;
    bool mode_set = false;
    std::string mode = "debug";
};

struct ProjectConfig {
    std::filesystem::path root;
    std::string name;
    std::string version = "0.1.0";
    std::string entry = "src/main.walk";
    BuildConfig build;
    std::vector<Dependency> dependencies;
    std::vector<std::string> warnings;
};

[[nodiscard]] bool valid_project_name(const std::string& name);
[[nodiscard]] Result<ProjectConfig> parse_project_config(const std::string& contents, const std::string& filename);
[[nodiscard]] Result<ProjectConfig> load_project_config_from_cwd();
[[nodiscard]] Result<ProjectConfig> load_project_config_at(const std::filesystem::path& root);
[[nodiscard]] Result<std::filesystem::path> project_path(const ProjectConfig& config, const std::string& rel);
[[nodiscard]] std::vector<std::filesystem::path> local_search_dirs(const ProjectConfig& config, const std::filesystem::path& source_path);
[[nodiscard]] std::vector<std::filesystem::path> test_files(const ProjectConfig& config);
[[nodiscard]] std::vector<std::filesystem::path> format_files(const ProjectConfig& config);
[[nodiscard]] Result<std::string> format_source(const std::string& source, const std::string& filename);
[[nodiscard]] Result<std::filesystem::path> init_project(const std::filesystem::path& project_path);
[[nodiscard]] Result<void> clean_project(const ProjectConfig& config);
[[nodiscard]] std::string first_path_part(const std::string& path);

}  // namespace walk::project
