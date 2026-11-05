"""Generate the Civic scanner schematic (hardware/civic_scanner.kicad_sch).

All coordinates are given in grid units G = 2.54 mm (half-units allowed) so every pin
lands on the 50 mil grid. Run with KiCad's bundled python:
    "C:/Program Files/KiCad/10.0/bin/python.exe" hardware/scripts/gen_sch.py
"""
import os
import uuid as _uuid

from sexp import S, Sym, dumps
import symlib

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "civic_scanner.kicad_sch")
PROJECT = "civic_scanner"
G = 2.54
ROOT_UUID = "3c1a1f6e-1b1e-4a8e-9a5e-2f9b6d0c0001"

_ns = _uuid.UUID("6f1c2b8e-7d0a-4c55-9e43-0a1b2c3d4e5f")


def uid(key):
    """Deterministic UUIDs so regenerating keeps sch<->pcb links stable."""
    return str(_uuid.uuid5(_ns, key))


OFF = 2  # sheet offset in grid units (keeps everything inside the A3 frame)


def mm(v):
    return round((v + OFF) * G, 4)


def font(size=1.27, bold=False):
    f = S("font", S("size", size, size))
    if bold:
        f.append(S("bold", Sym("yes")))
    return f


# ----------------------------------------------------------------------------
# Net colours (wire/label colouring by function)
COL = {
    "VBAT": (200, 0, 0), "5V": (230, 110, 0), "3V3": (180, 0, 160), "GND": (60, 60, 60),
    "KLINE": (0, 120, 220), "USB": (0, 150, 70), "LED": (150, 120, 0), "CTRL": (90, 60, 160),
    "UART": (0, 140, 140),
}
NET_COLOR = {
    "KLINE": "KLINE", "K_RX": "KLINE", "K_TX": "KLINE",
    "USB_DP": "USB", "USB_DN": "USB", "CC1": "USB", "CC2": "USB",
    "LED_STAT": "LED", "LED_BLE": "LED", "LED_ECU": "LED",
    "EN": "CTRL", "BOOT": "CTRL", "U0TXD": "UART", "U0RXD": "UART",
    "VBAT_IN": "VBAT", "VBAT_F": "VBAT", "5V_BUCK": "5V", "SW": "5V",
}


class Part:
    def __init__(self, sch, ref, lib_id, value, pos, rot=0, mirror=None, fields=None,
                 ref_at=None, val_at=None, hide_value=False, in_bom=True, on_board=True, k_mm=False):
        self.sch, self.ref, self.lib_id, self.value = sch, ref, lib_id, value
        self.X, self.Y = (pos if k_mm else (mm(pos[0]), mm(pos[1])))
        self.rot, self.mirror = rot, mirror
        self.fields = fields or {}
        self.ref_at, self.val_at, self.hide_value = ref_at, val_at, hide_value
        self.in_bom, self.on_board = in_bom, on_board
        self.pins = symlib.pins(lib_id)
        self.uuid = uid("sym:" + ref)

    def p(self, num):
        x, y, a, _n, _t = self.pins[num]
        return symlib.xform(x, y, self.X, self.Y, self.rot, self.mirror)

    def out(self, num):
        return symlib.outward_dir(self.pins[num][2], self.rot, self.mirror)


class Sch:
    def __init__(self):
        self.parts = []
        self.items = []
        self.lib_ids = set()
        self.pwr_n = 0
        self.wire_pts = []

    # -- symbols -----------------------------------------------------------
    def part(self, *a, **k):
        p = Part(self, *a, **k)
        self.parts.append(p)
        self.lib_ids.add(p.lib_id)
        return p

    def power(self, name, at_mm, rot=0, lib=None):
        self.pwr_n += 1
        ref = "#PWR%03d" % self.pwr_n if name != "PWR_FLAG" else "#FLG%03d" % self.pwr_n
        lib_id = lib or ("power:" + name)
        p = Part(self, ref, lib_id, name, at_mm, rot=rot, in_bom=False, on_board=False, k_mm=True)
        p.is_power = True
        self.parts.append(p)
        self.lib_ids.add(lib_id)
        return p

    # -- wiring ------------------------------------------------------------
    def wire(self, *pts, net=None):
        pts = [pt if isinstance(pt[0], float) and False else pt for pt in pts]
        for a, b in zip(pts, pts[1:]):
            self.items.append(("wire", a, b, net))
            self.wire_pts += [a, b]

    def junction(self, pt):
        self.items.append(("junction", pt))

    def label(self, name, pt, direction):
        """Local label at pt; direction = unit vector the text runs toward."""
        self.items.append(("label", name, pt, direction))

    def nc(self, pt):
        self.items.append(("nc", pt))

    def text(self, s, at_g, size=2.0, bold=True, color=None):
        self.items.append(("text", s, (mm(at_g[0]), mm(at_g[1])), size, bold, color))

    def box(self, a_g, b_g, title, color):
        self.items.append(("box", (mm(a_g[0]), mm(a_g[1])), (mm(b_g[0]), mm(b_g[1])), color))
        self.text(title, (a_g[0] + 0.6, a_g[1] + 1.0), size=2.0, bold=True, color=color)

    # helpers ---------------------------------------------------------------
    def pin_label(self, part, num, name, stub=1):
        """Short stub from pin outward, then a label pointing away from the part."""
        x, y = part.p(num)
        dx, dy = part.out(num)
        end = (round(x + dx * stub * G, 4), round(y + dy * stub * G, 4))
        if stub:
            self.wire((x, y), end, net=name)
        self.label(name, end, (dx, dy))
        return end

    def pin_power(self, part, num, name, stub=0):
        x, y = part.p(num)
        dx, dy = part.out(num)
        end = (round(x + dx * stub * G, 4), round(y + dy * stub * G, 4))
        if stub:
            self.wire((x, y), end, net=name)
        rot = 0
        if name == "GND":
            if (dx, dy) != (0, 1):
                # horizontal pin: drop down one grid to the ground symbol
                down = (end[0], round(end[1] + G, 4))
                self.wire(end, down, net=name)
                end = down
        self.power(name, end, rot=rot)
        return end

    def pin_nc(self, part, num):
        self.nc(part.p(num))


# ----------------------------------------------------------------------------
LCSC = {}


def F(fp, lcsc=None, mpn=None, mfr=None, desc=None):
    d = {"Footprint": fp}
    if mpn:
        d["MPN"] = mpn
    if mfr:
        d["Manufacturer"] = mfr
    if lcsc:
        d["LCSC"] = lcsc
    if desc:
        d["Description"] = desc
    return d


R0603 = "Resistor_SMD:R_0603_1608Metric"
C0603 = "Capacitor_SMD:C_0603_1608Metric"
C0805 = "Capacitor_SMD:C_0805_2012Metric"
C1206 = "Capacitor_SMD:C_1206_3216Metric"
C1210 = "Capacitor_SMD:C_1210_3225Metric"
LED0603 = "LED_SMD:LED_0603_1608Metric"

PART_DB = {
    "10k": F(R0603, "C25804", "0603WAF1002T5E", "UNI-ROYAL"),
    "5.1k": F(R0603, "C23186", "0603WAF5101T5E", "UNI-ROYAL"),
    "1k": F(R0603, "C21190", "0603WAF1001T5E", "UNI-ROYAL"),
    "330R": F(R0603, "C23138", "0603WAF3300T5E", "UNI-ROYAL"),
    "1k_2512": F("Resistor_SMD:R_2512_6332Metric", "C54315", "25121WF1001T4E", "UNI-ROYAL"),
    "100n50": F(C0603, "C14663", "CC0603KRX7R9BB104", "YAGEO"),
    "1u": F(C0603, "C15849", "CL10A105KB8NNNC", "Samsung"),
    "10u25": F(C0805, "C15850", "CL21A106KAYNNNE", "Samsung"),
    "22u16": F(C0805, "C45783", "CL21A226MAQNNNE", "Samsung"),
    "22u25": F(C1206, "C12891", "CL31A226KAHNNNE", "Samsung"),
    "10u50": F(C1210, "C77102", "GRM32ER71H106KA12L", "Murata"),
    "LED_R": F(LED0603, "C2286", "KT-0603R", "Hubei KENTO"),
    "LED_Y": F(LED0603, "C89811", "NCD0603Y2", "NATIONSTAR"),      # JLC preferred (no fee)
    "LED_W": F(LED0603, "C2290", "KT-0603W", "Hubei KENTO"),       # JLC basic
}


def build():
    s = Sch()
    U = lambda x, y: (mm(x), mm(y))  # noqa: E731

    # ===================== A. DLC input + protection ======================
    s.box((3, 3), (41, 30), "ENTRADA DLC HONDA + PROTECCION", (200, 0, 0))
    J1 = s.part("J1", "Connector_Generic:Conn_01x03", "Cable DLC Honda", (7, 14), mirror="y",
                fields=F("civic_scanner:Cable_Wire_1x03_P3.96mm", None, None, None,
                         "1=K-Line 2=+12V 3=GND (Honda DLC)"),
                ref_at=(4.5, 10.2), val_at=(4.5, 11.3))
    s.pin_label(J1, "1", "KLINE", stub=1)
    F1 = s.part("F1", "Device:Polyfuse_Small", "mSMD050-60V", (14, 14), rot=90,
                fields=F("Fuse:Fuse_1812_4532Metric", "C70113", "mSMD050-60V", "Shenzhen JDT", "PTC 0.5A hold 60V"),
                ref_at=(14, 12.4), val_at=(14, 15.9))
    s.wire(J1.p("2"), F1.p("1"), net="VBAT_IN")
    s.label("VBAT_IN", U(10, 14), (1, 0)) if False else None
    s.wire(J1.p("3"), U(11, 15), U(11, 17), net="GND")
    s.power("GND", U(11, 17))
    D1 = s.part("D1", "Device:D_Zener_Small", "SMBJ18A", (18, 17), rot=270,
                fields=F("Diode_SMD:D_SMB", "C353379", "SMBJ18A", "Jingdao", "TVS 600W 18V unidirectional"),
                ref_at=(19.6, 16.6), val_at=(19.6, 17.7))
    D2 = s.part("D2", "Device:D_Schottky_Small", "SS34", (24, 14), rot=180,
                fields=F("Diode_SMD:D_SMA", "C8678", "SS34", "MDD", "Reverse polarity 40V 3A"),
                ref_at=(24, 12.4), val_at=(24, 15.9))
    s.wire(F1.p("2"), D2.p("2"), net="VBAT_F")
    s.label("VBAT_F", U(19.5, 14), (1, 0))
    s.wire(U(18, 14), D1.p("1"), net="VBAT_F")
    s.junction(U(18, 14))
    s.pin_power(D1, "2", "GND")
    C1 = s.part("C1", "Device:C_Small", "10uF 50V", (30, 17), fields=PART_DB["10u50"], ref_at=(30.8, 16.6), val_at=(30.8, 17.7))
    C2 = s.part("C2", "Device:C_Small", "100nF 50V", (35, 17), fields=PART_DB["100n50"], ref_at=(35.8, 16.6), val_at=(35.8, 17.7))
    s.wire(D2.p("1"), U(39, 14), net="VBAT")
    for c in (C1, C2):
        s.wire((c.p("1")[0], mm(14)), c.p("1"))
        s.junction((c.p("1")[0], mm(14)))
        s.pin_power(c, "2", "GND")
    s.power("+BATT", U(39, 14))
    s.power("PWR_FLAG", U(27, 14))
    s.junction(U(27, 14))
    s.text("Cable adaptador Honda 3p -> OBD2 cortado, soldado a J1:\n1 = K-Line (OBD2 p7)  2 = +12V (p16)  3 = GND (p4/5)", (4.5, 22), size=1.27, bold=False)
    s.text("PTC 0.5A + TVS 18V (descarga de carga)\n+ Schottky en serie (polaridad invertida)", (4.5, 25.5), size=1.27, bold=False)

    # ===================== B. Buck 12V -> 5V =============================
    s.box((43, 3), (100, 30), "BUCK 12V -> 5V  (AP63205, 2A)", (230, 110, 0))
    U1 = s.part("U1", "Regulator_Switching:AP63205WU", "AP63205WU-7", (64, 15),
                fields=F("Package_TO_SOT_SMD:TSOT-23-6", "C2071056", "AP63205WU-7", "Diodes Inc"),
                ref_at=(60, 9.6), val_at=(60, 10.7))
    s.power("+BATT", U(45, 14))
    C3 = s.part("C3", "Device:C_Small", "10uF 50V", (48, 17), fields=PART_DB["10u50"], ref_at=(48.8, 16.6), val_at=(48.8, 17.7))
    C4 = s.part("C4", "Device:C_Small", "100nF 50V", (53, 17), fields=PART_DB["100n50"], ref_at=(53.8, 16.6), val_at=(53.8, 17.7))
    s.wire(U(45, 14), U1.p("3"), net="VBAT")
    for c in (C3, C4):
        s.wire((c.p("1")[0], mm(14)), c.p("1"))
        s.junction((c.p("1")[0], mm(14)))
    s.wire(U1.p("2"), U(58, 16), U(58, 14))
    s.junction(U(58, 14))
    s.wire(U(48, 18), U(48, 20), U(64, 20), U1.p("4"))
    s.wire(U(53, 18), U(53, 20))
    s.junction(U(53, 20))
    s.junction(U(64, 20))
    s.wire(U(64, 20), U(64, 21))
    s.power("GND", U(64, 21))
    C5 = s.part("C5", "Device:C_Small", "100nF", (71, 15), rot=90, fields=PART_DB["100n50"],
                ref_at=(71, 11.9), val_at=(71, 12.9))
    s.wire(U1.p("6"), C5.p("1"))
    s.wire(C5.p("2"), U(72, 14))
    s.junction(U(72, 14))
    L1 = s.part("L1", "Device:L_Small", "4.7uH", (78, 14), rot=90,
                fields=F("Inductor_SMD:L_Changjiang_FNR4030S", "C167874", "FNR4030S4R7MT", "Changjiang", "4.7uH 2.3A"),
                ref_at=(78, 12.4), val_at=(78, 15.9))
    s.wire(U1.p("5"), L1.p("1"), net="SW")
    s.label("SW", U(74, 14), (1, 0))
    fx, fy = U1.p("1")
    s.wire((fx, fy), U(69, 16), U(69, 18), net="5V_BUCK")
    s.label("5V_BUCK", U(69, 18), (1, 0))
    C6 = s.part("C6", "Device:C_Small", "22uF 25V", (84, 17), fields=PART_DB["22u25"], ref_at=(84.8, 16.6), val_at=(84.8, 17.7))
    C7 = s.part("C7", "Device:C_Small", "22uF 25V", (89, 17), fields=PART_DB["22u25"], ref_at=(89.8, 16.6), val_at=(89.8, 17.7))
    D3 = s.part("D3", "Device:D_Schottky_Small", "B5819W", (94, 14), rot=180,
                fields=F("Diode_SMD:D_SOD-123", "C8598", "B5819W SL", "Changjiang"),
                ref_at=(94, 12.4), val_at=(94, 15.9))
    s.wire(L1.p("2"), D3.p("2"), net="5V_BUCK")
    s.label("5V_BUCK", U(80, 14), (1, 0))
    for c in (C6, C7):
        s.wire((c.p("1")[0], mm(14)), c.p("1"))
        s.junction((c.p("1")[0], mm(14)))
    s.wire(U(84, 18), U(84, 20), U(89, 20), U(89, 18))
    s.wire(U(86.5, 20), U(86.5, 21))
    s.junction(U(86.5, 20))
    s.power("GND", U(86.5, 21))
    s.wire(D3.p("1"), U(97, 14))
    s.power("+5V", U(97, 14))
    s.text("Valores del datasheet AP63205 (fig. 1): L 4.7uH,\nCin 10uF, Cout 2x22uF, BST 100nF. EN atado a VIN.", (44.5, 23), size=1.27, bold=False)
    s.text("D3 / D4: OR de diodos entre\n5V del buck y VBUS del USB", (76, 23), size=1.27, bold=False)

    # ===================== C. LDO 3.3V ====================================
    s.box((102, 3), (140, 30), "LDO 3.3V  (AP2112K, 600mA)", (180, 0, 160))
    U2 = s.part("U2", "Regulator_Linear:AP2112K-3.3", "AP2112K-3.3TRG1", (120, 15),
                fields=F("Package_TO_SOT_SMD:SOT-23-5", "C51118", "AP2112K-3.3TRG1", "Diodes Inc"),
                ref_at=(117, 9.6), val_at=(117, 10.7))
    s.power("+5V", U(105, 14))
    s.power("PWR_FLAG", U(108, 14))
    s.junction(U(108, 14))
    C8 = s.part("C8", "Device:C_Small", "10uF", (110, 17), fields=PART_DB["10u25"], ref_at=(110.8, 16.6), val_at=(110.8, 17.7))
    s.wire(U(105, 14), U2.p("1"))
    s.wire(U(110, 14), C8.p("1"))
    s.junction(U(110, 14))
    s.wire(U2.p("3"), U(114, 15), U(114, 14))
    s.junction(U(114, 14))
    s.pin_nc(U2, "4")
    C9 = s.part("C9", "Device:C_Small", "10uF", (130, 17), fields=PART_DB["10u25"], ref_at=(130.8, 16.6), val_at=(130.8, 17.7))
    s.wire(U(110, 18), U(110, 20), U(130, 20), U(130, 18))
    s.wire(U2.p("2"), U(120, 20))
    s.junction(U(120, 20))
    s.wire(U(120, 20), U(120, 21))
    s.power("GND", U(120, 21))
    s.power("PWR_FLAG", U(115, 20))
    s.junction(U(115, 20))
    s.wire(U2.p("5"), U(135, 14))
    s.wire(U(130, 14), C9.p("1"))
    s.junction(U(130, 14))
    s.power("+3V3", U(135, 14))
    s.text("Alimenta ESP32-C3 (pico TX ~350mA)\ny VCC del L9637D.", (103.5, 23), size=1.27, bold=False)

    # ===================== D. USB-C =======================================
    s.box((106, 33), (152, 75), "USB-C  (programacion / alimentacion)", (0, 150, 70))
    J2 = s.part("J2", "Connector:USB_C_Receptacle_USB2.0_16P", "USB-C", (114, 53),
                fields=F("Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12", "C165948", "TYPE-C-31-M-12", "Korean Hroparts"),
                ref_at=(110, 42.6), val_at=(110, 43.7))
    D4 = s.part("D4", "Device:D_Schottky_Small", "B5819W", (131, 47), rot=180,
                fields=F("Diode_SMD:D_SOD-123", "C8598", "B5819W SL", "Changjiang"),
                ref_at=(131, 44.4), val_at=(131, 45.5))
    s.wire(J2.p("A4"), D4.p("2"))
    s.power("VBUS", U(121, 47))
    s.junction(U(121, 47))
    s.power("PWR_FLAG", U(124.5, 47))
    s.junction(U(124.5, 47))
    s.wire(D4.p("1"), U(134, 47))
    s.power("+5V", U(134, 47))
    R4 = s.part("R4", "Device:R_Small_US", "5.1k", (139, 50), fields=PART_DB["5.1k"], ref_at=(139.8, 49.6), val_at=(139.8, 50.7))
    R5 = s.part("R5", "Device:R_Small_US", "5.1k", (134, 51), fields=PART_DB["5.1k"], ref_at=(134.8, 50.6), val_at=(134.8, 51.7))
    s.wire(J2.p("A5"), R4.p("1"), net="CC1")
    s.wire(J2.p("B5"), R5.p("1"), net="CC2")
    s.label("CC1", U(123, 49), (1, 0))
    s.label("CC2", U(123, 50), (1, 0))
    s.pin_power(R4, "2", "GND")
    s.pin_power(R5, "2", "GND")
    s.wire(J2.p("A7"), U(121, 52), U(121, 53), J2.p("B7"), net="USB_DN")
    s.wire(U(121, 52), U(124, 52), net="USB_DN")
    s.junction(U(121, 52))
    s.label("USB_DN", U(124, 52), (1, 0))
    s.wire(J2.p("A6"), U(121, 54), U(121, 55), J2.p("B6"), net="USB_DP")
    s.wire(U(121, 55), U(124, 55), net="USB_DP")
    s.junction(U(121, 55))
    s.label("USB_DP", U(124, 55), (1, 0))
    s.pin_nc(J2, "A8")
    s.pin_nc(J2, "B8")
    s.wire(J2.p("SH"), U(111, 64), U(114, 64))
    s.wire(J2.p("A1"), U(114, 64))
    s.junction(U(114, 64))
    s.wire(U(114, 64), U(114, 65))
    s.power("GND", U(114, 65))
    U4 = s.part("U4", "Power_Protection:USBLC6-2SC6", "USBLC6-2SC6", (140, 62),
                fields=F("Package_TO_SOT_SMD:SOT-23-6", "C2687116", "USBLC6-2SC6", "UMW"),
                ref_at=(141.5, 58.3), val_at=(141.5, 67.6))
    # I/O1 = D-, I/O2 = D+ (pin swap for a clean layout; the TVS lines are symmetric)
    s.pin_label(U4, "1", "USB_DN")
    s.pin_label(U4, "3", "USB_DP")
    s.pin_label(U4, "6", "USB_DN")
    s.pin_label(U4, "4", "USB_DP")
    s.pin_power(U4, "5", "VBUS")
    s.pin_power(U4, "2", "GND")
    s.text("CC1/CC2 5.1k: sink USB 2.0.  ESD en D+/D-/VBUS (U4).", (107.5, 70.5), size=1.27, bold=False)

    # ===================== E. ESP32-C3 ====================================
    s.box((3, 33), (62, 75), "MCU  ESP32-C3-WROOM-02  (BLE)", (90, 60, 160))
    U3 = s.part("U3", "RF_Module:ESP32-C3-WROOM-02", "ESP32-C3-WROOM-02-N4", (36, 58),
                fields=F("civic_scanner:ESP32-C3-WROOM-02_JLC", "C2934560", "ESP32-C3-WROOM-02-N4", "Espressif"),
                ref_at=(37.2, 67.6), val_at=(37.2, 68.7))
    for pn, net in (("3", "K_RX"), ("4", "K_TX"), ("7", "LED_STAT"),
                    ("13", "USB_DN"), ("14", "USB_DP")):
        s.pin_label(U3, pn, net, stub=2)
    for pn in ("18", "17", "16", "15", "10", "5", "6"):
        s.pin_nc(U3, pn)
    s.pin_label(U3, "11", "U0RXD", stub=2)
    s.pin_label(U3, "12", "U0TXD", stub=2)
    s.pin_power(U3, "9", "GND")
    x33, y33 = U3.p("1")
    s.wire((x33, y33), (x33, mm(45)), U(47, 45))
    s.power("+3V3", (x33, mm(45)))
    C10 = s.part("C10", "Device:C_Small", "22uF", (42, 46), fields=PART_DB["22u16"], ref_at=(42.8, 45.6), val_at=(42.8, 46.7))
    C11 = s.part("C11", "Device:C_Small", "100nF", (47, 46), fields=PART_DB["100n50"], ref_at=(47.8, 45.6), val_at=(47.8, 46.7))
    s.junction(U(42, 45))
    for c in (C10, C11):
        s.pin_power(c, "2", "GND")
    # EN: RC + RESET button
    xen, yen = U3.p("2")
    s.wire((xen, yen), (mm(10), yen), net="EN")
    s.label("EN", (mm(23), yen), (1, 0))
    R1 = s.part("R1", "Device:R_Small_US", "10k", (19, 50), fields=PART_DB["10k"], ref_at=(19.8, 49.3), val_at=(19.8, 50.4))
    s.pin_power(R1, "1", "+3V3")
    s.junction((mm(19), yen))
    C12 = s.part("C12", "Device:C_Small", "1uF", (15, 53), fields=PART_DB["1u"], ref_at=(15.8, 52.6), val_at=(15.8, 53.7))
    s.wire((mm(15), yen), C12.p("1"))
    s.junction((mm(15), yen))
    s.pin_power(C12, "2", "GND")
    SW1 = s.part("SW1", "Switch:SW_Push", "RESET", (10, 54), rot=90,
                 fields=F("Button_Switch_SMD:SW_Push_1P1T_XKB_TS-1187A", "C318884", "TS-1187A-B-A-B", "XKB Connection"),
                 ref_at=(7.0, 53.4), val_at=(7.0, 54.6))
    s.wire((mm(10), yen), SW1.p("2"))
    s.pin_power(SW1, "1", "GND")
    # BOOT (IO9)
    xb, yb = U3.p("8")
    s.wire((xb, yb), (mm(10), yb), net="BOOT")
    s.label("BOOT", (mm(22), yb), (1, 0))
    R2 = s.part("R2", "Device:R_Small_US", "10k", (19, 61), fields=PART_DB["10k"], ref_at=(19.8, 60.3), val_at=(19.8, 61.4))
    s.pin_power(R2, "1", "+3V3")
    s.junction((mm(19), yb))
    SW2 = s.part("SW2", "Switch:SW_Push", "BOOT", (10, 65), rot=90,
                 fields=F("Button_Switch_SMD:SW_Push_1P1T_XKB_TS-1187A", "C318884", "TS-1187A-B-A-B", "XKB Connection"),
                 ref_at=(7.0, 64.4), val_at=(7.0, 65.6))
    s.wire((mm(10), yb), SW2.p("2"))
    s.pin_power(SW2, "1", "GND")
    s.text("IO9 = BOOT (pull-up 10k + boton a GND).  IO8 debe estar ALTO al arrancar (R3).\nIO2 libre (strap con pull-up interno).  USB nativo: IO18 = D-, IO19 = D+.\nEN: RC 10k / 1uF (rampa del LDO < 1 ms).",
           (4.5, 70.8), size=1.27, bold=False)

    # ===================== F. K-Line transceiver ==========================
    s.box((64, 33), (104, 75), "TRANSCEPTOR K-LINE  (L9637D, ISO 9141)", (0, 120, 220))
    U5 = s.part("U5", "civic_scanner:L9637D", "L9637D", (84, 51),
                fields=F("Package_SO:SOIC-8_3.9x4.9mm_P1.27mm", "C153038", "E-L9637D013TR", "STMicroelectronics"),
                ref_at=(85.2, 55.4), val_at=(85.2, 56.5))
    s.pin_label(U5, "1", "K_RX", stub=3)
    s.pin_label(U5, "4", "K_TX", stub=3)
    s.pin_nc(U5, "2")
    s.pin_power(U5, "5", "GND")
    xv, yv = U5.p("3")
    s.wire((xv, yv), (xv, mm(42)), U(73, 42))
    s.power("+3V3", U(73, 42))
    C13 = s.part("C13", "Device:C_Small", "100nF", (76, 43), fields=PART_DB["100n50"], ref_at=(76.8, 42.6), val_at=(76.8, 43.7))
    s.junction(U(76, 42))
    s.pin_power(C13, "2", "GND")
    xs, ys = U5.p("7")
    s.wire((xs, ys), (xs, mm(42)), U(98, 42))
    s.power("+BATT", U(88, 42))
    s.junction(U(88, 42))
    R6 = s.part("R6", "Device:R_Small_US", "1k 1W", (93, 43), fields=PART_DB["1k_2512"], ref_at=(93.8, 42.6), val_at=(93.8, 43.7))
    s.junction(U(93, 42))
    C14 = s.part("C14", "Device:C_Small", "100nF 50V", (98, 43), fields=PART_DB["100n50"], ref_at=(98.8, 42.6), val_at=(98.8, 43.7))
    s.pin_power(C14, "2", "GND")
    xk, yk = U5.p("6")
    s.wire((xk, yk), (mm(100), yk), net="KLINE")
    s.wire(R6.p("2"), (mm(93), yk), net="KLINE")
    s.junction((mm(93), yk))
    s.label("KLINE", (mm(100), yk), (1, 0))
    s.pin_power(U5, "8", "GND", stub=1)
    s.text("VCC = 3.3V: RX/TX con niveles del ESP32 (VCC 3-7V, VIH(TX) 2.5V).\nPull-up K 1k / 1W a VS (datasheet: RKO <= 5k).\nLI a GND (comparador L sin uso, LO sin conectar).",
           (65.5, 60.5), size=1.27, bold=False)

    # ===================== G. LEDs =========================================
    s.box((3, 77), (62, 100), "LEDs INDICADORES", (150, 120, 0))
    leds = [
        ("D5", "R7", "PWR", "LED_W", "1k", None, "pwr", 7),
        ("D6", "R8", "STATUS", "LED_Y", "1k", "LED_STAT", "sink", 16),
        ("D7", "R9", "K-LINE", "LED_R", "1k", "K_RX", "sink", 25),
    ]
    cname = {"LED_R": "rojo", "LED_Y": "amarillo", "LED_W": "blanco"}
    for dref, rref, name, col, rv, net, mode, x in leds:
        R = s.part(rref, "Device:R_Small_US", rv, (x, 83), fields=PART_DB[rv], ref_at=(x + 0.8, 82.6), val_at=(x + 0.8, 83.7))
        D = s.part(dref, "Device:LED_Small", name, (x, 88), rot=90, fields=PART_DB[col],
                   ref_at=(x + 2.6, 87.6), val_at=(x + 2.6, 88.7))
        s.wire(R.p("2"), D.p("2"))
        if mode == "pwr":
            s.pin_power(R, "1", "+5V")          # white LED (Vf ~3 V) needs more than 3.3 V
        elif mode == "sink":
            s.pin_power(R, "1", "+3V3")
        else:
            rx, ry = R.p("1")
            s.wire((rx, ry), (rx, ry - G), (rx - 1 * G, ry - G), net=net)
            s.label(net, (rx - 1 * G, ry - G), (-1, 0))
        if mode == "sink":
            kx, ky = D.p("1")
            s.wire((kx, ky), (kx, ky + G), (kx - G, ky + G), net=net)
            s.label(net, (kx - G, ky + G), (-1, 0))
        else:
            s.pin_power(D, "1", "GND")
        s.text(cname[col], (x + 1.3, 89.6), size=1.27, bold=False)
    R3 = s.part("R3", "Device:R_Small_US", "10k", (53, 83), fields=PART_DB["10k"], ref_at=(53.8, 82.6), val_at=(53.8, 83.7))
    s.pin_power(R3, "1", "+3V3")
    r3x, r3y = R3.p("2")
    s.wire((r3x, r3y), (r3x, r3y + G), (r3x - G, r3y + G), net="LED_STAT")
    s.label("LED_STAT", (r3x - G, r3y + G), (-1, 0))
    s.text("PWR: +5V presente.  STATUS (IO8): activo en bajo; R3 asegura IO8 alto al arrancar.\nK-LINE: parpadea con el trafico del bus (salida RX del L9637D), sin firmware.  IO6/IO7 libres.",
           (4.5, 95.5), size=1.27, bold=False)

    # ===================== H. Test points / mech ===========================
    s.box((64, 77), (114, 100), "PUNTOS DE PRUEBA / MECANICO", (90, 90, 90))
    tps = [("TP1", "KLINE"), ("TP2", "U0TXD"), ("TP3", "U0RXD"), ("TP4", "+BATT"), ("TP5", "+5V"), ("TP6", "+3V3"), ("TP7", "GND")]
    for i, (ref, net) in enumerate(tps):
        x = 67 + i * 6.5
        pwr = net.startswith("+")
        tp = s.part(ref, "Connector:TestPoint", net, (x, 86 if pwr else 84), rot=180 if pwr else 0,
                    fields=F("TestPoint:TestPoint_Pad_D1.5mm", desc="Test pad"),
                    ref_at=(x + 1.2, 87.2 if pwr else 82.6), val_at=(x + 1.2, 83.4), hide_value=True)
        tp.in_bom = False
    s.part("H1", "Mechanical:MountingHole", "M3", (70, 94), fields=F("MountingHole:MountingHole_3.2mm_M3"),
           ref_at=(71.5, 93.4), val_at=(71.5, 94.6), in_bom=False)
    s.part("H2", "Mechanical:MountingHole", "M3", (80, 94), fields=F("MountingHole:MountingHole_3.2mm_M3"),
           ref_at=(81.5, 93.4), val_at=(81.5, 94.6), in_bom=False)
    return s


def connect_testpoints(s):
    for p in s.parts:
        if not p.ref.startswith("TP"):
            continue
        net = p.value
        x, y = p.p("1")
        if net == "GND":
            s.power("GND", (x, y))
        elif net.startswith("+"):
            s.power(net, (x, y))          # TP rotated 180: pin on top, arrow up
        else:
            s.wire((x, y), (x, round(y + G, 4)), net=net)
            s.label(net, (x, round(y + G, 4)), (1, 0))


# ----------------------------------------------------------------------------
def prop(name, value, at, hide=False, justify=None, size=1.27, angle=0):
    eff = S("effects", font(size))
    if justify:
        eff.append([Sym("justify")] + [Sym(j) for j in justify])
    node = S("property", name, value, S("at", at[0], at[1], angle))
    if hide:
        node.append(S("hide", Sym("yes")))
    node.append(eff)
    return node


def emit_symbol(p):
    rot = p.rot
    at = S("at", p.X, p.Y, rot)
    node = S("symbol", S("lib_id", p.lib_id), at)
    if p.mirror:
        node.append(S("mirror", Sym(p.mirror)))
    node += [S("unit", 1), S("exclude_from_sim", Sym("no")), S("in_bom", Sym("yes" if p.in_bom else "no")),
             S("on_board", Sym("yes" if p.on_board else "no")), S("dnp", Sym("no")), S("uuid", p.uuid)]
    is_power = getattr(p, "is_power", False)
    # field text angle: keep text horizontal regardless of symbol rotation
    fang = 0 if rot in (0, 180) else 90
    if is_power:
        if p.value == "GND":
            vat = (p.X, p.Y + 3.81)
        elif p.value == "PWR_FLAG":
            vat = (p.X, p.Y - 3.4)
        else:
            vat = (p.X, p.Y - 3.56)
        node.append(prop("Reference", p.ref, (p.X, p.Y), hide=True))
        node.append(prop("Value", p.value, vat))
    else:
        rat = (mm(p.ref_at[0]), mm(p.ref_at[1])) if p.ref_at else (p.X + 2.54, p.Y - 1.27)
        vat = (mm(p.val_at[0]), mm(p.val_at[1])) if p.val_at else (p.X + 2.54, p.Y + 1.27)
        just = ["left"] if rot in (0, 180) or p.rot == 270 else None
        if just and ((p.mirror == "y") != (rot == 180)):
            just = ["right"]
        node.append(prop("Reference", p.ref, rat, justify=just, angle=fang))
        node.append(prop("Value", p.value, vat, justify=just, hide=p.hide_value, angle=fang))
    fields = dict(p.fields)
    node.append(prop("Footprint", fields.pop("Footprint", ""), (p.X, p.Y), hide=True))
    node.append(prop("Datasheet", fields.pop("Datasheet", ""), (p.X, p.Y), hide=True))
    node.append(prop("Description", fields.pop("Description", ""), (p.X, p.Y), hide=True))
    for k, v in fields.items():
        node.append(prop(k, v, (p.X, p.Y), hide=True))
    for num in p.pins:
        node.append(S("pin", num, S("uuid", uid("pin:%s:%s" % (p.ref, num)))))
    node.append(S("instances", S("project", PROJECT,
                                 S("path", "/" + ROOT_UUID, S("reference", p.ref), S("unit", 1)))))
    return node


def stroke(color=None, width=0, typ="default"):
    st = S("stroke", S("width", width), S("type", Sym(typ)))
    if color:
        st.append(S("color", color[0], color[1], color[2], 1))
    return st


def net_color(net):
    if not net:
        return None
    key = NET_COLOR.get(net)
    return COL[key] if key else None


def emit_items(s):
    out = []
    n = 0
    for it in s.items:
        n += 1
        kind = it[0]
        if kind == "wire":
            _, a, b, net = it
            if a == b:
                continue
            out.append(S("wire", S("pts", S("xy", a[0], a[1]), S("xy", b[0], b[1])),
                         stroke(net_color(net)), S("uuid", uid("w%d:%s:%s" % (n, a, b)))))
        elif kind == "junction":
            _, pt = it
            out.append(S("junction", S("at", pt[0], pt[1]), S("diameter", 0), S("color", 0, 0, 0, 0),
                         S("uuid", uid("j%d:%s" % (n, pt)))))
        elif kind == "nc":
            _, pt = it
            out.append(S("no_connect", S("at", pt[0], pt[1]), S("uuid", uid("nc%d:%s" % (n, pt)))))
        elif kind == "label":
            _, name, pt, d = it
            ang = {(1, 0): 0, (0, -1): 90, (-1, 0): 180, (0, 1): 270}[tuple(d)]
            just = {0: ["left", "bottom"], 90: ["left", "bottom"], 180: ["right", "bottom"], 270: ["right", "bottom"]}[ang]
            eff = S("effects", font(1.27), [Sym("justify")] + [Sym(j) for j in just])
            lab = S("label", name, S("at", pt[0], pt[1], ang), S("fields_autoplaced", Sym("yes")), eff,
                    S("uuid", uid("l%d:%s:%s" % (n, name, pt))))
            out.append(lab)
        elif kind == "text":
            _, txt, at, size, bold, color = it
            f = font(size, bold)
            if color:
                f.append(S("color", color[0], color[1], color[2], 1))
            out.append(S("text", txt, S("exclude_from_sim", Sym("yes")), S("at", at[0], at[1], 0),
                         S("effects", f, S("justify", Sym("left"), Sym("top"))), S("uuid", uid("t%d" % n))))
        elif kind == "box":
            _, a, b, color = it
            out.append(S("rectangle", S("start", a[0], a[1]), S("end", b[0], b[1]),
                         stroke(color, 0.3, "dash"), S("fill", S("type", Sym("none"))), S("uuid", uid("b%d" % n))))
    return out


def main():
    s = build()
    connect_testpoints(s)
    lib_syms = [symlib.load_symbol(l) for l in sorted(s.lib_ids)]
    tree = S("kicad_sch", S("version", 20250610), S("generator", "eeschema"), S("generator_version", "10.0"),
             S("uuid", ROOT_UUID), S("paper", "A3"),
             S("title_block", S("title", "Escaner Honda Civic K-Line (ESP32-C3 + L9637D)"), S("date", "2026-11-04"),
               S("rev", "1.0"), S("company", "honda-civic-scanner"),
               S("comment", 1, "Generado por hardware/scripts/gen_sch.py")),
             [Sym("lib_symbols")] + lib_syms)
    tree += emit_items(s)
    for p in s.parts:
        tree.append(emit_symbol(p))
    tree.append(S("sheet_instances", S("path", "/", S("page", "1"))))
    tree.append(S("embedded_fonts", Sym("no")))
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write(dumps(tree) + "\n")
    print("wrote", os.path.abspath(OUT), "parts:", len([p for p in s.parts if not getattr(p, "is_power", False)]))


if __name__ == "__main__":
    main()
