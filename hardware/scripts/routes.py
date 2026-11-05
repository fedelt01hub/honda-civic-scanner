"""Routing, zones and silkscreen for the Civic scanner board (called from gen_pcb.py)."""
import pcbnew

LAYERS = {"F": pcbnew.F_Cu, "B": pcbnew.B_Cu}


def MM(v):
    return pcbnew.FromMM(float(v))


def V(x, y):
    return pcbnew.VECTOR2I(MM(x), MM(y))


class Router:
    def __init__(self, board, fps, netinfo):
        self.b, self.fps, self.nets = board, fps, netinfo

    def pad(self, ref, num, idx=0):
        pads = [p for p in self.fps[ref].Pads() if p.GetNumber() == str(num)]
        p = pads[idx]
        pos = p.GetPosition()
        return (pcbnew.ToMM(pos.x), pcbnew.ToMM(pos.y))

    def padnet(self, ref, num):
        return [p for p in self.fps[ref].Pads() if p.GetNumber() == str(num)][0].GetNetname()

    def _pt(self, p):
        if isinstance(p[0], str):
            return self.pad(*p)
        return p

    def track(self, net, pts, w=0.25, layer="F"):
        pts = [self._pt(p) for p in pts]
        for a, b in zip(pts, pts[1:]):
            if a == b:
                continue
            t = pcbnew.PCB_TRACK(self.b)
            t.SetStart(V(*a))
            t.SetEnd(V(*b))
            t.SetWidth(MM(w))
            t.SetLayer(LAYERS[layer])
            t.SetNet(self.nets[net])
            self.b.Add(t)

    def via(self, net, p, d=0.6, drill=0.3):
        p = self._pt(p)
        v = pcbnew.PCB_VIA(self.b)
        v.SetPosition(V(*p))
        v.SetWidth(MM(d))
        v.SetDrill(MM(drill))
        v.SetNet(self.nets[net])
        self.b.Add(v)

    def zone(self, net, layer, poly, priority=0, clearance=0.25, min_w=0.25, thermal=0.5, spoke=0.5, solid=False):
        z = pcbnew.ZONE(self.b)
        z.SetLayer(LAYERS[layer])
        if net:
            z.SetNet(self.nets[net])
        ol = z.Outline()
        ol.NewOutline()
        for x, y in poly:
            ol.Append(MM(x), MM(y))
        z.SetAssignedPriority(priority)
        z.SetLocalClearance(MM(clearance))
        z.SetMinThickness(MM(min_w))
        z.SetThermalReliefGap(MM(thermal))
        z.SetThermalReliefSpokeWidth(MM(spoke))
        z.SetPadConnection(pcbnew.ZONE_CONNECTION_FULL if solid else pcbnew.ZONE_CONNECTION_THT_THERMAL)
        z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
        self.b.Add(z)
        return z

    def keepout(self, poly, layers=("F", "B")):
        for lay in layers:
            z = pcbnew.ZONE(self.b)
            z.SetLayer(LAYERS[lay])
            z.SetIsRuleArea(True)
            z.SetDoNotAllowTracks(True)
            z.SetDoNotAllowVias(True)
            z.SetDoNotAllowZoneFills(True)
            z.SetDoNotAllowPads(False)
            z.SetDoNotAllowFootprints(False)
            ol = z.Outline()
            ol.NewOutline()
            for x, y in poly:
                ol.Append(MM(x), MM(y))
            self.b.Add(z)

    def text(self, s, p, size=1.0, layer=pcbnew.F_SilkS, bold=False, justify=0, thick=None, rot=0):
        t = pcbnew.PCB_TEXT(self.b)
        t.SetText(s)
        t.SetPosition(V(*p))
        t.SetLayer(layer)
        t.SetTextSize(V(size, size))
        t.SetTextThickness(MM(thick or size * 0.15))
        t.SetBold(bold)
        if justify < 0:
            t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_LEFT)
        elif justify > 0:
            t.SetHorizJustify(pcbnew.GR_TEXT_H_ALIGN_RIGHT)
        if rot:
            t.SetTextAngleDegrees(rot)
        if layer in (pcbnew.B_SilkS,):
            t.SetMirrored(True)
        self.b.Add(t)


def apply(board, fps, netinfo, W, H):
    r = Router(board, fps, netinfo)
    route(r, W, H)
    zones(r, W, H)
    silk(r, W, H)
    filler = pcbnew.ZONE_FILLER(board)
    filler.Fill(board.Zones())


LABELS = [
    ("K", (2.9, 9.0)), ("12V", (2.8, 12.96)), ("GND", (2.8, 16.92)),
    ("RST", (28.7, 3.0)), ("BOOT", (25.4, 21.2)),
    ("PWR", (56.6, 26.4)), ("STAT", (56.4, 29.2)), ("K", (57.4, 32.0)),
    ("USB-C", (63.3, 21.6)), ("RX", (51.4, 25.0)), ("TX", (54.6, 25.0)),
]

USB_W = 0.2

POWER = [("Net-(J1-Pin_2)", 0.6), ("/VBAT_F", 0.6), ("/SW", 0.6), ("/5V_BUCK", 0.6), ("+BATT", 0.6),
         ("+5V", 0.6), ("VBUS", 0.4), ("/KLINE", 0.4), ("+3V3", 0.4)]


def route_usb(r):
    """Hand-routed USB-C area.

    Pads (west column of J2, top->bottom): GND, VBUS, CC2, SBU1, D+(B6), D-(A7), D+(A6), D-(B7),
    CC1, SBU2, VBUS, GND.  Outer data pads go straight to the module, the inner ones are joined
    on F.Cu (D+) and through a short B.Cu jumper (D-).  The ESD array taps both lines.
    """
    dp, dn = "/USB_DP", "/USB_DN"
    W_ = USB_W
    # D+ main: module pin14 -> B6
    r.track(dp, [("U3", "14"), (52.8, 13.1), (53.2, 12.7), (56.85, 12.7), (57.25, 13.1), ("J2", "B6")], w=W_)
    # D+ inner pad A6 joins the main line
    r.track(dp, [("J2", "A6"), (55.6, 14.1), (55.2, 13.7), (55.2, 13.1), (54.8, 12.7)], w=W_)
    # D+ to the ESD array (I/O2: pins 3 and 4, linked under the body)
    r.track(dp, [(53.36, 12.7), ("U4", "3")], w=W_)
    r.track(dp, [("U4", "3"), ("U4", "4")], w=W_)
    # D- main: module pin13 -> B7
    r.track(dn, [("U3", "13"), ("J2", "B7")], w=W_)
    # D- inner pad A7 -> via -> B.Cu jumper to the main line
    r.track(dn, [("J2", "A7"), (56.9, 13.6), (56.4, 13.45)], w=W_)
    r.via(dn, (56.4, 13.45))
    r.track(dn, [(56.4, 13.45), (54.2, 14.6)], w=W_, layer="B")
    r.via(dn, (54.2, 14.6))
    # D- to the ESD array (I/O1: pins 1 and 6) through B.Cu
    r.track(dn, [(56.4, 13.45), (55.6, 12.65), (55.6, 8.4)], w=W_, layer="B")
    r.via(dn, (55.6, 8.4))
    r.track(dn, [(55.6, 8.4), ("U4", "6")], w=W_)
    r.track(dn, [("U4", "1"), ("U4", "6")], w=W_)
    # ESD ground: small via under the package centre
    r.track("GND", [("U4", "2"), (54.3, 10.4)], w=0.3)
    r.via("GND", (54.3, 10.4), d=0.5, drill=0.3)
    # VBUS: both VBUS pads -> vias -> B.Cu link; upper via also feeds the ESD VBUS pin
    r.track("VBUS", [("J2", "A9"), (57.1, 11.15)], w=0.3)
    r.via("VBUS", (57.1, 11.15))
    r.track("VBUS", [("J2", "A4"), (57.1, 16.55)], w=0.3)
    r.via("VBUS", (57.1, 16.55))
    r.track("VBUS", [(57.1, 11.15), (57.1, 16.55)], w=0.3, layer="B")
    r.track("VBUS", [(57.1, 11.15), (57.1, 10.4), ("U4", "5")], w=0.25)
    # GND pads tied to the shield tabs (through-hole -> both pours)
    r.track("GND", [("J2", "A12"), (58.45, 10.6), (58.45, 9.8)], w=0.3)
    r.track("GND", [("J2", "A1"), (58.45, 17.1), (58.45, 17.9)], w=0.3)
    # CC1 -> R4
    r.track("/CC1", [("J2", "A5"), (56.3, 15.1), (55.6, 15.8), ("R4", "1")], w=W_)
    # CC2 -> via -> B.Cu north -> R5
    r.track("/CC2", [("J2", "B5"), (56.45, 12.1)], w=W_)
    r.via("/CC2", (56.45, 12.1))
    r.track("/CC2", [(56.45, 12.1), (56.45, 7.9)], w=W_, layer="B")
    r.via("/CC2", (56.45, 7.9))
    r.track("/CC2", [(56.45, 7.9), (57.0, 7.6), ("R5", "1")], w=W_)

def route(r, W, H):
    import autoroute
    route_usb(r)
    ar = autoroute.AutoRouter(r, W, H)
    g = ar.grid
    F, B = autoroute.F, autoroute.B
    # no copper under the module body (F) nor under the USB receptacle body (F)
    g.block_rect([F], 35.3, 0.0, 50.7, 20.2)
    g.block_rect([F], 59.3, 9.0, W, 18.7)
    g.penalize_rect(B, 34.0, 0.0, 52.0, 20.0, 4.0)
    for net, w in POWER:
        ar.route_net(net, w=w, via_cost=40.0)
    skip = {"GND", "/USB_DP", "/USB_DN"} | {n for n, _ in POWER}
    nets = sorted({p.GetNetname() for fp in r.b.GetFootprints() for p in fp.Pads()
                   if p.GetNetname() and not p.GetNetname().startswith("unconnected") and p.GetNetname() not in skip})
    def span(net):
        pts = [(pcbnew.ToMM(p.GetPosition().x), pcbnew.ToMM(p.GetPosition().y))
               for fp in r.b.GetFootprints() for p in fp.Pads() if p.GetNetname() == net]
        xs, ys = [q[0] for q in pts], [q[1] for q in pts]
        return (max(xs) - min(xs)) + (max(ys) - min(ys))
    for net in sorted(nets, key=span):
        ar.route_net(net, w=0.25, via_cost=25.0)
    # ground: a stitching via at every SMD ground pad
    for fp in r.b.GetFootprints():
        ref = fp.GetReference()
        for p in fp.Pads():
            if p.GetNetname() == "GND" and p.GetAttribute() == pcbnew.PAD_ATTRIB_SMD:
                if ref == "U3" and p.GetNumber() == "19":
                    continue
                ar.gnd_via_near(ref, p.GetNumber())
    # stitching grid
    import numpy as np
    code = g.code("GND")
    for y in np.arange(2.0, H - 1.0, 3.0):
        for x in np.arange(2.0, W - 1.0, 3.0):
            j, i = int(round(y / autoroute.RES)), int(round(x / autoroute.RES))
            ok = True
            for c in (0.6,):
                for lay in (F, B):
                    sl = g.own[c][lay][max(j - 4, 0):j + 5, max(i - 4, 0):i + 5]
                    if not np.all((sl == 0) | (sl == code)):
                        ok = False
            near_label = any(abs(x - lx) < len(s) * 0.33 + 0.8 and abs(y - ly) < 1.0 for s, (lx, ly) in LABELS)
            if ok and not near_label and ar.via_ok(float(x), float(y)):
                r.via("GND", (round(x, 2), round(y, 2)))
                ar.vias.append((float(x), float(y)))
                for lay in (F, B):
                    g.paint_seg(lay, (x, y), (x, y), 0.3, code)
    if ar.failed:
        print("ROUTER FAILED:", ar.failed)


def zones(r, W, H):
    m = 0.0
    poly = [(m, m), (W - m, m), (W - m, H - m), (m, H - m)]
    r.zone("GND", "F", poly, priority=0)
    r.zone("GND", "B", poly, priority=0)


def _bbox_text(cx, cy, s, h):
    w = max(len(s), 1) * h * 0.72 + 0.1
    return (cx - w / 2, cy - h / 2 - 0.05, cx + w / 2, cy + h / 2 + 0.05)


def _hit(a, b, m=0.0):
    return not (a[2] + m <= b[0] or b[2] + m <= a[0] or a[3] + m <= b[1] or b[3] + m <= a[1])


def silk(r, W, H):
    b = r.b
    T = pcbnew.ToMM
    pads, crt = [], {}
    for fp in b.GetFootprints():
        for p in fp.Pads():
            bb = p.GetBoundingBox()
            pads.append((T(bb.GetLeft()), T(bb.GetTop()), T(bb.GetRight()), T(bb.GetBottom())))
        c = fp.GetCourtyard(pcbnew.F_CrtYd)
        cb = c.BBox() if c.OutlineCount() else fp.GetBoundingBox(False)
        crt[fp.GetReference()] = (T(cb.GetLeft()), T(cb.GetTop()), T(cb.GetRight()), T(cb.GetBottom()))
    for v in b.GetTracks():
        if v.GetClass() == "PCB_VIA":
            x, y = T(v.GetPosition().x), T(v.GetPosition().y)
            pads.append((x - 0.3, y - 0.3, x + 0.3, y + 0.3))
    placed = []

    def free(bx, own=None, check_crt=True):
        if bx[0] < 0.6 or bx[1] < 0.6 or bx[2] > W - 0.6 or bx[3] > H - 0.6:
            return False
        if any(_hit(bx, p, 0.15) for p in pads):
            return False
        if any(_hit(bx, q, 0.1) for q in placed):
            return False
        if check_crt and any(_hit(bx, c, -0.05) for k, c in crt.items() if k != own):
            return False
        return True

    # fixed labels first
    labels = LABELS
    # LED / LED-resistor references are replaced by the function labels above
    for fp in b.GetFootprints():
        if fp.GetReference() in ("D5", "D6", "D7", "R7", "R8", "R9", "J1"):
            fp.Reference().SetVisible(False)
    for s, (x, y) in labels:
        bx = _bbox_text(x, y, s, 0.9)
        if free(bx, check_crt=False):
            r.text(s, (x, y), size=0.9, thick=0.15)
            placed.append(bx)
        else:
            print("label skipped:", s)
    hidden = []
    for fp in sorted(b.GetFootprints(), key=lambda f: f.GetReference()):
        ref = fp.GetReference()
        f = fp.Reference()
        if not f.IsVisible():
            continue
        h = 0.8
        f.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(h), pcbnew.FromMM(h)))
        f.SetTextThickness(pcbnew.FromMM(0.13))
        f.SetTextAngleDegrees(0)
        f.SetKeepUpright(True)
        c = crt[ref]
        cx, cy = (c[0] + c[2]) / 2, (c[1] + c[3]) / 2
        cands = []
        for off in (0.55, 0.9, 1.3):
            cands += [(cx, c[1] - off), (cx, c[3] + off), (c[0] - off - len(ref) * 0.3, cy), (c[2] + off + len(ref) * 0.3, cy)]
        cands += [(cx, cy)]  # inside the courtyard (e.g. large parts)
        done = False
        for (x, y) in cands:
            bx = _bbox_text(x, y, ref, h)
            if free(bx, own=ref, check_crt=(x, y) != (cx, cy)):
                f.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(x), pcbnew.FromMM(y)))
                placed.append(bx)
                done = True
                break
        if not done:
            f.SetVisible(False)
            hidden.append(ref)
    if hidden:
        print("refs hidden (no room):", hidden)
    # bottom silkscreen: title + pinout
    r.text("CIVIC K-LINE BLE SCANNER  v1.0", (W / 2, H - 6.0), size=1.4, layer=pcbnew.B_SilkS, thick=0.22)
    r.text("Cable Honda->OBD2 cortado: K=OBD2 p7  12V=p16  GND=p4/5", (W / 2, H - 3.8), size=0.9,
           layer=pcbnew.B_SilkS, thick=0.15)
