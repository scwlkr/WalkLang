package main

import (
	"encoding/json"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestLayoutUsesRootFaviconAndCompactSidebarIcon(t *testing.T) {
	pages := []docPage{{Output: "docs/index.html", Title: "Docs", Group: "Start"}}
	got := layout("docs/index.html", "Docs", pages, "<article></article>")

	if !strings.Contains(got, `<link rel="icon" type="image/svg+xml" href="../favicon.svg">`) {
		t.Fatalf("layout should use root favicon.svg link, got:\n%s", got)
	}
	if !strings.Contains(got, `<img src="../assets/icon.svg" alt="">`) {
		t.Fatalf("sidebar brand should use compact icon.svg, got:\n%s", got)
	}
}

func TestSiteCSSUsesDarkWalkLangBlueTheme(t *testing.T) {
	css, err := os.ReadFile(filepath.Join("..", "site", "assets", "site.css"))
	if err != nil {
		t.Fatal(err)
	}
	text := string(css)

	for _, token := range []string{"color-scheme: dark;", "--bg: #0d0f14;", "--accent: #5c6cff;", "--accent-strong: #d7dcff;", "max-width: 100vw;"} {
		if !strings.Contains(text, token) {
			t.Fatalf("site CSS should include dark blue theme token %s", token)
		}
	}
	for _, oldGreen := range []string{"#16865e", "#0f6044", "#e7ece8", "#23332c", "#f7f8f5"} {
		if strings.Contains(text, oldGreen) {
			t.Fatalf("site CSS still contains old green token %s", oldGreen)
		}
	}
}

func TestVSCodeLanguageIconExists(t *testing.T) {
	data, err := os.ReadFile(filepath.Join("..", "editors", "vscode", "package.json"))
	if err != nil {
		t.Fatal(err)
	}
	var manifest struct {
		Contributes struct {
			Languages []struct {
				ID   string `json:"id"`
				Icon struct {
					Light string `json:"light"`
					Dark  string `json:"dark"`
				} `json:"icon"`
			} `json:"languages"`
		} `json:"contributes"`
	}
	if err := json.Unmarshal(data, &manifest); err != nil {
		t.Fatal(err)
	}

	for _, language := range manifest.Contributes.Languages {
		if language.ID != "walk" {
			continue
		}
		if language.Icon.Light != "./icons/walklang.svg" || language.Icon.Dark != "./icons/walklang.svg" {
			t.Fatalf("walk language should use the WalkLang icon for light and dark themes")
		}
		if _, err := os.Stat(filepath.Join("..", "editors", "vscode", strings.TrimPrefix(language.Icon.Light, "./"))); err != nil {
			t.Fatalf("walk language icon is missing: %v", err)
		}
		return
	}
	t.Fatal("walk language contribution not found")
}
