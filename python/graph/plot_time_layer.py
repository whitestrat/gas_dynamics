"""Пространственные профили величин f_xt в выбранный момент времени."""

import matplotlib.pyplot as plt

from .style import DARK, LINES, make_axes


def plot_time_layer(calc, *names, layer=-1):
    if not names:
        raise ValueError("Укажите хотя бы одну величину")

    for name in names:
        if name not in calc.quantities:
            raise ValueError(f"Величина {name!r} не найдена")
        if calc.quantities[name]["type"] != "f_xt":
            raise ValueError(f"'{name}' не относится к f_xt")
    
    y_label = names[0] if len(names) == 1 else ""

    x = calc.load("x")
    t = calc.load("t")
    fig, ax = make_axes(f"t = {t[layer]:.4g}", r"$x$", ylabel=y_label)

    for i, name in enumerate(names):
        ax.plot(x, calc.load(name)[layer], color=LINES[i % len(LINES)],
                linewidth=2, label=name)

    if len(names) > 1: ax.legend(labelcolor=DARK)
    return fig, ax
