package main

import (
	"encoding/json"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestV4LSPDiagnosticsFormattingAndCompletion(t *testing.T) {
	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")

	diagnostics := diagnosticsForLSP(sourcePath, "var: age int = 'old'\n")
	if got, want := len(diagnostics), 1; got != want {
		t.Fatalf("want %d diagnostic, got %d: %#v", want, got, diagnostics)
	}
	if diagnostics[0].Severity != 1 || !strings.Contains(diagnostics[0].Message, "type error: age is int, got string") {
		t.Fatalf("unexpected diagnostic: %#v", diagnostics[0])
	}

	server := newLSPServer(strings.NewReader(""), &strings.Builder{})
	uri := pathToURI(sourcePath)
	server.documents[uri] = lspDocument{URI: uri, Path: sourcePath, Text: "out:+ 1 2\n"}
	params, err := json.Marshal(lspFormattingParams{TextDocument: lspTextDocumentIdentifier{URI: uri}})
	if err != nil {
		t.Fatal(err)
	}
	result, rpcErr := server.handleFormatting(params)
	if rpcErr != nil {
		t.Fatal(rpcErr.Message)
	}
	edits, ok := result.([]lspTextEdit)
	if !ok || len(edits) != 1 {
		t.Fatalf("expected one text edit, got %#v", result)
	}
	if got, want := edits[0].NewText, "out: + 1 2\n"; got != want {
		t.Fatalf("want formatted %q, got %q", want, got)
	}

	completions := completionItemsForSource(sourcePath, "func: cube(x int) int\n    return: * x x x\n\nout: cube(3)\n")
	if !hasCompletion(completions, "cube") || !hasCompletion(completions, "math.sqrt") || !hasCompletion(completions, "func") {
		t.Fatalf("missing expected completions: %#v", completions)
	}

	initResult, initErr := server.handleRequest(lspMessage{Method: "initialize"})
	if initErr != nil {
		t.Fatal(initErr.Message)
	}
	capabilities := initResult.(map[string]any)["capabilities"].(map[string]any)
	for _, capability := range []string{"hoverProvider", "definitionProvider", "referencesProvider", "documentFormattingProvider", "renameProvider"} {
		if capabilities[capability] != true {
			t.Fatalf("expected %s capability: %#v", capability, capabilities)
		}
	}
}

func TestV4NavigationHoverReferencesAndRename(t *testing.T) {
	dir := t.TempDir()
	modulePath := filepath.Join(dir, "math_extra.walk")
	mainPath := filepath.Join(dir, "main.walk")
	writeFile(t, modulePath, strings.Join([]string{
		"func: double(x int) int",
		"    return: * x 2",
		"",
		"exp: double",
		"",
	}, "\n"))
	source := strings.Join([]string{
		"imp: math_extra",
		"",
		"out: math_extra.double(4)",
		"out: math_extra.double(5)",
		"",
	}, "\n")
	writeFile(t, mainPath, source)

	def, ok := definitionAtPosition(mainPath, source, 2, strings.Index(sourceLine(source, 3), "double"))
	if !ok {
		t.Fatal("expected definition for double")
	}
	if def.Filename != modulePath || def.Line != 1 {
		t.Fatalf("unexpected definition: %#v", def)
	}

	hover, ok := hoverAtPosition(mainPath, source, 2, strings.Index(sourceLine(source, 3), "double"))
	if !ok || !strings.Contains(hover, "func double(x int) int") {
		t.Fatalf("unexpected hover: %q", hover)
	}

	refs := referencesAtPosition(mainPath, source, 2, strings.Index(sourceLine(source, 3), "double"))
	if got, want := len(refs), 4; got != want {
		t.Fatalf("want %d references, got %d: %#v", want, got, refs)
	}

	changes, err := renameEditsAtPosition(mainPath, source, 2, strings.Index(sourceLine(source, 3), "double"), "twice")
	if err != nil {
		t.Fatal(err)
	}
	if got, want := len(changes[mainPath]), 2; got != want {
		t.Fatalf("want %d main edits, got %d: %#v", want, got, changes[mainPath])
	}
	if got, want := len(changes[modulePath]), 2; got != want {
		t.Fatalf("want %d module edits, got %d: %#v", want, got, changes[modulePath])
	}
}

func TestV4DocsAndDebugMapCommands(t *testing.T) {
	dir := t.TempDir()
	sourcePath := filepath.Join(dir, "main.walk")
	writeFile(t, sourcePath, strings.Join([]string{
		"struct: Point",
		"    x int",
		"    y int",
		"",
		"func: double(x int) int",
		"    return: * x 2",
		"",
		"out: double(4)",
		"",
	}, "\n"))

	docsPath := filepath.Join(dir, "api.md")
	if err := docsCommand([]string{"-o", docsPath, sourcePath}); err != nil {
		t.Fatal(err)
	}
	docs := readText(t, docsPath)
	for _, want := range []string{"# WalkLang API", "struct Point", "func double(x int) int"} {
		if !strings.Contains(docs, want) {
			t.Fatalf("docs missing %q:\n%s", want, docs)
		}
	}

	debugPath := filepath.Join(dir, "debug.json")
	if err := debugMapCommand([]string{"-o", debugPath, sourcePath}); err != nil {
		t.Fatal(err)
	}
	var payload debugMap
	if err := json.Unmarshal([]byte(readText(t, debugPath)), &payload); err != nil {
		t.Fatal(err)
	}
	if payload.Version != 1 || !debugMapHasSymbol(payload, "double", "function") || !debugMapHasSymbol(payload, "Point", "struct") {
		t.Fatalf("unexpected debug map: %#v", payload)
	}
}

func hasCompletion(items []toolingSymbol, label string) bool {
	for _, item := range items {
		if item.Name == label {
			return true
		}
	}
	return false
}

func debugMapHasSymbol(payload debugMap, name string, kind string) bool {
	for _, symbol := range payload.Symbols {
		if symbol.Name == name && symbol.Kind == kind {
			return true
		}
	}
	return false
}

func sourceLine(source string, oneBasedLine int) string {
	lines := strings.Split(source, "\n")
	return lines[oneBasedLine-1]
}

func readText(t *testing.T, path string) string {
	t.Helper()
	data, err := os.ReadFile(path)
	if err != nil {
		t.Fatal(err)
	}
	return string(data)
}
