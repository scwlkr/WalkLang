package main

import (
	"bufio"
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"unicode"
)

const packageLockName = "walk.lock"

type projectDependency struct {
	name    string
	version string
}

type packageLockEntry struct {
	name     string
	version  string
	checksum string
}

func packageCommand(args []string) error {
	if len(args) == 0 {
		return fmt.Errorf("usage: walk package <init|resolve|publish>")
	}
	switch args[0] {
	case "init":
		return packageInitCommand(args[1:])
	case "resolve":
		return packageResolveCommand(args[1:])
	case "publish":
		return packagePublishCommand(args[1:])
	default:
		return fmt.Errorf("unknown package command %q", args[0])
	}
}

func packageInitCommand(args []string) error {
	if len(args) != 1 {
		return fmt.Errorf("usage: walk package init <package-name>")
	}
	projectPath := filepath.Clean(args[0])
	name := filepath.Base(projectPath)
	if !validPackageName(name) {
		return fmt.Errorf("package name %q may contain only letters, numbers, and underscore, and must start with a letter or underscore", name)
	}
	if _, err := os.Stat(projectPath); err == nil {
		return fmt.Errorf("package already exists: %s", projectPath)
	} else if !os.IsNotExist(err) {
		return fmt.Errorf("package check failed: %w", err)
	}
	for _, dir := range []string{
		filepath.Join(projectPath, "src", name),
		filepath.Join(projectPath, "tests"),
		filepath.Join(projectPath, "build"),
	} {
		if err := os.MkdirAll(dir, 0o755); err != nil {
			return err
		}
	}
	files := map[string]string{
		projectConfigName:                       initialPackageConfig(name),
		"README.md":                             initialPackageReadme(name),
		"src/main.walk":                         initialPackageMainSource(name),
		filepath.Join("src", name, "core.walk"): initialPackageModuleSource(),
		"tests/main_test.walk":                  initialPackageTestSource(name),
	}
	for rel, contents := range files {
		if err := os.WriteFile(filepath.Join(projectPath, rel), []byte(contents), 0o644); err != nil {
			return err
		}
	}
	fmt.Println(projectPath)
	return nil
}

func packageResolveCommand(args []string) error {
	if len(args) != 1 {
		return fmt.Errorf("usage: walk package resolve <registry-dir>")
	}
	config, err := loadProjectConfigFromCwd()
	if err != nil {
		return err
	}
	if len(config.dependencies) == 0 {
		fmt.Println("ok 0 dependencies")
		return nil
	}
	registryRoot := filepath.Clean(args[0])
	entries := map[string]packageLockEntry{}
	for _, dependency := range config.dependencies {
		if err := resolvePackageDependency(config, registryRoot, dependency, entries); err != nil {
			return err
		}
	}
	if err := writePackageLock(config.root, sortedPackageLockEntries(entries)); err != nil {
		return err
	}
	fmt.Printf("resolved %d package(s)\n", len(entries))
	return nil
}

func resolvePackageDependency(config projectConfig, registryRoot string, dependency projectDependency, entries map[string]packageLockEntry) error {
	if existing, ok := entries[dependency.name]; ok {
		if existing.version != dependency.version {
			return fmt.Errorf("package error: dependency %s is required at both %s and %s", dependency.name, existing.version, dependency.version)
		}
		return nil
	}
	sourceRoot := filepath.Join(registryRoot, dependency.name, dependency.version)
	info, err := os.Stat(sourceRoot)
	if err != nil {
		if os.IsNotExist(err) {
			return fmt.Errorf("package error: dependency %s@%s not found in registry %s", dependency.name, dependency.version, registryRoot)
		}
		return err
	}
	if !info.IsDir() {
		return fmt.Errorf("package error: dependency %s@%s is not a directory", dependency.name, dependency.version)
	}
	dependencyConfig, err := loadPackageConfigAt(sourceRoot)
	if err != nil {
		return err
	}
	if dependencyConfig.name != dependency.name || dependencyConfig.version != dependency.version {
		return fmt.Errorf("package error: registry package %s@%s has manifest %s@%s", dependency.name, dependency.version, dependencyConfig.name, dependencyConfig.version)
	}
	cacheRoot := packageCacheRoot(config.root, dependency)
	if err := os.RemoveAll(cacheRoot); err != nil {
		return err
	}
	if err := copyPackageTree(sourceRoot, cacheRoot); err != nil {
		return err
	}
	checksum, err := packageChecksum(cacheRoot)
	if err != nil {
		return err
	}
	entries[dependency.name] = packageLockEntry{
		name:     dependency.name,
		version:  dependency.version,
		checksum: "sha256:" + checksum,
	}
	for _, child := range dependencyConfig.dependencies {
		if err := resolvePackageDependency(config, registryRoot, child, entries); err != nil {
			return err
		}
	}
	return nil
}

func packagePublishCommand(args []string) error {
	if len(args) != 1 {
		return fmt.Errorf("usage: walk package publish <registry-dir>")
	}
	config, err := loadProjectConfigFromCwd()
	if err != nil {
		return err
	}
	if !validPackageName(config.name) {
		return fmt.Errorf("package error: package name %q may contain only letters, numbers, and underscore, and must start with a letter or underscore", config.name)
	}
	if !validSemver(config.version) {
		return fmt.Errorf("package error: package version %q must be MAJOR.MINOR.PATCH", config.version)
	}
	if err := requirePackageDocs(config.root); err != nil {
		return err
	}
	if err := projectCheckCommand([]string{"--warnings=error"}); err != nil {
		return fmt.Errorf("package check failed: %w", err)
	}
	if err := projectTestCommand([]string{"--warnings=error"}); err != nil {
		return fmt.Errorf("package tests failed: %w", err)
	}
	registryRoot := filepath.Clean(args[0])
	destination := filepath.Join(registryRoot, config.name, config.version)
	if _, err := os.Stat(destination); err == nil {
		return fmt.Errorf("package error: %s@%s already exists in registry", config.name, config.version)
	} else if !os.IsNotExist(err) {
		return err
	}
	tmp := destination + ".tmp"
	if err := os.RemoveAll(tmp); err != nil {
		return err
	}
	if pathInside(config.root, tmp) {
		return fmt.Errorf("package error: registry destination must not be inside the package being published")
	}
	if err := copyPackageTree(config.root, tmp); err != nil {
		_ = os.RemoveAll(tmp)
		return err
	}
	if err := os.MkdirAll(filepath.Dir(destination), 0o755); err != nil {
		_ = os.RemoveAll(tmp)
		return err
	}
	if err := os.Rename(tmp, destination); err != nil {
		_ = os.RemoveAll(tmp)
		return err
	}
	fmt.Println(destination)
	return nil
}

func packageSearchDirs(config projectConfig) ([]string, error) {
	if len(config.dependencies) == 0 {
		return nil, nil
	}
	entries, err := readPackageLock(config.root)
	if err != nil {
		if os.IsNotExist(err) {
			return nil, fmt.Errorf("package error: dependencies are not locked; run walk package resolve <registry-dir>")
		}
		return nil, err
	}
	for _, dependency := range config.dependencies {
		entry, ok := entries[dependency.name]
		if !ok {
			return nil, fmt.Errorf("package error: dependency %s@%s is not locked; run walk package resolve <registry-dir>", dependency.name, dependency.version)
		}
		if entry.version != dependency.version {
			return nil, fmt.Errorf("package error: dependency %s@%s does not match walk.lock %s; run walk package resolve <registry-dir>", dependency.name, dependency.version, entry.version)
		}
	}
	var dirs []string
	for _, entry := range sortedPackageLockEntries(entries) {
		cacheRoot := packageCacheRoot(config.root, projectDependency{name: entry.name, version: entry.version})
		checksum, err := packageChecksum(cacheRoot)
		if err != nil {
			return nil, fmt.Errorf("package error: dependency %s@%s cache is unavailable: %w", entry.name, entry.version, err)
		}
		if entry.checksum != "sha256:"+checksum {
			return nil, fmt.Errorf("package error: dependency %s@%s cache does not match walk.lock; run walk package resolve <registry-dir>", entry.name, entry.version)
		}
		dirs = append(dirs, filepath.Join(cacheRoot, "src"))
	}
	return dirs, nil
}

func loadPackageConfigAt(root string) (projectConfig, error) {
	contents, err := os.ReadFile(filepath.Join(root, projectConfigName))
	if err != nil {
		return projectConfig{}, fmt.Errorf("package manifest read failed: %w", err)
	}
	config, err := parseProjectConfig(string(contents), filepath.Join(root, projectConfigName))
	if err != nil {
		return projectConfig{}, err
	}
	config.root = root
	return config, nil
}

func writePackageLock(root string, entries []packageLockEntry) error {
	var builder strings.Builder
	builder.WriteString("# Generated by walk package resolve. Do not edit.\n")
	for _, entry := range entries {
		builder.WriteString("\n[[package]]\n")
		builder.WriteString(fmt.Sprintf("name = %q\n", entry.name))
		builder.WriteString(fmt.Sprintf("version = %q\n", entry.version))
		builder.WriteString(fmt.Sprintf("checksum = %q\n", entry.checksum))
	}
	return os.WriteFile(filepath.Join(root, packageLockName), []byte(builder.String()), 0o644)
}

func readPackageLock(root string) (map[string]packageLockEntry, error) {
	path := filepath.Join(root, packageLockName)
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	entries := map[string]packageLockEntry{}
	current := packageLockEntry{}
	inPackage := false
	flush := func(line int) error {
		if !inPackage {
			return nil
		}
		if current.name == "" || current.version == "" || current.checksum == "" {
			return fmt.Errorf("%s:%d: incomplete package lock entry", path, line)
		}
		if !validPackageName(current.name) {
			return fmt.Errorf("%s:%d: invalid package name %q", path, line, current.name)
		}
		if !validSemver(current.version) {
			return fmt.Errorf("%s:%d: invalid package version %q", path, line, current.version)
		}
		if !strings.HasPrefix(current.checksum, "sha256:") {
			return fmt.Errorf("%s:%d: invalid package checksum", path, line)
		}
		if _, exists := entries[current.name]; exists {
			return fmt.Errorf("%s:%d: duplicate package %q", path, line, current.name)
		}
		entries[current.name] = current
		current = packageLockEntry{}
		return nil
	}

	scanner := bufio.NewScanner(file)
	lineNumber := 0
	for scanner.Scan() {
		lineNumber++
		line := strings.TrimSpace(stripTomlComment(scanner.Text()))
		if line == "" {
			continue
		}
		if line == "[[package]]" {
			if err := flush(lineNumber); err != nil {
				return nil, err
			}
			inPackage = true
			continue
		}
		if !inPackage {
			return nil, fmt.Errorf("%s:%d: expected [[package]]", path, lineNumber)
		}
		key, value, ok := strings.Cut(line, "=")
		if !ok {
			return nil, fmt.Errorf("%s:%d: expected key = value", path, lineNumber)
		}
		parsed, err := parseTomlString(strings.TrimSpace(value), path, lineNumber)
		if err != nil {
			return nil, err
		}
		switch strings.TrimSpace(key) {
		case "name":
			current.name = parsed
		case "version":
			current.version = parsed
		case "checksum":
			current.checksum = parsed
		default:
			return nil, fmt.Errorf("%s:%d: unknown package lock key %q", path, lineNumber, strings.TrimSpace(key))
		}
	}
	if err := scanner.Err(); err != nil {
		return nil, err
	}
	if err := flush(lineNumber + 1); err != nil {
		return nil, err
	}
	return entries, nil
}

func sortedPackageLockEntries(entries map[string]packageLockEntry) []packageLockEntry {
	names := make([]string, 0, len(entries))
	for name := range entries {
		names = append(names, name)
	}
	sort.Strings(names)
	result := make([]packageLockEntry, 0, len(names))
	for _, name := range names {
		result = append(result, entries[name])
	}
	return result
}

func packageCacheRoot(root string, dependency projectDependency) string {
	return filepath.Join(root, ".walk", "packages", dependency.name, dependency.version)
}

func packageChecksum(root string) (string, error) {
	var files []string
	if err := filepath.WalkDir(root, func(path string, entry os.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if path == root {
			return nil
		}
		if shouldSkipPackagePath(entry) {
			if entry.IsDir() {
				return filepath.SkipDir
			}
			return nil
		}
		if entry.IsDir() {
			return nil
		}
		rel, err := filepath.Rel(root, path)
		if err != nil {
			return err
		}
		files = append(files, filepath.ToSlash(rel))
		return nil
	}); err != nil {
		return "", err
	}
	sort.Strings(files)
	hash := sha256.New()
	for _, rel := range files {
		hash.Write([]byte(rel))
		hash.Write([]byte{0})
		contents, err := os.ReadFile(filepath.Join(root, filepath.FromSlash(rel)))
		if err != nil {
			return "", err
		}
		hash.Write(contents)
		hash.Write([]byte{0})
	}
	return hex.EncodeToString(hash.Sum(nil)), nil
}

func copyPackageTree(sourceRoot string, destinationRoot string) error {
	if err := os.MkdirAll(destinationRoot, 0o755); err != nil {
		return err
	}
	return filepath.WalkDir(sourceRoot, func(path string, entry os.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if path == sourceRoot {
			return nil
		}
		if shouldSkipPackagePath(entry) {
			if entry.IsDir() {
				return filepath.SkipDir
			}
			return nil
		}
		rel, err := filepath.Rel(sourceRoot, path)
		if err != nil {
			return err
		}
		target := filepath.Join(destinationRoot, rel)
		info, err := entry.Info()
		if err != nil {
			return err
		}
		if entry.IsDir() {
			return os.MkdirAll(target, info.Mode().Perm())
		}
		if err := os.MkdirAll(filepath.Dir(target), 0o755); err != nil {
			return err
		}
		return copyFile(path, target, info.Mode().Perm())
	})
}

func copyFile(source string, destination string, mode os.FileMode) error {
	input, err := os.Open(source)
	if err != nil {
		return err
	}
	defer input.Close()
	output, err := os.OpenFile(destination, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, mode)
	if err != nil {
		return err
	}
	defer output.Close()
	_, err = io.Copy(output, input)
	return err
}

func shouldSkipPackagePath(entry os.DirEntry) bool {
	name := entry.Name()
	if entry.IsDir() {
		return name == "build" || name == ".walk" || name == ".git"
	}
	return name == packageLockName || strings.HasSuffix(name, ".tmp")
}

func requirePackageDocs(root string) error {
	path := filepath.Join(root, "README.md")
	contents, err := os.ReadFile(path)
	if err != nil {
		if os.IsNotExist(err) {
			return fmt.Errorf("package error: README.md is required before publish")
		}
		return err
	}
	if strings.TrimSpace(string(contents)) == "" {
		return fmt.Errorf("package error: README.md is required before publish")
	}
	return nil
}

func pathInside(root string, path string) bool {
	absRoot, err := filepath.Abs(root)
	if err != nil {
		return false
	}
	absPath, err := filepath.Abs(path)
	if err != nil {
		return false
	}
	rel, err := filepath.Rel(absRoot, absPath)
	if err != nil {
		return false
	}
	return rel == "." || (!strings.HasPrefix(rel, "..") && !filepath.IsAbs(rel))
}

func validPackageName(name string) bool {
	if name == "" {
		return false
	}
	for i, r := range name {
		if i == 0 {
			if !unicode.IsLetter(r) && r != '_' {
				return false
			}
			continue
		}
		if unicode.IsLetter(r) || unicode.IsDigit(r) || r == '_' {
			continue
		}
		return false
	}
	return true
}

func validSemver(version string) bool {
	parts := strings.Split(version, ".")
	if len(parts) != 3 {
		return false
	}
	for _, part := range parts {
		if part == "" {
			return false
		}
		for _, r := range part {
			if r < '0' || r > '9' {
				return false
			}
		}
	}
	return true
}

func initialPackageConfig(name string) string {
	return fmt.Sprintf("name = %q\nversion = \"0.1.0\"\nentry = \"src/main.walk\"\n\n[build]\noutput = %q\nrelease = false\n\n[dependencies]\n", name, filepath.ToSlash(filepath.Join("build", name)))
}

func initialPackageReadme(name string) string {
	return fmt.Sprintf("# %s\n\nA WalkLang package.\n", name)
}

func initialPackageMainSource(name string) string {
	return strings.Join([]string{
		"imp: " + name + ".core",
		"",
		"out: " + name + ".core.double(3)",
		"",
	}, "\n")
}

func initialPackageModuleSource() string {
	return strings.Join([]string{
		"func: double(x int) int",
		"    return: * x 2",
		"",
		"exp: double",
		"",
	}, "\n")
}

func initialPackageTestSource(name string) string {
	return strings.Join([]string{
		"imp: " + name + ".core",
		"",
		"test: 'double works'",
		"    assert: == " + name + ".core.double(3) 6",
		"",
	}, "\n")
}
