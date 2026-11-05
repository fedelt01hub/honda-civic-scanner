"""Low-fidelity VRML models for parts whose KiCad 3D models are not installed.

Coordinates are given in footprint millimetres (x right, y down, z up from the board);
they are converted to KiCad's VRML convention (0.1 inch units, y up).
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "lib", "3d")
U = 2.54


def box(x0, y0, x1, y1, z0, z1, rgb):
    cx, cy, cz = (x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2
    sx, sy, sz = abs(x1 - x0), abs(y1 - y0), abs(z1 - z0)
    return ("Transform { translation %.4f %.4f %.4f children [ Shape { appearance Appearance { material Material "
            "{ diffuseColor %.3f %.3f %.3f specularColor 0.3 0.3 0.3 shininess 0.4 } } geometry Box { size %.4f %.4f %.4f } } ] }\n"
            % (cx / U, -cy / U, cz / U, rgb[0], rgb[1], rgb[2], sx / U, sy / U, sz / U))


def write(name, boxes):
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, name), "w", newline="\n") as f:
        f.write("#VRML V2.0 utf8\n")
        for b in boxes:
            f.write(box(*b))


METAL = (0.78, 0.78, 0.80)
DARK = (0.08, 0.08, 0.09)
NYLON = (0.93, 0.92, 0.86)
TIN = (0.85, 0.85, 0.85)

# HRO TYPE-C-31-M-12: shell 8.94 x 7.3 x 3.26 mm, opening at +y (board edge)
write("USB_C_Receptacle_HRO_TYPE-C-31-M-12.wrl", [
    (-4.47, -3.65, 4.47, 3.65, 0.0, 3.26, METAL),
    (-3.6, 3.0, 3.6, 3.75, 0.55, 2.7, DARK),         # receptacle mouth
    (-3.4, -4.6, 3.4, -3.65, 0.0, 0.15, TIN),         # SMD leads
])

# JST B3P-VH vertical: nylon housing with polarising ramp, three 1.14 mm square posts
write("JST_VH_B3P-VH_1x03_P3.96mm_Vertical.wrl", [
    (-1.95, -2.0, 9.87, 4.8, 0.0, 9.4, NYLON),
    (-0.75, -3.7, 8.67, -2.0, 0.0, 7.5, NYLON),
    (-0.57, -0.57, 0.57, 0.57, 0.0, 8.9, TIN),
    (3.39, -0.57, 4.53, 0.57, 0.0, 8.9, TIN),
    (7.35, -0.57, 8.49, 0.57, 0.0, 8.9, TIN),
])
print("models written to", os.path.abspath(OUT))
