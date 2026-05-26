package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"html"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
)

type docPage struct {
	Source      string
	Output      string
	Title       string
	NavTitle    string
	Group       string
	HideFromNav bool
}

type searchEntry struct {
	Title   string `json:"title"`
	Section string `json:"section,omitempty"`
	Group   string `json:"group"`
	URL     string `json:"url"`
	Summary string `json:"summary"`
	Text    string `json:"text"`
	Level   int    `json:"level"`
}

type searchSection struct {
	Heading string
	Level   int
	Anchor  string
	Lines   []string
}

func main() {
	docsDir := flag.String("docs", "docs", "documentation source directory")
	publicDir := flag.String("public", "public", "static site output directory")
	flag.Parse()

	if err := buildSite(*docsDir, *publicDir); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}

func buildSite(docsDir string, publicDir string) error {
	pages := sitePages()
	for i := range pages {
		pages[i].Source = filepath.Join(docsDir, pages[i].Source)
	}
	if err := hydratePageTitles(pages); err != nil {
		return err
	}

	if err := os.RemoveAll(publicDir); err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Join(publicDir, "assets"), 0o755); err != nil {
		return err
	}
	if err := copyFile("logo_WalkLang.svg", filepath.Join(publicDir, "assets", "logo.svg")); err != nil {
		return err
	}
	if err := copyFile("icon_WalkLang.svg", filepath.Join(publicDir, "assets", "icon.svg")); err != nil {
		return err
	}
	if err := copyFile("icon_WalkLang.svg", filepath.Join(publicDir, "favicon.svg")); err != nil {
		return err
	}
	if err := copyFile(filepath.Join("site", "assets", "site.css"), filepath.Join(publicDir, "assets", "site.css")); err != nil {
		return err
	}
	if err := copyFile(filepath.Join("site", "assets", "site.js"), filepath.Join(publicDir, "assets", "site.js")); err != nil {
		return err
	}
	if err := os.WriteFile(filepath.Join(publicDir, "CNAME"), []byte("walklang.wlkrlabs.com\n"), 0o644); err != nil {
		return err
	}
	if err := os.WriteFile(filepath.Join(publicDir, ".nojekyll"), []byte(""), 0o644); err != nil {
		return err
	}

	if err := writeHTML(publicDir, "index.html", renderHome("index.html", pages)); err != nil {
		return err
	}
	if err := writeHTML(publicDir, "docs/reference/index.html", renderReferenceIndex("docs/reference/index.html", pages)); err != nil {
		return err
	}
	if err := copyFile(filepath.Join(docsDir, "reference", "api.md"), filepath.Join(publicDir, "docs", "reference", "api.md")); err != nil {
		return err
	}
	if err := copyFile(filepath.Join(docsDir, "reference", "api.json"), filepath.Join(publicDir, "docs", "reference", "api.json")); err != nil {
		return err
	}
	searchIndex, err := searchIndexJSON(pages)
	if err != nil {
		return err
	}
	if err := writeFile(publicDir, "docs/search.json", searchIndex); err != nil {
		return err
	}

	for _, page := range pages {
		source, err := os.ReadFile(page.Source)
		if err != nil {
			return err
		}
		title := page.Title
		if title == "" {
			title = markdownTitle(string(source))
		}
		body := markdownToHTML(string(source), page.Output)
		if err := writeHTML(publicDir, page.Output, layout(page.Output, title, pages, body)); err != nil {
			return err
		}
	}
	return validateSiteLinks(publicDir)
}

func hydratePageTitles(pages []docPage) error {
	for i := range pages {
		if pages[i].Title != "" {
			continue
		}
		source, err := os.ReadFile(pages[i].Source)
		if err != nil {
			return err
		}
		pages[i].Title = markdownTitle(string(source))
	}
	return nil
}

func sitePages() []docPage {
	return []docPage{
		{Source: "README.md", Output: "docs/index.html", Title: "WalkLang Documentation", NavTitle: "Documentation", Group: "Start"},
		{Source: "INSTALL.md", Output: "docs/INSTALL.html", NavTitle: "Install", Group: "Start"},
		{Source: "PROJECTS.md", Output: "docs/PROJECTS.html", NavTitle: "Project Mode", Group: "Start"},
		{Source: "SYNTAX.md", Output: "docs/SYNTAX.html", NavTitle: "Syntax", Group: "Language"},
		{Source: "SPEC.md", Output: "docs/SPEC.html", NavTitle: "Specification", Group: "Language"},
		{Source: "LANGUAGE_CONCEPTS.md", Output: "docs/LANGUAGE_CONCEPTS.html", NavTitle: "Language Concepts", Group: "Language"},
		{Source: "STDLIB.md", Output: "docs/STDLIB.html", NavTitle: "Standard Library", Group: "Language"},
		{Source: "ERRORS.md", Output: "docs/ERRORS.html", NavTitle: "Diagnostics", Group: "Language"},
		{Source: "COMPATIBILITY.md", Output: "docs/COMPATIBILITY.html", NavTitle: "Compatibility", Group: "Language"},
		{Source: "MIGRATING.md", Output: "docs/MIGRATING.html", NavTitle: "Migration", Group: "Language"},
		{Source: "DEPRECATION.md", Output: "docs/DEPRECATION.html", NavTitle: "Deprecation", Group: "Language"},
		{Source: "STABLE_FEATURES.md", Output: "docs/STABLE_FEATURES.html", Group: "Releases", HideFromNav: true},
		{Source: "EXPERIMENTAL_COMPOSITION.md", Output: "docs/EXPERIMENTAL_COMPOSITION.html", Group: "Releases", HideFromNav: true},
		{Source: "PACKAGES.md", Output: "docs/PACKAGES.html", Group: "Releases", HideFromNav: true},
		{Source: "TOOLING.md", Output: "docs/TOOLING.html", NavTitle: "Editor And Docs Tools", Group: "Tools"},
		{Source: "RUNTIME_BACKEND.md", Output: "docs/RUNTIME_BACKEND.html", NavTitle: "Runtime And Backend", Group: "Tools"},
		{Source: "NETWORKING.md", Output: "docs/NETWORKING.html", NavTitle: "Draft Networking", Group: "Tools"},
		{Source: "RICH_RUNTIMES.md", Output: "docs/RICH_RUNTIMES.html", NavTitle: "Rich Runtimes", Group: "Tools"},
		{Source: "reference/api.md", Output: "docs/reference/api.html", Title: "WalkLang API Reference", NavTitle: "API Reference", Group: "Reference"},
		{Source: "DOCS_SITE.md", Output: "docs/DOCS_SITE.html", NavTitle: "Docs Site", Group: "Reference"},
		{Source: "ARCHITECTURE.md", Output: "docs/ARCHITECTURE.html", NavTitle: "Architecture", Group: "Project"},
		{Source: "DESIGN_RULES.md", Output: "docs/DESIGN_RULES.html", NavTitle: "Design Rules", Group: "Project"},
		{Source: "EXPLICIT_SYSTEMS_TRACK.md", Output: "docs/EXPLICIT_SYSTEMS_TRACK.html", NavTitle: "Explicit Systems Track", Group: "Project"},
		{Source: "DOCS_STYLE_GUIDE.md", Output: "docs/DOCS_STYLE_GUIDE.html", NavTitle: "Docs Style Guide", Group: "Project"},
		{Source: "PURPOSE.md", Output: "docs/PURPOSE.html", NavTitle: "Purpose", Group: "Project"},
		{Source: "ROADMAP.md", Output: "docs/ROADMAP.html", NavTitle: "Roadmap", Group: "Project"},
		{Source: "STATUS.md", Output: "docs/STATUS.html", NavTitle: "Status", Group: "Project"},
		{Source: "RELEASE_NOTES.md", Output: "docs/RELEASE_NOTES.html", NavTitle: "Version History", Group: "Releases"},
	}
}

func renderHome(current string, pages []docPage) string {
	install := relURL(current, "docs/INSTALL.html")
	docs := relURL(current, "docs/index.html")
	reference := relURL(current, "docs/reference/api.html")
	body := strings.Join([]string{
		`<section class="hero">`,
		`<div>`,
		`<h1>WalkLang Docs</h1>`,
		`<p>A small compiled language with deterministic syntax, native C-backed output, project tooling, and generated API reference docs.</p>`,
		`<div class="quick-links">`,
		`<a class="button" href="` + docs + `">Read the docs</a>`,
		`<a class="button secondary" href="` + install + `">Install WalkLang</a>`,
		`<a class="button secondary" href="` + reference + `">API reference</a>`,
		`</div>`,
		`</div>`,
		`<div class="hero-visual">`,
		`<img src="assets/logo.svg" alt="WalkLang logo">`,
		`<pre><code>walk init hello
walk check
walk build
./build/hello</code></pre>`,
		`</div>`,
		`</section>`,
		`<section class="grid" aria-label="Documentation entry points">`,
		tile("Build", "Install the compiler, create a project, and produce a native executable.", install),
		tile("Learn", "Read the stable syntax, standard library, diagnostics, and compatibility docs.", docs),
		tile("Reference", "Browse generated API docs produced by structured comments and walk docs.", reference),
		`</section>`,
	}, "\n")
	return layout(current, "WalkLang Docs", pages, body)
}

func renderReferenceIndex(current string, pages []docPage) string {
	api := relURL(current, "docs/reference/api.html")
	json := relURL(current, "docs/reference/api.json")
	raw := relURL(current, "docs/reference/api.md")
	body := strings.Join([]string{
		`<article class="doc">`,
		`<h1>Reference</h1>`,
		`<p>The reference section is generated from structured comments in real WalkLang source. The Markdown and JSON outputs are committed so the hosted site and tooling consume the same API index.</p>`,
		`<div class="grid">`,
		tile("API Reference", "Rendered generated Markdown reference.", api),
		tile("docs.json", "Machine-readable symbol index from walk docs.", json),
		tile("Raw Markdown", "Generated Markdown artifact.", raw),
		`</div>`,
		`</article>`,
	}, "\n")
	return layout(current, "Reference", pages, body)
}

func tile(title string, body string, href string) string {
	return `<a class="tile" href="` + href + `"><h2>` + html.EscapeString(title) + `</h2><p>` + html.EscapeString(body) + `</p></a>`
}

func layout(current string, title string, pages []docPage, body string) string {
	assetPrefix := assetPrefixFor(current)
	var out strings.Builder
	out.WriteString("<!doctype html>\n")
	out.WriteString(`<html lang="en">` + "\n")
	out.WriteString("<head>\n")
	out.WriteString(`<meta charset="utf-8">` + "\n")
	out.WriteString(`<meta name="viewport" content="width=device-width, initial-scale=1">` + "\n")
	out.WriteString("<title>" + html.EscapeString(title) + " - WalkLang</title>\n")
	out.WriteString(`<link rel="icon" type="image/svg+xml" href="` + assetPrefix + `favicon.svg">` + "\n")
	out.WriteString(`<link rel="stylesheet" href="` + assetPrefix + `assets/site.css">` + "\n")
	out.WriteString(`<script defer src="` + assetPrefix + `assets/site.js"></script>` + "\n")
	out.WriteString("</head>\n<body>\n")
	out.WriteString(`<div class="shell">` + "\n")
	out.WriteString(renderSidebar(current, pages))
	out.WriteString(`<main class="content">` + "\n")
	out.WriteString(`<div class="topbar"><a href="` + relURL(current, "docs/index.html") + `">Docs</a><a href="` + relURL(current, "docs/reference/api.html") + `">Reference</a><a href="https://github.com/scwlkr/WalkLang">GitHub</a></div>` + "\n")
	out.WriteString(`<div class="page">` + "\n")
	out.WriteString(body)
	out.WriteString(`<footer class="footer">Generated from repository docs and structured WalkLang source comments.</footer>` + "\n")
	out.WriteString("</div>\n</main>\n</div>\n</body>\n</html>\n")
	return out.String()
}

func renderSidebar(current string, pages []docPage) string {
	var out strings.Builder
	out.WriteString(`<aside class="sidebar">` + "\n")
	out.WriteString(`<a class="brand" href="` + relURL(current, "index.html") + `"><img src="` + assetPrefixFor(current) + `assets/icon.svg" alt=""><span><strong>WalkLang</strong><span>Docs and reference</span></span></a>` + "\n")
	out.WriteString(renderSearch(current))
	out.WriteString(`<nav class="nav" aria-label="Documentation">` + "\n")
	groups := groupedPages(pages)
	for _, group := range sortedGroups(groups) {
		out.WriteString("<h2>" + html.EscapeString(group) + "</h2>\n")
		for _, page := range groups[group] {
			if page.HideFromNav {
				continue
			}
			title := page.NavTitle
			if title == "" {
				title = page.Title
			}
			if title == "" {
				title = strings.TrimSuffix(filepath.Base(page.Output), ".html")
			}
			currentAttr := ""
			if page.Output == current {
				currentAttr = ` aria-current="page"`
			}
			out.WriteString(`<a href="` + relURL(current, page.Output) + `"` + currentAttr + `>` + html.EscapeString(shortTitle(title)) + `</a>` + "\n")
		}
		if group == "Reference" {
			out.WriteString(`<a href="` + relURL(current, "docs/reference/index.html") + `">Reference Index</a>` + "\n")
		}
	}
	out.WriteString("</nav>\n</aside>\n")
	return out.String()
}

func renderSearch(current string) string {
	root := assetPrefixFor(current)
	return strings.Join([]string{
		`<form class="doc-search" role="search" data-site-root="` + root + `" data-search-index="` + root + `docs/search.json">`,
		`<label for="doc-search-input">Search docs</label>`,
		`<input id="doc-search-input" type="search" autocomplete="off" placeholder="try push or array.push">`,
		`<div class="search-results" id="doc-search-results" aria-live="polite"></div>`,
		`</form>`,
	}, "\n") + "\n"
}

func groupedPages(pages []docPage) map[string][]docPage {
	groups := map[string][]docPage{}
	for _, page := range pages {
		if page.HideFromNav {
			continue
		}
		groups[page.Group] = append(groups[page.Group], page)
	}
	return groups
}

func sortedGroups(groups map[string][]docPage) []string {
	order := []string{"Start", "Language", "Reference", "Tools", "Project", "Releases"}
	seen := map[string]bool{}
	var result []string
	for _, group := range order {
		if _, ok := groups[group]; ok {
			result = append(result, group)
			seen[group] = true
		}
	}
	var rest []string
	for group := range groups {
		if !seen[group] {
			rest = append(rest, group)
		}
	}
	sort.Strings(rest)
	return append(result, rest...)
}

func shortTitle(title string) string {
	title = strings.TrimPrefix(title, "WalkLang ")
	title = strings.TrimPrefix(title, "Official ")
	return title
}

func markdownTitle(source string) string {
	for _, line := range strings.Split(source, "\n") {
		if strings.HasPrefix(line, "# ") {
			return strings.TrimSpace(strings.TrimPrefix(line, "# "))
		}
	}
	return "Documentation"
}

func searchIndexJSON(pages []docPage) ([]byte, error) {
	var entries []searchEntry
	for _, page := range pages {
		source, err := os.ReadFile(page.Source)
		if err != nil {
			return nil, err
		}
		title := page.Title
		if title == "" {
			title = markdownTitle(string(source))
		}
		for _, section := range searchSections(string(source)) {
			sectionTitle := section.Heading
			if sectionTitle == "" {
				sectionTitle = title
			}
			url := filepath.ToSlash(page.Output)
			if section.Anchor != "" {
				url += "#" + section.Anchor
			}
			entries = append(entries, searchEntry{
				Title:   title,
				Section: sectionTitle,
				Group:   page.Group,
				URL:     url,
				Summary: searchSummary(section.Lines),
				Text:    searchText(title, section),
				Level:   section.Level,
			})
		}
	}
	data, err := json.MarshalIndent(entries, "", "  ")
	if err != nil {
		return nil, err
	}
	return append(data, '\n'), nil
}

func searchSections(source string) []searchSection {
	var sections []searchSection
	current := searchSection{}
	inCode := false

	flush := func() {
		if current.Heading == "" && len(current.Lines) == 0 {
			return
		}
		sections = append(sections, current)
	}

	for _, line := range strings.Split(source, "\n") {
		trimmed := strings.TrimSpace(line)
		if strings.HasPrefix(trimmed, "```") {
			inCode = !inCode
			current.Lines = append(current.Lines, line)
			continue
		}
		if !inCode {
			if level, text, ok := heading(trimmed); ok {
				flush()
				current = searchSection{
					Heading: text,
					Level:   level,
					Anchor:  slug(text),
				}
				continue
			}
		}
		current.Lines = append(current.Lines, line)
	}
	flush()

	if len(sections) == 0 {
		return []searchSection{{Lines: []string{source}}}
	}
	return sections
}

func searchText(title string, section searchSection) string {
	parts := []string{title, section.Heading}
	parts = append(parts, section.Lines...)
	return cleanSearchText(strings.Join(parts, " "))
}

func searchSummary(lines []string) string {
	prose := summaryCandidates(lines, false)
	if len(prose) == 0 {
		prose = summaryCandidates(lines, true)
	}
	if len(prose) == 0 {
		return "Open this section in the WalkLang docs."
	}
	return shortenSummary(strings.Join(prose, " "), 210)
}

func summaryCandidates(lines []string, includeCode bool) []string {
	var result []string
	inCode := false
	for _, line := range lines {
		trimmed := strings.TrimSpace(line)
		if strings.HasPrefix(trimmed, "```") {
			inCode = !inCode
			continue
		}
		if trimmed == "" || strings.HasPrefix(trimmed, "---") {
			continue
		}
		if inCode && !includeCode {
			continue
		}
		if !includeCode && summaryMetadataLine(trimmed) {
			continue
		}
		cleaned := cleanSummaryLine(trimmed, inCode)
		if cleaned == "" {
			continue
		}
		result = append(result, cleaned)
		if len(result) >= 2 {
			break
		}
	}
	return result
}

func summaryMetadataLine(line string) bool {
	for _, prefix := range []string{"Date:", "Status:", "Stability:", "Effect status:"} {
		if strings.HasPrefix(line, prefix) {
			return true
		}
	}
	return false
}

func cleanSummaryLine(line string, inCode bool) string {
	if strings.HasPrefix(line, "- ") {
		line = strings.TrimSpace(strings.TrimPrefix(line, "- "))
	}
	line = strings.TrimPrefix(line, "> ")
	line = strings.TrimSpace(line)
	if inCode {
		return line
	}
	return cleanSearchText(line)
}

func cleanSearchText(source string) string {
	source = linkRE.ReplaceAllString(source, "$1")
	source = strings.ReplaceAll(source, "`", "")
	source = strings.ReplaceAll(source, "\\", "")
	source = strings.ReplaceAll(source, "#", "")
	source = strings.ReplaceAll(source, "*", "")
	source = strings.ReplaceAll(source, "_", " ")
	source = strings.ReplaceAll(source, "[", "")
	source = strings.ReplaceAll(source, "]", "")
	source = strings.ReplaceAll(source, "(", " ")
	source = strings.ReplaceAll(source, ")", " ")
	source = strings.ReplaceAll(source, "{", " ")
	source = strings.ReplaceAll(source, "}", " ")
	return strings.Join(strings.Fields(source), " ")
}

func shortenSummary(text string, limit int) string {
	if len(text) <= limit {
		return text
	}
	cut := strings.LastIndex(text[:limit], " ")
	if cut < 80 {
		cut = limit
	}
	return strings.TrimSpace(text[:cut]) + "..."
}

func markdownToHTML(source string, current string) string {
	var out strings.Builder
	var paragraph []string
	inList := false
	inCode := false
	codeLang := ""
	var code bytes.Buffer

	flushParagraph := func() {
		if len(paragraph) == 0 {
			return
		}
		out.WriteString("<p>" + inlineMarkdown(strings.Join(paragraph, " "), current) + "</p>\n")
		paragraph = nil
	}
	closeList := func() {
		if inList {
			out.WriteString("</ul>\n")
			inList = false
		}
	}
	flushCode := func() {
		class := ""
		if codeLang != "" {
			class = ` class="language-` + html.EscapeString(codeLang) + `"`
		}
		out.WriteString("<pre><code" + class + ">" + html.EscapeString(strings.TrimRight(code.String(), "\n")) + "</code></pre>\n")
		code.Reset()
		codeLang = ""
	}

	for _, line := range strings.Split(source, "\n") {
		trimmed := strings.TrimSpace(line)
		if strings.HasPrefix(trimmed, "```") {
			if inCode {
				flushCode()
				inCode = false
				continue
			}
			flushParagraph()
			closeList()
			inCode = true
			codeLang = strings.TrimSpace(strings.TrimPrefix(trimmed, "```"))
			continue
		}
		if inCode {
			code.WriteString(line + "\n")
			continue
		}
		if trimmed == "" {
			flushParagraph()
			closeList()
			continue
		}
		if level, text, ok := heading(trimmed); ok {
			flushParagraph()
			closeList()
			id := slug(text)
			out.WriteString(fmt.Sprintf("<h%d id=\"%s\">%s</h%d>\n", level, id, inlineMarkdown(text, current), level))
			continue
		}
		if strings.HasPrefix(trimmed, "- ") {
			flushParagraph()
			if !inList {
				out.WriteString("<ul>\n")
				inList = true
			}
			out.WriteString("<li>" + inlineMarkdown(strings.TrimSpace(strings.TrimPrefix(trimmed, "- ")), current) + "</li>\n")
			continue
		}
		paragraph = append(paragraph, trimmed)
	}
	if inCode {
		flushCode()
	}
	flushParagraph()
	closeList()
	return `<article class="doc">` + "\n" + out.String() + "</article>\n"
}

func heading(line string) (int, string, bool) {
	level := 0
	for level < len(line) && line[level] == '#' {
		level++
	}
	if level == 0 || level > 4 || level >= len(line) || line[level] != ' ' {
		return 0, "", false
	}
	return level, strings.TrimSpace(line[level+1:]), true
}

var linkRE = regexp.MustCompile(`\[([^\]]+)\]\(([^)]+)\)`)
var siteRefRE = regexp.MustCompile(`(?:href|src)="([^"]+)"`)

func inlineMarkdown(text string, current string) string {
	parts := strings.Split(text, "`")
	var out strings.Builder
	for i, part := range parts {
		if i%2 == 1 {
			out.WriteString("<code>" + html.EscapeString(part) + "</code>")
			continue
		}
		escaped := html.EscapeString(part)
		escaped = linkRE.ReplaceAllStringFunc(escaped, func(match string) string {
			parts := linkRE.FindStringSubmatch(match)
			if len(parts) != 3 {
				return match
			}
			href := rewriteMarkdownHref(parts[2], current)
			return `<a href="` + html.EscapeString(href) + `">` + parts[1] + `</a>`
		})
		out.WriteString(escaped)
	}
	return out.String()
}

func rewriteMarkdownHref(href string, current string) string {
	if strings.HasPrefix(href, "http://") || strings.HasPrefix(href, "https://") || strings.HasPrefix(href, "#") || strings.HasPrefix(href, "mailto:") {
		return href
	}
	anchor := ""
	if index := strings.Index(href, "#"); index >= 0 {
		anchor = href[index:]
		href = href[:index]
	}
	if strings.HasSuffix(href, ".md") {
		target := strings.TrimSuffix(href, ".md") + ".html"
		if filepath.Base(href) == "README.md" {
			target = filepath.Join(filepath.Dir(href), "index.html")
		}
		return relURL(current, filepath.ToSlash(filepath.Join("docs", target))) + anchor
	}
	return href + anchor
}

func slug(text string) string {
	text = strings.ToLower(text)
	var out strings.Builder
	lastDash := false
	for _, r := range text {
		switch {
		case r >= 'a' && r <= 'z', r >= '0' && r <= '9':
			out.WriteRune(r)
			lastDash = false
		case !lastDash:
			out.WriteByte('-')
			lastDash = true
		}
	}
	return strings.Trim(out.String(), "-")
}

func assetPrefixFor(current string) string {
	depth := strings.Count(filepath.ToSlash(filepath.Dir(current)), "/")
	if filepath.Dir(current) == "." {
		return ""
	}
	return strings.Repeat("../", depth+1)
}

func relURL(from string, to string) string {
	base := filepath.Dir(from)
	if base == "." {
		return filepath.ToSlash(to)
	}
	rel, err := filepath.Rel(base, to)
	if err != nil {
		return filepath.ToSlash(to)
	}
	return filepath.ToSlash(rel)
}

func writeHTML(root string, output string, contents string) error {
	return writeFile(root, output, []byte(contents))
}

func writeFile(root string, output string, contents []byte) error {
	path := filepath.Join(root, output)
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		return err
	}
	return os.WriteFile(path, contents, 0o644)
}

func copyFile(source string, target string) error {
	data, err := os.ReadFile(source)
	if err != nil {
		return err
	}
	if err := os.MkdirAll(filepath.Dir(target), 0o755); err != nil {
		return err
	}
	return os.WriteFile(target, data, 0o644)
}

func validateSiteLinks(publicDir string) error {
	var missing []string
	err := filepath.WalkDir(publicDir, func(path string, entry os.DirEntry, err error) error {
		if err != nil {
			return err
		}
		if entry.IsDir() || filepath.Ext(path) != ".html" {
			return nil
		}
		data, err := os.ReadFile(path)
		if err != nil {
			return err
		}
		for _, match := range siteRefRE.FindAllStringSubmatch(string(data), -1) {
			if len(match) != 2 {
				continue
			}
			href := match[1]
			if externalOrAnchor(href) {
				continue
			}
			if index := strings.Index(href, "#"); index >= 0 {
				href = href[:index]
			}
			if href == "" {
				continue
			}
			target := filepath.Clean(filepath.Join(filepath.Dir(path), filepath.FromSlash(href)))
			if _, err := os.Stat(target); err != nil {
				relPath, _ := filepath.Rel(publicDir, path)
				missing = append(missing, filepath.ToSlash(relPath)+" -> "+match[1])
			}
		}
		return nil
	})
	if err != nil {
		return err
	}
	if len(missing) > 0 {
		sort.Strings(missing)
		return fmt.Errorf("site link check failed: %s", strings.Join(missing, "; "))
	}
	return nil
}

func externalOrAnchor(href string) bool {
	return strings.HasPrefix(href, "http://") ||
		strings.HasPrefix(href, "https://") ||
		strings.HasPrefix(href, "mailto:") ||
		strings.HasPrefix(href, "#")
}
