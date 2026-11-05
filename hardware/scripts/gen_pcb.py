"""Generate hardware/civic_scanner.kicad_pcb from the schematic netlist.

Run with KiCad's python:  "C:/Program Files/KiCad/10.0/bin/python.exe" gen_pcb.py
Steps: export netlist (kicad-cli) -> footprints + nets -> placement -> outline ->
routing (routes.py) -> zones -> fill -> save.
"""
import json
import math
import os
import subprocess
import xml.etree.ElementTree as ET

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
HW = os.path.abspath(os.path.join(HERE, ".."))
BUILD = os.path.join(HW, "build")
CLI = r"C:/Program Files/KiCad/10.0/bin/kicad-cli.exe"
SCH = os.path.join(HW, "civic_scanner.kicad_sch")
PCB = os.path.join(HW, "civic_scanner.kicad_pcb")
FPDIR = r"C:/Program Files/KiCad/10.0/share/kicad/footprints"
LOCAL_FP = {"civic_scanner": os.path.join(HW, "lib", "civic_scanner.pretty")}

W, H = 66.0, 42.0          # board size (mm)
CORNER = 2.0               # outline corner radius


def MM(v):
    return pcbnew.FromMM(v)


def V(x, y):
    return pcbnew.VECTOR2I(MM(x), MM(y))


# ---------------------------------------------------------------------------
# Placement: ref -> (x, y, rotation_deg)   (all top side)
ESP_X, ESP_Y = 42.3, 13.1      # module rot 0: antenna at the top board edge
PLACE = {
    # --- ESP32 module (antenna flush with top edge, pins 1-9 face west, 10-18 face east)
    "U3": (ESP_X, ESP_Y, 0),
    "C11": (31.4, 8.4, 270),
    # --- USB-C on the east edge, ESD between connector and module
    "J2": (W - 3.65, 13.85, 90),
    "U4": (54.3, 10.4, 0),
    "R5": (58.5, 7.6, 0), "R4": (55.6, 16.9, 270),
    "D4": (58.6, 21.6, 0),
    "TP2": (54.6, 23.0, 0), "TP3": (51.4, 23.0, 0),
    # --- DLC connector + K-line transceiver (top-left)
    "J1": (6.2, 9.0, -90),
    "U5": (17.0, 5.0, 90),
    "C14": (12.2, 2.4, 90), "C13": (17.6, 10.3, 270),
    "R6": (7.4, 3.0, 180),
    # --- LEDs (row under the module): LED above, series resistor below
    # LEDs on the right edge, under the USB-C: LED at the edge, resistor beside it
    "D5": (63.4, 26.4, 0), "R7": (60.2, 26.4, 180),
    "D6": (63.4, 29.2, 0), "R8": (60.2, 29.2, 180),
    "D7": (63.4, 32.0, 0), "R9": (60.2, 32.0, 180),
    # --- protection (left, below the K-line lanes)
    "F1": (14.5, 14.6, 0),
    "D1": (17.8, 20.4, 270),
    "D2": (22.0, 14.6, 180),
    "C1": (27.4, 17.2, 90), "C2": (27.4, 13.6, 0),
    # --- buck (bottom-left)
    "U1": (12.0, 33.0, 0),
    "C3": (11.6, 37.2, 0), "C4": (8.4, 34.6, 90),
    "C5": (14.6, 29.8, 0),
    "L1": (17.6, 33.0, 0),
    "C6": (21.6, 33.4, 90), "C7": (24.4, 33.4, 90),
    "D3": (23.0, 38.6, 0),
    # --- LDO
    "U2": (31.0, 35.6, 0),
    "C8": (27.8, 35.6, 90), "C9": (34.2, 35.6, 90), "C10": (36.6, 35.6, 90),
    # --- buttons + RC
    "SW1": (24.2, 3.0, 0), "R1": (21.4, 7.2, 0), "C12": (24.4, 7.2, 0),
    "SW2": (25.4, 24.6, 0), "R2": (30.8, 21.0, 90), "R3": (30.4, 16.6, 90),
    # --- test points / holes
    "TP1": (8.8, 22.0, 0), "TP7": (12.0, 22.0, 0),
    "TP4": (29.6, 13.0, 0) if False else (21.4, 19.8, 0), "TP5": (27.6, 39.6, 0), "TP6": (39.4, 36.4, 0),
    "H1": (3.6, 38.4, 0), "H2": (62.4, 38.4, 0),
}


def export_netlist():
    out = os.path.join(BUILD, "netlist.xml")
    subprocess.run([CLI, "sch", "export", "netlist", "--format", "kicadxml", "-o", out, SCH], check=True,
                   capture_output=True)
    root = ET.parse(out).getroot()
    comps = {}
    for c in root.iter("comp"):
        ref = c.get("ref")
        fields = {f.get("name"): (f.text or "") for f in c.iter("field")}
        tstamps = c.find("tstamps")
        sheetpath = c.find("sheetpath")
        comps[ref] = {
            "value": c.findtext("value"),
            "footprint": c.findtext("footprint"),
            "fields": fields,
            "uuid": tstamps.text.strip() if tstamps is not None else c.findtext("tstamp"),
            "sheet_tstamps": sheetpath.get("tstamps") if sheetpath is not None else "/",
        }
    nets = {}
    for n in root.iter("net"):
        nets[n.get("name")] = [(x.get("ref"), x.get("pin")) for x in n.iter("node")]
    return comps, nets


def load_fp(fpid):
    lib, name = fpid.split(":")
    path = LOCAL_FP.get(lib) or os.path.join(FPDIR, lib + ".pretty")
    fp = pcbnew.FootprintLoad(path, name)
    if fp is None:
        raise RuntimeError("footprint not found: " + fpid)
    fp.SetFPID(pcbnew.LIB_ID(lib, name))
    return fp


def build_board():
    comps, nets = export_netlist()
    board = pcbnew.CreateEmptyBoard()
    board.SetCopperLayerCount(2)
    ds = board.GetDesignSettings()
    ds.SetBoardThickness(MM(1.6))

    # nets
    netinfo = {}
    for name in nets:
        ni = pcbnew.NETINFO_ITEM(board, name)
        board.Add(ni)
        netinfo[name] = ni
    pad_net = {}
    for name, nodes in nets.items():
        for ref, pin in nodes:
            pad_net[(ref, pin)] = name

    fps = {}
    for ref, c in sorted(comps.items()):
        if ref not in PLACE:
            raise RuntimeError("no placement for " + ref)
        fp = load_fp(c["footprint"])
        fp.SetReference(ref)
        fp.SetValue(c["value"])
        if c["fields"].get("Description"):
            try:
                fp.GetField(pcbnew.FIELD_T_DESCRIPTION).SetText(c["fields"]["Description"])
            except Exception:
                fp.SetLibDescription(c["fields"]["Description"])
        for k in ("LCSC", "MPN", "Manufacturer"):
            if c["fields"].get(k):
                fp.SetField(k, c["fields"][k])
                fld = fp.GetField(k)
                fld.SetVisible(False)
                fld.SetLayer(pcbnew.F_Fab)
        sheet = c["sheet_tstamps"]
        if not sheet.endswith("/"):
            sheet += "/"
        fp.SetPath(pcbnew.KIID_PATH(sheet + c["uuid"]))
        models = fp.Models()
        for k in range(len(models)):
            m = models[k]
            base = os.path.basename(m.m_Filename).replace(".step", ".wrl")
            if os.path.exists(os.path.join(HW, "lib", "3d", base)):
                m.m_Filename = "${KIPRJMOD}/lib/3d/" + base
                models[k] = m
        board.Add(fp)
        x, y, rot = PLACE[ref]
        fp.SetPosition(V(x, y))
        fp.SetOrientationDegrees(rot)
        for pad in fp.Pads():
            key = (ref, pad.GetNumber())
            if key in pad_net:
                pad.SetNet(netinfo[pad_net[key]])
        fps[ref] = fp
    return board, fps, netinfo


def outline(board):
    r = CORNER
    pts = []
    # rounded rectangle from arcs + lines
    def seg(a, b):
        s = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_SEGMENT)
        s.SetStart(V(*a)); s.SetEnd(V(*b)); s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(MM(0.1))
        board.Add(s)

    def arc(c, start, mid, end):
        s = pcbnew.PCB_SHAPE(board, pcbnew.SHAPE_T_ARC)
        s.SetArcGeometry(V(*start), V(*mid), V(*end)); s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(MM(0.1))
        board.Add(s)

    k = r * (1 - math.sqrt(0.5))
    seg((r, 0), (W - r, 0)); seg((W, r), (W, H - r)); seg((W - r, H), (r, H)); seg((0, H - r), (0, r))
    arc(None, (W - r, 0), (W - k, k), (W, r))
    arc(None, (W, H - r), (W - k, H - k), (W - r, H))
    arc(None, (r, H), (k, H - k), (0, H - r))
    arc(None, (0, r), (k, k), (r, 0))


def main():
    board, fps, netinfo = build_board()
    outline(board)
    import routes
    routes.apply(board, fps, netinfo, W, H)
    pcbnew.SaveBoard(PCB, board)
    # restore project rules (SaveBoard rewrites the .kicad_pro with board defaults)
    import gen_pro
    gen_pro.main()
    print("saved", PCB, "footprints:", len(fps))


if __name__ == "__main__":
    main()
