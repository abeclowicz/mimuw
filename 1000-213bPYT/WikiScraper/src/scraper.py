import argparse
import json
import logging
from collections import Counter
from pathlib import Path

import pandas as pd
from wordfreq import get_frequency_dict, top_n_list, word_frequency

from wiki_utils import (
    BadArticle,
    FigureSaveError,
    Scraper,
    save_comparison_chart,
)

logger = logging.getLogger(__name__)

attribution = """
    This program uses material from the Half-Life & Portal Encyclopedia at
    Fandom and is licensed under the Creative Commons Attribution-Share Alike
    License.
"""

parser = argparse.ArgumentParser(prog="WikiScraper", epilog=attribution)

parser.add_argument("--summary")
parser.add_argument("--table")
parser.add_argument("--number")
parser.add_argument("--count-words")
parser.add_argument("--analyze-relative-word-frequency", action="store_true")
parser.add_argument("--mode", choices=["article", "language"])
parser.add_argument("--chart")
parser.add_argument("--count")
parser.add_argument("--auto-count-words")
parser.add_argument("--depth")
parser.add_argument("--wait")

args = parser.parse_args()


class WikiScraper:
    def __init__(self):
        self._scraper = Scraper()

    def _get_word_counts(self) -> Counter[str]:
        """
        Returns a dictionary of word frequency counts from 'word-counts.json`.
        """

        path = Path("word-counts.json")

        # Return empty if file does not exist.
        if not (path.exists() and path.is_file()):
            return Counter()

        try:
            # Try to parse file and return its contents.
            # Return empty if file is empty or corrupt.
            with open(path.resolve()) as file:
                try:
                    data = json.load(file)
                    return Counter(data)
                except json.JSONDecodeError:
                    return Counter()
        except Exception as e:
            logger.error("Failed to read the 'word-counts.json' file!")

    def _add_word_counts(self, word_counts: Counter[str]):
        """
        Appends a dictionary of word frequency counts to `word-counts.json`,
        preserving and incrementing existing values.

        Args:
            word_counts (Counter[str]): A dictionary of word frequency counts.
        """

        try:
            # Append a dictionary to the existing values.
            total = self._get_word_counts() + word_counts

            # Update the `word-counts.json` file. If none, create it.
            with open("./word-counts.json", "w") as file:
                json.dump(total, file)
        except Exception as e:
            logger.error("Failed to save the 'word-counts.json' file!")

    def summary(self, name: str, html: str | None = None):
        """
        Prints a summary of the article (i.e., the text content of the first
        paragraph).

        Initializes the article once, then caches it. If `html` is not
        provided, fetches it via an HTTP request.

        Args:
            name (str): The name of the article. Ignored if `html` is provided.
            html (str, optional): HTML content of the article. Defaults to
                                  `None`.

        Note:
            If successful, the article and its summary are cached. Subsequent
            calls with the same parameters will use the cached values.
        """

        try:
            print(self._scraper.summary(name, html))
        except (BadArticle, ValueError) as e:
            # Log the exception message.
            logger.error(str(e))

    def table(self, name: str, index: int, html: str | None = None):
        """
        Saves the table at the given `index` (1-based) from the article to a
        `<name>.csv` file. Prints a summary counting the occurrences of each
        value in the table.

        Initializes the article once, then caches it. If `html` is not
        provided, fetches it via an HTTP request.

        Args:
            name (str): The name of the article and the CSV file.
            index (int): 1-based index of the table to extract from the
                         article.
            html (str, optional): HTML content of the article. Defaults to
                                  `None`.

        Note:
            If successful, the article and all its tables are cached.
            Subsequent calls with the same parameters will use the cached
            values.
        """

        try:
            df = self._scraper.table(name, index, html)

            # Prepare and print the summary.
            counts = df.stack().value_counts()
            summary = counts.reset_index()
            summary.columns = ["Value", "Count"]
            print(summary.to_string(index=False, max_colwidth=50))

            try:
                # Save the table to a CSV file.
                df.to_csv(f"{name}.csv", index=False)
            except Exception as e:
                logger.error("Failed to save the table to a CSV file!")

        except (BadArticle, IndexError, ValueError) as e:
            # Log the exception message.
            logger.error(str(e))

    def count_words(self, name: str, html: str | None = None):
        """
        Appends a dictionary of word frequency counts in the article to
        `word-counts.json`, preserving and incrementing existing values.

        Initializes the article once, then caches it. If `html` is not
        provided, fetches it via an HTTP request.

        Args:
            name (str): The name of the article. Ignored if `html` is provided.
            html (str, optional): HTML content of the article. Defaults to
                                  `None`.

        Note:
            If successful, the article and its word frequency counts are
            cached. Subsequent calls with the same parameters will use the
            cached values.
        """

        try:
            self._add_word_counts(self._scraper.count_words(name, html))
        except BadArticle as e:
            # Log the exception message.
            logger.error(str(e))

    def analyze_relative_word_frequency(
        self, mode: str, count: int, path: str | None = None
    ):
        """
        Prints a table (with `count` rows) comparing word frequencies from
        `word-counts.json` with these in the English language.

        When `mode` equals:
        - `article`, words are sorted by their frequency in `word-counts.json`,
        - `language`, words are sorted by their frequency in the English
        language.

        If `path` is provided, a chart representing the table data is saved to
        a file.

        Args:
            mode (str): The comparison mode. Allowed values are `article` and
                        `language`.
            count (int): Number of the most common words to compare.
            path (str, optional): The location where the chart should be saved.

        Note:
            Depending on `mode`, words that are not present in
            `word-counts.json` or in the English language result in the
            respective frequency cells being empty.
        """

        allowed_modes = ["article", "language"]
        if mode not in allowed_modes:
            logger.error(f"Unknown comparison mode '{mode}'!")
            return

        if count < 0:
            logger.error("Word count is negative!")
            return

        word_counts = self._get_word_counts()

        if mode == "article":
            if count > len(word_counts):
                logger.error("Not enough entries in word-counts.json!")
                return

            # Get `count` most common words (with their counts) from
            # `word-counts.json`.
            words, counts_wiki = zip(*word_counts.most_common(count))
        else:
            if count > len(get_frequency_dict("en")):
                logger.error("Not enough entries in the language dataset!")
                return

            # Get `count` most common words in the English language and find
            # their respective counts in `word-counts.json`.
            words = top_n_list("en", count)
            counts_wiki = [word_counts.get(word) for word in words]

        freq_en = [word_frequency(word, "en") for word in words]

        # Divide each count by a total count to represent their share.
        # Words not present in `word-counts.json` result in empty cells.
        total_count = word_counts.total()
        freq_wiki = [
            count / total_count if count is not None else ""
            for count in counts_wiki
        ]

        # To account for uncommon words, a minimum frequency of 0.01% is set.
        # Words not present in the English language result in empty cells.
        freq_en = [max(freq, 1e-4) if freq > 0 else "" for freq in freq_en]

        df = pd.DataFrame(
            {
                "Word": words,
                "Frequency on Wiki": freq_wiki,
                "Frequency in the English language": freq_en,
            }
        )

        # Print the comparison table. Frequencies are formatted as percentage
        # values, each with 2 decimal places.
        print(
            df.to_string(
                float_format=lambda x: f"{100 * x:.2f}%",
                index=False,
                max_colwidth=50,
            )
        )

        # If given `path`, save the chart to a file.
        if path is not None:
            try:
                save_comparison_chart(df, Path(path))
            except FigureSaveError as e:
                # Log the exception message.
                logger.error(str(e))

    def auto_count_words(
        self,
        name: str,
        depth: int,
        wait=0.0,
        html: str | None = None,
        htmls: dict[str, str] | None = None,
    ):
        """
        Appends a dictionary of combined word frequency counts in the source
        article and all linked articles (within the same site), up to a given
        `depth` to `word-counts.json`, preserving and incrementing existing
        values.

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

        Note:
            If successful, each article and its word frequency counts are
            cached. Subsequent calls with the same parameters will use the
            cached values. Subsequent calls without the same parameters may use
            some of the cached values.
        """

        try:
            self._add_word_counts(
                self._scraper.auto_count_words(name, depth, wait, html, htmls)
            )
        except (BadArticle, ValueError) as e:
            # Log the exception message.
            logger.error(str(e))


class WikiParser:
    def __init__(self, args: argparse.Namespace):
        self._args = args
        self._wikiscraper = WikiScraper()

    def _validate_args(self) -> bool:
        """
        Returns `True` if parsed arguments are valid, `False` otherwise.
        """

        if self._args.table:
            # Check if any required arguments are missing.
            if not self._args.number:
                logger.error(
                    "The --table argument requires the --number argument!"
                )
                return False

            # Check if non-string arguments are convertible.
            if not self._args.number.removeprefix("-").isdigit():
                logger.error("The --number argument is not a digit!")
                return False

        if self._args.analyze_relative_word_frequency:
            # Check if any required arguments are missing.
            if not self._args.mode or not self._args.count:
                logger.error(
                    "The --analyze-relative-word-frequency argument requires "
                    "the --mode and --count arguments!"
                )
                return False

            # Check if non-string arguments are convertible.
            if not self._args.count.removeprefix("-").isdigit():
                logger.error("The --count argument is not a digit!")
                return False

        if self._args.auto_count_words:
            # Check if any required arguments are missing.
            if not self._args.depth or not self._args.wait:
                logger.error(
                    "The --auto-count-words argument requires the --depth and "
                    "--wait arguments!"
                )
                return False

            # Check if non-string arguments are convertible.
            if not self._args.depth.removeprefix("-").isdigit():
                logger.error("The --depth argument is not a digit!")
                return False
            try:
                float(self._args.wait)
            except ValueError:
                logger.error("The --wait argument is not a decimal!")
                return False

        return True

    def run(self):
        # Validate all arguments first. If any of them is invalid, return.
        if not self._validate_args():
            return

        if self._args.summary:
            self._wikiscraper.summary(self._args.summary)

        if self._args.table:
            self._wikiscraper.table(self._args.table, int(self._args.number))

        if self._args.count_words:
            self._wikiscraper.count_words(self._args.count_words)

        if self._args.analyze_relative_word_frequency:
            self._wikiscraper.analyze_relative_word_frequency(
                self._args.mode, int(self._args.count), self._args.chart
            )

        if self._args.auto_count_words:
            self._wikiscraper.auto_count_words(
                self._args.auto_count_words,
                int(self._args.depth),
                float(self._args.wait),
            )


if __name__ == "__main__":
    WikiParser(parser.parse_args()).run()
