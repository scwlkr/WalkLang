package main

import (
	"errors"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"testing"
)

func TestWalktopProjectFixtureProof(t *testing.T) {
	requireCC(t)

	root := repoRoot(t)
	toolDir := filepath.Join(root, "tools", "walktop")
	exePath := filepath.Join(t.TempDir(), executableName("walktop"))

	if err := withWorkingDir(toolDir, func() error {
		if err := run([]string{"check", "--warnings=error"}); err != nil {
			return err
		}
		if err := run([]string{"test", "--warnings=error"}); err != nil {
			return err
		}
		return run([]string{
			"build",
			"--mode", "release",
			"--warnings=error",
			filepath.Join("src", "main.walk"),
			"-o", exePath,
		})
	}); err != nil {
		t.Fatal(err)
	}

	command := exec.Command(exePath, "--once", "--fixture", filepath.Join(root, "tools", "walktop", "testdata", "basic"))
	command.Env = append(os.Environ(), "NO_COLOR=1")
	output, err := command.CombinedOutput()
	if err != nil {
		t.Fatalf("walktop fixture run failed: %v\n%s", err, string(output))
	}

	want := strings.Join([]string{
		"WalkTop  v0.1.0",
		"OS       Darwin fixture",
		"Load     [##########----------] 50%",
		"Memory   [##############------] 70%",
		"Disk     [#########-----------] 45%",
		"Procs    128",
		"Frame    1/1",
		"Status   ok",
		"",
	}, "\n")
	if got := string(output); got != want {
		t.Fatalf("walktop fixture output mismatch:\nwant %q\ngot  %q", want, got)
	}
}

func TestWalktopInvalidArgsFailClearly(t *testing.T) {
	requireCC(t)

	root := repoRoot(t)
	exePath := filepath.Join(t.TempDir(), executableName("walktop"))

	if err := run([]string{
		"build",
		"--mode", "release",
		"--warnings=error",
		filepath.Join(root, "tools", "walktop", "src", "main.walk"),
		"-o", exePath,
	}); err != nil {
		t.Fatal(err)
	}

	command := exec.Command(exePath, "--frames", "0")
	output, err := command.CombinedOutput()
	var exitErr *exec.ExitError
	if !errors.As(err, &exitErr) {
		t.Fatalf("expected walktop to fail, got err=%v output=%q", err, string(output))
	}
	if code := exitErr.ExitCode(); code != 2 {
		t.Fatalf("want exit code 2, got %d with output %q", code, string(output))
	}
	if got, want := string(output), "walktop: --frames must be greater than zero\n"; got != want {
		t.Fatalf("walktop invalid arg output mismatch:\nwant %q\ngot  %q", want, got)
	}
}

func executableName(name string) string {
	if runtime.GOOS == "windows" {
		return name + ".exe"
	}
	return name
}
