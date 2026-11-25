"""Единая цветовая схема и оформление графиков."""

import matplotlib.pyplot as plt

DARK = "#1E3B2F"       # Tекст
MUTED =  "#7c8a58" #"#7F8C6A"      # Линии
ACCENT = "#DB9558"     # Акценты
SAND = "#808080" #"#C8B48A"       # Сетка и вспомогательные линии
LIGHT = "#F5F0E6"      # Фон
BASELINE = "#DB9558"

LINES = (ACCENT, MUTED, DARK, "#2A4380", SAND)


def make_axes(title, xlabel, ylabel, figsize=(8.5, 4.5)):
    
    plt.rcParams.update({
        "font.family": "serif",
        "font.serif": ["cmr10"],       # Computer Modern Roman
        "mathtext.fontset": "cm",      # формулы в стиле Computer Modern
        "axes.formatter.use_mathtext": True     # формулы в том же стиле
    })
    
    fig, ax = plt.subplots(figsize=figsize, dpi=120, facecolor=LIGHT)
    ax.set_facecolor(LIGHT)
    ax.set_title(title, fontsize=16)
    ax.set_xlabel(xlabel, fontsize=14)
    ax.set_ylabel(ylabel, fontsize=14)
    #Скрываем рамки сверху и справа
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.spines["bottom"].set_color(SAND)
    ax.spines["left"].set_color(SAND)
    ax.tick_params(colors=DARK, labelsize=14)
    ax.xaxis.label.set_color(DARK)
    ax.yaxis.label.set_color(DARK)
    ax.title.set_color(DARK)
    ax.grid(color=SAND)
    ax.set_axisbelow(True)
    return fig, ax
