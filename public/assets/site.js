(function () {
  function normalize(value) {
    return value.trim().toLowerCase();
  }

  function resultScore(item, query, terms) {
    var title = item.title.toLowerCase();
    var text = item.text.toLowerCase();
    var score = 0;

    if (title.indexOf(query) !== -1) {
      score += 20;
    }
    if (text.indexOf(query) !== -1) {
      score += 10;
    }
    terms.forEach(function (term) {
      var occurrences = text.split(term).length - 1;
      score += occurrences;
      if (title.indexOf(term) !== -1) {
        score += 8;
      }
      if (text.indexOf("### " + term) !== -1 || text.indexOf("### array." + term) !== -1) {
        score += 28;
      }
      if (text.indexOf("`array." + term) !== -1) {
        score += 6;
      }
      if (text.indexOf(term) !== -1) {
        score += 3;
      }
    });
    return score;
  }

  function snippet(text, terms) {
    var lower = text.toLowerCase();
    var index = -1;

    for (var i = 0; i < terms.length; i += 1) {
      index = lower.indexOf(terms[i]);
      if (index !== -1) {
        break;
      }
    }
    if (index === -1) {
      index = 0;
    }

    var start = Math.max(0, index - 58);
    var end = Math.min(text.length, index + 112);
    return (start > 0 ? "..." : "") + text.slice(start, end).trim() + (end < text.length ? "..." : "");
  }

  function renderResults(root, results, resultsEl) {
    resultsEl.textContent = "";

    if (results.length === 0) {
      var empty = document.createElement("p");
      empty.className = "search-empty";
      empty.textContent = "No matches";
      resultsEl.appendChild(empty);
      return;
    }

    results.forEach(function (result) {
      var link = document.createElement("a");
      var title = document.createElement("strong");
      var summary = document.createElement("span");

      link.href = root + result.item.url;
      title.textContent = result.item.title;
      summary.textContent = result.snippet;

      link.appendChild(title);
      link.appendChild(summary);
      resultsEl.appendChild(link);
    });
  }

  function initSearch(form) {
    var input = form.querySelector("input[type='search']");
    var resultsEl = form.querySelector(".search-results");
    var root = form.getAttribute("data-site-root") || "";
    var indexURL = form.getAttribute("data-search-index");
    var entries = [];

    if (!input || !resultsEl || !indexURL) {
      return;
    }

    form.addEventListener("submit", function (event) {
      event.preventDefault();
    });

    fetch(indexURL)
      .then(function (response) {
        if (!response.ok) {
          throw new Error("search index unavailable");
        }
        return response.json();
      })
      .then(function (data) {
        entries = data;
      })
      .catch(function () {
        input.placeholder = "search unavailable";
      });

    input.addEventListener("input", function () {
      var query = normalize(input.value);
      if (query === "") {
        resultsEl.textContent = "";
        return;
      }

      var terms = query.split(/\s+/).filter(Boolean);
      var results = entries
        .map(function (item) {
          return {
            item: item,
            score: resultScore(item, query, terms),
            snippet: snippet(item.text, terms),
          };
        })
        .filter(function (result) {
          return result.score > 0;
        })
        .sort(function (a, b) {
          if (b.score !== a.score) {
            return b.score - a.score;
          }
          return a.item.title.localeCompare(b.item.title);
        })
        .slice(0, 6);

      renderResults(root, results, resultsEl);
    });
  }

  document.addEventListener("DOMContentLoaded", function () {
    document.querySelectorAll(".doc-search").forEach(initSearch);
  });
})();
