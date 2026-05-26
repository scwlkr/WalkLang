#include "docs/sitegen.h"

#include "docs/api_docs.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace walk::docs {
namespace {

struct DocPage {
    std::string source;
    std::string output;
    std::string title;
    std::string nav_title;
    std::string group;
    bool hide_from_nav = false;
};

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::ostringstream text;
    text << input.rdbuf();
    return text.str();
}

Result<void> write_file(const std::filesystem::path& path, const std::string& text) {
    try {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path, std::ios::binary);
        if (!output) {
            return Result<void>::failure("could not write " + path.string());
        }
        output << text;
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<void>::failure(error.what());
    }
    return Result<void>::success();
}

Result<void> copy_asset(const std::filesystem::path& source, const std::filesystem::path& target) {
    try {
        std::filesystem::create_directories(target.parent_path());
        std::filesystem::copy_file(source, target, std::filesystem::copy_options::overwrite_existing);
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<void>::failure(error.what());
    }
    return Result<void>::success();
}

std::string trim(const std::string& text) {
    const std::size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

bool starts_with(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    if (!text.empty() && text.back() == '\n') {
        lines.emplace_back();
    }
    return lines;
}

std::string html_escape(const std::string& text) {
    std::ostringstream out;
    for (const char ch : text) {
        switch (ch) {
        case '&':
            out << "&amp;";
            break;
        case '<':
            out << "&lt;";
            break;
        case '>':
            out << "&gt;";
            break;
        case '"':
            out << "&#34;";
            break;
        case '\'':
            out << "&#39;";
            break;
        default:
            out << ch;
            break;
        }
    }
    return out.str();
}

std::vector<DocPage> site_pages() {
    return {
        {"README.md", "docs/index.html", "WalkLang Documentation", "Documentation", "Start", false},
        {"INSTALL.md", "docs/INSTALL.html", "", "Install", "Start", false},
        {"PROJECTS.md", "docs/PROJECTS.html", "", "Project Mode", "Start", false},
        {"SYNTAX.md", "docs/SYNTAX.html", "", "Syntax", "Language", false},
        {"SPEC.md", "docs/SPEC.html", "", "Specification", "Language", false},
        {"LANGUAGE_CONCEPTS.md", "docs/LANGUAGE_CONCEPTS.html", "", "Language Concepts", "Language", false},
        {"STDLIB.md", "docs/STDLIB.html", "", "Standard Library", "Language", false},
        {"ERRORS.md", "docs/ERRORS.html", "", "Diagnostics", "Language", false},
        {"COMPATIBILITY.md", "docs/COMPATIBILITY.html", "", "Compatibility", "Language", false},
        {"MIGRATING.md", "docs/MIGRATING.html", "", "Migration", "Language", false},
        {"DEPRECATION.md", "docs/DEPRECATION.html", "", "Deprecation", "Language", false},
        {"STABLE_FEATURES.md", "docs/STABLE_FEATURES.html", "", "", "Releases", true},
        {"EXPERIMENTAL_COMPOSITION.md", "docs/EXPERIMENTAL_COMPOSITION.html", "", "", "Releases", true},
        {"PACKAGES.md", "docs/PACKAGES.html", "", "", "Releases", true},
        {"TOOLING.md", "docs/TOOLING.html", "", "Editor And Docs Tools", "Tools", false},
        {"RUNTIME_BACKEND.md", "docs/RUNTIME_BACKEND.html", "", "Runtime And Backend", "Tools", false},
        {"NETWORKING.md", "docs/NETWORKING.html", "", "Draft Networking", "Tools", false},
        {"RICH_RUNTIMES.md", "docs/RICH_RUNTIMES.html", "", "Rich Runtimes", "Tools", false},
        {"reference/api.md", "docs/reference/api.html", "WalkLang API Reference", "API Reference", "Reference", false},
        {"DOCS_SITE.md", "docs/DOCS_SITE.html", "", "Docs Site", "Reference", false},
        {"ARCHITECTURE.md", "docs/ARCHITECTURE.html", "", "Architecture", "Project", false},
        {"DESIGN_RULES.md", "docs/DESIGN_RULES.html", "", "Design Rules", "Project", false},
        {"SYSTEMS_COMPILER_PORT_PLAN.md", "docs/SYSTEMS_COMPILER_PORT_PLAN.html", "", "Systems Compiler Port", "Project", false},
        {"STANDARD_PLATFORM.md", "docs/STANDARD_PLATFORM.html", "", "Standard Platform", "Project", false},
        {"EXPLICIT_SYSTEMS_TRACK.md", "docs/EXPLICIT_SYSTEMS_TRACK.html", "", "Explicit Systems Track", "Project", false},
        {"DOCS_STYLE_GUIDE.md", "docs/DOCS_STYLE_GUIDE.html", "", "Docs Style Guide", "Project", false},
        {"PURPOSE.md", "docs/PURPOSE.html", "", "Purpose", "Project", false},
        {"ROADMAP.md", "docs/ROADMAP.html", "", "Roadmap", "Project", false},
        {"STATUS.md", "docs/STATUS.html", "", "Status", "Project", false},
        {"RELEASE_NOTES.md", "docs/RELEASE_NOTES.html", "", "Version History", "Releases", false},
    };
}

std::string markdown_title(const std::string& source) {
    for (const std::string& line : split_lines(source)) {
        if (starts_with(line, "# ")) {
            return trim(line.substr(2));
        }
    }
    return "Documentation";
}

bool heading(const std::string& line, int& level, std::string& text) {
    level = 0;
    while (level < static_cast<int>(line.size()) && line[static_cast<std::size_t>(level)] == '#') {
        ++level;
    }
    if (level == 0 || level > 4 || level >= static_cast<int>(line.size()) || line[static_cast<std::size_t>(level)] != ' ') {
        return false;
    }
    text = trim(line.substr(static_cast<std::size_t>(level) + 1));
    return true;
}

std::string slug(const std::string& text) {
    std::ostringstream out;
    bool last_dash = false;
    for (const unsigned char raw : text) {
        const char ch = static_cast<char>(std::tolower(raw));
        if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
            out << ch;
            last_dash = false;
        } else if (!last_dash) {
            out << '-';
            last_dash = true;
        }
    }
    std::string result = out.str();
    while (!result.empty() && result.front() == '-') {
        result.erase(result.begin());
    }
    while (!result.empty() && result.back() == '-') {
        result.pop_back();
    }
    return result;
}

std::string asset_prefix_for(const std::string& current) {
    const std::filesystem::path dir = std::filesystem::path(current).parent_path();
    if (dir.empty()) {
        return "";
    }
    const std::string generic = dir.generic_string();
    const int depth = static_cast<int>(std::count(generic.begin(), generic.end(), '/'));
    std::string prefix;
    for (int index = 0; index < depth + 1; ++index) {
        prefix += "../";
    }
    return prefix;
}

std::string rel_url(const std::string& from, const std::string& to) {
    const std::filesystem::path base = std::filesystem::path(from).parent_path();
    if (base.empty()) {
        return std::filesystem::path(to).generic_string();
    }
    const std::filesystem::path rel = std::filesystem::path(to).lexically_relative(base);
    const std::string generic = rel.generic_string();
    return generic.empty() ? std::filesystem::path(to).generic_string() : generic;
}

std::string short_title(std::string title) {
    for (const std::string& prefix : {"WalkLang ", "Official "}) {
        if (starts_with(title, prefix)) {
            title = title.substr(prefix.size());
        }
    }
    return title;
}

std::vector<std::string> sorted_groups(const std::vector<DocPage>& pages) {
    const std::vector<std::string> order = {"Start", "Language", "Reference", "Tools", "Project", "Releases"};
    std::set<std::string> present;
    for (const DocPage& page : pages) {
        if (!page.hide_from_nav) {
            present.insert(page.group);
        }
    }
    std::vector<std::string> result;
    for (const std::string& group : order) {
        if (present.erase(group) != 0) {
            result.push_back(group);
        }
    }
    result.insert(result.end(), present.begin(), present.end());
    return result;
}

std::string render_shortcuts(const std::string& current) {
    std::ostringstream out;
    out << "<nav class=\"doc-shortcuts\" aria-label=\"Docs shortcuts\">\n";
    out << "<a href=\"" << rel_url(current, "docs/INSTALL.html") << "\">Install</a>\n";
    out << "<a href=\"" << rel_url(current, "docs/reference/api.html") << "\">API Reference</a>\n";
    out << "<a href=\"" << rel_url(current, "docs/STATUS.html") << "\">Status</a>\n";
    out << "</nav>\n";
    return out.str();
}

std::string render_sidebar(const std::string& current, const std::vector<DocPage>& pages) {
    std::ostringstream out;
    out << "<aside class=\"sidebar\">\n";
    out << "<a class=\"brand\" href=\"" << rel_url(current, "index.html") << "\"><img src=\"" << asset_prefix_for(current)
        << "assets/icon.svg\" alt=\"\"><span><strong>WalkLang</strong><span>Docs and reference</span></span></a>\n";
    out << render_shortcuts(current);
    out << "<nav class=\"nav\" aria-label=\"Documentation\">\n";
    for (const std::string& group : sorted_groups(pages)) {
        out << "<h2>" << html_escape(group) << "</h2>\n";
        for (const DocPage& page : pages) {
            if (page.hide_from_nav || page.group != group) {
                continue;
            }
            std::string title = page.nav_title.empty() ? page.title : page.nav_title;
            if (title.empty()) {
                title = std::filesystem::path(page.output).stem().string();
            }
            out << "<a href=\"" << rel_url(current, page.output) << "\"";
            if (page.output == current) {
                out << " aria-current=\"page\"";
            }
            out << ">" << html_escape(short_title(title)) << "</a>\n";
        }
        if (group == "Reference") {
            out << "<a href=\"" << rel_url(current, "docs/reference/index.html") << "\">Reference Index</a>\n";
        }
    }
    out << "</nav>\n</aside>\n";
    return out.str();
}

std::string rewrite_markdown_href(std::string href, const std::string& current) {
    if (starts_with(href, "http://") || starts_with(href, "https://") || starts_with(href, "#") || starts_with(href, "mailto:")) {
        return href;
    }
    std::string anchor;
    const std::size_t anchor_pos = href.find('#');
    if (anchor_pos != std::string::npos) {
        anchor = href.substr(anchor_pos);
        href = href.substr(0, anchor_pos);
    }
    if (href.size() >= 3 && href.substr(href.size() - 3) == ".md") {
        std::string target = href.substr(0, href.size() - 3) + ".html";
        if (std::filesystem::path(href).filename() == "README.md") {
            target = (std::filesystem::path(href).parent_path() / "index.html").generic_string();
        }
        return rel_url(current, (std::filesystem::path("docs") / target).generic_string()) + anchor;
    }
    return href + anchor;
}

std::string linkify_escaped_text(const std::string& escaped, const std::string& current) {
    static const std::regex link_re(R"(\[([^\]]+)\]\(([^)]+)\))");
    std::ostringstream out;
    std::sregex_iterator it(escaped.begin(), escaped.end(), link_re);
    std::sregex_iterator end;
    std::size_t last = 0;
    for (; it != end; ++it) {
        out << escaped.substr(last, static_cast<std::size_t>(it->position()) - last);
        const std::string label = (*it)[1].str();
        const std::string href = rewrite_markdown_href((*it)[2].str(), current);
        out << "<a href=\"" << html_escape(href) << "\">" << label << "</a>";
        last = static_cast<std::size_t>(it->position() + it->length());
    }
    out << escaped.substr(last);
    return out.str();
}

std::string inline_markdown(const std::string& text, const std::string& current) {
    std::ostringstream out;
    std::size_t start = 0;
    bool code = false;
    for (;;) {
        const std::size_t tick = text.find('`', start);
        const std::string part = text.substr(start, tick == std::string::npos ? std::string::npos : tick - start);
        if (code) {
            out << "<code>" << html_escape(part) << "</code>";
        } else {
            out << linkify_escaped_text(html_escape(part), current);
        }
        if (tick == std::string::npos) {
            break;
        }
        code = !code;
        start = tick + 1;
    }
    return out.str();
}

std::string markdown_to_html(const std::string& source, const std::string& current) {
    std::ostringstream out;
    std::vector<std::string> paragraph;
    bool in_list = false;
    bool in_code = false;
    std::string code_lang;
    std::ostringstream code;

    auto flush_paragraph = [&] {
        if (paragraph.empty()) {
            return;
        }
        std::ostringstream joined;
        for (std::size_t index = 0; index < paragraph.size(); ++index) {
            if (index != 0) {
                joined << ' ';
            }
            joined << paragraph[index];
        }
        out << "<p>" << inline_markdown(joined.str(), current) << "</p>\n";
        paragraph.clear();
    };
    auto close_list = [&] {
        if (in_list) {
            out << "</ul>\n";
            in_list = false;
        }
    };
    auto flush_code = [&] {
        std::string class_attr;
        if (!code_lang.empty()) {
            class_attr = " class=\"language-" + html_escape(code_lang) + "\"";
        }
        std::string code_text = code.str();
        while (!code_text.empty() && code_text.back() == '\n') {
            code_text.pop_back();
        }
        out << "<pre><code" << class_attr << ">" << html_escape(code_text) << "</code></pre>\n";
        code.str("");
        code.clear();
        code_lang.clear();
    };

    for (const std::string& line : split_lines(source)) {
        const std::string trimmed = trim(line);
        if (starts_with(trimmed, "```")) {
            if (in_code) {
                flush_code();
                in_code = false;
                continue;
            }
            flush_paragraph();
            close_list();
            in_code = true;
            code_lang = trim(trimmed.substr(3));
            continue;
        }
        if (in_code) {
            code << line << '\n';
            continue;
        }
        if (trimmed.empty()) {
            flush_paragraph();
            close_list();
            continue;
        }
        int level = 0;
        std::string heading_text;
        if (heading(trimmed, level, heading_text)) {
            flush_paragraph();
            close_list();
            const std::string id = slug(heading_text);
            out << "<h" << level << " id=\"" << id << "\">" << inline_markdown(heading_text, current) << "</h" << level << ">\n";
            continue;
        }
        if (starts_with(trimmed, "- ")) {
            flush_paragraph();
            if (!in_list) {
                out << "<ul>\n";
                in_list = true;
            }
            out << "<li>" << inline_markdown(trim(trimmed.substr(2)), current) << "</li>\n";
            continue;
        }
        paragraph.push_back(trimmed);
    }
    if (in_code) {
        flush_code();
    }
    flush_paragraph();
    close_list();
    return "<article class=\"doc\">\n" + out.str() + "</article>\n";
}

std::string layout(const std::string& current, const std::string& title, const std::vector<DocPage>& pages, const std::string& body) {
    const std::string asset_prefix = asset_prefix_for(current);
    std::ostringstream out;
    out << "<!doctype html>\n";
    out << "<html lang=\"en\">\n";
    out << "<head>\n";
    out << "<meta charset=\"utf-8\">\n";
    out << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
    out << "<title>" << html_escape(title) << " - WalkLang</title>\n";
    out << "<link rel=\"icon\" type=\"image/svg+xml\" href=\"" << asset_prefix << "favicon.svg\">\n";
    out << "<link rel=\"stylesheet\" href=\"" << asset_prefix << "assets/site.css\">\n";
    out << "</head>\n<body>\n";
    out << "<div class=\"shell\">\n";
    out << render_sidebar(current, pages);
    out << "<main class=\"content\">\n";
    out << "<div class=\"topbar\"><a href=\"" << rel_url(current, "docs/index.html") << "\">Docs</a><a href=\""
        << rel_url(current, "docs/reference/api.html") << "\">Reference</a><a href=\"https://github.com/scwlkr/WalkLang\">GitHub</a></div>\n";
    out << "<div class=\"page\">\n";
    out << body;
    out << "<footer class=\"footer\">Generated from repository docs and structured WalkLang source comments.</footer>\n";
    out << "</div>\n</main>\n</div>\n</body>\n</html>\n";
    return out.str();
}

std::string tile(const std::string& title, const std::string& body, const std::string& href) {
    return "<a class=\"tile\" href=\"" + href + "\"><h2>" + html_escape(title) + "</h2><p>" + html_escape(body) + "</p></a>";
}

std::string render_home(const std::string& current, const std::vector<DocPage>& pages) {
    const std::string install = rel_url(current, "docs/INSTALL.html");
    const std::string docs_url = rel_url(current, "docs/index.html");
    const std::string reference = rel_url(current, "docs/reference/api.html");
    std::ostringstream body;
    body << "<section class=\"hero\">\n";
    body << "<div>\n";
    body << "<h1>WalkLang Docs</h1>\n";
    body << "<p>A small compiled language with deterministic syntax, native C-backed output, project tooling, and generated API reference docs.</p>\n";
    body << "<div class=\"quick-links\">\n";
    body << "<a class=\"button\" href=\"" << docs_url << "\">Read the docs</a>\n";
    body << "<a class=\"button secondary\" href=\"" << install << "\">Install WalkLang</a>\n";
    body << "<a class=\"button secondary\" href=\"" << reference << "\">API reference</a>\n";
    body << "</div>\n</div>\n";
    body << "<div class=\"hero-visual\">\n";
    body << "<img src=\"assets/logo.svg\" alt=\"WalkLang logo\">\n";
    body << "<pre><code>walk init hello\nwalk check\nwalk build\n./build/hello</code></pre>\n";
    body << "</div>\n</section>\n";
    body << "<section class=\"grid\" aria-label=\"Documentation entry points\">\n";
    body << tile("Build", "Install the compiler, create a project, and produce a native executable.", install) << "\n";
    body << tile("Learn", "Read the stable syntax, standard library, diagnostics, and compatibility docs.", docs_url) << "\n";
    body << tile("Reference", "Browse generated API docs produced by structured comments and walk docs.", reference) << "\n";
    body << "</section>";
    return layout(current, "WalkLang Docs", pages, body.str());
}

std::string render_reference_index(const std::string& current, const std::vector<DocPage>& pages) {
    const std::string api = rel_url(current, "docs/reference/api.html");
    const std::string json = rel_url(current, "docs/reference/api.json");
    const std::string raw = rel_url(current, "docs/reference/api.md");
    std::ostringstream body;
    body << "<article class=\"doc\">\n";
    body << "<h1>Reference</h1>\n";
    body << "<p>The reference section is generated from structured comments in real WalkLang source. The Markdown and JSON outputs are committed so the hosted site and tooling consume the same API index.</p>\n";
    body << "<div class=\"grid\">\n";
    body << tile("API Reference", "Rendered generated Markdown reference.", api) << "\n";
    body << tile("docs.json", "Machine-readable symbol index from walk docs.", json) << "\n";
    body << tile("Raw Markdown", "Generated Markdown artifact.", raw) << "\n";
    body << "</div>\n</article>";
    return layout(current, "Reference", pages, body.str());
}

bool external_or_anchor(const std::string& href) {
    return starts_with(href, "http://") || starts_with(href, "https://") || starts_with(href, "mailto:") || starts_with(href, "#");
}

Result<void> validate_site_links(const std::filesystem::path& public_dir) {
    static const std::regex ref_re(R"ref((?:href|src)="([^"]+)")ref");
    std::vector<std::string> missing;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(public_dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".html") {
            continue;
        }
        const std::string html = read_file(entry.path());
        std::sregex_iterator it(html.begin(), html.end(), ref_re);
        std::sregex_iterator end;
        for (; it != end; ++it) {
            std::string href = (*it)[1].str();
            if (external_or_anchor(href)) {
                continue;
            }
            const std::size_t anchor = href.find('#');
            if (anchor != std::string::npos) {
                href = href.substr(0, anchor);
            }
            if (href.empty()) {
                continue;
            }
            const std::filesystem::path target = (entry.path().parent_path() / std::filesystem::path(href)).lexically_normal();
            if (!std::filesystem::exists(target)) {
                const std::filesystem::path rel = entry.path().lexically_relative(public_dir);
                missing.push_back(rel.generic_string() + " -> " + (*it)[1].str());
            }
        }
    }
    if (!missing.empty()) {
        std::sort(missing.begin(), missing.end());
        std::ostringstream out;
        for (std::size_t index = 0; index < missing.size(); ++index) {
            if (index != 0) {
                out << "; ";
            }
            out << missing[index];
        }
        return Result<void>::failure("site link check failed: " + out.str());
    }
    return Result<void>::success();
}

Result<void> hydrate_page_titles(std::vector<DocPage>& pages) {
    for (DocPage& page : pages) {
        if (!page.title.empty()) {
            continue;
        }
        page.title = markdown_title(read_file(page.source));
    }
    return Result<void>::success();
}

}  // namespace

Result<void> build_site(const std::string& docs_dir, const std::string& public_dir) {
    std::vector<DocPage> pages = site_pages();
    for (DocPage& page : pages) {
        page.source = (std::filesystem::path(docs_dir) / page.source).string();
    }
    Result<void> titles = hydrate_page_titles(pages);
    if (!titles.ok()) {
        return titles;
    }

    try {
        std::filesystem::remove_all(public_dir);
        std::filesystem::create_directories(std::filesystem::path(public_dir) / "assets");
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<void>::failure(error.what());
    }

    for (const auto& copy : {
             std::pair<std::filesystem::path, std::filesystem::path>{"logo_WalkLang.svg", std::filesystem::path(public_dir) / "assets" / "logo.svg"},
             {"icon_WalkLang.svg", std::filesystem::path(public_dir) / "assets" / "icon.svg"},
             {"icon_WalkLang.svg", std::filesystem::path(public_dir) / "favicon.svg"},
             {std::filesystem::path("site") / "assets" / "site.css", std::filesystem::path(public_dir) / "assets" / "site.css"},
         }) {
        Result<void> copied = copy_asset(copy.first, copy.second);
        if (!copied.ok()) {
            return copied;
        }
    }
    for (const auto& write : {
             std::pair<std::filesystem::path, std::string>{std::filesystem::path(public_dir) / "CNAME", "walklang.wlkrlabs.com\n"},
             {std::filesystem::path(public_dir) / ".nojekyll", ""},
         }) {
        Result<void> written = write_file(write.first, write.second);
        if (!written.ok()) {
            return written;
        }
    }

    Result<void> home = write_file(std::filesystem::path(public_dir) / "index.html", render_home("index.html", pages));
    if (!home.ok()) {
        return home;
    }
    Result<void> reference = write_file(std::filesystem::path(public_dir) / "docs" / "reference" / "index.html", render_reference_index("docs/reference/index.html", pages));
    if (!reference.ok()) {
        return reference;
    }
    for (const auto& copy : {
             std::pair<std::filesystem::path, std::filesystem::path>{std::filesystem::path(docs_dir) / "reference" / "api.md", std::filesystem::path(public_dir) / "docs" / "reference" / "api.md"},
             {std::filesystem::path(docs_dir) / "reference" / "api.json", std::filesystem::path(public_dir) / "docs" / "reference" / "api.json"},
         }) {
        Result<void> copied = copy_asset(copy.first, copy.second);
        if (!copied.ok()) {
            return copied;
        }
    }
    for (const DocPage& page : pages) {
        const std::string source = read_file(page.source);
        std::string title = page.title;
        if (title.empty()) {
            title = markdown_title(source);
        }
        Result<void> written = write_file(std::filesystem::path(public_dir) / page.output, layout(page.output, title, pages, markdown_to_html(source, page.output)));
        if (!written.ok()) {
            return written;
        }
    }
    return validate_site_links(public_dir);
}

}  // namespace walk::docs
