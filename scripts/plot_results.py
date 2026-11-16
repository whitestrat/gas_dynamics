#!/usr/bin/env python3
"""Шесть отдельных профилей, x-t и p-u для сохранённых расчётов."""

import argparse
from pathlib import Path
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))
from calculation import Calculation
from graph import plot_time_layer, riemann_xt, riemann_pu
from graph.plot_time_layer import FIELDS


def plot_case(path, dpi):
    calc = Calculation.open(path)
    folder = path / "plots"
    folder.mkdir(exist_ok=True)
    def save(fig, name):
        fig.savefig(folder / f"{name}.png", dpi=dpi)
        plt.close(fig)
    for name in FIELDS:
        save(plot_time_layer(calc, name, show=False)[0], name)
    save(riemann_xt(calc, show=False)[0], "xt")
    save(riemann_pu(calc, show=False)[0], "pu")
    print(f"{path.name}: 8 графиков в {folder}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path, nargs="?", default=ROOT / "results/toro")
    parser.add_argument("--dpi", type=int, default=160)
    args = parser.parse_args()
    if args.dpi < 50:
        parser.error("--dpi должен быть не меньше 50")
    path = args.directory.expanduser().resolve()
    cases = [path] if (path / "c.csv").is_file() else [path / f"test_{i}" for i in range(1, 6)]
    try:
        for case in cases:
            plot_case(case, args.dpi)
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, f"Ошибка построения графиков: {error}\n")


if __name__ == "__main__":
    main()
