"""Графики величин, зависящих только от x или только от t."""

import matplotlib.pyplot as plt

from .style import DARK, LINES, make_axes


def plot(calc, *names):
    if not names:
        raise ValueError("Укажите хотя бы одну величину")

    for name in names:
        if name not in calc.quantities:
            raise ValueError(f"Величина {name!r} не найдена")

    kinds = [calc.quantities[name]["type"] for name in names]
    if len(set(kinds)) != 1 or kinds[0] not in ("f_x", "f_t"):
        raise ValueError("plot() принимает величины одного типа: f_x или f_t")

    axis = "x" if kinds[0] == "f_x" else "t"
    coordinates = calc.load(axis)
    y_label = names[0] if len(names) == 1 else ""
    title = names[0] if len(names) == 1 else ""
    fig, ax = make_axes(title, axis, y_label)

    for i, name in enumerate(names):
        ax.plot(coordinates, calc.load(name), color=LINES[i % len(LINES)],
                linewidth=2, label=name.replace("_", " "))

    ax.legend(frameon=False, labelcolor=DARK, fontsize=12)
    return fig, ax
