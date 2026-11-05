"""Reproducible fabrication export for JLCPCB (run with KiCad's python or any python 3).

Outputs to hardware/fab/:
  civic_scanner-erc.rpt / -drc.rpt   checks (export aborts on errors)
  gerbers/ (+ drill) and civic_scanner-gerbers.zip
  civic_scanner-bom-jlcpcb.csv        Comment, Designator, Footprint, LCSC
  civic_scanner-cpl-jlcpcb.csv        Designator, Mid X, Mid Y, Layer, Rotation
  civic_scanner-schematic.pdf, 3D renders
"""
import csv
import os
import shutil
import subprocess
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
HW = os.path.abspath(os.path.join(HERE, ".."))
CLI = r"C:/Program Files/KiCad/10.0/bin/kicad-cli.exe"
SCH = os.path.join(HW, "civic_scanner.kicad_sch")
PCB = os.path.join(HW, "civic_scanner.kicad_pcb")
FAB = os.path.join(HW, "fab")
GBR = os.path.join(FAB, "gerbers")
LAYERS = "F.Cu,B.Cu,F.Paste,B.Paste,F.SilkS,B.SilkS,F.Mask,B.Mask,Edge.Cuts"


def cli(*args, ok=(0,)):
    r = subprocess.run([CLI, *args], capture_output=True, text=True)
    if r.returncode not in ok:
        print(r.stdout[-2000:], r.stderr[-2000:])
        raise SystemExit("kicad-cli failed: " + " ".join(args[:3]))
    return r


def main():
    override = "--override" in sys.argv
    shutil.rmtree(GBR, ignore_errors=True)
    os.makedirs(GBR, exist_ok=True)
    cli("version")
    r = cli("sch", "erc", "-o", os.path.join(FAB, "civic_scanner-erc.rpt"), "--format", "report", "--units", "mm",
            "--severity-warning", "--severity-error", "--exit-code-violations", SCH, ok=(0, 5))
    erc_fail = r.returncode != 0
    r = cli("pcb", "drc", "-o", os.path.join(FAB, "civic_scanner-drc.rpt"), "--format", "report", "--units", "mm",
            "--severity-error", "--refill-zones", "--schematic-parity", "--exit-code-violations", PCB, ok=(0, 5))
    drc_fail = r.returncode != 0
    if (erc_fail or drc_fail) and not override:
        raise SystemExit("FAILURE: ERC/DRC errors (see fab/*.rpt). Re-run with --override to export anyway.")

    cli("pcb", "export", "gerbers", "-o", GBR + os.sep, "--layers", LAYERS, "--subtract-soldermask",
        "--precision", "6", PCB)
    cli("pcb", "export", "drill", "-o", GBR + os.sep, "--format", "excellon", "--drill-origin", "absolute",
        "--excellon-units", "mm", "--excellon-zeros-format", "decimal", "--generate-map", "--map-format", "gerberx2",
        PCB)
    zpath = os.path.join(FAB, "civic_scanner-gerbers.zip")
    with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(os.listdir(GBR)):
            z.write(os.path.join(GBR, f), f)

    # BOM (JLCPCB columns), grouped by value + footprint + LCSC, only parts in BOM
    raw = os.path.join(FAB, "bom_raw.csv")
    cli("sch", "export", "bom", "-o", raw, "--fields", "${QUANTITY},Value,Reference,Footprint,LCSC,MPN,Manufacturer",
        "--labels", "Qty,Comment,Designator,Footprint,LCSC,MPN,Manufacturer",
        "--group-by", "Value,Footprint,LCSC", "--sort-field", "Reference", "--exclude-dnp", SCH)
    with open(raw, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    out = os.path.join(FAB, "civic_scanner-bom-jlcpcb.csv")
    with open(out, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["Comment", "Designator", "Footprint", "LCSC", "Qty", "MPN", "Manufacturer"])
        for row in rows:
            if not row["LCSC"]:
                continue  # test points, mounting holes, power symbols
            w.writerow([row["Comment"], row["Designator"], row["Footprint"].split(":")[-1], row["LCSC"], row["Qty"],
                        row["MPN"], row["Manufacturer"]])
    os.remove(raw)

    # placement (JLCPCB CPL)
    rawpos = os.path.join(FAB, "pos_raw.csv")
    cli("pcb", "export", "pos", "-o", rawpos, "--format", "csv", "--units", "mm", "--side", "both", "--exclude-dnp",
        PCB)
    bom_refs = set()
    with open(out, newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            bom_refs |= {x.strip() for x in row["Designator"].split(",")}
    cpl = os.path.join(FAB, "civic_scanner-cpl-jlcpcb.csv")
    with open(rawpos, newline="", encoding="utf-8") as src, open(cpl, "w", newline="", encoding="utf-8") as dst:
        w = csv.writer(dst)
        w.writerow(["Designator", "Mid X", "Mid Y", "Layer", "Rotation"])
        for row in csv.DictReader(src):
            if row["Ref"] not in bom_refs:
                continue
            layer = "T" if row["Side"].strip().lower() in ("top", "front") else "B"
            w.writerow([row["Ref"], row["PosX"] + "mm", row["PosY"] + "mm", layer, row["Rot"]])
    os.remove(rawpos)

    cli("sch", "export", "pdf", "-o", os.path.join(FAB, "civic_scanner-schematic.pdf"), SCH)
    for side in ("top", "bottom"):
        cli("pcb", "render", "-o", os.path.join(FAB, "render-%s.png" % side), "--side", side, "--width", "1600",
            "--height", "1000", "--quality", "high", "--background", "opaque", PCB)
    print("fabrication files in", FAB)


if __name__ == "__main__":
    main()
