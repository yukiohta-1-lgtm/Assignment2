#!/usr/bin/env python3
"""Plot saved results; requires matplotlib. No experimental data is fabricated."""
import argparse
import csv
from collections import defaultdict
from pathlib import Path


def load_rows(path):
    with path.open(newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", default="results/results_summary.csv")
    parser.add_argument("--output", default="results/plots")
    args = parser.parse_args()
    # Import here for clearer error when plotting is optional.
    try:
        import matplotlib.pyplot as plt
    except ModuleNotFoundError as e:
        raise SystemExit("Install matplotlib first: python3 -m pip install matplotlib") from e
    rows = load_rows(Path(args.input))
    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)

    for group, variable in (("text_length", "text_length"),
                            ("pattern_length", "pattern_length"),
                            ("alphabet_size", "alphabet_size")):
        fig, ax = plt.subplots(figsize=(7, 4.4))
        for algorithm in ("Karp-Rabin", "Boyer-Moore"):
            relevant = [r for r in rows if r["experiment"] == group and r["algorithm"] == algorithm]
            relevant.sort(key=lambda r: int(r[variable]))
            ax.plot([int(r[variable]) for r in relevant],
                    [float(r["median_microseconds"]) for r in relevant],
                    marker="o", label=algorithm)
        ax.set(xlabel=variable.replace("_", " ").title(),
               ylabel="Median runtime (microseconds)",
               title=f"Runtime vs {variable.replace('_', ' ')}")
        ax.legend()
        ax.grid(alpha=0.25)
        fig.tight_layout()
        fig.savefig(output / f"{group}.png", dpi=180)
        plt.close(fig)

    for group in ("input_type", "hash_collision", "match_presence"):
        by_case = defaultdict(dict)
        for r in rows:
            if r["experiment"] == group:
                by_case[r["case_id"]][r["algorithm"]] = float(r["median_microseconds"])
        cases = [c for c, vals in by_case.items() if len(vals) == 2]
        if not cases:
            continue
        labels = [c.replace("collision_", "").replace("type_", "") for c in cases]
        xs = list(range(len(cases)))
        fig, ax = plt.subplots(figsize=(max(7, len(cases) * 1.6), 4.4))
        for algorithm, dx in (("Karp-Rabin", -0.19), ("Boyer-Moore", 0.19)):
            ax.bar([x + dx for x in xs], [by_case[c][algorithm] for c in cases],
                   width=0.38, label=algorithm)
        ax.set(xticks=xs, xticklabels=labels,
               ylabel="Median runtime (microseconds)",
               title=group.replace("_", " ").title())
        ax.legend()
        ax.grid(axis="y", alpha=0.25)
        fig.tight_layout()
        fig.savefig(output / f"{group}.png", dpi=180)
        plt.close(fig)
    print(f"Saved plots in: {output}")


if __name__ == "__main__":
    main()
