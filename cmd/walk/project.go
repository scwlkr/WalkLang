package main

import (
	"bufio"
	"fmt"
	"io/fs"
	"os"
	"os/exec"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
	"unicode"

	walkfmt "walklang/internal/format"
)

const projectConfigName = "walk.toml"

type projectConfig struct {
	root    string
	name    string
	version string
	entry   string
	build   projectBuildConfig
}

type projectBuildConfig struct {
	output  string
	release bool
}

type projectBuildArgs struct {
	native      nativeBuildOptions
	warningMode warningMode
}

type projectCheckArgs struct {
	warningMode warningMode
}

func initCommand(args []string) error {
	if len(args) != 1 {
		return fmt.Errorf("usage: walk init <project-name>")
	}
	projectPath := filepath.Clean(args[0])
	name := filepath.Base(projectPath)
	if !validProjectName(name) {
		return fmt.Errorf("project name %q may contain only letters, numbers, underscore, and dash", name)
	}
	if _, err := os.Stat(projectPath); err == nil {
		return fmt.Errorf("project already exists: %s", projectPath)
	} else if !os.IsNotExist(err) {
		return fmt.Errorf("project check failed: %w", err)
	}
	if err := os.MkdirAll(filepath.Join(projectPath, "src"), 0o755); err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Join(projectPath, "tests"), 0o755); err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Join(projectPath, "build"), 0o755); err != nil {
		return err
	}
	files := map[string]string{
		projectConfigName:      initialProjectConfig(name),
		"src/main.walk":        initialMainSource(),
		"src/math_extra.walk":  initialModuleSource(),
		"tests/main_test.walk": initialTestSource(),
	}
	for rel, contents := range files {
		if err := os.WriteFile(filepath.Join(projectPath, rel), []byte(contents), 0o644); err != nil {
			return err
		}
	}
	fmt.Println(projectPath)
	return nil
}

func useProjectBuild(args []string) bool {
	if len(args) == 0 {
		return true
	}
	for i := 0; i < len(args); i++ {
		arg := args[i]
		switch {
		case arg == "--release":
			continue
		case arg == "--cc" || arg == "--cflag" || arg == "--warnings":
			i++
			if i >= len(args) {
				return false
			}
		case strings.HasPrefix(arg, "--warnings="):
			continue
		default:
			return false
		}
	}
	return true
}

func projectBuildCommand(args []string) error {
	parsed, err := parseProjectBuildArgs(args)
	if err != nil {
		return err
	}
	config, err := loadProjectConfigFromCwd()
	if err != nil {
		return err
	}
	entryPath, err := projectPath(config, config.entry)
	if err != nil {
		return err
	}
	outputPath, err := projectPath(config, config.build.output)
	if err != nil {
		return err
	}
	parsed.native.release = config.build.release || parsed.native.release
	cCode, warnings, err := compileFileToCWithSearchDirs(entryPath, projectSearchDirs(config, entryPath), false)
	if err != nil {
		return err
	}
	if err := handleWarnings(warnings, parsed.warningMode); err != nil {
		return err
	}
	if err := buildC(cCode, outputPath+".c", outputPath, parsed.native); err != nil {
		return err
	}
	fmt.Println(outputPath)
	return nil
}

func projectCheckCommand(args []string) error {
	parsed, err := parseProjectCheckArgs(args, "check")
	if err != nil {
		return err
	}
	config, err := loadProjectConfigFromCwd()
	if err != nil {
		return err
	}
	entryPath, err := projectPath(config, config.entry)
	if err != nil {
		return err
	}
	warnings, err := checkFileWithSearchDirs(entryPath, projectSearchDirs(config, entryPath))
	if err != nil {
		return err
	}
	if err := handleWarnings(warnings, parsed.warningMode); err != nil {
		return err
	}
	for _, testPath := range projectTestFiles(config) {
		warnings, err := checkFileWithSearchDirs(testPath, projectSearchDirs(config, testPath))
		if err != nil {
			return err
		}
		if err := handleWarnings(warnings, parsed.warningMode); err != nil {
			return err
		}
	}
	fmt.Println("ok")
	return nil
}

func useProjectCheckLike(args []string) bool {
	if len(args) == 0 {
		return true
	}
	for i := 0; i < len(args); i++ {
		arg := args[i]
		switch {
		case arg == "--warnings":
			i++
			if i >= len(args) {
				return false
			}
		case strings.HasPrefix(arg, "--warnings="):
			continue
		default:
			return false
		}
	}
	return true
}

func projectTestCommand(args []string) error {
	parsed, err := parseProjectCheckArgs(args, "test")
	if err != nil {
		return err
	}
	config, err := loadProjectConfigFromCwd()
	if err != nil {
		return err
	}
	testFiles := projectTestFiles(config)
	if len(testFiles) == 0 {
		fmt.Println("ok 0 test files")
		return nil
	}
	for _, testPath := range testFiles {
		if err := runProjectTestFile(config, testPath, parsed.warningMode); err != nil {
			return err
		}
	}
	return nil
}

func runProjectTestFile(config projectConfig, testPath string, mode warningMode) error {
	cCode, warnings, err := compileFileToCWithSearchDirs(testPath, projectSearchDirs(config, testPath), true)
	if err != nil {
		return err
	}
	if err := handleWarnings(warnings, mode); err != nil {
		return err
	}
	dir, err := os.MkdirTemp("", "walk-project-test-*")
	if err != nil {
		return err
	}
	defer os.RemoveAll(dir)
	exePath := filepath.Join(dir, "tests")
	if err := buildC(cCode, filepath.Join(dir, "tests.c"), exePath, nativeBuildOptions{}); err != nil {
		return err
	}
	command := exec.Command(exePath)
	command.Stdout = os.Stdout
	command.Stderr = os.Stderr
	if err := command.Run(); err != nil {
		return fmt.Errorf("tests failed: %w", err)
	}
	return nil
}

func projectFmtCommand(_ bool) error {
	config, err := loadProjectConfigFromCwd()
	if err != nil {
		return err
	}
	files := projectFormatFiles(config)
	for _, path := range files {
		source, err := os.ReadFile(path)
		if err != nil {
			return fmt.Errorf("source read failed: %w", err)
		}
		formatted, err := walkfmt.Format(string(source), path)
		if err != nil {
			return err
		}
		if string(source) != formatted {
			if err := os.WriteFile(path, []byte(formatted), 0o644); err != nil {
				return err
			}
		}
		rel, err := filepath.Rel(config.root, path)
		if err != nil {
			rel = path
		}
		fmt.Println(rel)
	}
	return nil
}

func cleanCommand(args []string) error {
	if len(args) != 0 {
		return fmt.Errorf("usage: walk clean")
	}
	config, err := loadProjectConfigFromCwd()
	if err != nil {
		return err
	}
	if firstProjectPathPart(config.build.output) == "build" {
		if err := os.RemoveAll(filepath.Join(config.root, "build")); err != nil {
			return err
		}
		fmt.Println("clean")
		return nil
	}
	outputPath, err := projectPath(config, config.build.output)
	if err != nil {
		return err
	}
	if err := os.Remove(outputPath); err != nil && !os.IsNotExist(err) {
		return err
	}
	if err := os.Remove(outputPath + ".c"); err != nil && !os.IsNotExist(err) {
		return err
	}
	fmt.Println("clean")
	return nil
}

func parseProjectBuildArgs(args []string) (projectBuildArgs, error) {
	config := projectBuildArgs{warningMode: warningDefault}
	for i := 0; i < len(args); i++ {
		switch args[i] {
		case "--release":
			config.native.release = true
		case "--cc":
			i++
			if i >= len(args) {
				return projectBuildArgs{}, fmt.Errorf("build requires a value after --cc")
			}
			config.native.cc = args[i]
		case "--cflag":
			i++
			if i >= len(args) {
				return projectBuildArgs{}, fmt.Errorf("build requires a value after --cflag")
			}
			config.native.cFlags = append(config.native.cFlags, args[i])
		default:
			if mode, ok, err := parseWarningArg(args, &i); ok || err != nil {
				if err != nil {
					return projectBuildArgs{}, err
				}
				config.warningMode = mode
				continue
			}
			return projectBuildArgs{}, fmt.Errorf("usage: walk build [--release] [--warnings=off|default|error]")
		}
	}
	return config, nil
}

func parseProjectCheckArgs(args []string, command string) (projectCheckArgs, error) {
	config := projectCheckArgs{warningMode: warningDefault}
	for i := 0; i < len(args); i++ {
		if mode, ok, err := parseWarningArg(args, &i); ok || err != nil {
			if err != nil {
				return projectCheckArgs{}, err
			}
			config.warningMode = mode
			continue
		}
		return projectCheckArgs{}, fmt.Errorf("usage: walk %s [--warnings=off|default|error]", command)
	}
	return config, nil
}

func loadProjectConfigFromCwd() (projectConfig, error) {
	wd, err := os.Getwd()
	if err != nil {
		return projectConfig{}, err
	}
	root, err := findProjectRoot(wd)
	if err != nil {
		return projectConfig{}, err
	}
	contents, err := os.ReadFile(filepath.Join(root, projectConfigName))
	if err != nil {
		return projectConfig{}, err
	}
	config, err := parseProjectConfig(string(contents), filepath.Join(root, projectConfigName))
	if err != nil {
		return projectConfig{}, err
	}
	config.root = root
	return config, nil
}

func findProjectRoot(start string) (string, error) {
	dir := filepath.Clean(start)
	for {
		if info, err := os.Stat(filepath.Join(dir, projectConfigName)); err == nil && !info.IsDir() {
			return dir, nil
		}
		parent := filepath.Dir(dir)
		if parent == dir {
			return "", fmt.Errorf("walk.toml not found; run walk init <project-name> or pass a .walk source file")
		}
		dir = parent
	}
}

func parseProjectConfig(contents string, filename string) (projectConfig, error) {
	config := projectConfig{
		version: "0.1.0",
		entry:   "src/main.walk",
	}
	section := ""
	scanner := bufio.NewScanner(strings.NewReader(contents))
	lineNumber := 0
	for scanner.Scan() {
		lineNumber++
		line := strings.TrimSpace(stripTomlComment(scanner.Text()))
		if line == "" {
			continue
		}
		if strings.HasPrefix(line, "[") && strings.HasSuffix(line, "]") {
			section = strings.TrimSpace(strings.TrimSuffix(strings.TrimPrefix(line, "["), "]"))
			if section != "build" {
				return projectConfig{}, fmt.Errorf("%s:%d: unknown section [%s]", filename, lineNumber, section)
			}
			continue
		}
		key, value, ok := strings.Cut(line, "=")
		if !ok {
			return projectConfig{}, fmt.Errorf("%s:%d: expected key = value", filename, lineNumber)
		}
		key = strings.TrimSpace(key)
		value = strings.TrimSpace(value)
		if err := assignProjectConfigValue(&config, section, key, value, filename, lineNumber); err != nil {
			return projectConfig{}, err
		}
	}
	if err := scanner.Err(); err != nil {
		return projectConfig{}, err
	}
	if config.name == "" {
		return projectConfig{}, fmt.Errorf("%s: name is required", filename)
	}
	if config.build.output == "" {
		config.build.output = filepath.ToSlash(filepath.Join("build", config.name))
	}
	return config, nil
}

func assignProjectConfigValue(config *projectConfig, section string, key string, value string, filename string, line int) error {
	switch section {
	case "":
		switch key {
		case "name":
			parsed, err := parseTomlString(value, filename, line)
			if err != nil {
				return err
			}
			if !validProjectName(parsed) {
				return fmt.Errorf("%s:%d: project name %q may contain only letters, numbers, underscore, and dash", filename, line, parsed)
			}
			config.name = parsed
		case "version":
			parsed, err := parseTomlString(value, filename, line)
			if err != nil {
				return err
			}
			config.version = parsed
		case "entry":
			parsed, err := parseTomlString(value, filename, line)
			if err != nil {
				return err
			}
			config.entry = parsed
		default:
			return fmt.Errorf("%s:%d: unknown project key %q", filename, line, key)
		}
	case "build":
		switch key {
		case "output":
			parsed, err := parseTomlString(value, filename, line)
			if err != nil {
				return err
			}
			config.build.output = parsed
		case "release":
			parsed, err := parseTomlBool(value, filename, line)
			if err != nil {
				return err
			}
			config.build.release = parsed
		default:
			return fmt.Errorf("%s:%d: unknown build key %q", filename, line, key)
		}
	}
	return nil
}

func parseTomlString(value string, filename string, line int) (string, error) {
	parsed, err := strconv.Unquote(value)
	if err != nil {
		return "", fmt.Errorf("%s:%d: expected quoted string", filename, line)
	}
	return parsed, nil
}

func parseTomlBool(value string, filename string, line int) (bool, error) {
	switch value {
	case "true":
		return true, nil
	case "false":
		return false, nil
	default:
		return false, fmt.Errorf("%s:%d: expected true or false", filename, line)
	}
}

func stripTomlComment(line string) string {
	inString := false
	escaped := false
	for i, r := range line {
		if escaped {
			escaped = false
			continue
		}
		if r == '\\' && inString {
			escaped = true
			continue
		}
		if r == '"' {
			inString = !inString
			continue
		}
		if r == '#' && !inString {
			return line[:i]
		}
	}
	return line
}

func projectPath(config projectConfig, rel string) (string, error) {
	if filepath.IsAbs(rel) {
		return "", fmt.Errorf("project path must be relative: %s", rel)
	}
	clean := filepath.Clean(rel)
	if clean == "." || clean == ".." || strings.HasPrefix(clean, ".."+string(os.PathSeparator)) {
		return "", fmt.Errorf("project path escapes project root: %s", rel)
	}
	return filepath.Join(config.root, clean), nil
}

func projectSearchDirs(config projectConfig, sourcePath string) []string {
	entryPath, err := projectPath(config, config.entry)
	if err != nil {
		return []string{filepath.Dir(sourcePath)}
	}
	return appendSearchDir([]string{filepath.Dir(sourcePath)}, filepath.Dir(entryPath))
}

func projectTestFiles(config projectConfig) []string {
	testsDir := filepath.Join(config.root, "tests")
	var files []string
	_ = filepath.WalkDir(testsDir, func(path string, entry fs.DirEntry, err error) error {
		if err != nil || entry.IsDir() {
			return nil
		}
		if strings.HasSuffix(entry.Name(), "_test.walk") {
			files = append(files, path)
		}
		return nil
	})
	sort.Strings(files)
	return files
}

func projectFormatFiles(config projectConfig) []string {
	entryPath, err := projectPath(config, config.entry)
	if err != nil {
		return nil
	}
	dirs := appendSearchDir(nil, filepath.Dir(entryPath), filepath.Join(config.root, "tests"))
	var files []string
	for _, dir := range dirs {
		_ = filepath.WalkDir(dir, func(path string, entry fs.DirEntry, err error) error {
			if err != nil || entry.IsDir() {
				return nil
			}
			if strings.HasSuffix(entry.Name(), ".walk") {
				files = append(files, path)
			}
			return nil
		})
	}
	sort.Strings(files)
	return files
}

func firstProjectPathPart(path string) string {
	parts := strings.Split(filepath.ToSlash(filepath.Clean(path)), "/")
	if len(parts) == 0 {
		return ""
	}
	return parts[0]
}

func validProjectName(name string) bool {
	if name == "" || name == "." || name == ".." {
		return false
	}
	for _, r := range name {
		if unicode.IsLetter(r) || unicode.IsDigit(r) || r == '_' || r == '-' {
			continue
		}
		return false
	}
	return true
}

func initialProjectConfig(name string) string {
	return fmt.Sprintf("name = %q\nversion = \"0.1.0\"\nentry = \"src/main.walk\"\n\n[build]\noutput = %q\nrelease = false\n", name, filepath.ToSlash(filepath.Join("build", name)))
}

func initialMainSource() string {
	return strings.Join([]string{
		"imp: math_extra",
		"",
		"out: math_extra.cube(3)",
		"",
	}, "\n")
}

func initialModuleSource() string {
	return strings.Join([]string{
		"func: cube(x int) int",
		"    return: * x x x",
		"",
		"exp: cube",
		"",
	}, "\n")
}

func initialTestSource() string {
	return strings.Join([]string{
		"imp: math_extra",
		"",
		"test: 'cube works'",
		"    assert: == math_extra.cube(3) 27",
		"",
	}, "\n")
}
