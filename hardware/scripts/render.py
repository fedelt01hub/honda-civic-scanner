"""Render helpers: schematic -> PDF -> PNG crops, ERC and netlist export.

Usage (KiCad python):
  python render.py sch [x0 y0 x1 y1 (mm)] [--dpi N] [--out name]
  python render.py erc
  python render.py net
"""
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HW = os.path.abspath(os.path.join(HERE, ".."))
BUILD = os.path.join(HW, "build")
CLI = r"C:/Program Files/KiCad/10.0/bin/kicad-cli.exe"
SCH = os.path.join(HW, "civic_scanner.kicad_sch")
PCB = os.path.join(HW, "civic_scanner.kicad_pcb")
os.makedirs(BUILD, exist_ok=True)


def run(args):
    r = subprocess.run([CLI] + args, capture_output=True, text=True)
    if r.returncode not in (0, 5):
        print(r.stdout, r.stderr)
    return r


def sch_png(box=None, dpi=200, name="sch"):
    import fitz
    pdf = os.path.join(BUILD, "sch.pdf")
    run(["sch", "export", "pdf", "--no-background-color", "-o", pdf, SCH])
    doc = fitz.open(pdf)
    page = doc[0]
    k = 72 / 25.4
    clip = fitz.Rect(box[0] * k, box[1] * k, box[2] * k, box[3] * k) if box else None
    pix = page.get_pixmap(dpi=dpi, clip=clip)
    out = os.path.join(BUILD, name + ".png")
    pix.save(out)
    print(out, pix.width, pix.height)


def erc():
    out = os.path.join(BUILD, "erc.json")
    run(["sch", "erc", "--format", "json", "--severity-all", "-o", out, SCH])
    d = json.load(open(out, encoding="utf-8"))
    n = 0
    for sh in d.get("sheets", []):
        for v in sh.get("violations", []):
            n += 1
            items = "; ".join(i.get("description", "") for i in v.get("items", []))
            print("[%s] %s: %s | %s" % (v.get("severity"), v.get("type"), v.get("description"), items))
    print("ERC violations:", n)


def net():
    out = os.path.join(BUILD, "netlist.xml")
    run(["sch", "export", "netlist", "--format", "kicadxml", "-o", out, SCH])
    import xml.etree.ElementTree as ET
    root = ET.parse(out).getroot()
    nets = {}
    for n in root.iter("net"):
        nodes = sorted("%s.%s" % (x.get("ref"), x.get("pin")) for x in n.iter("node"))
        nets[n.get("name")] = nodes
    for k in sorted(nets):
        print("%-28s %s" % (k, " ".join(nets[k])))
    json.dump(nets, open(os.path.join(BUILD, "nets.json"), "w"), indent=1)


def pcb_png(layers, name, box=None, dpi=600, mirror=False):
    """Plot PCB layers to SVG with kicad-cli and rasterise (box in board mm)."""
    import fitz
    svg = os.path.join(BUILD, name + ".svg")
    args = ["pcb", "export", "svg", "--layers", layers, "--mode-single", "--fit-page-to-board",
            "--exclude-drawing-sheet", "--drill-shape-opt", "2", "-o", svg, PCB]
    if mirror:
        args.insert(3, "--mirror")
    run(args)
    doc = fitz.open(svg)
    page = doc[0]
    clip = None
    if box:
        # page is fitted to board bbox; board origin (0,0) at page origin (outline starts at 0,0)
        sx = page.rect.width / BOARD_W
        sy = page.rect.height / BOARD_H
        clip = fitz.Rect(box[0] * sx, box[1] * sy, box[2] * sx, box[3] * sy)
    pix = page.get_pixmap(dpi=dpi, clip=clip, alpha=False)
    out = os.path.join(BUILD, name + ".png")
    pix.save(out)
    print(out, pix.width, pix.height)


BOARD_W, BOARD_H = 66.0, 42.0


def drc():
    out = os.path.join(BUILD, "drc.json")
    run(["pcb", "drc", "--format", "json", "--severity-all", "--schematic-parity", "--refill-zones",
         "-o", out, PCB])
    d = json.load(open(out, encoding="utf-8"))
    from collections import Counter
    cnt = Counter()
    lines = []
    for key in ("violations", "unconnected_items", "schematic_parity"):
        for v in d.get(key, []):
            cnt[(key, v.get("type"), v.get("severity"))] += 1
            items = " | ".join("%s @(%.2f,%.2f)" % (i.get("description", ""), i.get("pos", {}).get("x", 0),
                                                   i.get("pos", {}).get("y", 0)) for i in v.get("items", []))
            lines.append("[%s/%s] %s: %s" % (key, v.get("severity"), v.get("type"), items))
    for l in lines[:int(os.environ.get("DRC_MAX", "60"))]:
        print(l)
    for k, n in sorted(cnt.items()):
        print(n, k)
    print("TOTAL", sum(cnt.values()))


def render3d(name="3d_top", side="top", zoom="1"):
    out = os.path.join(BUILD, name + ".png")
    run(["pcb", "render", "--side", side, "--width", "1800", "--height", "1100", "--quality", "basic",
         "--zoom", zoom, "-o", out, PCB])
    print(out)


if __name__ == "__main__":
    if sys.argv[1] == "pcb":
        lay = sys.argv[2]
        name = sys.argv[3]
        box = [float(a) for a in sys.argv[4:8]] if len(sys.argv) >= 8 else None
        pcb_png(lay, name, box, dpi=int(os.environ.get("DPI", "600")), mirror=os.environ.get("MIRROR") == "1")
        sys.exit(0)
    if sys.argv[1] == "drc":
        drc()
        sys.exit(0)
    if sys.argv[1] == "3d":
        render3d(*sys.argv[2:])
        sys.exit(0)
    cmd = sys.argv[1]
    if cmd == "sch":
        args = [a for a in sys.argv[2:] if not a.startswith("--")]
        dpi = 200
        name = "sch"
        if "--dpi" in sys.argv:
            dpi = int(sys.argv[sys.argv.index("--dpi") + 1])
            args.remove(str(dpi))
        if "--out" in sys.argv:
            name = sys.argv[sys.argv.index("--out") + 1]
            args.remove(name)
        box = [float(a) for a in args] if len(args) == 4 else None
        sch_png(box, dpi, name)
    elif cmd == "erc":
        erc()
    elif cmd == "net":
        net()
