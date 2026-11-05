"""Small 2-layer grid router (A*) used to route the low-speed nets of the scanner board.

Critical nets (input power chain, buck loop, USB pair) are hand-routed in routes.py first;
this router then connects the remaining nets one by one, respecting clearances to
everything already on the board, keep-outs, the board edge and per-net track widths.
"""
import heapq
import math

import numpy as np
import pcbnew

RES = 0.1                  # grid resolution (mm)
CLR = 0.2                  # copper clearance (mm)
MARGIN = 0.06              # quantisation safety margin
EDGE_CLR = 0.35
CLASSES = (0.25, 0.4, 0.6)  # supported track widths (0.6 class also used for vias)
VIA_D, VIA_DRILL = 0.6, 0.3
F, B = 0, 1
KLAYER = {F: pcbnew.F_Cu, B: pcbnew.B_Cu}


def mm(v):
    return pcbnew.ToMM(v)


class Grid:
    def __init__(self, board, W, H):
        self.b = board
        self.W, self.H = W, H
        self.nx, self.ny = int(round(W / RES)) + 1, int(round(H / RES)) + 1
        self.own = {c: np.zeros((2, self.ny, self.nx), dtype=np.int32) for c in CLASSES}
        self.cost = np.ones((2, self.ny, self.nx), dtype=np.float32)
        self.cost[B] *= 2.0
        self.netcode = {}
        self.names = {}
        xs = np.arange(self.nx) * RES
        ys = np.arange(self.ny) * RES
        self.X, self.Y = np.meshgrid(xs, ys)
        for c in CLASSES:
            m = EDGE_CLR + c / 2
            o = self.own[c]
            o[:, self.Y[:, 0] < m, :] = -1
            o[:, self.Y[:, 0] > H - m, :] = -1
            o[:, :, self.X[0] < m] = -1
            o[:, :, self.X[0] > W - m] = -1
            # rounded corners (r = 2 mm): block outside the arc
            r = 2.0
            for cx, cy in ((r, r), (W - r, r), (r, H - r), (W - r, H - r)):
                outside = ((self.X - cx) ** 2 + (self.Y - cy) ** 2 > (r - m) ** 2)
                corner = ((np.abs(self.X - cx) <= r) & (np.abs(self.Y - cy) <= r) &
                          (np.sign(self.X - cx) == np.sign(cx - W / 2)) & (np.sign(self.Y - cy) == np.sign(cy - H / 2)))
                o[:, outside & corner] = -1

    # --- net codes ---------------------------------------------------------
    def code(self, name):
        if not name:
            return -1
        if name not in self.netcode:
            self.netcode[name] = len(self.netcode) + 1
            self.names[self.netcode[name]] = name
        return self.netcode[name]

    # --- painting ------------------------------------------------------------
    def _win(self, x0, y0, x1, y1):
        i0 = max(int(math.floor(x0 / RES)), 0)
        i1 = min(int(math.ceil(x1 / RES)), self.nx - 1)
        j0 = max(int(math.floor(y0 / RES)), 0)
        j1 = min(int(math.ceil(y1 / RES)), self.ny - 1)
        return slice(j0, j1 + 1), slice(i0, i1 + 1)

    def _apply(self, c, layer, sl, mask, code):
        o = self.own[c][layer][sl]
        if code < 0:
            o[mask] = -1
            return
        cur = o[mask]
        cur = np.where((cur == 0) | (cur == code), code, -1)
        o[mask] = cur

    def paint_seg(self, layer, a, b, hw, code, extra=None):
        """Segment a-b with half width hw, inflated per class."""
        for c in CLASSES:
            d = hw + CLR + c / 2 + MARGIN if extra is None else hw + extra + c / 2
            sl = self._win(min(a[0], b[0]) - d, min(a[1], b[1]) - d, max(a[0], b[0]) + d, max(a[1], b[1]) + d)
            X, Y = self.X[sl], self.Y[sl]
            dx, dy = b[0] - a[0], b[1] - a[1]
            L2 = dx * dx + dy * dy
            if L2 == 0:
                dist2 = (X - a[0]) ** 2 + (Y - a[1]) ** 2
            else:
                t = np.clip(((X - a[0]) * dx + (Y - a[1]) * dy) / L2, 0, 1)
                dist2 = (X - a[0] - t * dx) ** 2 + (Y - a[1] - t * dy) ** 2
            self._apply(c, layer, sl, dist2 <= d * d, code)

    def paint_rect(self, layer, cx, cy, sx, sy, ang, code, extra=None):
        for c in CLASSES:
            d = CLR + c / 2 + MARGIN if extra is None else extra + c / 2
            R = math.hypot(sx, sy) / 2 + d
            sl = self._win(cx - R, cy - R, cx + R, cy + R)
            X, Y = self.X[sl] - cx, self.Y[sl] - cy
            ca, sa = math.cos(math.radians(ang)), math.sin(math.radians(ang))
            lx = X * ca - Y * sa
            ly = X * sa + Y * ca
            mask = (np.abs(lx) <= sx / 2 + d) & (np.abs(ly) <= sy / 2 + d)
            self._apply(c, layer, sl, mask, code)

    def paint_poly_block(self, layers, pts, extra=0.0):
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        for c in CLASSES:
            d = extra + c / 2
            sl = self._win(min(xs) - d, min(ys) - d, max(xs) + d, max(ys) + d)
            X, Y = self.X[sl], self.Y[sl]
            mask = (X >= min(xs) - d) & (X <= max(xs) + d) & (Y >= min(ys) - d) & (Y <= max(ys) + d)
            for lay in layers:
                self.own[c][lay][sl][mask] = -1

    def add_board_items(self):
        for fp in self.b.GetFootprints():
            for pad in fp.Pads():
                code = self.code(pad.GetNetname()) if pad.GetNetname() else -1
                pos = pad.GetPosition()
                cx, cy = mm(pos.x), mm(pos.y)
                sz = pad.GetSize(pcbnew.F_Cu) if hasattr(pad, "GetSize") else pad.GetSize()
                sx, sy = mm(sz.x), mm(sz.y)
                ang = pad.GetOrientation().AsDegrees()
                layers = []
                if pad.IsOnLayer(pcbnew.F_Cu):
                    layers.append(F)
                if pad.IsOnLayer(pcbnew.B_Cu):
                    layers.append(B)
                if pad.GetAttribute() in (pcbnew.PAD_ATTRIB_PTH, pcbnew.PAD_ATTRIB_NPTH):
                    layers = [F, B]
                    if pad.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH:
                        dr = pad.GetDrillSize()
                        sx, sy = max(sx, mm(dr.x)), max(sy, mm(dr.y))
                for lay in layers:
                    self.paint_rect(lay, cx, cy, sx, sy, ang, code)
            for z in fp.Zones():
                if z.GetIsRuleArea():
                    bb = z.GetBoundingBox()
                    pts = [(mm(bb.GetLeft()), mm(bb.GetTop())), (mm(bb.GetRight()), mm(bb.GetBottom()))]
                    self.paint_poly_block([F, B], pts)
        for t in self.b.GetTracks():
            code = self.code(t.GetNetname())
            if t.GetClass() == "PCB_VIA":
                p = t.GetPosition()
                for lay in (F, B):
                    self.paint_seg(lay, (mm(p.x), mm(p.y)), (mm(p.x), mm(p.y)), mm(t.GetWidth(pcbnew.F_Cu)) / 2, code)
            else:
                lay = F if t.GetLayer() == pcbnew.F_Cu else B
                s, e = t.GetStart(), t.GetEnd()
                self.paint_seg(lay, (mm(s.x), mm(s.y)), (mm(e.x), mm(e.y)), mm(t.GetWidth()) / 2, code)

    def block_rect(self, layers, x0, y0, x1, y1):
        self.paint_poly_block(layers, [(x0, y0), (x1, y1)])

    def penalize_rect(self, layer, x0, y0, x1, y1, k):
        sl = self._win(x0, y0, x1, y1)
        self.cost[layer][sl] *= k

    # --- routing -------------------------------------------------------------
    def pad_cells(self, pad, code):
        pos = pad.GetPosition()
        cx, cy = mm(pos.x), mm(pos.y)
        bb = pad.GetBoundingBox()
        layers = [F, B] if pad.GetAttribute() == pcbnew.PAD_ATTRIB_PTH else ([F] if pad.IsOnLayer(pcbnew.F_Cu) else [B])
        sl = self._win(mm(bb.GetLeft()), mm(bb.GetTop()), mm(bb.GetRight()), mm(bb.GetBottom()))
        cells = []
        for j in range(sl[0].start, sl[0].stop):
            for i in range(sl[1].start, sl[1].stop):
                p = pcbnew.VECTOR2I(pcbnew.FromMM(i * RES), pcbnew.FromMM(j * RES))
                # require the cell (and a small ring) to be inside the copper
                if pad.HitTest(p, -pcbnew.FromMM(0.06)):
                    for lay in layers:
                        cells.append((lay, j, i))
        if not cells:
            j, i = int(round(cy / RES)), int(round(cx / RES))
            cells = [(lay, j, i) for lay in layers]
        return cells, (cx, cy)

    def astar(self, sources, targets, tgt_xy, code, w, via_cost=30.0, max_exp=900000, via_ok=None):
        own = self.own[min(CLASSES, key=lambda c: abs(c - w) if c >= w - 1e-6 else 99)]
        vown = self.own[0.6]
        tset = set(targets)
        ny, nx = self.ny, self.nx
        gx, gy = tgt_xy[0] / RES, tgt_xy[1] / RES
        INF = 1e18
        g = {}
        parent = {}
        heap = []
        for s in sources:
            g[s] = 0.0
            parent[s] = None
            heapq.heappush(heap, (self._h(s, gx, gy), 0.0, s))
        dirs = [(1, 0, 1.0), (-1, 0, 1.0), (0, 1, 1.0), (0, -1, 1.0),
                (1, 1, 1.5), (1, -1, 1.5), (-1, 1, 1.5), (-1, -1, 1.5)]
        exp = 0
        cost = self.cost
        while heap:
            f, gc, cur = heapq.heappop(heap)
            if gc > g.get(cur, INF) + 1e-9:
                continue
            if cur in tset:
                path = []
                while cur is not None:
                    path.append(cur)
                    cur = parent[cur]
                return path[::-1]
            exp += 1
            if exp > max_exp:
                return None
            lay, j, i = cur
            pj = parent[cur]
            pdir = None
            if pj is not None and pj[0] == lay:
                pdir = (i - pj[2], j - pj[1])
            for di, dj, sc in dirs:
                ii, jj = i + di, j + dj
                if ii < 0 or jj < 0 or ii >= nx or jj >= ny:
                    continue
                o = own[lay, jj, ii]
                if o != 0 and o != code:
                    continue
                if di and dj:
                    # no corner cutting
                    o1 = own[lay, j, ii]
                    o2 = own[lay, jj, i]
                    if (o1 != 0 and o1 != code) or (o2 != 0 and o2 != code):
                        continue
                ng = gc + sc * cost[lay, jj, ii]
                if pdir is not None and pdir != (di, dj):
                    ng += 2.0
                nb = (lay, jj, ii)
                if ng < g.get(nb, INF) - 1e-9:
                    g[nb] = ng
                    parent[nb] = cur
                    heapq.heappush(heap, (ng + self._h(nb, gx, gy), ng, nb))
            # via
            ol = 1 - lay
            o = own[ol, j, i]
            vo1, vo2 = vown[0, j, i], vown[1, j, i]
            if (o == 0 or o == code) and (vo1 == 0 or vo1 == code) and (vo2 == 0 or vo2 == code) and \
                    (via_ok is None or via_ok(i * RES, j * RES)):
                nb = (ol, j, i)
                ng = gc + via_cost
                if ng < g.get(nb, INF) - 1e-9:
                    g[nb] = ng
                    parent[nb] = cur
                    heapq.heappush(heap, (ng + self._h(nb, gx, gy), ng, nb))
        return None

    @staticmethod
    def _h(n, gx, gy):
        dx, dy = abs(n[2] - gx), abs(n[1] - gy)
        return (max(dx, dy) + 0.4142 * min(dx, dy)) * 0.999


def simplify(path):
    """Split path into per-layer polylines and via points; drop collinear points."""
    segs = []
    vias = []
    cur = [path[0]]
    for a, b in zip(path, path[1:]):
        if a[0] != b[0]:
            segs.append(cur)
            vias.append((a[1], a[2]))
            cur = [b]
        else:
            cur.append(b)
    segs.append(cur)
    out = []
    for s in segs:
        if len(s) < 2:
            out.append((s[0][0], [s[0]]))
            continue
        pts = [s[0]]
        for k in range(1, len(s) - 1):
            d1 = (s[k][1] - s[k - 1][1], s[k][2] - s[k - 1][2])
            d2 = (s[k + 1][1] - s[k][1], s[k + 1][2] - s[k][2])
            if d1 != d2:
                pts.append(s[k])
        pts.append(s[-1])
        out.append((s[0][0], pts))
    return out, vias


class AutoRouter:
    def __init__(self, router, W, H):
        self.r = router
        self.grid = Grid(router.b, W, H)
        self.grid.add_board_items()
        self.failed = []
        self.vias = [(mm(t.GetPosition().x), mm(t.GetPosition().y)) for t in router.b.GetTracks()
                     if t.GetClass() == "PCB_VIA"]

    def via_ok(self, x, y, min_d=0.9):
        return all(math.hypot(x - vx, y - vy) >= min_d for vx, vy in self.vias)

    def _tree_cells(self, netname):
        """All cells currently holding copper of this net (tracks/vias) -> sources."""
        cells = []
        for t in self.r.b.GetTracks():
            if t.GetNetname() != netname:
                continue
            if t.GetClass() == "PCB_VIA":
                p = t.GetPosition()
                j, i = int(round(mm(p.y) / RES)), int(round(mm(p.x) / RES))
                cells += [(F, j, i), (B, j, i)]
            else:
                lay = F if t.GetLayer() == pcbnew.F_Cu else B
                s, e = t.GetStart(), t.GetEnd()
                x0, y0, x1, y1 = mm(s.x), mm(s.y), mm(e.x), mm(e.y)
                n = max(int(math.hypot(x1 - x0, y1 - y0) / RES), 1)
                for k in range(n + 1):
                    x = x0 + (x1 - x0) * k / n
                    y = y0 + (y1 - y0) * k / n
                    cells.append((lay, int(round(y / RES)), int(round(x / RES))))
        return cells

    def route_net(self, netname, w=0.25, pads=None, via_cost=30.0):
        g = self.grid
        code = g.code(netname)
        allpads = []
        for fp in self.r.b.GetFootprints():
            for p in fp.Pads():
                if p.GetNetname() == netname:
                    allpads.append((fp.GetReference(), p))
        if pads is not None:
            allpads = [x for x in allpads if (x[0], x[1].GetNumber()) in pads or x[0] in pads]
        # dedupe pads that are already connected through the footprint (same ref+number)
        groups = {}
        for k, (ref, p) in enumerate(allpads):
            groups[(ref, p.GetNumber(), k)] = [p]
        keys = list(groups.keys())
        if len(keys) < 2 and not self._tree_cells(netname):
            return True
        # start with the existing tree (if any) else first pad group
        tree = self._tree_cells(netname)
        connected = set()
        if not tree:
            k0 = keys[0]
            for p in groups[k0]:
                tree += g.pad_cells(p, code)[0]
            connected.add(k0)
        else:
            # pads touching the tree are connected
            tset = set(tree)
            for k in keys:
                for p in groups[k]:
                    if any(c in tset for c in g.pad_cells(p, code)[0]):
                        connected.add(k)
        remaining = [k for k in keys if k not in connected]
        ok = True
        while remaining:
            # nearest remaining pad to the tree (by centre distance to any tree cell, sampled)
            sample = tree[::max(1, len(tree) // 200)]
            best = None
            for k in remaining:
                cells, (cx, cy) = g.pad_cells(groups[k][0], code)
                d = min(math.hypot(c[2] * RES - cx, c[1] * RES - cy) for c in sample)
                if best is None or d < best[0]:
                    best = (d, k)
            k = best[1]
            targets = []
            for p in groups[k]:
                targets += g.pad_cells(p, code)[0]
            cx, cy = g.pad_cells(groups[k][0], code)[1]
            path = g.astar(tree, targets, (cx, cy), code, w, via_cost=via_cost, via_ok=self.via_ok)
            remaining.remove(k)
            if path is None:
                self.failed.append((netname, k))
                ok = False
                continue
            self._commit(netname, path, w)
            tree += path
            for p in groups[k]:
                tree += g.pad_cells(p, code)[0]
        return ok

    def _commit(self, netname, path, w):
        g = self.grid
        code = g.code(netname)
        polys, vias = simplify(path)
        for lay, pts in polys:
            xy = [(p[2] * RES, p[1] * RES) for p in pts]
            if len(xy) >= 2:
                self.r.track(netname, xy, w=w, layer="F" if lay == F else "B")
                for a, b in zip(xy, xy[1:]):
                    g.paint_seg(lay, a, b, w / 2, code)
        for (j, i) in vias:
            x, y = i * RES, j * RES
            self.r.via(netname, (x, y), d=VIA_D, drill=VIA_DRILL)
            self.vias.append((x, y))
            for lay in (F, B):
                g.paint_seg(lay, (x, y), (x, y), VIA_D / 2, code)

    def gnd_via_near(self, ref, num, max_d=2.2, net="GND"):
        """Drop a stitching via next to a GND pad with a short top-layer stub."""
        g = self.grid
        code = g.code(net)
        fp = self.r.fps[ref]
        pads = [p for p in fp.Pads() if p.GetNumber() == str(num)]
        for pad in pads:
            pos = pad.GetPosition()
            cx, cy = mm(pos.x), mm(pos.y)
            best = None
            for rr in np.arange(0.6, max_d + 0.01, 0.2):
                for a in range(0, 360, 15):
                    x = round((cx + rr * math.cos(math.radians(a))) / RES) * RES
                    y = round((cy + rr * math.sin(math.radians(a))) / RES) * RES
                    j, i = int(round(y / RES)), int(round(x / RES))
                    if not (0 <= j < g.ny and 0 <= i < g.nx):
                        continue
                    ok = all(g.own[0.6][l, j, i] in (0, code) for l in (F, B)) and self.via_ok(x, y)
                    if not ok:
                        continue
                    # stub clear?
                    n = max(int(rr / RES), 1)
                    clear = True
                    for k in range(n + 1):
                        xx = cx + (x - cx) * k / n
                        yy = cy + (y - cy) * k / n
                        o = g.own[0.4][F, int(round(yy / RES)), int(round(xx / RES))]
                        if o not in (0, code):
                            clear = False
                            break
                    if clear:
                        best = (x, y)
                        break
                if best:
                    break
            if best:
                self.r.track(net, [(cx, cy), best], w=0.4, layer="F")
                self.r.via(net, best, d=VIA_D, drill=VIA_DRILL)
                self.vias.append(best)
                g.paint_seg(F, (cx, cy), best, 0.2, code)
                for lay in (F, B):
                    g.paint_seg(lay, best, best, VIA_D / 2, code)
            else:
                self.failed.append((net, (ref, num)))
