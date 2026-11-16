"""Общее оформление графиков и обозначения волн."""

import matplotlib.pyplot as plt

DARK, MUTED, ACCENT = "#1E3B2F", "#7F8C6A", "#DB9558"
SAND, LIGHT = "#C8B48A", "#F5F0E6"
LINES = (DARK, ACCENT, MUTED, SAND)
LABELS = {"shock": "Ударная волна", "contact": "Контакт", "rarefaction": "Разрежение"}
COLORS = {"shock": "#BA3D32", "contact": "#49416D", "rarefaction": "#287356"}


def wave_kind(name):
    return "contact" if name == "x_contact" else "shock" if "shock" in name else "rarefaction"


def make_axes(title, xlabel, ylabel, figsize=(9, 4.5)):
    fig, ax = plt.subplots(figsize=figsize, dpi=120, facecolor=LIGHT)
    ax.set_facecolor(LIGHT)
    ax.set(title=title, xlabel=xlabel, ylabel=ylabel)
    ax.spines[["top", "right"]].set_visible(False)
    ax.tick_params(colors=DARK)
    ax.xaxis.label.set_color(DARK)
    ax.yaxis.label.set_color(DARK)
    ax.title.set_color(DARK)
    ax.grid(color=SAND, alpha=0.5)
    ax.set_axisbelow(True)
    return fig, ax
