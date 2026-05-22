package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"

	"walklang/internal/checker"
	"walklang/internal/emitter"
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
	source, err := os.ReadFile(sourcePath)
	if err != nil {
		return "", fmt.Errorf("source read failed: %w", err)
	}
	program, err := parser.ParseSource(string(source), sourcePath)
	if err != nil {
		return "", err
	}
	if err := checker.Check(program); err != nil {
		return "", err
	}
	return emitter.EmitC(program)
}
