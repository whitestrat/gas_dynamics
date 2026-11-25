"""Диаграмма x-t задачи Римана."""

import matplotlib.pyplot as plt

from .style import MUTED, SAND, DARK, make_axes


def riemann_xt(calc):
    t = calc.load("t")
    x = calc.load("x")

    fig, ax = make_axes("", "x", "t")

    def draw(name, label=None, linestyle="-", width=2):
        if name in calc.quantities:
            ax.plot(
                calc.load(name), t,
                color=SAND,
                linestyle=linestyle,
                linewidth=width,
                label=label
            )
            return True
        return False

    # Левая и правая волны
    for side, title in (("left", "Left"), ("right", "Right")):

        # Ударная волна
        if draw(f"x_{side}_shock", f"{title} shock", width=2.5):
            continue

        # Волна разрежения
        head = f"x_{side}_head"
        tail = f"x_{side}_tail"

        if head in calc.quantities and tail in calc.quantities:
            draw(head, f"{title} rarefaction")
            draw(tail)

            for i in range(1, 6):
                draw(f"x_{side}_fan_{i}", width=1)

    # Контактный разрыв
    if "x_contact" in calc.quantities:
        ax.plot(
            calc.load("x_contact"), t,
            color=MUTED,
            linestyle="--",
            linewidth=2,
            label="Contact discontinuity"
        )

    ax.set_xlim(x.min(), x.max())
    ax.set_ylim(t.min(), t.max())

    ax.legend(frameon=False, labelcolor=DARK, fontsize=12)

    return fig, ax
