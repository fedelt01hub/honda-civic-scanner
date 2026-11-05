"""Write hardware/civic_scanner.kicad_pro with JLCPCB-friendly rules and net classes."""
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
HW = os.path.abspath(os.path.join(HERE, ".."))
PRO = os.path.join(HW, "civic_scanner.kicad_pro")
TEMPLATE = r"C:/Program Files/KiCad/10.0/share/kicad/template/kicad.kicad_pro"


def netclass(name, tw, cl, vd=0.6, vdr=0.3):
    return {"name": name, "clearance": cl, "track_width": tw, "via_diameter": vd, "via_drill": vdr,
            "microvia_diameter": 0.3, "microvia_drill": 0.1, "diff_pair_width": 0.2, "diff_pair_gap": 0.25,
            "diff_pair_via_gap": 0.25, "line_style": 0, "wire_width": 6, "bus_width": 12,
            "schematic_color": "rgba(0, 0, 0, 0.000)", "pcb_color": "rgba(0, 0, 0, 0.000)",
            "priority": 2147483647 if name == "Default" else 0}


def main():
    if os.path.exists(PRO):
        pro = json.load(open(PRO, encoding="utf-8"))
    else:
        pro = json.load(open(TEMPLATE, encoding="utf-8"))
    pro.setdefault("meta", {})["filename"] = "civic_scanner.kicad_pro"
    ds = pro.setdefault("board", {}).setdefault("design_settings", {})
    rules = ds.setdefault("rules", {})
    rules.update({
        "min_clearance": 0.15, "min_track_width": 0.15, "min_via_diameter": 0.5, "min_via_annular_width": 0.1,
        "min_through_hole_diameter": 0.3, "min_hole_to_hole": 0.5, "min_hole_clearance": 0.25,
        "min_copper_edge_clearance": 0.3, "min_silk_clearance": 0.0, "min_text_height": 0.8,
        "min_text_thickness": 0.12, "solder_mask_to_copper_clearance": 0.0,
    })
    ds["track_widths"] = [0.0, 0.25, 0.4, 0.6, 0.8]
    ds["via_dimensions"] = [{"diameter": 0.0, "drill": 0.0}, {"diameter": 0.6, "drill": 0.3},
                            {"diameter": 0.8, "drill": 0.4}]
    power = ["+BATT", "GND", "+5V", "VBUS", "/5V_BUCK", "/SW", "/VBAT_F", "Net-(J1-Pin_2)"]
    pro["net_settings"] = {
        "classes": [netclass("Default", 0.25, 0.2), netclass("Power", 0.6, 0.2, 0.8, 0.4),
                    netclass("KLine", 0.4, 0.25), netclass("USB", 0.2, 0.15)],
        "meta": {"version": 4},
        "netclass_patterns": [{"netclass": "Power", "pattern": p} for p in power]
        + [{"netclass": "KLine", "pattern": "/KLINE"}]
        + [{"netclass": "USB", "pattern": p} for p in ("/USB_DP", "/USB_DN")],
    }
    with open(PRO, "w", encoding="utf-8", newline="\n") as f:
        json.dump(pro, f, indent=2)


if __name__ == "__main__":
    main()
