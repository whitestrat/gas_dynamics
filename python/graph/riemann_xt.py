"""Диаграмма x-t: поле плотности и сохранённые траектории волн."""

import matplotlib.pyplot as plt
import matplotlib.patheffects as pe
from .style import COLORS, LABELS, make_axes, wave_kind


def riemann_xt(calc, show=True):
    x, t = calc.load("x"), calc.load("t")
    fig, ax = make_axes("Диаграмма x–t", "x", "t", figsize=(9.2, 5.7))
    image = ax.pcolormesh(x, t, calc.load("density"), shading="auto", cmap="Blues")
    fig.colorbar(image, ax=ax, label=r"$\rho$")
    used = set()
    for name, info in calc.quantities.items():
        if info["type"] != "f_t" or not name.startswith("x_"):
            continue
        kind = wave_kind(name)
        width = 1.8 if kind == "shock" else 1.4 if kind == "contact" else 0.8
        line, = ax.plot(calc.load(name), t, color=COLORS[kind], linewidth=width,
                        linestyle="--" if kind == "contact" else "-",
                        label=LABELS[kind] if kind not in used else None)
        line.set_path_effects([pe.Stroke(linewidth=width + 1, foreground="white"), pe.Normal()])
        used.add(kind)
    def constant(name):
        return float(calc.load(name)[0])
    def speed(side, edge):
        key = f"{side}_{'shock' if constant(side + '_is_shock') else edge}_speed"
        return constant(key)
    x0, contact = constant("x0"), constant("u_star")
    # Подписи внутри четырёх постоянных областей; разные высоты у узких *-областей.
    regions = [
        (None, speed("left", "head"), r"$W_L$", 0.85),
        (speed("left", "tail"), contact, r"$W_{*L}$", 0.70),
        (contact, speed("right", "tail"), r"$W_{*R}$", 0.90),
        (speed("right", "head"), None, r"$W_R$", 0.85),
    ]
    for left_speed, right_speed, label, fraction in regions:
        time = t[-1] * fraction
        left = x[0] if left_speed is None else max(x[0], x0 + left_speed * time)
        right = x[-1] if right_speed is None else min(x[-1], x0 + right_speed * time)
        if right > left:
            ax.text((left + right) / 2, time, label, ha="center", va="center", fontsize=10,
                    bbox={"facecolor": "white", "edgecolor": "none", "alpha": 0.85, "pad": 2})
    ax.set(xlim=(x[0], x[-1]), ylim=(t[0], t[-1]))
    ax.legend(loc="lower left", fontsize=9, framealpha=0.9)
    fig.tight_layout()
    if show:
        plt.show()
    return fig, ax
