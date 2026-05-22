package main

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"sort"
	"strings"

	"walklang/internal/ast"
	"walklang/internal/checker"
	"walklang/internal/diagnostic"
	"walklang/internal/emitter"
	walkfmt "walklang/internal/format"
	"walklang/internal/parser"
)

var version = "dev"

func main() {
	if err := run(os.Args[1:]); err != nil {
		fmt.Fprintln(os.Stderr, diagnostic.FormatError(err))
		os.Exit(1)
	}
}

func run(args []string) error {
	if len(args) == 0 {
		return fmt.Errorf("usage: walk <init|build|check|test|fmt|clean|emit-c|repl|version>")
	}
	switch args[0] {
	case "init":
		return initCommand(args[1:])
	case "build":
		return build(args[1:])
	case "emit-c":
		return emitCCommand(args[1:])
	case "check":
		return checkCommand(args[1:])
	case "fmt":
		return fmtCommand(args[1:])
	case "test":
		return testCommand(args[1:])
	case "repl":
		return replCommand(args[1:])
	case "clean":
		return cleanCommand(args[1:])
	case "version":
		fmt.Println(version)
		return nil
	default:
		return fmt.Errorf("unknown command %q", args[0])
	}
}

func build(args []string) error {
	if useProjectBuild(args) {
		return projectBuildCommand(args)
	}
	config, err := parseBuildArgs(args)
	if err != nil {
		return err
	}
	if config.cOutput == "" {
		config.cOutput = config.output + ".c"
	}
	cCode, warnings, err := compileFileToCWithOptions(config.sourcePath, false)
	if err != nil {
		return err
	}
	if err := handleWarnings(warnings, config.warningMode); err != nil {
		return err
	}
	if err := buildC(cCode, config.cOutput, config.output, config.native); err != nil {
		return err
	}
	fmt.Println(config.output)
	return nil
}

func emitCCommand(args []string) error {
	config, err := parseEmitCArgs(args)
	if err != nil {
		return err
	}

	cCode, warnings, err := compileFileToCWithOptions(config.sourcePath, false)
	if err != nil {
		return err
	}
	if err := handleWarnings(warnings, config.warningMode); err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(config.output), 0o755); err != nil {
		return err
	}
	if err := os.WriteFile(config.output, []byte(cCode), 0o644); err != nil {
		return err
	}
	fmt.Println(config.output)
	return nil
}

func fmtCommand(args []string) error {
	write := false
	var sourcePath string
	for _, arg := range args {
		if arg == "-w" {
			write = true
			continue
		}
		if sourcePath != "" {
			return fmt.Errorf("usage: walk fmt [-w] <source.walk>")
		}
		sourcePath = arg
	}
	if sourcePath == "" {
		return projectFmtCommand(write)
	}
	source, err := os.ReadFile(sourcePath)
	if err != nil {
		return fmt.Errorf("source read failed: %w", err)
	}
	formatted, err := walkfmt.Format(string(source), sourcePath)
	if err != nil {
		return err
	}
	if write {
		return os.WriteFile(sourcePath, []byte(formatted), 0o644)
	}
	fmt.Print(formatted)
	return nil
}

func testCommand(args []string) error {
	if useProjectCheckLike(args) {
		return projectTestCommand(args)
	}
	config, err := parseCheckLikeArgs(args, "test")
	if err != nil {
		return err
	}
	cCode, warnings, err := compileFileToCWithOptions(config.sourcePath, true)
	if err != nil {
		return err
	}
	if err := handleWarnings(warnings, config.warningMode); err != nil {
		return err
	}
	dir, err := os.MkdirTemp("", "walk-test-*")
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

func checkCommand(args []string) error {
	if useProjectCheckLike(args) {
		return projectCheckCommand(args)
	}
	config, err := parseCheckLikeArgs(args, "check")
	if err != nil {
		return err
	}
	warnings, err := checkFile(config.sourcePath)
	if err != nil {
		return err
	}
	if err := handleWarnings(warnings, config.warningMode); err != nil {
		return err
	}
	fmt.Println("ok")
	return nil
}

func replCommand(args []string) error {
	if len(args) != 0 {
		return fmt.Errorf("usage: walk repl")
	}
	scanner := bufio.NewScanner(os.Stdin)
	for {
		fmt.Print("walk> ")
		if !scanner.Scan() {
			return scanner.Err()
		}
		line := strings.TrimSpace(scanner.Text())
		if line == "" {
			continue
		}
		if line == ":quit" || line == ":exit" {
			return nil
		}
		output, err := runSource(replSource(line), "<repl>")
		if err != nil {
			fmt.Fprintln(os.Stderr, diagnostic.FormatError(err))
			continue
		}
		fmt.Print(output)
	}
}

func replSource(expression string) string {
	return strings.Join([]string{
		"imp: math",
		"imp: string",
		"imp: array",
		"imp: random",
		"imp: time",
		"imp: testing",
		"out: " + expression,
		"",
	}, "\n")
}

type warningMode string

const (
	warningDefault warningMode = "default"
	warningOff     warningMode = "off"
	warningError   warningMode = "error"
)

type nativeBuildOptions struct {
	cc      string
	release bool
	cFlags  []string
}

type buildConfig struct {
	sourcePath  string
	output      string
	cOutput     string
	native      nativeBuildOptions
	warningMode warningMode
}

type emitCConfig struct {
	sourcePath  string
	output      string
	warningMode warningMode
}

type sourceCheckConfig struct {
	sourcePath  string
	warningMode warningMode
}

func parseBuildArgs(args []string) (buildConfig, error) {
	config := buildConfig{warningMode: warningDefault}
	for i := 0; i < len(args); i++ {
		switch args[i] {
		case "-o":
			i++
			if i >= len(args) {
				return buildConfig{}, fmt.Errorf("build requires a value after -o")
			}
			config.output = args[i]
		case "--emit-c":
			i++
			if i >= len(args) {
				return buildConfig{}, fmt.Errorf("build requires a value after --emit-c")
			}
			config.cOutput = args[i]
		case "--release":
			config.native.release = true
		case "--cc":
			i++
			if i >= len(args) {
				return buildConfig{}, fmt.Errorf("build requires a value after --cc")
			}
			config.native.cc = args[i]
		case "--cflag":
			i++
			if i >= len(args) {
				return buildConfig{}, fmt.Errorf("build requires a value after --cflag")
			}
			config.native.cFlags = append(config.native.cFlags, args[i])
		default:
			if mode, ok, err := parseWarningArg(args, &i); ok || err != nil {
				if err != nil {
					return buildConfig{}, err
				}
				config.warningMode = mode
				continue
			}
			if config.sourcePath != "" {
				return buildConfig{}, fmt.Errorf("usage: walk build <source.walk> -o <output>")
			}
			config.sourcePath = args[i]
		}
	}
	if config.sourcePath == "" || config.output == "" {
		return buildConfig{}, fmt.Errorf("usage: walk build <source.walk> -o <output>")
	}
	return config, nil
}

func parseEmitCArgs(args []string) (emitCConfig, error) {
	config := emitCConfig{warningMode: warningDefault}
	for i := 0; i < len(args); i++ {
		switch args[i] {
		case "-o":
			i++
			if i >= len(args) {
				return emitCConfig{}, fmt.Errorf("emit-c requires a value after -o")
			}
			config.output = args[i]
		default:
			if mode, ok, err := parseWarningArg(args, &i); ok || err != nil {
				if err != nil {
					return emitCConfig{}, err
				}
				config.warningMode = mode
				continue
			}
			if config.sourcePath != "" {
				return emitCConfig{}, fmt.Errorf("usage: walk emit-c <source.walk> -o <output.c>")
			}
			config.sourcePath = args[i]
		}
	}
	if config.sourcePath == "" || config.output == "" {
		return emitCConfig{}, fmt.Errorf("usage: walk emit-c <source.walk> -o <output.c>")
	}
	return config, nil
}

func parseCheckLikeArgs(args []string, command string) (sourceCheckConfig, error) {
	config := sourceCheckConfig{warningMode: warningDefault}
	for i := 0; i < len(args); i++ {
		if mode, ok, err := parseWarningArg(args, &i); ok || err != nil {
			if err != nil {
				return sourceCheckConfig{}, err
			}
			config.warningMode = mode
			continue
		}
		if config.sourcePath != "" {
			return sourceCheckConfig{}, fmt.Errorf("usage: walk %s [--warnings=off|default|error] <source.walk>", command)
		}
		config.sourcePath = args[i]
	}
	if config.sourcePath == "" {
		return sourceCheckConfig{}, fmt.Errorf("usage: walk %s [--warnings=off|default|error] <source.walk>", command)
	}
	return config, nil
}

func parseWarningArg(args []string, index *int) (warningMode, bool, error) {
	arg := args[*index]
	if arg == "--warnings" {
		*index = *index + 1
		if *index >= len(args) {
			return "", true, fmt.Errorf("--warnings requires off, default, or error")
		}
		mode, err := parseWarningMode(args[*index])
		return mode, true, err
	}
	if strings.HasPrefix(arg, "--warnings=") {
		mode, err := parseWarningMode(strings.TrimPrefix(arg, "--warnings="))
		return mode, true, err
	}
	return "", false, nil
}

func parseWarningMode(value string) (warningMode, error) {
	switch warningMode(value) {
	case warningOff, warningDefault, warningError:
		return warningMode(value), nil
	default:
		return "", fmt.Errorf("unknown warning mode %q", value)
	}
}

func handleWarnings(warnings []checker.Warning, mode warningMode) error {
	if mode == warningOff {
		return nil
	}
	for _, warning := range warnings {
		fmt.Fprintln(os.Stderr, diagnostic.FormatWarning(warning.Location, warning.Message))
	}
	if mode == warningError && len(warnings) > 0 {
		return fmt.Errorf("warnings-as-errors: %d warning(s)", len(warnings))
	}
	return nil
}

func compileToC(sourcePath string) (string, error) {
	cCode, _, err := compileFileToCWithOptions(sourcePath, false)
	return cCode, err
}

func compileFileToC(sourcePath string, testsOnly bool) (string, error) {
	cCode, _, err := compileFileToCWithOptions(sourcePath, testsOnly)
	return cCode, err
}

func compileFileToCWithOptions(sourcePath string, testsOnly bool) (string, []checker.Warning, error) {
	return compileFileToCWithSearchDirs(sourcePath, nil, testsOnly)
}

func compileFileToCWithSearchDirs(sourcePath string, searchDirs []string, testsOnly bool) (string, []checker.Warning, error) {
	program, modules, err := loadProgramWithSearchDirs(sourcePath, searchDirs)
	if err != nil {
		return "", nil, err
	}
	warnings, err := checkPrograms(program, modules)
	if err != nil {
		return "", warnings, err
	}
	modulePrograms := moduleProgramMap(modules)
	if testsOnly {
		cCode, err := emitter.EmitTestCWithModules(program, modulePrograms)
		return cCode, warnings, err
	}
	cCode, err := emitter.EmitCWithModules(program, modulePrograms)
	return cCode, warnings, err
}

func checkFile(sourcePath string) ([]checker.Warning, error) {
	program, modules, err := loadProgramWithSearchDirs(sourcePath, nil)
	if err != nil {
		return nil, err
	}
	return checkPrograms(program, modules)
}

func checkFileWithSearchDirs(sourcePath string, searchDirs []string) ([]checker.Warning, error) {
	program, modules, err := loadProgramWithSearchDirs(sourcePath, searchDirs)
	if err != nil {
		return nil, err
	}
	return checkPrograms(program, modules)
}

func compileSourceToC(source string, filename string, testsOnly bool) (string, error) {
	program, err := parser.ParseSource(source, filename)
	if err != nil {
		return "", err
	}
	warnings, err := checker.CheckWithOptions(program, checker.Options{})
	if err != nil {
		return "", err
	}
	if err := handleWarnings(warnings, warningDefault); err != nil {
		return "", err
	}
	if testsOnly {
		return emitter.EmitTestC(program)
	}
	return emitter.EmitC(program)
}

func loadProgram(sourcePath string) (*ast.Program, map[string]*checker.Module, error) {
	return loadProgramWithSearchDirs(sourcePath, nil)
}

func loadProgramWithSearchDirs(sourcePath string, searchDirs []string) (*ast.Program, map[string]*checker.Module, error) {
	source, err := os.ReadFile(sourcePath)
	if err != nil {
		return nil, nil, fmt.Errorf("source read failed: %w", err)
	}
	program, err := parser.ParseSource(string(source), sourcePath)
	if err != nil {
		return nil, nil, err
	}
	loader := moduleLoader{
		modules:    map[string]*checker.Module{},
		loading:    map[string]bool{},
		searchDirs: cleanSearchDirs(searchDirs),
	}
	if err := loader.loadImports(program, filepath.Dir(sourcePath)); err != nil {
		return nil, nil, err
	}
	return program, loader.modules, nil
}

type moduleLoader struct {
	modules    map[string]*checker.Module
	loading    map[string]bool
	searchDirs []string
}

func (l *moduleLoader) loadImports(program *ast.Program, baseDir string) error {
	for _, imp := range imports(program) {
		if checker.IsBuiltinModule(imp.Module) {
			continue
		}
		if l.loading[imp.Module] {
			return errorAt(imp.Location, "module error: import cycle includes %s", imp.Module)
		}
		if _, ok := l.modules[imp.Module]; ok {
			continue
		}
		modulePath, ok := l.findModulePath(imp.Module, baseDir)
		if !ok {
			if len(l.searchDirs) == 0 {
				return errorAt(imp.Location, "module error: module %s not found at %s", imp.Module, filepath.Join(baseDir, imp.Module+".walk"))
			}
			return errorAt(imp.Location, "module error: module %s not found in project search paths", imp.Module)
		}
		source, err := os.ReadFile(modulePath)
		if err != nil {
			return errorAt(imp.Location, "module error: module %s could not be read at %s", imp.Module, modulePath)
		}
		moduleProgram, err := parser.ParseSource(string(source), modulePath)
		if err != nil {
			return err
		}
		if err := validateModuleSurface(moduleProgram); err != nil {
			return err
		}
		l.modules[imp.Module] = &checker.Module{Name: imp.Module, Program: moduleProgram}
		l.loading[imp.Module] = true
		if err := l.loadImports(moduleProgram, filepath.Dir(modulePath)); err != nil {
			return err
		}
		delete(l.loading, imp.Module)
	}
	return nil
}

func (l *moduleLoader) findModulePath(module string, baseDir string) (string, bool) {
	for _, dir := range appendSearchDir([]string{baseDir}, l.searchDirs...) {
		path := filepath.Join(dir, module+".walk")
		if info, err := os.Stat(path); err == nil && !info.IsDir() {
			return path, true
		}
	}
	return "", false
}

func cleanSearchDirs(dirs []string) []string {
	return appendSearchDir(nil, dirs...)
}

func appendSearchDir(existing []string, dirs ...string) []string {
	result := make([]string, 0, len(existing)+len(dirs))
	seen := map[string]bool{}
	add := func(dir string) {
		if dir == "" {
			return
		}
		clean := filepath.Clean(dir)
		if seen[clean] {
			return
		}
		seen[clean] = true
		result = append(result, clean)
	}
	for _, dir := range existing {
		add(dir)
	}
	for _, dir := range dirs {
		add(dir)
	}
	return result
}

func validateModuleSurface(program *ast.Program) error {
	for _, statement := range program.Statements {
		switch statement.(type) {
		case *ast.Import, *ast.FuncDecl, *ast.StructDecl, *ast.Export:
			continue
		default:
			return errorAt(statement.Loc(), "module error: modules may contain only imp, struct, func, and exp at top level")
		}
	}
	return nil
}

func imports(program *ast.Program) []*ast.Import {
	var result []*ast.Import
	for _, statement := range program.Statements {
		if imp, ok := statement.(*ast.Import); ok {
			result = append(result, imp)
		}
	}
	return result
}

func checkPrograms(program *ast.Program, modules map[string]*checker.Module) ([]checker.Warning, error) {
	var warnings []checker.Warning
	structs, err := collectStructDefinitions(program, modules)
	if err != nil {
		return warnings, err
	}
	checked := map[string]bool{}
	var checkModule func(string) error
	checkModule = func(name string) error {
		if checked[name] {
			return nil
		}
		module := modules[name]
		for _, imp := range imports(module.Program) {
			if !checker.IsBuiltinModule(imp.Module) {
				if err := checkModule(imp.Module); err != nil {
					return err
				}
			}
		}
		moduleWarnings, err := checker.CheckWithOptions(module.Program, checker.Options{Modules: modules, Structs: structs})
		warnings = append(warnings, moduleWarnings...)
		if err != nil {
			return err
		}
		exports, err := checker.ExportedFunctions(module.Program)
		if err != nil {
			return err
		}
		module.Exports = exports
		checked[name] = true
		return nil
	}

	names := make([]string, 0, len(modules))
	for name := range modules {
		names = append(names, name)
	}
	sort.Strings(names)
	for _, name := range names {
		if err := checkModule(name); err != nil {
			return warnings, err
		}
	}

	entryWarnings, err := checker.CheckWithOptions(program, checker.Options{Modules: modules, Structs: structs})
	warnings = append(warnings, entryWarnings...)
	return warnings, err
}

func collectStructDefinitions(program *ast.Program, modules map[string]*checker.Module) (map[string]checker.StructDef, error) {
	structs := map[string]checker.StructDef{}
	add := func(program *ast.Program) error {
		local, err := checker.StructDefinitions(program)
		if err != nil {
			return err
		}
		for name, def := range local {
			if _, exists := structs[name]; exists {
				return errorAt(def.Location, "type error: struct %s is already defined", name)
			}
			structs[name] = def
		}
		return nil
	}
	names := make([]string, 0, len(modules))
	for name := range modules {
		names = append(names, name)
	}
	sort.Strings(names)
	for _, name := range names {
		if err := add(modules[name].Program); err != nil {
			return nil, err
		}
	}
	if err := add(program); err != nil {
		return nil, err
	}
	return structs, nil
}

func moduleProgramMap(modules map[string]*checker.Module) map[string]*ast.Program {
	result := map[string]*ast.Program{}
	for name, module := range modules {
		result[name] = module.Program
	}
	return result
}

func buildC(cCode string, cPath string, output string, options nativeBuildOptions) error {
	if err := os.MkdirAll(filepath.Dir(cPath), 0o755); err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(output), 0o755); err != nil {
		return err
	}
	if err := os.WriteFile(cPath, []byte(cCode), 0o644); err != nil {
		return err
	}
	cc := options.cc
	if cc == "" {
		cc = os.Getenv("WALK_CC")
	}
	if cc == "" {
		cc = "cc"
	}
	command := exec.Command(cc, nativeBuildArgs(cPath, output, options)...)
	result, err := command.CombinedOutput()
	if err != nil {
		return fmt.Errorf("native build failed: %s", string(result))
	}
	return nil
}

func nativeBuildArgs(cPath string, output string, options nativeBuildOptions) []string {
	args := []string{cPath, "-o", output}
	if options.release {
		args = append(args, "-O2", "-DNDEBUG")
	}
	args = append(args, options.cFlags...)
	args = append(args, "-lm")
	return args
}

func errorAt(location ast.Location, format string, args ...any) error {
	return diagnostic.Errorf(location, format, args...)
}

func runSource(source string, filename string) (string, error) {
	cCode, err := compileSourceToC(source, filename, false)
	if err != nil {
		return "", err
	}
	dir, err := os.MkdirTemp("", "walk-repl-*")
	if err != nil {
		return "", err
	}
	defer os.RemoveAll(dir)
	exePath := filepath.Join(dir, "repl")
	if err := buildC(cCode, filepath.Join(dir, "repl.c"), exePath, nativeBuildOptions{}); err != nil {
		return "", err
	}
	output, err := exec.Command(exePath).CombinedOutput()
	if err != nil {
		return "", fmt.Errorf("program failed: %s", string(output))
	}
	return string(output), nil
}
