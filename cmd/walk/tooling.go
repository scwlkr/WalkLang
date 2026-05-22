package main

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"unicode"

	"walklang/internal/ast"
	"walklang/internal/checker"
	"walklang/internal/lexer"
	"walklang/internal/parser"
)

type toolingAnalysis struct {
	Filename  string
	Source    string
	Program   *ast.Program
	Modules   map[string]*checker.Module
	Warnings  []checker.Warning
	Err       error
	Documents map[string]*ast.Program
	Sources   map[string]string
}

type toolingToken struct {
	Text      string
	Kind      lexer.TokenKind
	Location  ast.Location
	EndColumn int
}

type toolingLocation struct {
	Filename  string
	Line      int
	Column    int
	EndColumn int
}

type toolingSymbol struct {
	Name     string
	Kind     string
	Detail   string
	Location ast.Location
}

type docsArgs struct {
	sourcePath string
	outputPath string
}

type debugMapArgs struct {
	sourcePath string
	outputPath string
}

type debugSymbol struct {
	Name   string `json:"name"`
	Kind   string `json:"kind"`
	Detail string `json:"detail,omitempty"`
	File   string `json:"file"`
	Line   int    `json:"line"`
	Column int    `json:"column"`
}

type debugMap struct {
	Version int           `json:"version"`
	Source  string        `json:"source"`
	Symbols []debugSymbol `json:"symbols"`
}

var walkKeywords = []string{
	"imp", "exp", "var", "const", "out", "if", "else", "while", "for",
	"repeat", "break", "continue", "func", "return", "test", "assert",
	"struct", "true", "false", "null", "and", "or", "not", "in",
}

var walkBuiltinDetails = map[string]string{
	"math":           "built-in module",
	"math.sqrt":      "func(number) float",
	"math.pow":       "func(number, number) float",
	"string":         "built-in module",
	"string.len":     "func(string) int",
	"array":          "built-in module",
	"array.len":      "func(array[T]) int",
	"time":           "built-in module",
	"time.now":       "func() int",
	"random":         "built-in module",
	"random.int":     "func(int, int) int",
	"testing":        "built-in module",
	"testing.assert": "func(bool) bool",
}

func docsCommand(args []string) error {
	parsed, err := parseDocsArgs(args)
	if err != nil {
		return err
	}
	sourcePath, err := sourcePathOrProjectEntry(parsed.sourcePath)
	if err != nil {
		return err
	}
	analysis, err := analyzeFileForTooling(sourcePath)
	if err != nil {
		return err
	}
	if analysis.Err != nil {
		return analysis.Err
	}
	markdown := generateDocsMarkdown(sourcePath, analysis)
	if parsed.outputPath == "" {
		fmt.Print(markdown)
		return nil
	}
	if err := os.MkdirAll(filepath.Dir(parsed.outputPath), 0o755); err != nil {
		return err
	}
	return os.WriteFile(parsed.outputPath, []byte(markdown), 0o644)
}

func debugMapCommand(args []string) error {
	parsed, err := parseDebugMapArgs(args)
	if err != nil {
		return err
	}
	sourcePath, err := sourcePathOrProjectEntry(parsed.sourcePath)
	if err != nil {
		return err
	}
	analysis, err := analyzeFileForTooling(sourcePath)
	if err != nil {
		return err
	}
	if analysis.Err != nil {
		return analysis.Err
	}
	payload, err := json.MarshalIndent(generateDebugMap(sourcePath, analysis), "", "  ")
	if err != nil {
		return err
	}
	payload = append(payload, '\n')
	if parsed.outputPath == "" {
		fmt.Print(string(payload))
		return nil
	}
	if err := os.MkdirAll(filepath.Dir(parsed.outputPath), 0o755); err != nil {
		return err
	}
	return os.WriteFile(parsed.outputPath, payload, 0o644)
}

func parseDocsArgs(args []string) (docsArgs, error) {
	var parsed docsArgs
	for i := 0; i < len(args); i++ {
		switch args[i] {
		case "-o", "--output":
			i++
			if i >= len(args) {
				return docsArgs{}, fmt.Errorf("docs requires a value after %s", args[i-1])
			}
			parsed.outputPath = args[i]
		default:
			if parsed.sourcePath != "" {
				return docsArgs{}, fmt.Errorf("usage: walk docs [-o output.md] [source.walk]")
			}
			parsed.sourcePath = args[i]
		}
	}
	return parsed, nil
}

func parseDebugMapArgs(args []string) (debugMapArgs, error) {
	var parsed debugMapArgs
	for i := 0; i < len(args); i++ {
		switch args[i] {
		case "-o", "--output":
			i++
			if i >= len(args) {
				return debugMapArgs{}, fmt.Errorf("debug-map requires a value after %s", args[i-1])
			}
			parsed.outputPath = args[i]
		default:
			if parsed.sourcePath != "" {
				return debugMapArgs{}, fmt.Errorf("usage: walk debug-map [-o output.json] [source.walk]")
			}
			parsed.sourcePath = args[i]
		}
	}
	return parsed, nil
}

func sourcePathOrProjectEntry(sourcePath string) (string, error) {
	if sourcePath != "" {
		return filepath.Abs(sourcePath)
	}
	config, err := loadProjectConfigFromCwd()
	if err != nil {
		return "", err
	}
	entryPath, err := projectPath(config, config.entry)
	if err != nil {
		return "", err
	}
	return filepath.Abs(entryPath)
}

func analyzeFileForTooling(sourcePath string) (toolingAnalysis, error) {
	absPath, err := filepath.Abs(sourcePath)
	if err != nil {
		return toolingAnalysis{}, err
	}
	source, err := os.ReadFile(absPath)
	if err != nil {
		return toolingAnalysis{}, fmt.Errorf("source read failed: %w", err)
	}
	return analyzeSourceForTooling(absPath, string(source)), nil
}

func analyzeSourceForTooling(filename string, source string) toolingAnalysis {
	searchDirs, err := toolingSearchDirsForFile(filename)
	if err != nil {
		return toolingAnalysis{Filename: filename, Source: source, Err: err}
	}
	return analyzeSourceWithSearchDirs(filename, source, searchDirs)
}

func analyzeSourceWithSearchDirs(filename string, source string, searchDirs []string) toolingAnalysis {
	analysis := toolingAnalysis{
		Filename:  filename,
		Source:    source,
		Modules:   map[string]*checker.Module{},
		Documents: map[string]*ast.Program{},
		Sources:   map[string]string{filename: source},
	}
	program, err := parser.ParseSource(source, filename)
	if err != nil {
		analysis.Err = err
		return analysis
	}
	analysis.Program = program
	analysis.Documents[filename] = program

	loader := moduleLoader{
		modules:    map[string]*checker.Module{},
		loading:    map[string]bool{},
		searchDirs: cleanSearchDirs(searchDirs),
	}
	if err := loader.loadImports(program, filepath.Dir(filename)); err != nil {
		analysis.Modules = loader.modules
		analysis.Err = err
		return analysis
	}
	analysis.Modules = loader.modules
	for _, module := range loader.modules {
		if module.Path == "" {
			continue
		}
		analysis.Documents[module.Path] = module.Program
		if source, err := os.ReadFile(module.Path); err == nil {
			analysis.Sources[module.Path] = string(source)
		}
	}

	warnings, err := checkPrograms(program, loader.modules)
	analysis.Warnings = warnings
	analysis.Err = err
	return analysis
}

func toolingSearchDirsForFile(filename string) ([]string, error) {
	absPath, err := filepath.Abs(filename)
	if err != nil {
		return nil, err
	}
	root, err := findProjectRoot(filepath.Dir(absPath))
	if err != nil {
		return nil, nil
	}
	contents, err := os.ReadFile(filepath.Join(root, projectConfigName))
	if err != nil {
		return nil, err
	}
	config, err := parseProjectConfig(string(contents), filepath.Join(root, projectConfigName))
	if err != nil {
		return nil, err
	}
	config.root = root
	return projectSearchDirs(config, absPath)
}

func collectSourceTokens(filename string, source string) []toolingToken {
	lines, err := lexer.Lex(source, filename)
	if err != nil {
		return nil
	}
	var tokens []toolingToken
	for _, line := range lines {
		for _, token := range line.Tokens {
			tokens = append(tokens, toolingToken{
				Text:      token.Value,
				Kind:      token.Kind,
				Location:  token.Location,
				EndColumn: token.Location.Column + len(token.Value),
			})
		}
	}
	return tokens
}

func wordAtPosition(filename string, source string, line int, character int) (toolingToken, bool) {
	targetLine := line + 1
	targetColumn := character + 1
	for _, token := range collectSourceTokens(filename, source) {
		if token.Kind != lexer.TokenName {
			continue
		}
		if token.Location.Line == targetLine && targetColumn >= token.Location.Column && targetColumn <= token.EndColumn {
			return token, true
		}
	}
	return toolingToken{}, false
}

func definitionAtPosition(filename string, source string, line int, character int) (toolingLocation, bool) {
	word, ok := wordAtPosition(filename, source, line, character)
	if !ok {
		return toolingLocation{}, false
	}
	analysis := analyzeSourceForTooling(filename, source)
	if modulePath, ok := modulePathForWord(analysis, word); ok {
		return toolingLocation{Filename: modulePath, Line: 1, Column: 1, EndColumn: 1}, true
	}
	for _, symbol := range sortedToolingSymbols(analysis) {
		if symbol.Name == word.Text || strings.HasSuffix(symbol.Name, "."+word.Text) {
			return locationFromAST(symbol.Location, len(word.Text)), true
		}
	}
	return toolingLocation{}, false
}

func referencesAtPosition(filename string, source string, line int, character int) []toolingLocation {
	word, ok := wordAtPosition(filename, source, line, character)
	if !ok {
		return nil
	}
	analysis := analyzeSourceForTooling(filename, source)
	var refs []toolingLocation
	for path, source := range analysis.Sources {
		for _, token := range collectSourceTokens(path, source) {
			if token.Kind == lexer.TokenName && token.Text == word.Text {
				refs = append(refs, toolingLocation{
					Filename:  path,
					Line:      token.Location.Line,
					Column:    token.Location.Column,
					EndColumn: token.EndColumn,
				})
			}
		}
	}
	sortToolingLocations(refs)
	return refs
}

func renameEditsAtPosition(filename string, source string, line int, character int, newName string) (map[string][]toolingLocation, error) {
	if !validRenameIdentifier(newName) {
		return nil, fmt.Errorf("rename target %q is not a valid WalkLang identifier", newName)
	}
	word, ok := wordAtPosition(filename, source, line, character)
	if !ok {
		return nil, fmt.Errorf("no symbol at cursor")
	}
	analysis := analyzeSourceForTooling(filename, source)
	changes := map[string][]toolingLocation{}
	for path, source := range analysis.Sources {
		for _, token := range collectSourceTokens(path, source) {
			if token.Kind == lexer.TokenName && token.Text == word.Text {
				changes[path] = append(changes[path], toolingLocation{
					Filename:  path,
					Line:      token.Location.Line,
					Column:    token.Location.Column,
					EndColumn: token.EndColumn,
				})
			}
		}
	}
	for path := range changes {
		sortToolingLocations(changes[path])
	}
	return changes, nil
}

func hoverAtPosition(filename string, source string, line int, character int) (string, bool) {
	word, ok := wordAtPosition(filename, source, line, character)
	if !ok {
		return "", false
	}
	if detail, ok := builtinDetailForWord(source, word); ok {
		return detail, true
	}
	analysis := analyzeSourceForTooling(filename, source)
	for _, symbol := range sortedToolingSymbols(analysis) {
		if symbol.Name == word.Text || strings.HasSuffix(symbol.Name, "."+word.Text) {
			return symbol.Detail, true
		}
	}
	return "", false
}

func completionItemsForSource(filename string, source string) []toolingSymbol {
	analysis := analyzeSourceForTooling(filename, source)
	seen := map[string]bool{}
	var items []toolingSymbol
	add := func(name string, kind string, detail string) {
		if name == "" || seen[name] {
			return
		}
		seen[name] = true
		items = append(items, toolingSymbol{Name: name, Kind: kind, Detail: detail})
	}
	for _, keyword := range walkKeywords {
		add(keyword, "keyword", "WalkLang keyword")
	}
	builtins := make([]string, 0, len(walkBuiltinDetails))
	for name := range walkBuiltinDetails {
		builtins = append(builtins, name)
	}
	sort.Strings(builtins)
	for _, name := range builtins {
		add(name, "builtin", walkBuiltinDetails[name])
	}
	for _, symbol := range sortedToolingSymbols(analysis) {
		add(symbol.Name, symbol.Kind, symbol.Detail)
	}
	sort.Slice(items, func(i, j int) bool {
		return items[i].Name < items[j].Name
	})
	return items
}

func modulePathForWord(analysis toolingAnalysis, word toolingToken) (string, bool) {
	for name, module := range analysis.Modules {
		if module.Path == "" {
			continue
		}
		if word.Text == name || word.Text == lastNamePart(name) {
			return module.Path, true
		}
		if word.Location.Line > 0 {
			for _, statement := range analysis.Program.Statements {
				imp, ok := statement.(*ast.Import)
				if ok && imp.Location.Line == word.Location.Line && imp.Module == name {
					return module.Path, true
				}
			}
		}
	}
	return "", false
}

func builtinDetailForWord(source string, word toolingToken) (string, bool) {
	if detail, ok := walkBuiltinDetails[word.Text]; ok {
		return detail, true
	}
	lines := strings.Split(source, "\n")
	if word.Location.Line < 1 || word.Location.Line > len(lines) {
		return "", false
	}
	line := lines[word.Location.Line-1]
	for name, detail := range walkBuiltinDetails {
		if strings.Contains(name, "."+word.Text) && strings.Contains(line, name) {
			return detail, true
		}
	}
	return "", false
}

func sortedToolingSymbols(analysis toolingAnalysis) []toolingSymbol {
	var symbols []toolingSymbol
	paths := make([]string, 0, len(analysis.Documents))
	for path := range analysis.Documents {
		paths = append(paths, path)
	}
	sort.Strings(paths)
	for _, path := range paths {
		symbols = append(symbols, collectProgramSymbols(path, analysis.Documents[path])...)
	}
	sort.Slice(symbols, func(i, j int) bool {
		if symbols[i].Name == symbols[j].Name {
			return symbols[i].Location.Filename < symbols[j].Location.Filename
		}
		return symbols[i].Name < symbols[j].Name
	})
	return symbols
}

func collectProgramSymbols(filename string, program *ast.Program) []toolingSymbol {
	if program == nil {
		return nil
	}
	var symbols []toolingSymbol
	var walkStatements func([]ast.Statement)
	walkStatements = func(statements []ast.Statement) {
		for _, statement := range statements {
			switch s := statement.(type) {
			case *ast.Import:
				symbols = append(symbols, toolingSymbol{Name: s.Module, Kind: "module", Detail: "module " + s.Module, Location: locInFile(filename, s.Location)})
			case *ast.FuncDecl:
				name := s.Name
				kind := "function"
				if s.Receiver != "" {
					name = s.Receiver + "." + s.Name
					kind = "method"
				}
				symbols = append(symbols, toolingSymbol{Name: name, Kind: kind, Detail: functionSignature(s), Location: locInFile(filename, s.Location)})
				for _, param := range s.Params {
					symbols = append(symbols, toolingSymbol{Name: param.Name, Kind: "parameter", Detail: param.Name + " " + param.Type.String(), Location: locInFile(filename, s.Location)})
				}
				walkStatements(s.Body)
			case *ast.StructDecl:
				symbols = append(symbols, toolingSymbol{Name: s.Name, Kind: "struct", Detail: structSignature(s), Location: locInFile(filename, s.Location)})
				for _, field := range s.Fields {
					symbols = append(symbols, toolingSymbol{Name: field.Name, Kind: "field", Detail: field.Name + " " + field.Type.String(), Location: locInFile(filename, field.Location)})
				}
			case *ast.VarDecl:
				symbols = append(symbols, toolingSymbol{Name: s.Name, Kind: varKind(s), Detail: varDetail(s), Location: locInFile(filename, s.Location)})
			case *ast.TestDecl:
				symbols = append(symbols, toolingSymbol{Name: s.Name, Kind: "test", Detail: "test " + s.Name, Location: locInFile(filename, s.Location)})
				walkStatements(s.Body)
			case *ast.If:
				walkStatements(s.Then)
				walkStatements(s.Else)
			case *ast.While:
				walkStatements(s.Body)
			case *ast.Repeat:
				walkStatements(s.Body)
			case *ast.For:
				symbols = append(symbols, toolingSymbol{Name: s.Name, Kind: "variable", Detail: s.Name, Location: locInFile(filename, s.Location)})
				walkStatements(s.Body)
			}
		}
	}
	walkStatements(program.Statements)
	return symbols
}

func locInFile(filename string, loc ast.Location) ast.Location {
	loc.Filename = filename
	return loc
}

func varKind(decl *ast.VarDecl) string {
	if decl.Mutable {
		return "variable"
	}
	return "constant"
}

func varDetail(decl *ast.VarDecl) string {
	typeName := decl.Annotation
	if typeName.Kind == ast.TypeInvalid && decl.Value != nil {
		typeName = decl.Value.ExprType()
	}
	if typeName.Kind == ast.TypeInvalid {
		return decl.Name
	}
	prefix := "const"
	if decl.Mutable {
		prefix = "var"
	}
	return fmt.Sprintf("%s %s %s", prefix, decl.Name, typeName.String())
}

func functionSignature(fn *ast.FuncDecl) string {
	name := fn.Name
	if fn.Receiver != "" {
		name = fn.Receiver + "." + fn.Name
	}
	if len(fn.TypeParams) > 0 {
		name += "[" + strings.Join(fn.TypeParams, ", ") + "]"
	}
	params := make([]string, 0, len(fn.Params))
	for _, param := range fn.Params {
		params = append(params, param.Name+" "+param.Type.String())
	}
	return fmt.Sprintf("func %s(%s) %s", name, strings.Join(params, ", "), fn.ReturnType.String())
}

func structSignature(decl *ast.StructDecl) string {
	fields := make([]string, 0, len(decl.Fields))
	for _, field := range decl.Fields {
		fields = append(fields, field.Name+" "+field.Type.String())
	}
	return fmt.Sprintf("struct %s { %s }", decl.Name, strings.Join(fields, ", "))
}

func generateDocsMarkdown(sourcePath string, analysis toolingAnalysis) string {
	var out strings.Builder
	out.WriteString("# WalkLang API\n\n")
	out.WriteString("Source: `" + filepath.ToSlash(sourcePath) + "`\n\n")

	paths := make([]string, 0, len(analysis.Documents))
	for path := range analysis.Documents {
		paths = append(paths, path)
	}
	sort.Strings(paths)
	for _, path := range paths {
		program := analysis.Documents[path]
		out.WriteString("## " + filepath.ToSlash(path) + "\n\n")
		writeDocSection(&out, "Structs", structDocs(program))
		writeDocSection(&out, "Functions", functionDocs(program))
		writeDocSection(&out, "Exports", exportDocs(program))
	}
	return out.String()
}

func writeDocSection(out *strings.Builder, title string, entries []string) {
	if len(entries) == 0 {
		return
	}
	out.WriteString("### " + title + "\n\n")
	for _, entry := range entries {
		out.WriteString("- `" + entry + "`\n")
	}
	out.WriteString("\n")
}

func structDocs(program *ast.Program) []string {
	var docs []string
	for _, statement := range program.Statements {
		if decl, ok := statement.(*ast.StructDecl); ok {
			docs = append(docs, structSignature(decl))
		}
	}
	sort.Strings(docs)
	return docs
}

func functionDocs(program *ast.Program) []string {
	var docs []string
	for _, statement := range program.Statements {
		if decl, ok := statement.(*ast.FuncDecl); ok {
			docs = append(docs, functionSignature(decl))
		}
	}
	sort.Strings(docs)
	return docs
}

func exportDocs(program *ast.Program) []string {
	var docs []string
	for _, statement := range program.Statements {
		if decl, ok := statement.(*ast.Export); ok {
			docs = append(docs, decl.Name)
		}
	}
	sort.Strings(docs)
	return docs
}

func generateDebugMap(sourcePath string, analysis toolingAnalysis) debugMap {
	symbols := sortedToolingSymbols(analysis)
	result := debugMap{Version: 1, Source: filepath.ToSlash(sourcePath)}
	for _, symbol := range symbols {
		result.Symbols = append(result.Symbols, debugSymbol{
			Name:   symbol.Name,
			Kind:   symbol.Kind,
			Detail: symbol.Detail,
			File:   filepath.ToSlash(symbol.Location.Filename),
			Line:   symbol.Location.Line,
			Column: symbol.Location.Column,
		})
	}
	return result
}

func locationFromAST(loc ast.Location, fallbackLength int) toolingLocation {
	if fallbackLength < 1 {
		fallbackLength = 1
	}
	return toolingLocation{
		Filename:  loc.Filename,
		Line:      loc.Line,
		Column:    loc.Column,
		EndColumn: loc.Column + fallbackLength,
	}
}

func sortToolingLocations(locations []toolingLocation) {
	sort.Slice(locations, func(i, j int) bool {
		if locations[i].Filename != locations[j].Filename {
			return locations[i].Filename < locations[j].Filename
		}
		if locations[i].Line != locations[j].Line {
			return locations[i].Line < locations[j].Line
		}
		return locations[i].Column < locations[j].Column
	})
}

func lastNamePart(name string) string {
	if index := strings.LastIndex(name, "."); index >= 0 {
		return name[index+1:]
	}
	return name
}

func validRenameIdentifier(name string) bool {
	if name == "" {
		return false
	}
	for i, r := range name {
		if i == 0 {
			if !unicode.IsLetter(r) && r != '_' {
				return false
			}
			continue
		}
		if !unicode.IsLetter(r) && !unicode.IsDigit(r) && r != '_' {
			return false
		}
	}
	return true
}
