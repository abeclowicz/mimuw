from hashlib import md5
from io import StringIO
from pathlib import Path
from unittest.mock import call, mock_open, patch

import pytest

from scraper import WikiScraper


@pytest.fixture
def mock_html() -> str:
    path = (
        Path(__file__).parents[1] / "data/html/The Orange Box Achievements.htm"
    )
    with open(path, "r", encoding="utf-8") as html:
        return html.read()


def test_table(mock_html: str):
    scraper = WikiScraper()

    with patch(
        "sys.stdout", new_callable=StringIO
    ) as mock_stdout, pytest.MonkeyPatch().context() as mpatch:
        # Mock the open() function, so the CSV file will not be created.
        m = mock_open()
        mpatch.setattr("builtins.open", m)

        scraper.table("The Orange Box Achievements", 1, mock_html)

        # Assert valid command-line output.
        out = mock_stdout.getvalue().encode()
        assert md5(out).hexdigest() == "aad8304b5052abc7f2844bd6d69daad3"

        # Assert valid CSV file content.
        m.assert_called_once_with(
            "The Orange Box Achievements.csv",
            "w",
            encoding="utf-8",
            errors="strict",
            newline="",
        )
        m().write.assert_has_calls(
            [
                call("Game,Achievements,Score\r\n"),
                call("Any Half-Life 2 game,6,40\r\n"),
                call("Half-Life 2,32,275\r\n"),
                call("Half-Life 2: Episode One,12,175\r\n"),
                call("Half-Life 2: Episode Two,18,210\r\n"),
                call("Portal,14,175\r\n"),
                call("Team Fortress 2,17,125\r\n"),
                call("Total,99,1000\r\n"),
            ]
        )


if __name__ == "__main__":
    pytest.main([__file__])
