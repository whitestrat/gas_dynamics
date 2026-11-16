"""Общие линии графиков и просмотр величин f_x / f_t."""

import matplotlib.pyplot as plt
from .style import LINES, make_axes


def draw_series(ax, coordinates, data, names):
    for i, (values, name) in enumerate(zip(data, names)):
        ax.plot(coordinates, values, color=LINES[i % len(LINES)], linewidth=1.8, label=name)


def plot(calc, *names, show=True):
    if not names:
        raise ValueError("Укажите хотя бы одну величину")
    kinds = [calc.quantities[name]["type"] for name in names]
    if len(set(kinds)) != 1 or kinds[0] not in ("f_x", "f_t"):
        raise ValueError("plot() принимает величины одного типа: f_x или f_t")
    coordinate = "x" if kinds[0] == "f_x" else "t"
    fig, ax = make_axes(", ".join(names), coordinate, "Значение")
    draw_series(ax, calc.load(coordinate), [calc.load(name) for name in names], names)
    ax.legend(frameon=False)
    fig.tight_layout()
    if show:
        plt.show()
    return fig, ax
