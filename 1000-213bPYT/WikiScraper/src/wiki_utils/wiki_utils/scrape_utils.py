import copy
import re
from collections import Counter
from functools import cached_property
from io import StringIO
from queue import Queue
from time import sleep
from urllib.error import URLError
from urllib.parse import urljoin, urlparse
from urllib.request import Request, urlopen

import pandas as pd
from bs4 import BeautifulSoup, Tag

BASE_URL = "https://half-life.fandom.com/wiki/"


class BadArticle(Exception):
    """
    Raised if the `Article` constructor fails.
    """

    pass


class Article:
    def __init__(self, name: str, html: str | None = None):
        """
        Initializes the article. If `html` is not provided, fetches it via an
        HTTP request.

        Extracts the main content from the `.mw-content-ltr` element and
        removes inline references (e.g., citation links like "[1]", "[2]").

        Args:
            name (str): The name of the article. Ignored if `html` is provided.
            html (str, optional): HTML content of the article. Defaults to
                                  `None`.

        Raises:
            BadArticle: If the initialization fails (e.g. article not found).
        """

        try:
            # Fetch the HTML content if not provided.
            if html is None:
                url = BASE_URL + name.replace(" ", "_")
                headers = {
                    "User-Agent": "Mozilla/5.0 "
                    "(Windows NT 10.0; Win64; x64; rv:147.0) "
                    "Gecko/20100101 Firefox/147.0",
                    "Connection": "keep-alive",
                    "Referer": "https://google.com",
                }
                request = Request(url, headers=headers)
                with urlopen(request) as response:
                    html = response.read()

            soup = BeautifulSoup(html, "html.parser")

            # Extract the article title and the root content element.
            self._title = self._get_title(soup)
            self._content = self._get_content(soup)
        except (URLError, ValueError) as e:
            raise BadArticle(str(e) + f" ({url})" if "url" in locals() else "")

        # Remove inline references (e.g. "[1]", "[2]").
        for ref in self._content.select(".reference"):
            ref.decompose()

    def _get_title(self, soup: BeautifulSoup) -> str:
        """
        Extracts and returns the title of the article.

        Defaults to a fallback value if the article does not contain a `title`
        element.

        Args:
            soup (BeautifulSoup): The `BeautifulSoup` instance of the article.
        """

        if not soup.title:
            # Fallback: hash the HTML content.
            return f"untitled-article {hash(str(soup))}"
        else:
            raw_title = soup.title.get_text(strip=True)
            return raw_title.split("|", 1)[0].strip()

    def _get_content(self, soup: BeautifulSoup) -> Tag:
        """
        Extracts and returns the root content element (`.mw-content-ltr`) of
        the article.

        Args:
            soup (BeautifulSoup): The `BeautifulSoup` instance of the article.

        Raises:
            ValueError: If the article does not contain a `.mw-content-ltr`
                        element.
        """

        content = soup.select_one(".mw-content-ltr")

        if content is None:
            raise ValueError("Article does not contain any content!")

        return content

    @cached_property
    def summary(self) -> str:
        """
        Returns a summary of the article (i.e., the text content of the first
        paragraph).

        Raises:
            ValueError: If the article does not contain a summary paragraph.

        Note:
            If successful, the summary is cached. Subsequent reads will return
            the cached value.
        """

        # Make a deep copy of the root content element.
        content_cpy = copy.deepcopy(self._content)

        # Remove any <aside> content.
        for aside in content_cpy.find_all("aside"):
            aside.decompose()

        paragraphs = content_cpy.find_all("p")

        if len(paragraphs) < 1:
            raise ValueError("Article does not contain a summary paragraph!")

        text = paragraphs[0].get_text().strip()

        if len(text) > 0:
            return text

        # Sometimes, the first paragraph is empty.
        # In that case, summary is the text content of the second paragraph.

        if len(paragraphs) < 2:
            raise ValueError("Article does not contain a summary paragraph!")

        return paragraphs[1].get_text().strip()

    @cached_property
    def tables(self) -> list[pd.DataFrame]:
        """
        Returns a list of all tables in the article.

        Note:
            All tables are cached. Subsequent reads will return the cached
            value.
        """

        try:
            return pd.read_html(StringIO(str(self._content)))
        except ValueError:
            # pd.read_html() raises ValueError if no tables are found.
            return []

    def get_table(self, index: int) -> pd.DataFrame:
        """
        Returns the table at the given `index` (1-based) from the article.

        Args:
            index (int): 1-based index of the table to extract from the
                         article.

        Raises:
            IndexError: If the article contains fewer than `index` tables.
            ValueError: If `index` is less than 1, below its minimum value.

        Note:
            If successful, all tables in the article are cached. Subsequent
            calls will return the cached result.
        """

        if index < 1:
            raise ValueError(f"Table index {index} is less than 1!")
        if len(self.tables) < index:
            raise IndexError(f"Article contains fewer than {index} tables!")

        return self.tables[index - 1]

    @cached_property
    def word_counts(self) -> Counter[str]:
        """
        Returns a dictionary of word frequency counts in the article.

        Note:
            If successful, the word frequency counts are cached. Subsequent
            reads will return the cached result.
        """

        # Convert text to lowercase.
        # This makes the counter case-insensitive.
        text = self._content.get_text().lower()

        # RegEx to separate words from punctuation.
        words = re.findall(r"\b\w+\b", text)

        return Counter(words)

    @cached_property
    def linked_articles(self) -> list[str]:
        """
        Returns a list of unique URLs to linked articles within the same site.

        Note:
            If successful, the URL list is cached. Subsequent reads will return
            the cached result.
        """

        hrefs = []
        for a in self._content.find_all("a", href=True):
            table = a.find_parent("table")

            # Do not include links from the navigational tables.
            if table is not None and "navi-table" in table.attrs.get(
                "class", []
            ):
                continue

            href = a["href"].strip()

            # Skip relative anchors.
            if href.startswith("#"):
                continue

            # Remove GET queries and anchors from the URL.
            parsed = urlparse(href)._replace(query="", fragment="")

            # Only allow relative URLs or http/https.
            if parsed.scheme and parsed.scheme not in ("http", "https"):
                continue

            hrefs.append(parsed.geturl())

        # Add `BASE_URL` prefix to relative links.
        hrefs = [urljoin(BASE_URL, href) for href in hrefs]

        # Skip categories, files and other internal, 'non-article' links.
        prefixes = (
            "Category:",
            "File:",
            "Special:",
            "Template:",
            "Half-Life_Wiki",
        )
        hrefs = filter(
            lambda href: not href.removeprefix(BASE_URL).startswith(prefixes),
            hrefs,
        )

        # Filter out URLs to other domains.
        hrefs = filter(lambda href: href.startswith(BASE_URL), hrefs)

        # Remove duplicate URLs.
        return list(set(hrefs))

    def __eq__(self, other: object) -> bool:
        # Assert `other` is of the same type.
        if not isinstance(other, Article):
            return False

        return self._title == other._title

    def __hash__(self) -> int:
        return hash(self._title)

    def __str__(self) -> str:
        return self._title


class Scraper:
    def __init__(self):
        # Articles are indexed by their titles.
        self._articles: dict[str, Article] = {}

    def _get_article(self, name: str, html: str | None = None) -> Article:
        """
        Returns the (possibly cached) article.

        Initializes the article once, on first invocation, then caches it.
        If `html` is not provided, fetches it via an HTTP request.

        Args:
            name (str): The name of the article. Ignored if `html` is provided.
            html (str, optional): HTML content of the article. Defaults to
                                  `None`.

        Raises:
            BadArticle: If the initialization fails (e.g. article not found).
        """

        # Fast path: `name` already matches a cached article title.
        if (html is None) and (name in self._articles):
            return self._articles[name]

        # Create a temporary article in order to resolve its title. Article
        # attributes are lazily initialized, so this is relatively cheap.
        dummy = Article(name, html)
        title = str(dummy)

        # Cache the article on first invocation.
        if title not in self._articles:
            self._articles[title] = dummy

        return self._articles[title]

    def _get_linked_articles(
        self,
        name: str,
        depth: int,
        wait=0.0,
        html: str | None = None,
        htmls: dict[str, str] | None = None,
    ) -> list[Article]:
        """
        Returns a list of the source article and all linked articles (within
        the same site), up to a given `depth`.

        Initializes the source article once, then caches it. If `html` is not
        provided, fetches it via an HTTP request.

        Each non-source article also gets cached. Similarly to `html`, their
        content can be provided explicitly via `htmls`.

        Args:
            name (str): The name of the source article. Ignored if `html` is
                        provided.
            depth (int): The maximum link distance from the source article.
            wait (float, optional): Seconds to wait after fetching each
                                    article. Defaults to `0.0`.
            html (str, optional): HTML content of the source article. Defaults
                                  to `None`.
            htmls (dict[str, str], optional): HTML content of the non-source
                                              articles, given as
                                              `(url: str, html: str)` pairs.

        Raises:
            BadArticle: If any initialization fails (e.g. article not found).
            ValueError: If `wait` is negative.

        Note:
            If successful, each article gets cached. Subsequent calls with the
            same parameters will return the cached result. Subsequent calls
            without the same parameters may use parts of the cached result.
        """

        article = self._get_article(name, html)

        # Set of the already visited articles.
        visited: set[Article] = set([article])

        # Queue of articles to be processed.
        q = Queue()
        q.put((article, 0))

        # Debug: print the source article title.
        print(str(article))

        while not q.empty():
            # Get the next entry to process.
            article, d = q.get()

            # Do not follow further than the maximum depth.
            if d >= depth:
                continue

            # Iterate over all URLs from the current article.
            for url in article.linked_articles:
                sleep(wait)

                # Initialize the linked article.
                name = url.removeprefix(BASE_URL)
                linked_article = self._get_article(
                    name, htmls.get(url) if htmls is not None else None
                )

                # If this is a new article, add it to the queue
                # and mark as visited, so it only gets processed once.
                if linked_article not in visited:
                    visited.add(linked_article)
                    q.put((linked_article, d + 1))

                    # Debug: print the current article title.
                    print(str(linked_article))

        return list(visited)

    def summary(self, name: str, html: str | None = None) -> str:
        """
        Returns a summary of the article (i.e., the text content of the first
        paragraph).

        Initializes the article once, then caches it. If `html` is not
        provided, fetches it via an HTTP request.

        Args:
            name (str): The name of the article. Ignored if `html` is provided.
            html (str, optional): HTML content of the article. Defaults to
                                  `None`.

        Raises:
            BadArticle: If the initialization fails (e.g., article not found).
            ValueError: If the article does not contain a summary paragraph.

        Note:
            If successful, the article and its summary are cached. Subsequent
            calls with the same parameters will return the cached result.
        """

        return self._get_article(name, html).summary

    def table(
        self, name: str, index: int, html: str | None = None
    ) -> pd.DataFrame:
        """
        Returns the table at the given `index` (1-based) from the article.

        Args:
            name (str): The name of the article. Ignored if `html` is provided.
            index (int): 1-based index of the table to extract from the
                         article.
            html (str, optional): HTML content of the article. Defaults to
                                  `None`.

        Raises:
            BadArticle: If the initialization fails (e.g., article not found).
            IndexError: If the article contains fewer than `index` tables.
            ValueError: If `index` is less than 1, below its minimum value.

        Note:
            If successful, the article and all its tables are cached.
            Subsequent calls with the same parameters will return the cached
            result.
        """

        return self._get_article(name, html).get_table(index)

    def count_words(self, name: str, html: str | None = None) -> Counter[str]:
        """
        Returns a dictionary of word frequency counts in the article.

        Initializes the article once, then caches it. If `html` is not
        provided, fetches it via an HTTP request.

        Args:
            name (str): The name of the article. Ignored if `html` is provided.
            html (str, optional): HTML content of the article. Defaults to
                                  `None`.

        Raises:
            BadArticle: If the initialization fails (e.g. article not found).

        Note:
            If successful, the article and its word frequency counts are
            cached. Subsequent calls with the same parameters will return the
            cached result.
        """

        return self._get_article(name, html).word_counts

    def auto_count_words(
        self,
        name: str,
        depth: int,
        wait=0.0,
        html: str | None = None,
        htmls: dict[str, str] | None = None,
    ) -> Counter[str]:
        """
        Returns a dictionary of combined word frequency counts in the source
        article and all linked articles (within the same site), up to a given
        `depth`.

        Initializes the source article once, then caches it. If `html` is not
        provided, fetches it via an HTTP request.

        Each non-source article also gets cached. Similarly to `html`, their
        content can be provided explicitly via `htmls`.

        Args:
            name (str): The name of the source article. Ignored if `html` is
                        provided.
            depth (int): The maximum link distance from the source article.
            wait (float, optional): Seconds to wait after fetching each
                                    article. Defaults to `0.0`.
            html (str, optional): HTML content of the source article. Defaults
                                  to `None`.
            htmls (dict[str, str], optional): HTML content of the non-source
                                              articles, given as
                                              `(url: str, html: str)` pairs.

        Raises:
            BadArticle: If any initialization fails (e.g. article not found).
            ValueError: If `wait` is negative.

        Note:
            If successful, each article and its word frequency counts are
            cached. Subsequent calls with the same parameters will return the
            cached result. Subsequent calls without the same parameters may use
            parts of the cached result.
        """

        if wait < 0.0:
            raise ValueError(f"Delay ({wait}s) is negative!")

        articles = self._get_linked_articles(name, depth, wait, html, htmls)
        return sum([article.word_counts for article in articles], Counter())
