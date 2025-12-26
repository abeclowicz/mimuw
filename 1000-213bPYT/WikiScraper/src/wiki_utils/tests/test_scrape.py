import json
from hashlib import md5
from pathlib import Path

import pandas as pd
import pytest

from wiki_utils import Article, BadArticle, Scraper

HTML_DIR = Path(__file__).parents[3] / "data/html/"


@pytest.mark.parametrize("mode", ["article", "scraper"])
def test_summary(mode: str):
    # Scenario 1.
    # Side infobox in the first paragraph, summary in the second paragraph.
    with open(
        HTML_DIR / "Valve Corporation.htm", "r", encoding="utf-8"
    ) as html:
        if mode == "article":
            article = Article("Valve Corporation", html.read())
            data = article.summary.encode()
        else:
            scraper = Scraper()
            data = scraper.summary("Valve Corporation", html.read()).encode()

        assert md5(data).hexdigest() == "ef3fdcad0ab607def8e5530d28489bc6"

    # Scenario 2.
    # Both side infobox and summary in the first paragraph.
    with open(HTML_DIR / "The Orange Box.htm", "r", encoding="utf-8") as html:
        if mode == "article":
            article = Article("The Orange Box", html.read())
            data = article.summary.encode()
        else:
            scraper = Scraper()
            data = scraper.summary("The Orange Box", html.read()).encode()

        assert md5(data).hexdigest() == "c561fb9c99f2966e0ee8d8212abbecfd"

    # Scenario 3.
    # No side infobox at all, summary in the first paragraph.
    with open(
        HTML_DIR / "The Orange Box Achievements.htm", "r", encoding="utf-8"
    ) as html:
        if mode == "article":
            article = Article("The Orange Box Achievements", html.read())
            data = article.summary.encode()
        else:
            scraper = Scraper()
            data = scraper.summary(
                "The Orange Box Achievements", html.read()
            ).encode()

        assert md5(data).hexdigest() == "a185f6e3bf6a9f706c0c95742ad2e142"


@pytest.mark.parametrize("mode", ["article", "scraper"])
def test_table(mode: str):
    with open(
        HTML_DIR / "The Orange Box Achievements.htm", "r", encoding="utf-8"
    ) as html:
        if mode == "article":
            article = Article("The Orange Box Achievements", html.read())

            # Scenario 1.
            # Index too small, less than 1 (indices are 1-based).
            with pytest.raises(ValueError):
                article.get_table(0)

            # Scenario 2.
            # Index too big, greater than the number of tables in the article.
            with pytest.raises(IndexError):
                article.get_table(5)

            tables = [article.get_table(i) for i in range(1, 5)]
        else:
            scraper, html_data = Scraper(), html.read()

            # Scenario 1.
            # Index too small, less than 1 (indices are 1-based).
            with pytest.raises(ValueError):
                scraper.table("The Orange Box Achievements", 0, html_data)

            # Scenario 2.
            # Index too big, greater than the number of tables in the article.
            with pytest.raises(IndexError):
                scraper.table("The Orange Box Achievements", 5, html_data)

            tables = [
                scraper.table("The Orange Box Achievements", i, html_data)
                for i in range(1, 5)
            ]

        data = [pd.util.hash_pandas_object(table).sum() for table in tables]

        # Scenario 3.
        # Index within a valid range.
        assert data[0] == 16874923049160413373
        assert data[1] == 2832123306562420684
        assert data[2] == 9953158448627556477
        assert data[3] == 10151947983326146521


@pytest.mark.parametrize("mode", ["article", "scraper"])
def test_count_words(mode: str):
    with open(
        HTML_DIR / "Valve Corporation.htm", "r", encoding="utf-8"
    ) as html:
        if mode == "article":
            article = Article("Valve Corporation", html.read())
            data = json.dumps(article.word_counts, sort_keys=True).encode()
        else:
            scraper = Scraper()
            data = json.dumps(
                scraper.count_words("Valve Corporation", html.read()),
                sort_keys=True,
            ).encode()

        assert md5(data).hexdigest() == "de454f47e6739dc1c80063d8b4d8b9c1"


def test_article_linked_articles():
    with open(
        HTML_DIR / "Valve Corporation.htm", "r", encoding="utf-8"
    ) as html:
        article = Article("Valve Corporation", html.read())
        data = json.dumps(sorted(article.linked_articles)).encode()

        assert md5(data).hexdigest() == "9dba487f3a3cc3f8fdbe11afbcc39b04"


def test_scraper_auto_count_words():
    # Article at depth 1 (of 2) with some outgoing links removed.
    with open(
        HTML_DIR / "Lighthouse Point.dummylinks.htm", "r", encoding="utf-8"
    ) as html:
        html_lighthouse = html.read()

    # Article at depth 2 (of 2).
    with open(HTML_DIR / "The Combine.htm", "r", encoding="utf-8") as html:
        html_combine = html.read()

    # Article at depth 2 (of 2).
    with open(HTML_DIR / "Gordon Freeman.htm", "r", encoding="utf-8") as html:
        html_freeman = html.read()

    # Article at depth 2 (of 2).
    with open(HTML_DIR / "Half-Life 2.htm", "r", encoding="utf-8") as html:
        html_hl2 = html.read()

    # Article at depth 2 (of 2).
    with open(HTML_DIR / "Resistance.htm", "r", encoding="utf-8") as html:
        html_resistance = html.read()

    # Article at depth 2 (of 2).
    with open(HTML_DIR / "The Uprising.htm", "r", encoding="utf-8") as html:
        html_uprising = html.read()

    # HTML content of the non-source articles, given as `(url, html)` pairs.
    htmls = {
        "https://half-life.fandom.com/wiki/Lighthouse_Point": html_lighthouse,
        "https://half-life.fandom.com/wiki/Combine": html_combine,
        "https://half-life.fandom.com/wiki/Gordon_Freeman": html_freeman,
        "https://half-life.fandom.com/wiki/Half-Life_2": html_hl2,
        "https://half-life.fandom.com/wiki/Resistance": html_resistance,
        "https://half-life.fandom.com/wiki/The_Uprising": html_uprising,
    }

    with open(
        HTML_DIR / "Car battery.dummylinks.htm", "r", encoding="utf-8"
    ) as html:
        scraper = Scraper()
        data = json.dumps(
            scraper.auto_count_words(
                "Car battery", 2, 0.1, html.read(), htmls
            ),
            sort_keys=True,
        ).encode()

        assert md5(data).hexdigest() == "fb5e57d667161d7d49f1d57a1ca0889f"


def test_article_init():
    with pytest.raises(BadArticle):
        with open(
            HTML_DIR / "Valve Corporation.nocontent.htm", "r", encoding="utf-8"
        ) as html:
            Article("Valve Corporation", html.read())


def test_article_eq():
    with open(
        HTML_DIR / "Valve Corporation.htm", "r", encoding="utf-8"
    ) as html:
        titled_article1 = Article("Valve Corporation", html.read())

    # Scenario 1.
    # Comparison with a different object.
    assert titled_article1 != [1, 2, 3, 4, 5]

    # Scenario 2.
    # Comparison with self.
    assert titled_article1 == titled_article1

    with open(
        HTML_DIR / "Valve Corporation.htm", "r", encoding="utf-8"
    ) as html:
        titled_article2 = Article("THIS IS A DIFFERENT TITLE", html.read())

    # Scenario 3.
    # Comparison with a different article with the same title.
    assert titled_article1 == titled_article2

    with open(
        HTML_DIR / "Valve Corporation.notitle.htm", "r", encoding="utf-8"
    ) as html:
        untitled_article = Article("Valve Corporation", html.read())

    # Scenario 4.
    # Comparison with an article with the same content, but different title.
    assert titled_article1 != untitled_article


def test_article_str():
    # Scenario 1.
    # Title tag present, same title given.
    with open(
        HTML_DIR / "Valve Corporation.htm", "r", encoding="utf-8"
    ) as html:
        article = Article("Valve Corporation", html.read())
        assert str(article) == "Valve Corporation"

    # Scenario 2.
    # Title tag present, different title given.
    with open(
        HTML_DIR / "Valve Corporation.htm", "r", encoding="utf-8"
    ) as html:
        article = Article("THIS IS A DIFFERENT TITLE", html.read())
        assert str(article) == "Valve Corporation"

    # Scenario 3.
    # Title tag not present, using fallback value.
    with open(
        HTML_DIR / "Valve Corporation.notitle.htm", "r", encoding="utf-8"
    ) as html:
        article = Article("Valve Corporation", html.read())
        assert str(article).startswith("untitled-article ")
