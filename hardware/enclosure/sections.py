"""2D cross-section drawings of the enclosure (exact, from the CSG model) for design review."""
import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import PathPatch
from matplotlib.path import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_case as g  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build")


def draw(ax, cs, color, alpha=1.0):
    polys = cs.to_polygons()
    verts, codes = [], []
    for p in polys:
        if len(p) < 3:
            continue
        verts += [tuple(v) for v in p] + [tuple(p[0])]
        codes += [Path.MOVETO] + [Path.LINETO] * (len(p) - 1) + [Path.CLOSEPOLY]
    if verts:
        ax.add_patch(PathPatch(Path(verts, codes), facecolor=color, edgecolor="k", lw=0.3, alpha=alpha))


def board(ax):
    ax.add_patch(plt.Rectangle((0, 0), 66, 42, fill=False, ec="green", lw=0.8, ls="--"))
    for ref, v in g.P.items():
        x0, y0, x1, y1 = v["crtyd"]
        ax.add_patch(plt.Rectangle((x0, y0), x1 - x0, y1 - y0, fill=False, ec="green", lw=0.3))
        ax.text(v["x"], v["y"], ref, fontsize=4, color="green", ha="center", va="center")


def sheet(model, cuts, name, with_board=True):
    fig, axs = plt.subplots(1, len(cuts), figsize=(6.5 * len(cuts), 4.6), dpi=150)
    for ax, (z, label) in zip(axs, cuts):
        draw(ax, model.slice(z), "#e0a040" if "base" in name else "#6688cc")
        if with_board:
            board(ax)
        ax.set_xlim(g.ox0 - 2, g.ox1 + 2)
        ax.set_ylim(g.oy1 + 2, g.oy0 - 2)
        ax.set_aspect("equal")
        ax.set_title("%s  z=%.1f mm" % (label, z), fontsize=9)
        ax.tick_params(labelsize=6)
    plt.tight_layout()
    p = os.path.join(OUT, name)
    fig.savefig(p)
    plt.close(fig)
    print(os.path.abspath(p))


def side(model, name, y):
    """Vertical section through the plane y=const (x-z view)."""
    import numpy as np
    # rotating +90 deg about x maps (x, y, z) -> (x, -z, y): slice at the old y, 2D = (x, -z)
    cs = model.rotate([90, 0, 0]).slice(y)
    fig, ax = plt.subplots(figsize=(10, 3), dpi=150)
    polys = cs.to_polygons()
    verts, codes = [], []
    for p in polys:
        if len(p) < 3:
            continue
        q = np.asarray(p) * [1, -1]
        verts += [tuple(v) for v in q] + [tuple(q[0])]
        codes += [Path.MOVETO] + [Path.LINETO] * (len(q) - 1) + [Path.CLOSEPOLY]
    ax.add_patch(PathPatch(Path(verts, codes), facecolor="#bbbbbb", edgecolor="k", lw=0.3))
    ax.autoscale_view()
    ax.set_aspect("equal")
    ax.set_title(name)
    p = os.path.join(OUT, name)
    fig.savefig(p)
    plt.close(fig)
    print(os.path.abspath(p))


if __name__ == "__main__":
    b, l, brd = g.base(), g.lid(), g.board_dummy()
    sheet(b, [(0.3, "base: grabado inferior"), (4.0, "base: soportes / puente"),
              (g.CABLE_Z, "base: cable / USB")], "sec_base.png")
    sheet(l, [(g.Z_TOP - 0.3, "tapa: grabado"), (g.Z_LID + 0.4, "tapa: pestañas"),
              (g.Z_LID - 1.0, "tapa: faldón / postes")], "sec_lid.png")
    side(b + l + brd, "sec_side_cable.png", y=g.CABLE_Y)
    side(b + l + brd, "sec_side_usb.png", y=g.J2["y"])
    side(b + l + brd, "sec_side_buttons.png", y=g.P["SW2"]["y"])
