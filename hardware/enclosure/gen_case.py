"""Parametric 3D-printable enclosure for the Civic K-Line scanner PCB.

Requirements (any CPython 3.10+):  pip install manifold3d trimesh numpy matplotlib
Run:  python gen_case.py            -> case_base.stl, case_lid.stl (print-ready orientation),
                                      case_assembly.stl (preview) in this folder.

Board coordinates (KiCad, mm, y down) are used for everything; the model is mirrored to a
right-handed frame (y up) at export.  Board data comes from board.json
(written by hardware/scripts/export_case_data.py).

Hardware: 4x M3x12 self-tapping screws for plastic (or M3x12 + 2.6 mm pilot holes), 1 zip tie
2.5 mm wide.  Print: PETG (car interior heat) or PLA, 0.2 mm layers, 3 perimeters, no supports.
"""
import json
import os

import numpy as np
import trimesh
from manifold3d import CrossSection, FillRule, JoinType, Manifold, set_circular_segments

set_circular_segments(64)
HERE = os.path.dirname(os.path.abspath(__file__))
BD = json.load(open(os.path.join(HERE, "board.json")))

# ----------------------------------------------------------------- parameters
CABLE_D = 5.0        # outer diameter of the adapter cable (measure yours!)
WALL = 2.0
FLOOR = 2.0
LID_T = 2.0
STANDOFF = 4.0       # board bottom height above the floor
PCB_T = BD["board"]["thickness"]
GAP_Z = 2.1          # air above the tallest part (3.3 mm: USB-C / module)
TALLEST = 3.3
BAY = 12.0           # cable bay on the left of the board
CLR = 0.6            # board-to-wall clearance (sides)
CLR_TOP = 1.0        # above the antenna edge
CLR_USB = 0.3        # USB-C side: keep the receptacle close to the wall
CORNER_R = 3.0
SCREW_PILOT = 2.6
SCREW_CLEAR = 3.4
FIT = 0.25           # lid skirt clearance

bx0, by0, bx1, by1 = 0.0, 0.0, 66.0, 42.0
Z_PCB_BOT = FLOOR + STANDOFF
Z_PCB_TOP = Z_PCB_BOT + PCB_T
Z_LID = Z_PCB_TOP + TALLEST + GAP_Z          # underside of the lid
Z_TOP = Z_LID + LID_T

# cavity (inner) and outer outline
cx0, cy0, cx1, cy1 = bx0 - BAY, by0 - CLR_TOP, bx1 + CLR_USB, by1 + CLR
ox0, oy0, ox1, oy1 = cx0 - WALL, cy0 - WALL, cx1 + WALL, cy1 + WALL

P = BD["parts"]
H1, H2 = (P["H1"]["x"], P["H1"]["y"]), (P["H2"]["x"], P["H2"]["y"])
J2 = P["J2"]
BAY_BOSSES = [(bx0 - BAY / 2, 4.5), (bx0 - BAY / 2, 37.5)]
CABLE_Y = (P["J1"]["y"] + 7.92 / 2)          # centred on the 3 solder pads
BRIDGE_X0, BRIDGE_X1 = bx0 - BAY + 3.0, bx0 - BAY + 8.0
BRIDGE_H = 3.0
CABLE_Z = FLOOR + BRIDGE_H + CABLE_D / 2


# ----------------------------------------------------------------- helpers
def rrect(x0, y0, x1, y1, r):
    r = min(r, (x1 - x0) / 2 - 1e-3, (y1 - y0) / 2 - 1e-3)
    cs = CrossSection.square([x1 - x0 - 2 * r, y1 - y0 - 2 * r]).translate([x0 + r, y0 + r])
    return cs.offset(r, JoinType.Round) if r > 0 else cs


def prism(cs, z0, z1):
    return cs.extrude(z1 - z0).translate([0, 0, z0])


def box(x0, y0, z0, x1, y1, z1):
    return Manifold.cube([x1 - x0, y1 - y0, z1 - z0]).translate([x0, y0, z0])


def cyl(x, y, z0, z1, d):
    return Manifold.cylinder(z1 - z0, d / 2).translate([x, y, z0])


def text_cs(s, x, y, size, rot=0.0, anchor="center"):
    from matplotlib.font_manager import FontProperties
    from matplotlib.textpath import TextPath
    tp = TextPath((0, 0), s, size=size, prop=FontProperties(family="DejaVu Sans", weight="bold"))
    polys = [np.asarray(p) for p in tp.to_polygons() if len(p) > 2]
    allp = np.vstack(polys)
    mn, mx = allp.min(0), allp.max(0)
    c = (mn + mx) / 2
    out = []
    a = np.radians(rot)
    R = np.array([[np.cos(a), -np.sin(a)], [np.sin(a), np.cos(a)]])
    for p in polys:
        q = (p - c) @ R.T
        q[:, 1] = -q[:, 1]                      # board frame is y-down
        out.append((q + [x, y]).tolist())
    return CrossSection(out, FillRule.EvenOdd)


# ----------------------------------------------------------------- base
def base():
    outer = rrect(ox0, oy0, ox1, oy1, CORNER_R)
    inner = rrect(cx0, cy0, cx1, cy1, max(CORNER_R - WALL, 0.5))
    m = prism(outer, 0, Z_LID) - prism(inner, FLOOR, Z_LID + 1)

    # board supports: standoffs at the mounting holes, rests at the top corners and under the buttons
    for (x, y) in (H1, H2):
        m += cyl(x, y, FLOOR - 0.01, Z_PCB_BOT, 6.0)
    for (x, y, d) in ((2.0, 2.0, 3.0), (64.0, 2.0, 3.0),
                      (P["SW1"]["x"], P["SW1"]["y"], 4.0), (P["SW2"]["x"], P["SW2"]["y"], 4.0)):
        m += cyl(x, y, FLOOR - 0.01, Z_PCB_BOT, d)
    # full-height screw bosses in the cable bay
    for (x, y) in BAY_BOSSES:
        m += cyl(x, y, FLOOR - 0.01, Z_LID, 6.5)
    # zip-tie bridge: the tie runs through the tunnel and around the cable lying on top
    yb0, yb1 = CABLE_Y - CABLE_D / 2 - 2.5, CABLE_Y + CABLE_D / 2 + 2.5
    m += box(BRIDGE_X0, yb0, FLOOR - 0.01, BRIDGE_X1, yb1, FLOOR + BRIDGE_H)
    m -= box(BRIDGE_X0 + 0.9, yb0 - 1, FLOOR - 0.5, BRIDGE_X1 - 0.9, yb1 + 1, FLOOR + 1.6)

    # pilot holes
    for (x, y) in (H1, H2):
        m -= cyl(x, y, 0.6, Z_PCB_BOT + 0.1, SCREW_PILOT)
    for (x, y) in BAY_BOSSES:
        m -= cyl(x, y, FLOOR + 0.5, Z_LID + 0.1, SCREW_PILOT)

    # cable entry (left wall)
    hole = Manifold.cylinder(WALL + 2, (CABLE_D + 0.6) / 2).rotate([0, 90, 0]).translate([ox0 - 1, CABLE_Y, CABLE_Z])
    m -= hole

    # USB-C opening (right wall) + outer recess for the plug overmould
    uz = Z_PCB_TOP + 1.63
    uy = J2["y"]
    slot = rrect(uy - 4.75, uz - 1.95, uy + 4.75, uz + 1.95, 1.6)          # in (y, z)
    m -= _yz_cut(slot, cx1 - 1.0, ox1 + 1.0)
    rec = rrect(uy - 6.2, uz - 3.4, uy + 6.2, uz + 3.4, 2.5)
    m -= _yz_cut(rec, ox1 - 0.8, ox1 + 1.0)

    # engraved label under the base
    m -= prism(text_cs("CIVIC K-LINE BLE", (ox0 + ox1) / 2, (oy0 + oy1) / 2 - 3, 4.0), -0.1, 0.5).mirror([0, 1, 0]) \
        .translate([0, oy0 + oy1, 0])
    m -= prism(text_cs("cable: K=p7  12V=p16  GND=p4/5", (ox0 + ox1) / 2, (oy0 + oy1) / 2 + 4, 2.6), -0.1, 0.5) \
        .mirror([0, 1, 0]).translate([0, oy0 + oy1, 0])
    return m


def _yz_cut(cs_yz, x0, x1):
    """Extrude a CrossSection drawn in (y, z) along +x from x0 to x1."""
    m = cs_yz.extrude(x1 - x0)                  # along z, profile in (y, z) -> map axes
    # extruded profile lives in (X=y, Y=z, Z=x); permute to (x, y, z)
    mesh = m.to_mesh()
    v = np.array(mesh.vert_properties)[:, :3]
    v = np.column_stack([v[:, 2] + x0, v[:, 0], v[:, 1]])
    tri = np.array(mesh.tri_verts)
    # (X, Y, Z) -> (Z, X, Y) is a cyclic (even) permutation: winding is preserved
    from manifold3d import Mesh
    return Manifold(Mesh(vert_properties=v.astype(np.float32), tri_verts=tri.astype(np.uint32)))


# ----------------------------------------------------------------- lid
def lid():
    outer = rrect(ox0, oy0, ox1, oy1, CORNER_R)
    m = prism(outer, Z_LID, Z_TOP)
    # skirt that locates the lid inside the walls
    sk_o = rrect(cx0 + FIT, cy0 + FIT, cx1 - FIT, cy1 - FIT, max(CORNER_R - WALL - FIT, 0.5))
    sk_i = rrect(cx0 + FIT + 1.2, cy0 + FIT + 1.2, cx1 - FIT - 1.2, cy1 - FIT - 1.2, 0.5)
    skirt = prism(sk_o, Z_LID - 2.0, Z_LID + 0.01) - prism(sk_i, Z_LID - 3, Z_LID + 1)
    skirt -= box(cx1 - 3, J2["y"] - 7, Z_LID - 3, cx1 + 1, J2["y"] + 7, Z_LID + 1)    # USB-C
    m += skirt

    # posts that clamp the board (mounting holes + top corners)
    for (x, y) in (H1, H2):
        m += cyl(x, y, Z_PCB_TOP + 0.05, Z_LID + 0.01, 6.0)
    for (x, y) in ((2.0, 6.0), (64.0, 2.0)):
        m += cyl(x, y, Z_PCB_TOP + 0.15, Z_LID + 0.01, 2.6)
    # cable hold-down rib over the zip-tie bridge
    m += box(BRIDGE_X0 + 0.5, CABLE_Y - CABLE_D / 2 - 1, CABLE_Z + CABLE_D / 2 - 0.4,
             BRIDGE_X1 - 0.5, CABLE_Y + CABLE_D / 2 + 1, Z_LID + 0.01)

    # screw holes
    for (x, y) in list((H1, H2)) + BAY_BOSSES:
        m -= cyl(x, y, Z_PCB_TOP - 1, Z_TOP + 1, SCREW_CLEAR)
        m -= cyl(x, y, Z_TOP - 0.8, Z_TOP + 1, 6.2)         # shallow seat for the head

    # flexure buttons (RESET / BOOT): U-slot, thinned tab, plunger nub
    for ref, hinge in (("SW1", +1), ("SW2", +1)):
        x, y = P[ref]["x"], P[ref]["y"]
        ya, yb = (y - 1.9, y + 7.0) if hinge > 0 else (y - 7.0, y + 1.9)
        tab = rrect(x - 3.1, ya - 0.1, x + 3.1, yb - 0.3, 1.1)   # overlaps the slot: no coincident faces
        ring = rrect(x - 3.6, ya - 0.6, x + 3.6, yb, 1.4) - rrect(x - 3.0, ya, x + 3.0, yb + 1, 1.0)
        m -= prism(ring, Z_LID - 1, Z_TOP + 1)
        m -= prism(tab, Z_LID - 1, Z_LID + 0.8)            # tab 1.2 mm thick
        z_btn = Z_PCB_TOP + 1.5
        m += cyl(x, y, z_btn + 0.4, Z_LID + 0.81, 2.6)
        # shallow finger dimple (the lid prints top-face-down, so nothing may stick out of the top)
        m -= prism(CrossSection.circle(1.8).translate([x, y]), Z_TOP - 0.4, Z_TOP + 1)

    # LED windows
    for ref in ("D5", "D6", "D7"):
        m -= cyl(P[ref]["x"], P[ref]["y"], Z_LID - 1, Z_TOP + 1, 2.0)

    # engraved legends (0.6 mm)
    eng = []
    for ref, s in (("D5", "PWR"), ("D6", "STAT"), ("D7", "K")):      # right of the USB, legend to the left
        eng.append(text_cs(s, P[ref]["x"] - 2.0 - len(s) * 0.75, P[ref]["y"], 2.0))
    eng.append(text_cs("RESET", P["SW1"]["x"] + 7.6, P["SW1"]["y"] + 1.0, 2.2))
    eng.append(text_cs("BOOT", P["SW2"]["x"] - 7.0, P["SW2"]["y"], 2.2))
    eng.append(text_cs("USB", cx1 - 6.0, J2["y"], 2.4, rot=-90))
    eng.append(text_cs("OBD", bx0 - BAY + 5.5, CABLE_Y + 9.5, 2.4))
    eng.append(text_cs("CIVIC K-LINE", 40.0, 38.6, 3.6))
    for cs in eng:
        m -= prism(cs, Z_TOP - 0.6, Z_TOP + 1)
    return m


# ----------------------------------------------------------------- board (for preview)
def board_dummy():
    m = prism(rrect(bx0, by0, bx1, by1, 2.0), Z_PCB_BOT, Z_PCB_TOP)
    for ref, v in P.items():
        if ref.startswith(("H", "TP")):
            continue
        x0, y0, x1, y1 = v["crtyd"]
        h = 3.2 if ref in ("U3", "J2") else 1.5
        m += box(max(x0, bx0) + 0.25, max(y0, by0) + 0.25, Z_PCB_TOP, min(x1, bx1) - 0.25, min(y1, by1) - 0.25,
                 Z_PCB_TOP + h)
    return m


def check(base_m, lid_m, brd):
    out = {}
    for name, a, b in (("base x board", base_m, brd), ("lid x board", lid_m, brd), ("lid x base", lid_m, base_m)):
        out[name] = round((a ^ b).volume(), 3)
    return out


def export(m, path, flip=False):
    mesh = m.to_mesh()
    v = np.array(mesh.vert_properties)[:, :3].astype(np.float64)
    f = np.array(mesh.tri_verts)
    v[:, 1] = -v[:, 1]                           # y-down -> y-up (mirror) ...
    f = f[:, ::-1]                               # ... keep normals outward
    if flip:                                     # lid: top face on the bed
        v[:, 2] = -v[:, 2]
        v[:, 1] = -v[:, 1]
    v[:, 2] -= v[:, 2].min()
    v[:, 0] -= v[:, 0].min()
    v[:, 1] -= v[:, 1].min()
    tm = trimesh.Trimesh(v, f, process=False)
    tm.export(path)
    return tm


def export_asm(m, path):
    """Assembly frame shared with KiCad's STEP export (--user-origin 0x0mm): x, -y, z."""
    mesh = m.to_mesh()
    v = np.array(mesh.vert_properties)[:, :3].astype(np.float64)
    v[:, 1] = -v[:, 1]
    trimesh.Trimesh(v, np.array(mesh.tri_verts)[:, ::-1], process=False).export(path)


def main():
    b, l, brd = base(), lid(), board_dummy()
    fc = os.path.join(HERE, "..", "freecad")
    os.makedirs(fc, exist_ok=True)
    export_asm(b, os.path.join(fc, "case_base_asm.stl"))
    export_asm(l, os.path.join(fc, "case_lid_asm.stl"))
    with open(os.path.join(fc, "case_params.json"), "w") as f:
        json.dump({"Z_PCB_BOT": Z_PCB_BOT, "Z_PCB_TOP": Z_PCB_TOP, "Z_LID": Z_LID, "Z_TOP": Z_TOP}, f)
    print("cavity %.1f x %.1f mm, outer %.1f x %.1f x %.1f mm" % (cx1 - cx0, cy1 - cy0, ox1 - ox0, oy1 - oy0, Z_TOP))
    print("interference volumes (mm^3, want 0):", check(b, l, brd))
    tb = export(b, os.path.join(HERE, "case_base.stl"))
    tl = export(l, os.path.join(HERE, "case_lid.stl"), flip=True)
    export(b + l, os.path.join(HERE, "case_assembly.stl"))
    export(b + brd + l.translate([0, 0, 14.0]), os.path.join(HERE, "case_preview_exploded.stl"))
    for n, t in (("base", tb), ("lid", tl)):
        print("%s: watertight=%s  volume=%.1f cm3  bbox=%s" % (n, t.is_watertight, t.volume / 1000,
                                                              np.round(t.extents, 1).tolist()))


if __name__ == "__main__":
    main()
