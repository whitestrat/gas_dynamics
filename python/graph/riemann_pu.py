"""Диаграмма p-u из CSV; математический решатель здесь не вызывается."""

import matplotlib.pyplot as plt
from .style import DARK, ACCENT, make_axes


def riemann_pu(calc, show=True):
    p = calc.load("curve_pressure")
    fig, ax = make_axes("Диаграмма p–u", "u", "p", figsize=(8, 5.2))
    for side, letter, color, label in (("left", "L", DARK, "Левая волна"),
                                       ("right", "R", ACCENT, "Правая волна")):
        u = calc.load(f"curve_u_{side}")
        initial = calc.task.states[side]
        pk, uk = initial["pressure"], initial["velocity"]
        for mask, style, kind in ((p <= pk, "--", "разрежение"), (p >= pk, "-", "ударная")):
            ax.plot(u[mask], p[mask], color=color, linestyle=style, linewidth=1.7,
                    label=f"{label}: {kind}")
        ax.scatter(uk, pk, color=color, s=45, zorder=4)
        ax.annotate(f"$W_{letter}$", (uk, pk), xytext=(7 if side == "left" else -28, 6),
                    textcoords="offset points", fontsize=11)
    us, ps = calc.load("u_star")[0], calc.load("p_star")[0]
    ax.scatter(us, ps, color="#BA3D32", marker="*", s=120, zorder=5, label=r"$(u_*,p_*)$")
    ax.annotate(r"$(u_*,p_*)$", (us, ps), xytext=(7, 6), textcoords="offset points")
    if calc.path.name in ("test_3", "test_4", "test_5"):
        ax.set_yscale("log")
    ax.legend(frameon=False, fontsize=9)
    fig.tight_layout()
    if show:
        plt.show()
    return fig, ax
