"""Диаграмма p-u задачи Римана по сохранённым волновым кривым."""

import json
import numpy as np
import matplotlib.pyplot as plt

from .style import DARK, ACCENT, LIGHT, SAND, make_axes


def riemann_pu(calc):
    p = calc.load("curve_pressure")
    u_left = calc.load("curve_u_left")
    u_right = calc.load("curve_u_right")

    with (calc.path / "config.json").open(encoding="utf-8") as file:
        states = json.load(file)["states"]

    fig, ax = make_axes("", "u", "p")

    for u, side, color, label in (
        (u_left, "left", DARK, "left"),
        (u_right, "right", ACCENT, "right"),
    ):
        p0 = states[side]["pressure"]
        ax.plot(u[p <= p0], p[p <= p0], color=color, linestyle="--",
                linewidth=2, label=label + ": rarefaction wave")
        ax.plot(u[p >= p0], p[p >= p0], color=color,
                linewidth=2, label=label + ": shock wave")
        ax.scatter(states[side]["velocity"], p0, color=color, s=65, zorder=4,
                   edgecolors=LIGHT)

    ax.scatter(calc.load("u_star")[0], calc.load("p_star")[0],
                   facecolor='none', marker="o", s=60, edgecolors=SAND, lw = 2,
                   zorder = 4, label="start state")

    positive = p[p > 0]
    if positive.size and positive.max() / positive.min() > 100:
        ax.set_yscale("log")

    ax.legend(frameon=False, labelcolor=DARK, fontsize=12)
    return fig, ax
