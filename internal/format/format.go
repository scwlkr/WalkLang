package format

import (
	"strings"

	"walklang/internal/lexer"
)

func Format(source string, filename string) (string, error) {
	lines, err := lexer.Lex(source, filename)
	if err != nil {
		return "", err
	}
	var out strings.Builder
	indentStack := []int{0}
	for _, line := range lines {
		for len(indentStack) > 1 && line.Indent < indentStack[len(indentStack)-1] {
			indentStack = indentStack[:len(indentStack)-1]
		}
		if line.Indent > indentStack[len(indentStack)-1] {
			indentStack = append(indentStack, line.Indent)
		}
		depth := len(indentStack) - 1
		out.WriteString(strings.Repeat(" ", depth*4))
		out.WriteString(formatTokens(line.Tokens))
		out.WriteByte('\n')
	}
	return out.String(), nil
}

func formatTokens(tokens []lexer.Token) string {
	var out strings.Builder
	for i, token := range tokens {
		if i > 0 && needsSpace(tokens[i-1].Value, token.Value) {
			out.WriteByte(' ')
		}
		if token.Kind == lexer.TokenString {
			out.WriteString("'")
			out.WriteString(escapeString(token.Value))
			out.WriteString("'")
		} else {
			out.WriteString(token.Value)
		}
	}
	return out.String()
}

func escapeString(value string) string {
	replacer := strings.NewReplacer("\\", "\\\\", "'", "\\'", "\n", "\\n", "\t", "\\t")
	return replacer.Replace(value)
}

func needsSpace(prev string, current string) bool {
	if current == ")" || current == "]" || current == "," || current == ":" || current == "." {
		return false
	}
	if current == "(" {
		return false
	}
	if prev == "(" || prev == "[" || prev == "." {
		return false
	}
	if prev == "," || prev == ":" {
		return true
	}
	if prev == "=" || current == "=" {
		return true
	}
	return true
}
