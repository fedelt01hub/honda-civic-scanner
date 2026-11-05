"""Quick shaded previews of the enclosure STLs (matplotlib, no GPU)."""
import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import trimesh
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "build")


def render(stl, out, elev, azim, color=(0.85, 0.55, 0.15), flipz=False, title=""):
    m = trimesh.load(os.path.join(HERE, stl))
    v = m.vertices.copy()
    if flipz:
        v[:, 2] = -v[:, 2]
        v[:, 1] = -v[:, 1]
    tris = v[m.faces]
    n = np.cross(tris[:, 1] - tris[:, 0], tris[:, 2] - tris[:, 0])
    n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-12
    if flipz:
        n = -n
    el, az = np.radians(elev), np.radians(azim)
    cam = np.array([np.cos(el) * np.cos(az), np.cos(el) * np.sin(az), np.sin(el)])
    light = cam + np.array([0.3, 0.2, 0.6])
    light /= np.linalg.norm(light)
    shade = np.clip(np.abs(n @ light), 0, 1) * 0.75 + 0.25
    cols = np.clip(np.outer(shade, color), 0, 1)
    fig = plt.figure(figsize=(9, 6), dpi=130)
    ax = fig.add_subplot(111, projection="3d", proj_type="ortho")
    pc = Poly3DCollection(tris, facecolors=cols, edgecolors="none", linewidths=0)
    ax.add_collection3d(pc)
    mn, mx = v.min(0), v.max(0)
    c = (mn + mx) / 2
    r = (mx - mn).max() / 2 * 0.62
    ax.set_xlim(c[0] - r, c[0] + r)
    ax.set_ylim(c[1] - r, c[1] + r)
    ax.set_zlim(c[2] - r, c[2] + r)
    ax.set_box_aspect((1, 1, 1))
    ax.view_init(elev=elev, azim=azim)
    ax.set_axis_off()
    ax.set_title(title)
    plt.tight_layout()
    fig.savefig(os.path.join(OUT, out))
    plt.close(fig)
    print(os.path.abspath(os.path.join(OUT, out)))


if __name__ == "__main__":
    render("case_base.stl", "case_base.png", 55, -60, title="Base")
    render("case_lid.stl", "case_lid_top.png", -60, -60, (0.3, 0.45, 0.8), title="Tapa (cara superior)")
    render("case_lid.stl", "case_lid_under.png", 55, -60, (0.3, 0.45, 0.8), title="Tapa (interior)")
    render("case_preview_exploded.stl", "case_exploded.png", 28, -120, (0.6, 0.6, 0.6), title="Despiece")
    render("case_assembly.stl", "case_closed_left.png", 22, 215, title="Cerrada - entrada de cable")
    render("case_assembly.stl", "case_closed_right.png", 22, -35, title="Cerrada - USB-C")
