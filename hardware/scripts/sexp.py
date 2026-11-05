"""Minimal S-expression parser/serializer for KiCad files."""
import re

_TOK = re.compile(r'\s*(?:(\()|(\))|("(?:[^"\\]|\\.)*")|([^\s()"]+))', re.S)


class Sym(str):
    """Bare (unquoted) atom."""


def parse(text):
    pos = 0
    stack = [[]]
    n = len(text)
    while pos < n:
        m = _TOK.match(text, pos)
        if not m:
            if text[pos:].strip() == "":
                break
            raise ValueError("parse error at %d: %r" % (pos, text[pos:pos + 40]))
        pos = m.end()
        o, c, s, a = m.groups()
        if o:
            stack.append([])
        elif c:
            node = stack.pop()
            stack[-1].append(node)
        elif s is not None:
            stack[-1].append(re.sub(r'\\(.)', lambda mm: "\n" if mm.group(1) == "n" else mm.group(1), s[1:-1]))
        elif a is not None:
            stack[-1].append(Sym(a))
    return stack[0][0] if len(stack[0]) == 1 else stack[0]


def _q(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def dumps(node, indent=0):
    if isinstance(node, Sym):
        return str(node)
    if isinstance(node, str):
        return _q(node)
    if isinstance(node, bool):
        return "yes" if node else "no"
    if isinstance(node, (int,)):
        return str(node)
    if isinstance(node, float):
        s = ("%.4f" % node).rstrip("0").rstrip(".")
        return "0" if s in ("-0", "") else s
    # list
    simple = all(not isinstance(x, list) for x in node)
    if simple:
        return "(" + " ".join(dumps(x) for x in node) + ")"
    pad = "\t" * (indent + 1)
    parts = ["(" + dumps(node[0])]
    for x in node[1:]:
        if isinstance(x, list):
            parts.append("\n" + pad + dumps(x, indent + 1))
        else:
            parts.append(" " + dumps(x))
    parts.append("\n" + "\t" * indent + ")")
    return "".join(parts)


def find(node, key):
    for x in node:
        if isinstance(x, list) and x and x[0] == key:
            return x
    return None


def findall(node, key):
    return [x for x in node if isinstance(x, list) and x and x[0] == key]


def S(*items):
    """Build list converting str keys at head to Sym."""
    out = []
    for i, it in enumerate(items):
        if i == 0 and isinstance(it, str):
            out.append(Sym(it))
        else:
            out.append(it)
    return out
