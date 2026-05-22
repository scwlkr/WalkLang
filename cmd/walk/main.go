package main

import (
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	"walklang/internal/checker"
	"walklang/internal/emitter"
	walkfmt "walklang/internal/format"
	"walklang/internal/parser"
)

func main() {
	if err := run(os.Args[1:]); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}

func run(args []string) error {
	if len(args) == 0 {
		return fmt.Errorf("usage: walk build <source.walk> -o <output>")
	}
	switch args[0] {
	case "build":
		return build(args[1:])
	case "emit-c":
		return emitCCommand(args[1:])
	case "fmt":
		return fmtCommand(args[1:])
	case "test":
		return testCommand(args[1:])
	case "repl":
		return replCommand(args[1:])
	default:
		return fmt.Errorf("unknown command %q", args[0])
	}
}

func build(args []string) error {
	sourcePath, output, cPath, err := parseBuildArgs(args)
	if err != nil {
		return err
	}
	if cPath == "" {
		cPath = output + ".c"
	}
	cCode, err := compileToC(sourcePath)
	if err != nil {
		return err
	}
	if err := buildC(cCode, cPath, output); err != nil {
		return err
	}
	fmt.Println(output)
	return nil
}

func emitCCommand(args []string) error {
	sourcePath, output, err := parseEmitCArgs(args)
	if err != nil {
		return err
	}

	cCode, err := compileToC(sourcePath)
	if err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(output), 0o755); err != nil {
		return err
	}
	if err := os.WriteFile(output, []byte(cCode), 0o644); err != nil {
		return err
	}
	fmt.Println(output)
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
		return fmt.Errorf("usage: walk fmt [-w] <source.walk>")
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
	if len(args) != 1 {
		return fmt.Errorf("usage: walk test <source.walk>")
	}
	cCode, err := compileFileToC(args[0], true)
	if err != nil {
		return err
	}
	dir, err := os.MkdirTemp("", "walk-test-*")
	if err != nil {
		return err
	}
	defer os.RemoveAll(dir)

	exePath := filepath.Join(dir, "tests")
	if err := buildC(cCode, filepath.Join(dir, "tests.c"), exePath); err != nil {
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
			fmt.Fprintln(os.Stderr, err)
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
		"out: " + expression,
		"",
	}, "\n")
}

func parseBuildArgs(args []string) (sourcePath string, output string, cOutput string, err error) {
	for i := 0; i < len(args); i++ {
		switch args[i] {
		case "-o":
			i++
			if i >= len(args) {
				return "", "", "", fmt.Errorf("build requires a value after -o")
			}
			output = args[i]
		case "--emit-c":
			i++
			if i >= len(args) {
				return "", "", "", fmt.Errorf("build requires a value after --emit-c")
			}
			cOutput = args[i]
		default:
			if sourcePath != "" {
				return "", "", "", fmt.Errorf("usage: walk build <source.walk> -o <output>")
			}
			sourcePath = args[i]
		}
	}
	if sourcePath == "" || output == "" {
		return "", "", "", fmt.Errorf("usage: walk build <source.walk> -o <output>")
	}
	return sourcePath, output, cOutput, nil
}

func parseEmitCArgs(args []string) (sourcePath string, output string, err error) {
	for i := 0; i < len(args); i++ {
		switch args[i] {
		case "-o":
			i++
			if i >= len(args) {
				return "", "", fmt.Errorf("emit-c requires a value after -o")
			}
			output = args[i]
		default:
			if sourcePath != "" {
				return "", "", fmt.Errorf("usage: walk emit-c <source.walk> -o <output.c>")
			}
			sourcePath = args[i]
		}
	}
	if sourcePath == "" || output == "" {
		return "", "", fmt.Errorf("usage: walk emit-c <source.walk> -o <output.c>")
	}
	return sourcePath, output, nil
}

func compileToC(sourcePath string) (string, error) {
	return compileFileToC(sourcePath, false)
}

func compileFileToC(sourcePath string, testsOnly bool) (string, error) {
	source, err := os.ReadFile(sourcePath)
	if err != nil {
		return "", fmt.Errorf("source read failed: %w", err)
	}
	return compileSourceToC(string(source), sourcePath, testsOnly)
}

func compileSourceToC(source string, filename string, testsOnly bool) (string, error) {
	program, err := parser.ParseSource(source, filename)
	if err != nil {
		return "", err
	}
	if err := checker.Check(program); err != nil {
		return "", err
	}
	if testsOnly {
		return emitter.EmitTestC(program)
	}
	return emitter.EmitC(program)
}

func buildC(cCode string, cPath string, output string) error {
	if err := os.MkdirAll(filepath.Dir(cPath), 0o755); err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(output), 0o755); err != nil {
		return err
	}
	if err := os.WriteFile(cPath, []byte(cCode), 0o644); err != nil {
		return err
	}
	command := exec.Command("cc", cPath, "-o", output, "-lm")
	result, err := command.CombinedOutput()
	if err != nil {
		return fmt.Errorf("native build failed: %s", string(result))
	}
	return nil
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
	if err := buildC(cCode, filepath.Join(dir, "repl.c"), exePath); err != nil {
		return "", err
	}
	output, err := exec.Command(exePath).CombinedOutput()
	if err != nil {
		return "", fmt.Errorf("program failed: %s", string(output))
	}
	return string(output), nil
}
