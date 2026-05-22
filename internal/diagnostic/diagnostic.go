package diagnostic

import (
	"errors"
	"fmt"
	"os"
	"strings"

	"walklang/internal/ast"
)

type Diagnostic struct {
	Location ast.Location
	Category string
	Message  string
}

func Errorf(location ast.Location, format string, args ...any) error {
	category, message := splitCategory(fmt.Sprintf(format, args...))
	return Diagnostic{Location: location, Category: category, Message: message}
}

func (d Diagnostic) Error() string {
	message := d.Message
	if d.Category != "" {
		message = d.Category + ": " + message
	}
	if d.Location.Filename == "" {
		return message
	}
	return fmt.Sprintf("%s:%d:%d: %s", d.Location.Filename, d.Location.Line, d.Location.Column, message)
}

func FormatError(err error) string {
	var d Diagnostic
	if !errors.As(err, &d) {
		return err.Error()
	}
	return Format(d)
}

func FormatWarning(location ast.Location, message string) string {
	return Format(Diagnostic{Location: location, Category: "warning", Message: message})
}

func Format(d Diagnostic) string {
	header := d.Error()
	line, ok := sourceLine(d.Location)
	if !ok || d.Location.Column < 1 {
		return header
	}

	caret := strings.Repeat(" ", d.Location.Column-1) + "^"
	if suggestion := suggestionFor(d); suggestion != "" {
		caret += " " + suggestion
	}
	return header + "\n\n" + line + "\n" + caret
}

func splitCategory(text string) (string, string) {
	for _, category := range []string{
		"syntax error",
		"type error",
		"name error",
		"module error",
		"warning",
		"internal error",
	} {
		prefix := category + ": "
		if strings.HasPrefix(text, prefix) {
			return category, strings.TrimPrefix(text, prefix)
		}
	}
	return "", text
}

func sourceLine(location ast.Location) (string, bool) {
	if location.Filename == "" || location.Line < 1 {
		return "", false
	}
	source, err := os.ReadFile(location.Filename)
	if err != nil {
		return "", false
	}
	lines := strings.Split(string(source), "\n")
	if location.Line > len(lines) {
		return "", false
	}
	return lines[location.Line-1], true
}

func suggestionFor(d Diagnostic) string {
	switch d.Category {
	case "syntax error":
		return syntaxSuggestion(d.Message)
	case "type error":
		return typeSuggestion(d.Message)
	case "name error":
		return nameSuggestion(d.Message)
	case "warning":
		return warningSuggestion(d.Message)
	}
	return ""
}

func syntaxSuggestion(message string) string {
	switch {
	case strings.Contains(message, "tabs are invalid"):
		return "replace tabs with spaces"
	case strings.Contains(message, "unexpected indentation"):
		return "align indentation with the surrounding block"
	case strings.Contains(message, "expected expression block"):
		return "indent the expression on the next line"
	}
	return ""
}

func typeSuggestion(message string) string {
	if before, after, ok := strings.Cut(message, " is "); ok {
		if expected, got, ok := strings.Cut(after, ", got "); ok && before != "" && expected != "" && got != "" {
			return got + " cannot initialize " + expected
		}
	}
	if strings.HasPrefix(message, "function returns ") {
		if expected, got, ok := strings.Cut(strings.TrimPrefix(message, "function returns "), ", got "); ok {
			return got + " cannot be returned from function returning " + expected
		}
	}
	switch {
	case strings.Contains(message, "condition must be bool"):
		return "use a bool expression for the condition"
	case strings.Contains(message, "assert needs bool"):
		return "assert a bool expression"
	case strings.Contains(message, "repeat count must be int"):
		return "use an int expression for the repeat count"
	}
	return ""
}

func nameSuggestion(message string) string {
	if strings.HasPrefix(message, "module ") && strings.Contains(message, " is not imported") {
		module := strings.TrimPrefix(message, "module ")
		module, _, _ = strings.Cut(module, " ")
		if module != "" {
			return "add imp: " + module
		}
	}
	if strings.Contains(message, " is not defined") {
		return "define the name before using it"
	}
	return ""
}

func warningSuggestion(message string) string {
	switch {
	case strings.Contains(message, "shadows outer name"):
		return "rename this binding or assign to the existing name"
	case strings.Contains(message, "unreachable statement"):
		return "remove this statement or move it before the terminating statement"
	}
	return ""
}
