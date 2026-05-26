package runtimebuild

import (
	"fmt"
	"os"
	"path/filepath"
	"runtime"
)

func FindRuntimeDir() (string, error) {
	if dir := os.Getenv("WALK_RUNTIME_DIR"); dir != "" {
		if isRuntimeDir(dir) {
			return filepath.Clean(dir), nil
		}
		return "", fmt.Errorf("WALK_RUNTIME_DIR does not contain walk_runtime.c: %s", dir)
	}

	var starts []string
	if wd, err := os.Getwd(); err == nil {
		starts = append(starts, wd)
	}
	if exe, err := os.Executable(); err == nil {
		exeDir := filepath.Dir(exe)
		starts = append(starts,
			exeDir,
			filepath.Join(exeDir, "runtime"),
			filepath.Join(exeDir, "..", "lib", "walk", "runtime"),
			filepath.Join(exeDir, "..", "share", "walk", "runtime"),
		)
	}
	if _, file, _, ok := runtime.Caller(0); ok {
		starts = append(starts, filepath.Join(filepath.Dir(file), "..", ".."))
	}

	for _, start := range starts {
		if dir, ok := findRuntimeDirFrom(start); ok {
			return dir, nil
		}
	}
	return "", fmt.Errorf("walk runtime not found; run from the repo or set WALK_RUNTIME_DIR")
}

func SourceFiles(runtimeDir string, goos string) []string {
	return []string{
		filepath.Join(runtimeDir, "walk_runtime.c"),
		filepath.Join(runtimeDir, "platform", PlatformSourceName(goos)),
	}
}

func PlatformSourceName(goos string) string {
	if goos == "windows" {
		return "walk_platform_windows.c"
	}
	return "walk_platform_posix.c"
}

func findRuntimeDirFrom(start string) (string, bool) {
	current := filepath.Clean(start)
	if isRuntimeDir(current) {
		return current, true
	}
	for {
		candidate := filepath.Join(current, "runtime")
		if isRuntimeDir(candidate) {
			return filepath.Clean(candidate), true
		}
		parent := filepath.Dir(current)
		if parent == current {
			return "", false
		}
		current = parent
	}
}

func isRuntimeDir(dir string) bool {
	if dir == "" {
		return false
	}
	if _, err := os.Stat(filepath.Join(dir, "walk_runtime.h")); err != nil {
		return false
	}
	if _, err := os.Stat(filepath.Join(dir, "walk_runtime.c")); err != nil {
		return false
	}
	return true
}
