package lexer

import (
	"fmt"
	"strings"
	"unicode"

	"walklang/internal/ast"
)

type TokenKind string

const (
	TokenName   TokenKind = "NAME"
	TokenNumber TokenKind = "NUMBER"
	TokenString TokenKind = "STRING"
	TokenSymbol TokenKind = "SYMBOL"
)

type Token struct {
	Kind     TokenKind
	Value    string
	Location ast.Location
}

type Line struct {
	Indent   int
	Tokens   []Token
	Location ast.Location
}

func Lex(source string, filename string) ([]Line, error) {
	var lines []Line
	for lineNumber, raw := range strings.Split(source, "\n") {
		line := lineNumber + 1
		if tab := strings.IndexRune(raw, '\t'); tab >= 0 {
			return nil, errorAt(ast.Location{Filename: filename, Line: line, Column: tab + 1}, "syntax error: tabs are invalid in v0 indentation")
		}

		body := stripComment(raw)
		if strings.TrimSpace(body) == "" {
			continue
		}

		indent := len(body) - len(strings.TrimLeft(body, " "))
		tokens, err := tokenizeLine(body[indent:], filename, line, indent+1)
		if err != nil {
			return nil, err
		}
		if len(tokens) > 0 {
			lines = append(lines, Line{Indent: indent, Tokens: tokens, Location: tokens[0].Location})
		}
	}
	return lines, nil
}

func stripComment(line string) string {
	inString := false
	escaped := false
	for i, r := range line {
		if inString {
			switch {
			case escaped:
				escaped = false
			case r == '\\':
				escaped = true
			case r == '\'':
				inString = false
			}
			continue
		}
		if r == '\'' {
			inString = true
			continue
		}
		if r == '#' {
			return line[:i]
		}
	}
	return line
}

func tokenizeLine(text string, filename string, line int, startColumn int) ([]Token, error) {
	var tokens []Token
	for i := 0; i < len(text); {
		ch := rune(text[i])
		column := startColumn + i

		if unicode.IsSpace(ch) {
			i++
			continue
		}

		location := ast.Location{Filename: filename, Line: line, Column: column}
		if ch == '\'' {
			value, next, err := readString(text, i, location)
			if err != nil {
				return nil, err
			}
			tokens = append(tokens, Token{Kind: TokenString, Value: value, Location: location})
			i = next
			continue
		}

		if startsNumber(text, i) {
			value, next := readNumber(text, i)
			tokens = append(tokens, Token{Kind: TokenNumber, Value: value, Location: location})
			i = next
			continue
		}

		if i+1 < len(text) {
			two := text[i : i+2]
			if two == ">=" || two == "<=" || two == "==" || two == "!=" {
				tokens = append(tokens, Token{Kind: TokenSymbol, Value: two, Location: location})
				i += 2
				continue
			}
		}

		if strings.ContainsRune("()[],:=+-*/^><?.", ch) {
			tokens = append(tokens, Token{Kind: TokenSymbol, Value: string(ch), Location: location})
			i++
			continue
		}

		if unicode.IsLetter(ch) || ch == '_' {
			value, next := readName(text, i)
			tokens = append(tokens, Token{Kind: TokenName, Value: value, Location: location})
			i = next
			continue
		}

		return nil, errorAt(location, "syntax error: unexpected character %q", ch)
	}
	return tokens, nil
}

func startsNumber(text string, i int) bool {
	if text[i] >= '0' && text[i] <= '9' {
		return true
	}
	return text[i] == '-' && i+1 < len(text) && text[i+1] >= '0' && text[i+1] <= '9' &&
		(i == 0 || unicode.IsSpace(rune(text[i-1])) || strings.ContainsRune("([,=:", rune(text[i-1])))
}

func readNumber(text string, i int) (string, int) {
	start := i
	if text[i] == '-' {
		i++
	}
	seenDot := false
	for i < len(text) {
		if text[i] == '.' && !seenDot {
			seenDot = true
			i++
			continue
		}
		if text[i] < '0' || text[i] > '9' {
			break
		}
		i++
	}
	return text[start:i], i
}

func readName(text string, i int) (string, int) {
	start := i
	for i < len(text) {
		r := rune(text[i])
		if !unicode.IsLetter(r) && !unicode.IsDigit(r) && r != '_' {
			break
		}
		i++
	}
	return text[start:i], i
}

func readString(text string, i int, location ast.Location) (string, int, error) {
	i++
	var out strings.Builder
	escaped := false
	for i < len(text) {
		ch := text[i]
		if escaped {
			switch ch {
			case 'n':
				out.WriteByte('\n')
			case 't':
				out.WriteByte('\t')
			case '\\', '\'':
				out.WriteByte(ch)
			default:
				out.WriteByte(ch)
			}
			escaped = false
		} else if ch == '\\' {
			escaped = true
		} else if ch == '\'' {
			return out.String(), i + 1, nil
		} else {
			out.WriteByte(ch)
		}
		i++
	}
	return "", 0, errorAt(location, "syntax error: unterminated string")
}

func errorAt(location ast.Location, format string, args ...any) error {
	return fmt.Errorf("%s:%d: %s", location.Filename, location.Line, fmt.Sprintf(format, args...))
}
