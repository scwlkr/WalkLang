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

func TestV3PackageLifecycle(t *testing.T) {
	requireCC(t)

	dir := t.TempDir()
	registry := filepath.Join(dir, "registry")
	packageDir := filepath.Join(dir, "geometry")
	if err := run([]string{"package", "init", packageDir}); err != nil {
		t.Fatal(err)
	}
	for _, rel := range []string{"README.md", "walk.toml", filepath.Join("src", "geometry", "core.walk"), "tests/main_test.walk"} {
		if _, err := os.Stat(filepath.Join(packageDir, rel)); err != nil {
			t.Fatalf("expected package file %s: %v", rel, err)
		}
	}
	if err := withWorkingDir(packageDir, func() error {
		return run([]string{"package", "publish", registry})
	}); err != nil {
		t.Fatal(err)
	}

	appDir := filepath.Join(dir, "shape_app")
	if err := run([]string{"init", appDir}); err != nil {
		t.Fatal(err)
	}
	writeFile(t, filepath.Join(appDir, "walk.toml"), strings.Join([]string{
		"name = \"shape_app\"",
		"version = \"0.1.0\"",
		"entry = \"src/main.walk\"",
		"",
		"[build]",
		"output = \"build/shape_app\"",
		"release = false",
		"",
		"[dependencies]",
		"geometry = \"0.1.0\"",
		"",
	}, "\n"))
	writeFile(t, filepath.Join(appDir, "src", "main.walk"), strings.Join([]string{
		"imp: geometry.core",
		"",
		"out: geometry.core.double(4)",
		"",
	}, "\n"))
	writeFile(t, filepath.Join(appDir, "tests", "main_test.walk"), strings.Join([]string{
		"imp: geometry.core",
		"",
		"test: 'package import works'",
		"    assert: == geometry.core.double(4) 8",
		"",
	}, "\n"))

	if err := withWorkingDir(appDir, func() error {
		err := run([]string{"check", "--warnings=error"})
		if err == nil {
			return fmt.Errorf("expected unlocked dependency to fail")
		}
		if !strings.Contains(err.Error(), "dependencies are not locked") {
			return fmt.Errorf("expected lock error, got %v", err)
		}
		if err := run([]string{"package", "resolve", registry}); err != nil {
			return err
		}
		lock, err := os.ReadFile(filepath.Join(appDir, "walk.lock"))
		if err != nil {
			return err
		}
		if !strings.Contains(string(lock), "name = \"geometry\"") || !strings.Contains(string(lock), "version = \"0.1.0\"") || !strings.Contains(string(lock), "checksum = \"sha256:") {
			return fmt.Errorf("walk.lock did not record geometry pin:\n%s", string(lock))
		}
		if err := run([]string{"check", "--warnings=error"}); err != nil {
			return err
		}
		if err := run([]string{"test", "--warnings=error"}); err != nil {
			return err
		}
		if err := run([]string{"build"}); err != nil {
			return err
		}
		output, err := exec.Command(filepath.Join(appDir, "build", "shape_app")).CombinedOutput()
		if err != nil {
			return err
		}
		if got, want := string(output), "8\n"; got != want {
			return fmt.Errorf("package app output: want %q, got %q", want, got)
		}
		cachedModule := filepath.Join(appDir, ".walk", "packages", "geometry", "0.1.0", "src", "geometry", "core.walk")
		if err := os.WriteFile(cachedModule, []byte("func: double(x int) int\n    return: + x 100\n\nexp: double\n"), 0o644); err != nil {
			return err
		}
		err = run([]string{"check", "--warnings=error"})
		if err == nil {
			return fmt.Errorf("expected package cache checksum mismatch")
		}
		if !strings.Contains(err.Error(), "cache does not match walk.lock") {
			return fmt.Errorf("expected checksum error, got %v", err)
		}
		return nil
	}); err != nil {
		t.Fatal(err)
	}
}

func TestV3PackageDependencyConfigValidation(t *testing.T) {
	_, err := parseProjectConfig(strings.Join([]string{
		"name = \"shape_app\"",
		"",
		"[dependencies]",
		"geometry = \"latest\"",
		"",
	}, "\n"), "walk.toml")
	if err == nil {
		t.Fatal("expected invalid dependency version")
	}
	if got, want := err.Error(), "walk.toml:4: dependency \"geometry\" version must be MAJOR.MINOR.PATCH"; got != want {
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
