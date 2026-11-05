"""Dump the board data the enclosure generator needs -> hardware/enclosure/board.json.

Run with KiCad's python:  "$KP" export_case_data.py
"""
import json
import os

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
HW = os.path.abspath(os.path.join(HERE, ".."))
T = pcbnew.ToMM


def main():
    b = pcbnew.LoadBoard(os.path.join(HW, "civic_scanner.kicad_pcb"))
    bb = b.GetBoardEdgesBoundingBox()
    data = {
        "board": {"x0": T(bb.GetLeft()), "y0": T(bb.GetTop()), "x1": T(bb.GetRight()), "y1": T(bb.GetBottom()),
                  "thickness": T(b.GetDesignSettings().GetBoardThickness())},
        "parts": {},
    }
    for fp in b.GetFootprints():
        c = fp.GetCourtyard(pcbnew.F_CrtYd)
        cb = c.BBox() if c.OutlineCount() else fp.GetBoundingBox(False)
        pos = fp.GetPosition()
        data["parts"][fp.GetReference()] = {
            "x": T(pos.x), "y": T(pos.y), "rot": fp.GetOrientationDegrees(),
            "side": "B" if fp.IsFlipped() else "F",
            "crtyd": [T(cb.GetLeft()), T(cb.GetTop()), T(cb.GetRight()), T(cb.GetBottom())],
            "tht": any(p.GetAttribute() == pcbnew.PAD_ATTRIB_PTH for p in fp.Pads()),
        }
    os.makedirs(os.path.join(HW, "enclosure"), exist_ok=True)
    out = os.path.join(HW, "enclosure", "board.json")
    with open(out, "w") as f:
        json.dump(data, f, indent=1)
    print("wrote", out, len(data["parts"]), "parts")


if __name__ == "__main__":
    main()
