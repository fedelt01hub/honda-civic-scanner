import json, os, pcbnew
HW = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
b = pcbnew.LoadBoard(os.path.join(HW, "civic_scanner.kicad_pcb"))
out = {}
for fp in b.GetFootprints():
    ref = fp.GetReference()
    pads = []
    for p in fp.Pads():
        pos = p.GetPosition()
        bb = p.GetBoundingBox()
        pads.append([p.GetNumber(), round(pcbnew.ToMM(pos.x), 3), round(pcbnew.ToMM(pos.y), 3), p.GetNetname(),
                     round(pcbnew.ToMM(bb.GetWidth()), 2), round(pcbnew.ToMM(bb.GetHeight()), 2)])
    cy = fp.GetCourtyard(pcbnew.F_Cu if False else pcbnew.F_CrtYd).BBox()
    out[ref] = {"pos": [pcbnew.ToMM(fp.GetPosition().x), pcbnew.ToMM(fp.GetPosition().y)],
                "rot": fp.GetOrientationDegrees(), "pads": pads,
                "crtyd": [round(pcbnew.ToMM(v), 2) for v in (cy.GetLeft(), cy.GetTop(), cy.GetRight(), cy.GetBottom())]}
json.dump(out, open(os.path.join(HW, "build", "pads.json"), "w"), indent=0)
import sys
for ref in sys.argv[1:]:
    d = out[ref]
    print(ref, d["pos"], d["rot"], "crtyd", d["crtyd"])
    for p in d["pads"]:
        if p[0] == "19" and ref == "U3":
            continue
        print("   ", p)
