package main

import (
	"bufio"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/url"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"walklang/internal/ast"
	"walklang/internal/diagnostic"
	walkfmt "walklang/internal/format"
)

type lspServer struct {
	in        *bufio.Reader
	out       io.Writer
	documents map[string]lspDocument
	shutdown  bool
}

type lspDocument struct {
	URI  string
	Path string
	Text string
}

type lspMessage struct {
	JSONRPC string           `json:"jsonrpc"`
	ID      *json.RawMessage `json:"id,omitempty"`
	Method  string           `json:"method"`
	Params  json.RawMessage  `json:"params,omitempty"`
}

type lspResponse struct {
	JSONRPC string            `json:"jsonrpc"`
	ID      json.RawMessage   `json:"id"`
	Result  any               `json:"result,omitempty"`
	Error   *lspResponseError `json:"error,omitempty"`
}

type lspResponseError struct {
	Code    int    `json:"code"`
	Message string `json:"message"`
}

type lspPosition struct {
	Line      int `json:"line"`
	Character int `json:"character"`
}

type lspRange struct {
	Start lspPosition `json:"start"`
	End   lspPosition `json:"end"`
}

type lspLocation struct {
	URI   string   `json:"uri"`
	Range lspRange `json:"range"`
}

type lspTextEdit struct {
	Range   lspRange `json:"range"`
	NewText string   `json:"newText"`
}

type lspDiagnostic struct {
	Range    lspRange `json:"range"`
	Severity int      `json:"severity"`
	Source   string   `json:"source"`
	Message  string   `json:"message"`
}

type lspMarkupContent struct {
	Kind  string `json:"kind"`
	Value string `json:"value"`
}

type lspHover struct {
	Contents lspMarkupContent `json:"contents"`
}

type lspCompletionList struct {
	IsIncomplete bool                `json:"isIncomplete"`
	Items        []lspCompletionItem `json:"items"`
}

type lspCompletionItem struct {
	Label         string `json:"label"`
	Kind          int    `json:"kind,omitempty"`
	Detail        string `json:"detail,omitempty"`
	Documentation string `json:"documentation,omitempty"`
}

type lspWorkspaceEdit struct {
	Changes map[string][]lspTextEdit `json:"changes"`
}

type lspTextDocumentIdentifier struct {
	URI string `json:"uri"`
}

type lspTextDocumentItem struct {
	URI        string `json:"uri"`
	LanguageID string `json:"languageId"`
	Version    int    `json:"version"`
	Text       string `json:"text"`
}

type lspVersionedTextDocumentIdentifier struct {
	URI     string `json:"uri"`
	Version int    `json:"version"`
}

type lspTextDocumentPositionParams struct {
	TextDocument lspTextDocumentIdentifier `json:"textDocument"`
	Position     lspPosition               `json:"position"`
}

type lspDidOpenParams struct {
	TextDocument lspTextDocumentItem `json:"textDocument"`
}

type lspDidChangeParams struct {
	TextDocument   lspVersionedTextDocumentIdentifier `json:"textDocument"`
	ContentChanges []struct {
		Text string `json:"text"`
	} `json:"contentChanges"`
}

type lspDidCloseParams struct {
	TextDocument lspTextDocumentIdentifier `json:"textDocument"`
}

type lspFormattingParams struct {
	TextDocument lspTextDocumentIdentifier `json:"textDocument"`
}

type lspReferenceParams struct {
	TextDocument lspTextDocumentIdentifier `json:"textDocument"`
	Position     lspPosition               `json:"position"`
}

type lspRenameParams struct {
	TextDocument lspTextDocumentIdentifier `json:"textDocument"`
	Position     lspPosition               `json:"position"`
	NewName      string                    `json:"newName"`
}

func lspCommand(args []string) error {
	if len(args) != 0 {
		return fmt.Errorf("usage: walk lsp")
	}
	return newLSPServer(os.Stdin, os.Stdout).Serve()
}

func newLSPServer(in io.Reader, out io.Writer) *lspServer {
	return &lspServer{
		in:        bufio.NewReader(in),
		out:       out,
		documents: map[string]lspDocument{},
	}
}

func (s *lspServer) Serve() error {
	for {
		body, err := readLSPMessage(s.in)
		if err == io.EOF {
			return nil
		}
		if err != nil {
			return err
		}
		exit, err := s.handleMessage(body)
		if err != nil {
			return err
		}
		if exit {
			return nil
		}
	}
}

func readLSPMessage(reader *bufio.Reader) ([]byte, error) {
	contentLength := -1
	for {
		line, err := reader.ReadString('\n')
		if err != nil {
			return nil, err
		}
		line = strings.TrimRight(line, "\r\n")
		if line == "" {
			break
		}
		key, value, ok := strings.Cut(line, ":")
		if !ok {
			continue
		}
		if strings.EqualFold(strings.TrimSpace(key), "Content-Length") {
			length, err := strconv.Atoi(strings.TrimSpace(value))
			if err != nil {
				return nil, err
			}
			contentLength = length
		}
	}
	if contentLength < 0 {
		return nil, fmt.Errorf("lsp message missing Content-Length")
	}
	body := make([]byte, contentLength)
	_, err := io.ReadFull(reader, body)
	return body, err
}

func (s *lspServer) handleMessage(body []byte) (bool, error) {
	var message lspMessage
	if err := json.Unmarshal(body, &message); err != nil {
		return false, err
	}
	if message.ID == nil {
		return s.handleNotification(message)
	}
	result, rpcErr := s.handleRequest(message)
	return false, s.writeResponse(message.ID, result, rpcErr)
}

func (s *lspServer) handleNotification(message lspMessage) (bool, error) {
	switch message.Method {
	case "initialized":
		return false, nil
	case "exit":
		return true, nil
	case "textDocument/didOpen":
		var params lspDidOpenParams
		if err := json.Unmarshal(message.Params, &params); err != nil {
			return false, err
		}
		path, err := uriToPath(params.TextDocument.URI)
		if err != nil {
			return false, err
		}
		doc := lspDocument{URI: params.TextDocument.URI, Path: path, Text: params.TextDocument.Text}
		s.documents[doc.URI] = doc
		return false, s.publishDiagnostics(doc)
	case "textDocument/didChange":
		var params lspDidChangeParams
		if err := json.Unmarshal(message.Params, &params); err != nil {
			return false, err
		}
		doc, ok := s.documents[params.TextDocument.URI]
		if !ok {
			path, err := uriToPath(params.TextDocument.URI)
			if err != nil {
				return false, err
			}
			doc = lspDocument{URI: params.TextDocument.URI, Path: path}
		}
		if len(params.ContentChanges) > 0 {
			doc.Text = params.ContentChanges[len(params.ContentChanges)-1].Text
		}
		s.documents[doc.URI] = doc
		return false, s.publishDiagnostics(doc)
	case "textDocument/didClose":
		var params lspDidCloseParams
		if err := json.Unmarshal(message.Params, &params); err != nil {
			return false, err
		}
		doc := s.documents[params.TextDocument.URI]
		delete(s.documents, params.TextDocument.URI)
		if doc.URI == "" {
			doc.URI = params.TextDocument.URI
		}
		return false, s.writeNotification("textDocument/publishDiagnostics", map[string]any{
			"uri":         doc.URI,
			"diagnostics": []lspDiagnostic{},
		})
	default:
		return false, nil
	}
}

func (s *lspServer) handleRequest(message lspMessage) (any, *lspResponseError) {
	switch message.Method {
	case "initialize":
		return map[string]any{
			"capabilities": map[string]any{
				"textDocumentSync": map[string]any{
					"openClose": true,
					"change":    1,
				},
				"documentFormattingProvider": true,
				"hoverProvider":              true,
				"definitionProvider":         true,
				"referencesProvider":         true,
				"renameProvider":             true,
				"completionProvider": map[string]any{
					"triggerCharacters": []string{"."},
				},
			},
		}, nil
	case "shutdown":
		s.shutdown = true
		return nil, nil
	case "textDocument/formatting":
		return s.handleFormatting(message.Params)
	case "textDocument/hover":
		return s.handleHover(message.Params)
	case "textDocument/definition":
		return s.handleDefinition(message.Params)
	case "textDocument/references":
		return s.handleReferences(message.Params)
	case "textDocument/completion":
		return s.handleCompletion(message.Params)
	case "textDocument/rename":
		return s.handleRename(message.Params)
	default:
		return nil, &lspResponseError{Code: -32601, Message: "method not found: " + message.Method}
	}
}

func (s *lspServer) handleFormatting(params json.RawMessage) (any, *lspResponseError) {
	var parsed lspFormattingParams
	if err := json.Unmarshal(params, &parsed); err != nil {
		return nil, invalidParams(err)
	}
	doc, err := s.document(parsed.TextDocument.URI)
	if err != nil {
		return nil, internalError(err)
	}
	formatted, err := walkfmt.Format(doc.Text, doc.Path)
	if err != nil {
		return nil, internalError(err)
	}
	return []lspTextEdit{{Range: fullDocumentRange(doc.Text), NewText: formatted}}, nil
}

func (s *lspServer) handleHover(params json.RawMessage) (any, *lspResponseError) {
	var parsed lspTextDocumentPositionParams
	if err := json.Unmarshal(params, &parsed); err != nil {
		return nil, invalidParams(err)
	}
	doc, err := s.document(parsed.TextDocument.URI)
	if err != nil {
		return nil, internalError(err)
	}
	detail, ok := hoverAtPosition(doc.Path, doc.Text, parsed.Position.Line, parsed.Position.Character)
	if !ok {
		return nil, nil
	}
	return lspHover{Contents: lspMarkupContent{Kind: "markdown", Value: "`" + detail + "`"}}, nil
}

func (s *lspServer) handleDefinition(params json.RawMessage) (any, *lspResponseError) {
	var parsed lspTextDocumentPositionParams
	if err := json.Unmarshal(params, &parsed); err != nil {
		return nil, invalidParams(err)
	}
	doc, err := s.document(parsed.TextDocument.URI)
	if err != nil {
		return nil, internalError(err)
	}
	location, ok := definitionAtPosition(doc.Path, doc.Text, parsed.Position.Line, parsed.Position.Character)
	if !ok {
		return nil, nil
	}
	return lspLocationFromTooling(location), nil
}

func (s *lspServer) handleReferences(params json.RawMessage) (any, *lspResponseError) {
	var parsed lspReferenceParams
	if err := json.Unmarshal(params, &parsed); err != nil {
		return nil, invalidParams(err)
	}
	doc, err := s.document(parsed.TextDocument.URI)
	if err != nil {
		return nil, internalError(err)
	}
	refs := referencesAtPosition(doc.Path, doc.Text, parsed.Position.Line, parsed.Position.Character)
	locations := make([]lspLocation, 0, len(refs))
	for _, ref := range refs {
		locations = append(locations, lspLocationFromTooling(ref))
	}
	return locations, nil
}

func (s *lspServer) handleCompletion(params json.RawMessage) (any, *lspResponseError) {
	var parsed lspTextDocumentPositionParams
	if err := json.Unmarshal(params, &parsed); err != nil {
		return nil, invalidParams(err)
	}
	doc, err := s.document(parsed.TextDocument.URI)
	if err != nil {
		return nil, internalError(err)
	}
	symbols := completionItemsForSource(doc.Path, doc.Text)
	items := make([]lspCompletionItem, 0, len(symbols))
	for _, symbol := range symbols {
		items = append(items, lspCompletionItem{
			Label:         symbol.Name,
			Kind:          completionKind(symbol.Kind),
			Detail:        symbol.Detail,
			Documentation: symbol.Detail,
		})
	}
	return lspCompletionList{IsIncomplete: false, Items: items}, nil
}

func (s *lspServer) handleRename(params json.RawMessage) (any, *lspResponseError) {
	var parsed lspRenameParams
	if err := json.Unmarshal(params, &parsed); err != nil {
		return nil, invalidParams(err)
	}
	doc, err := s.document(parsed.TextDocument.URI)
	if err != nil {
		return nil, internalError(err)
	}
	changes, err := renameEditsAtPosition(doc.Path, doc.Text, parsed.Position.Line, parsed.Position.Character, parsed.NewName)
	if err != nil {
		return nil, invalidParams(err)
	}
	lspChanges := map[string][]lspTextEdit{}
	for path, locations := range changes {
		uri := pathToURI(path)
		for _, location := range locations {
			lspChanges[uri] = append(lspChanges[uri], lspTextEdit{
				Range:   lspRangeFromTooling(location),
				NewText: parsed.NewName,
			})
		}
	}
	return lspWorkspaceEdit{Changes: lspChanges}, nil
}

func (s *lspServer) document(uri string) (lspDocument, error) {
	if doc, ok := s.documents[uri]; ok {
		return doc, nil
	}
	path, err := uriToPath(uri)
	if err != nil {
		return lspDocument{}, err
	}
	source, err := os.ReadFile(path)
	if err != nil {
		return lspDocument{}, err
	}
	return lspDocument{URI: uri, Path: path, Text: string(source)}, nil
}

func (s *lspServer) publishDiagnostics(doc lspDocument) error {
	return s.writeNotification("textDocument/publishDiagnostics", map[string]any{
		"uri":         doc.URI,
		"diagnostics": diagnosticsForLSP(doc.Path, doc.Text),
	})
}

func diagnosticsForLSP(path string, source string) []lspDiagnostic {
	analysis := analyzeSourceForTooling(path, source)
	var diagnostics []lspDiagnostic
	if analysis.Err != nil {
		diagnostics = append(diagnostics, diagnosticFromError(path, analysis.Err))
	}
	for _, warning := range analysis.Warnings {
		if warning.Location.Filename != "" && !sameFilePath(warning.Location.Filename, path) {
			continue
		}
		diagnostics = append(diagnostics, lspDiagnostic{
			Range:    lspRangeFromLocation(warning.Location, 1),
			Severity: 2,
			Source:   "walk",
			Message:  warning.Message,
		})
	}
	return diagnostics
}

func diagnosticFromError(path string, err error) lspDiagnostic {
	var d diagnostic.Diagnostic
	if errors.As(err, &d) {
		message := d.Message
		if d.Category != "" {
			message = d.Category + ": " + message
		}
		if d.Location.Filename == "" || sameFilePath(d.Location.Filename, path) {
			return lspDiagnostic{
				Range:    lspRangeFromLocation(d.Location, 1),
				Severity: 1,
				Source:   "walk",
				Message:  message,
			}
		}
		return lspDiagnostic{
			Range:    lspRangeFromLocation(ast.Location{Filename: path, Line: 1, Column: 1}, 1),
			Severity: 1,
			Source:   "walk",
			Message:  err.Error(),
		}
	}
	return lspDiagnostic{
		Range:    lspRangeFromLocation(ast.Location{Filename: path, Line: 1, Column: 1}, 1),
		Severity: 1,
		Source:   "walk",
		Message:  err.Error(),
	}
}

func (s *lspServer) writeResponse(id *json.RawMessage, result any, rpcErr *lspResponseError) error {
	idValue := json.RawMessage("null")
	if id != nil {
		idValue = *id
	}
	response := lspResponse{JSONRPC: "2.0", ID: idValue, Result: result, Error: rpcErr}
	return s.writePayload(response)
}

func (s *lspServer) writeNotification(method string, params any) error {
	return s.writePayload(map[string]any{
		"jsonrpc": "2.0",
		"method":  method,
		"params":  params,
	})
}

func (s *lspServer) writePayload(payload any) error {
	body, err := json.Marshal(payload)
	if err != nil {
		return err
	}
	if _, err := fmt.Fprintf(s.out, "Content-Length: %d\r\n\r\n", len(body)); err != nil {
		return err
	}
	_, err = s.out.Write(body)
	return err
}

func invalidParams(err error) *lspResponseError {
	return &lspResponseError{Code: -32602, Message: err.Error()}
}

func internalError(err error) *lspResponseError {
	return &lspResponseError{Code: -32603, Message: err.Error()}
}

func completionKind(kind string) int {
	switch kind {
	case "keyword":
		return 14
	case "function":
		return 3
	case "method":
		return 2
	case "field":
		return 5
	case "variable", "parameter":
		return 6
	case "constant":
		return 21
	case "module", "builtin":
		return 9
	case "struct":
		return 23
	default:
		return 1
	}
}

func fullDocumentRange(source string) lspRange {
	lines := strings.Split(source, "\n")
	lastLine := len(lines) - 1
	lastChar := 0
	if lastLine >= 0 {
		lastChar = len(lines[lastLine])
	}
	return lspRange{
		Start: lspPosition{Line: 0, Character: 0},
		End:   lspPosition{Line: lastLine, Character: lastChar},
	}
}

func lspLocationFromTooling(location toolingLocation) lspLocation {
	return lspLocation{URI: pathToURI(location.Filename), Range: lspRangeFromTooling(location)}
}

func lspRangeFromTooling(location toolingLocation) lspRange {
	line := location.Line - 1
	start := location.Column - 1
	end := location.EndColumn - 1
	if line < 0 {
		line = 0
	}
	if start < 0 {
		start = 0
	}
	if end <= start {
		end = start + 1
	}
	return lspRange{
		Start: lspPosition{Line: line, Character: start},
		End:   lspPosition{Line: line, Character: end},
	}
}

func lspRangeFromLocation(location ast.Location, fallbackLength int) lspRange {
	if fallbackLength < 1 {
		fallbackLength = 1
	}
	return lspRangeFromTooling(toolingLocation{
		Filename:  location.Filename,
		Line:      location.Line,
		Column:    location.Column,
		EndColumn: location.Column + fallbackLength,
	})
}

func uriToPath(raw string) (string, error) {
	parsed, err := url.Parse(raw)
	if err != nil {
		return "", err
	}
	if parsed.Scheme != "file" {
		return "", fmt.Errorf("unsupported URI scheme %q", parsed.Scheme)
	}
	path, err := url.PathUnescape(parsed.Path)
	if err != nil {
		return "", err
	}
	return filepath.FromSlash(path), nil
}

func pathToURI(path string) string {
	abs, err := filepath.Abs(path)
	if err != nil {
		abs = path
	}
	return (&url.URL{Scheme: "file", Path: filepath.ToSlash(abs)}).String()
}

func sameFilePath(left string, right string) bool {
	leftAbs, leftErr := filepath.Abs(left)
	rightAbs, rightErr := filepath.Abs(right)
	if leftErr == nil && rightErr == nil {
		return filepath.Clean(leftAbs) == filepath.Clean(rightAbs)
	}
	return filepath.Clean(left) == filepath.Clean(right)
}
