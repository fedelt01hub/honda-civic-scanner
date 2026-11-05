import pcbnew, sys
FP = r"C:/Program Files/KiCad/10.0/share/kicad/footprints"
for full in sys.argv[1:]:
    lib, name = full.split(":")
    fp = pcbnew.FootprintLoad(FP + "/" + lib + ".pretty", name)
    bb = fp.GetBoundingBox(False)
    cy = fp.GetCourtyard(pcbnew.F_CrtYd).BBox() if fp.GetCourtyard(pcbnew.F_CrtYd).OutlineCount() else bb
    print("== %s  bbox %.2fx%.2f  (%.2f,%.2f)-(%.2f,%.2f)  crtyd (%.2f,%.2f)-(%.2f,%.2f)" % (
        full, bb.GetWidth() / 1e6, bb.GetHeight() / 1e6, bb.GetLeft() / 1e6, bb.GetTop() / 1e6, bb.GetRight() / 1e6, bb.GetBottom() / 1e6,
        cy.GetLeft() / 1e6, cy.GetTop() / 1e6, cy.GetRight() / 1e6, cy.GetBottom() / 1e6))
    seen = set()
    for p in fp.Pads():
        k = (p.GetNumber(), round(p.GetPosition().x / 1e6, 2), round(p.GetPosition().y / 1e6, 2))
        if k in seen:
            continue
        seen.add(k)
        if len(seen) > 40:
            break
        print("   pad %-4s (%.2f, %.2f) size %.2fx%.2f %s" % (p.GetNumber(), p.GetPosition().x / 1e6, p.GetPosition().y / 1e6,
                                                         p.GetSize().x / 1e6, p.GetSize().y / 1e6, "TH" if p.GetAttribute() == pcbnew.PAD_ATTRIB_PTH else ""))
