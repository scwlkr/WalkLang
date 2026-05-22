package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"testing"
)

func TestV12ProjectLifecycle(t *testing.T) {
	requireCC(t)

	dir := t.TempDir()
	projectDir := filepath.Join(dir, "hello")
	if err := run([]string{"init", projectDir}); err != nil {
		t.Fatal(err)
	}
	for _, rel := range []string{"walk.toml", "src/main.walk", "src/math_extra.walk", "tests/main_test.walk"} {
		if _, err := os.Stat(filepath.Join(projectDir, rel)); err != nil {
			t.Fatalf("expected %s: %v", rel, err)
		}
	}

	err := withWorkingDir(projectDir, func() error {
		if err := run([]string{"check", "--warnings=error"}); err != nil {
			return err
		}
		if err := run([]string{"build"}); err != nil {
			return err
		}
		output, err := exec.Command(filepath.Join(projectDir, "build", "hello")).CombinedOutput()
		if err != nil {
			return err
		}
		if got, want := string(output), "27\n"; got != want {
			return fmt.Errorf("project output: want %q, got %q", want, got)
		}
		if err := run([]string{"test"}); err != nil {
			return err
		}
		if err := os.WriteFile(filepath.Join(projectDir, "src", "messy.walk"), []byte("out:+ 1 2\n"), 0o644); err != nil {
			return err
		}
		if err := run([]string{"fmt"}); err != nil {
			return err
		}
		formatted, err := os.ReadFile(filepath.Join(projectDir, "src", "messy.walk"))
		if err != nil {
			return err
		}
		if got, want := string(formatted), "out: + 1 2\n"; got != want {
			return fmt.Errorf("formatted source: want %q, got %q", want, got)
		}
		if err := run([]string{"clean"}); err != nil {
			return err
		}
		if _, err := os.Stat(filepath.Join(projectDir, "build")); !os.IsNotExist(err) {
			return fmt.Errorf("build directory should be removed, stat err: %v", err)
		}
		return nil
	})
	if err != nil {
		t.Fatal(err)
	}
}

func TestV12ProjectConfigValidation(t *testing.T) {
	_, err := parseProjectConfig(strings.Join([]string{
		"name = \"bad name\"",
		"entry = \"src/main.walk\"",
		"",
	}, "\n"), "walk.toml")
	if err == nil {
		t.Fatal("expected invalid project name")
	}
	if got, want := err.Error(), "walk.toml:1: project name \"bad name\" may contain only letters, numbers, underscore, and dash"; got != want {
		t.Fatalf("want %q, got %q", want, got)
	}
}

func withWorkingDir(dir string, fn func() error) error {
	previous, err := os.Getwd()
	if err != nil {
		return err
	}
	if err := os.Chdir(dir); err != nil {
		return err
	}
	defer os.Chdir(previous)
	return fn()
}
