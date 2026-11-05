"""Build hardware/freecad/civic_scanner_ensamble.FCStd (run with FreeCAD's freecadcmd).

  "C:/Program Files/FreeCAD 1.1/bin/freecadcmd.exe" build_fcstd.py

Contents:
  PCB         KiCad STEP export (--user-origin 0x0mm), raised to the standoff height
  Faltantes   placeholder boxes for parts without a STEP model (USB-C, buttons, PTC)
  Base, Tapa  solids converted from the enclosure meshes (gen_case.py), in assembly position
"""
import json
import os

import FreeCAD as App
import Import
import Mesh
import Part

HERE = os.path.dirname(os.path.abspath(__file__))
HW = os.path.abspath(os.path.join(HERE, ".."))
P = json.load(open(os.path.join(HERE, "case_params.json")))
BD = json.load(open(os.path.join(HW, "enclosure", "board.json")))["parts"]
OUT = os.path.join(HERE, "civic_scanner_ensamble.FCStd")


def main():
    if os.path.exists(OUT):
        os.remove(OUT)
    doc = App.newDocument("civic_scanner_ensamble")

    # --- PCB from KiCad
    before = set(o.Name for o in doc.Objects)
    Import.insert(os.path.join(HERE, "civic_scanner_pcb.step"), doc.Name)
    pcb_objs = [o for o in doc.Objects if o.Name not in before]
    roots = [o for o in pcb_objs if not o.InList]
    pcb = doc.addObject("App::Part", "PCB")
    for o in roots:
        pcb.addObject(o)
    pcb.Placement.Base = App.Vector(0, 0, P["Z_PCB_BOT"])

    # --- placeholders for parts without STEP models (board frame: x, -y)
    miss = doc.addObject("App::Part", "Faltantes")
    miss.Placement.Base = App.Vector(0, 0, P["Z_PCB_TOP"])
    boxes = {
        "J2_USB_C": (BD["J2"]["x"] - 3.65, BD["J2"]["y"], 7.3, 8.94, 3.26),
        "SW1_RESET": (BD["SW1"]["x"], BD["SW1"]["y"], 5.1, 5.1, 1.5),
        "SW2_BOOT": (BD["SW2"]["x"], BD["SW2"]["y"], 5.1, 5.1, 1.5),
        "F1_PTC": (BD["F1"]["x"], BD["F1"]["y"], 4.5, 3.2, 1.2),
    }
    for name, (x, y, sx, sy, sz) in boxes.items():
        if name == "J2_USB_C":
            x = BD["J2"]["x"]
        b = doc.addObject("Part::Box", name)
        b.Length, b.Width, b.Height = sx, sy, sz
        b.Placement.Base = App.Vector(x - sx / 2, -y - sy / 2, 0)
        miss.addObject(b)

    # --- enclosure
    for name, fn in (("Base", "case_base_asm.stl"), ("Tapa", "case_lid_asm.stl")):
        m = Mesh.Mesh(os.path.join(HERE, fn))
        sh = Part.Shape()
        sh.makeShapeFromMesh(m.Topology, 0.01)
        solid = Part.makeSolid(sh)
        try:
            solid = solid.removeSplitter()
        except Exception as e:  # keep the triangulated solid if refinement fails
            print("removeSplitter failed for", name, e)
        o = doc.addObject("Part::Feature", name)
        o.Shape = solid
        print(name, "valid:", solid.isValid(), "faces:", len(solid.Faces), "volume: %.1f cm3" % (solid.Volume / 1000))

    doc.recompute()
    doc.saveAs(OUT)
    print("saved", OUT)


main()  # freecadcmd does not always set __name__ == "__main__"
