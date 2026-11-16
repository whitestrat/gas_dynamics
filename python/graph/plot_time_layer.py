"""Профили f_xt с положениями фронтов волн."""

import numpy as np
import matplotlib.pyplot as plt
from .plot import draw_series
from .style import COLORS, LABELS, make_axes, wave_kind

FIELDS = {
    "density": (r"$\rho$", "Плотность"),
    "velocity": (r"$u$", "Скорость"),
    "pressure": (r"$p$", "Давление"),
    "internal_energy": (r"$e$", "Удельная внутренняя энергия"),
    "sound_speed": (r"$a$", "Скорость звука"),
    "entropy": (r"$s=p/\rho^\gamma$", "Энтропийная функция"),
}


def plot_time_layer(calc, *names, step=-1, show=True):
    if not names or any(calc.quantities[name]["type"] != "f_xt" for name in names):
        raise ValueError("Укажите величины типа f_xt")
    x, t = calc.load("x"), calc.load("t")
    data = [calc.load(name)[step] for name in names]
    ylabel, title = FIELDS.get(names[0], (names[0], names[0]))
    fig, ax = make_axes(f"{title}, t = {t[step]:.4g}", "x", ylabel)
    draw_series(ax, x, data, names)
    used = set()
    for name, info in calc.quantities.items():
        if info["type"] != "f_t" or not name.startswith("x_") or "_fan_" in name:
            continue
        position = calc.load(name)[step]
        if not x[0] <= position <= x[-1]:
            continue
        kind = wave_kind(name)
        ax.axvline(position, color=COLORS[kind], linewidth=0.9,
                   linestyle="--" if kind == "contact" else ":" if kind == "rarefaction" else "-",
                   label=LABELS[kind] if kind not in used else None)
        used.add(kind)
    if len(data) == 1 and np.ptp(data[0]) <= 1e-12 * max(1.0, np.max(np.abs(data[0]))):
        center = float(np.mean(data[0]))
        width = 0.05 * max(1.0, abs(center))
        ax.set_ylim(center - width, center + width)
    ax.set_xlim(x[0], x[-1])
    ax.legend(frameon=False, fontsize=9)
    fig.tight_layout()
    if show:
        plt.show()
    return fig, ax
