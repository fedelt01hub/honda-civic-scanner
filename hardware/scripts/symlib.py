"""Load KiCad library symbols (resolving `extends`) and compute pin geometry."""
import copy
import math
import os

from sexp import Sym, find, findall, parse

KICAD_SYM_DIR = r"C:/Program Files/KiCad/10.0/share/kicad/symbols"
HERE = os.path.dirname(os.path.abspath(__file__))
LOCAL_LIBS = {"civic_scanner": os.path.join(HERE, "..", "lib", "civic_scanner.kicad_sym")}

_cache = {}


def _lib(libname):
    if libname not in _cache:
        path = LOCAL_LIBS.get(libname) or os.path.join(KICAD_SYM_DIR, libname + ".kicad_sym")
        with open(path, encoding="utf-8") as f:
            tree = parse(f.read())
        _cache[libname] = {x[1]: x for x in findall(tree, "symbol")}
    return _cache[libname]


def load_symbol(lib_id):
    """Return a deep-copied symbol node named lib_id (e.g. 'Device:R'), extends resolved."""
    libname, name = lib_id.split(":", 1)
    syms = _lib(libname)
    node = copy.deepcopy(syms[name])
    ext = find(node, "extends")
    if ext:
        parent = copy.deepcopy(syms[ext[1]])
        pname = parent[1]
        # child properties override parent's
        child_props = {p[1]: p for p in findall(node, "property")}
        out = [Sym("symbol"), name]
        for item in parent[2:]:
            if isinstance(item, list) and item and item[0] == "property" and item[1] in child_props:
                out.append(child_props.pop(item[1]))
            elif isinstance(item, list) and item and item[0] == "symbol":
                sub = copy.deepcopy(item)
                sub[1] = sub[1].replace(pname, name, 1)
                out.append(sub)
            else:
                out.append(item)
        # insert remaining child props after last property
        idx = max(i for i, x in enumerate(out) if isinstance(x, list) and x and x[0] == "property")
        for p in child_props.values():
            idx += 1
            out.insert(idx, p)
        node = out
    node[1] = lib_id
    return node


def pins(lib_id, unit=1):
    """dict pin_number -> (x, y, angle, name, etype) in symbol coordinates (y up)."""
    node = load_symbol(lib_id)
    short = lib_id.split(":", 1)[1]
    res = {}
    for sub in findall(node, "symbol"):
        nm = sub[1]
        suffix = nm.rsplit("_", 2)
        try:
            u = int(suffix[-2])
        except (ValueError, IndexError):
            u = 0
        if u not in (0, unit):
            continue
        for p in findall(sub, "pin"):
            at = find(p, "at")
            num = find(p, "number")[1]
            name = find(p, "name")[1]
            res[num] = (float(at[1]), float(at[2]), float(at[3]) if len(at) > 3 else 0.0, name, str(p[1]))
    return res


def xform(px, py, X, Y, rot=0, mirror=None):
    """Symbol-space point -> schematic point for instance at (X,Y), rotation rot (deg CCW)."""
    x, y = px, py
    if mirror == "x":
        y = -y
    elif mirror == "y":
        x = -x
    a = math.radians(rot)
    xr = x * math.cos(a) - y * math.sin(a)
    yr = x * math.sin(a) + y * math.cos(a)
    return (round(X + xr, 4), round(Y - yr, 4))


def outward_dir(angle, rot=0, mirror=None):
    """Unit vector (schematic coords, y down) pointing away from the body for a pin."""
    a = (angle + 180) % 360
    dx, dy = math.cos(math.radians(a)), math.sin(math.radians(a))
    if mirror == "x":
        dy = -dy
    elif mirror == "y":
        dx = -dx
    r = math.radians(rot)
    xr = dx * math.cos(r) - dy * math.sin(r)
    yr = dx * math.sin(r) + dy * math.cos(r)
    return (round(xr), round(-yr))


if __name__ == "__main__":
    import sys
    for lid in sys.argv[1:]:
        print("==", lid)
        for k, v in sorted(pins(lid).items(), key=lambda kv: kv[0]):
            print("  ", k, v)
