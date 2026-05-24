(function () {
  function normalize(value) {
    return value.trim().toLowerCase();
  }

  function resultScore(item, query, terms) {
    var title = item.title.toLowerCase();
    var section = (item.section || "").toLowerCase();
    var summary = (item.summary || "").toLowerCase();
    var text = item.text.toLowerCase();
    var score = 0;

    if (section === query) {
      score += 60;
    }
    if (section.indexOf(query) !== -1) {
      score += 42;
    }
    if (title.indexOf(query) !== -1) {
      score += 20;
    }
    if (summary.indexOf(query) !== -1) {
      score += 16;
    }
    if (text.indexOf(query) !== -1) {
      score += 8;
    }
    terms.forEach(function (term) {
      var occurrences = text.split(term).length - 1;
      score += occurrences;
      if (section.indexOf(term) !== -1) {
        score += 18;
      }
      if (title.indexOf(term) !== -1) {
        score += 8;
      }
      if (summary.indexOf(term) !== -1) {
        score += 8;
      }
      if (section.indexOf(term + "(") !== -1 || section.indexOf("array." + term) !== -1) {
        score += 28;
      }
      if (text.indexOf("array." + term) !== -1) {
        score += 6;
      }
      if (text.indexOf(term) !== -1) {
        score += 3;
      }
    });
    return score;
  }

  function resultTitle(item) {
    return item.section || item.title;
  }

  function resultContext(item) {
    if (item.section && item.section !== item.title) {
      return item.title + " / " + item.group;
    }
    return item.group;
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
      var context = document.createElement("small");
      var summary = document.createElement("span");

      link.href = root + result.item.url;
      title.textContent = resultTitle(result.item);
      context.textContent = resultContext(result.item);
      summary.textContent = result.item.summary || "Open this section in the WalkLang docs.";

      link.appendChild(title);
      link.appendChild(context);
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
