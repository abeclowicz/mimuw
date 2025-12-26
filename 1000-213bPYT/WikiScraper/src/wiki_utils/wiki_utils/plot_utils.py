from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib import ticker


class FigureSaveError(Exception):
    """
    Raised if saving a figure to a file (i.e., `matplotlib.pyplot.savefig()`)
    fails.
    """

    pass


def save_comparison_chart(df: pd.DataFrame, path: Path):
    """
    Saves a chart representing word frequency comparison to a file.

    Args:
        df (pd.DataFrame): Table containing word frequency comparison data.
        path (Path): The location where the chart should be saved.

    Raises:
        FigureSaveError: If saving the chart fails (e.g., invalid extension).

    Note:
        Assumes `df` contains valid word frequency comparison data.
    """

    words, freq_wiki, freq_en = df.values.T

    # Convert empty cells to zeros.
    freq_wiki, freq_en = [
        [freq if isinstance(freq, float) else 0.0 for freq in freq_wiki],
        [freq if isinstance(freq, float) else 0.0 for freq in freq_en],
    ]

    # Bar width and x locations.
    w, x = 0.4, np.arange(len(words))

    fig, ax = plt.subplots()
    ax.bar(x - w / 2, freq_wiki, width=w, label="Wiki")
    ax.bar(x + w / 2, freq_en, width=w, label="English")

    ax.set_xticks(x)
    ax.set_xticklabels(words)
    ax.set_ylabel("frequency")
    ax.set_title("Frequency of some words on Wiki")
    ax.legend()

    # Format frequencies (from [0, 1] range) as percentage values.
    ax.yaxis.set_major_formatter(ticker.PercentFormatter(xmax=1, decimals=0))

    # Rotate labels on the X-axis if any of them overlap.
    bboxes = [label.get_window_extent() for label in ax.get_xticklabels()]
    if any(bboxes[i].overlaps(bboxes[i + 1]) for i in range(len(bboxes) - 1)):
        fig.autofmt_xdate(rotation=90, ha="center")

    # Save the comparison chart to a file.
    try:
        plt.savefig(path, dpi=400)
    except Exception as e:
        raise FigureSaveError("Failed to save the comparison chart! " + str(e))
