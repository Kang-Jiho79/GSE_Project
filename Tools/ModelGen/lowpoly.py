"""Low-poly mesh toolkit for The Darkest Light.

Conventions (every model in Assets/Models follows them):
  * right-handed, Y up, the model faces +Z, units are metres
  * NPC parts share one origin: the point on the ground between the feet
  * flat shaded (one normal per face), no textures - colour comes from the
    named materials in Assets/Models/Materials.mtl
"""
import math
import random

# ---------------------------------------------------------------- vectors


def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def mul(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def norm(a):
    n = math.sqrt(dot(a, a))
    return (0.0, 1.0, 0.0) if n < 1e-9 else (a[0] / n, a[1] / n, a[2] / n)


# ---------------------------------------------------------------- faces


def _clean(face):
    out = []
    for p in face:
        if not out or sub(p, out[-1]) != (0, 0, 0) and math.dist(p, out[-1]) > 1e-7:
            out.append(p)
    if len(out) > 1 and math.dist(out[0], out[-1]) <= 1e-7:
        out.pop()
    return out


def _newell(f):
    nx = ny = nz = 0.0
    for i in range(len(f)):
        a, b = f[i], f[(i + 1) % len(f)]
        nx += (a[1] - b[1]) * (a[2] + b[2])
        ny += (a[2] - b[2]) * (a[0] + b[0])
        nz += (a[0] - b[0]) * (a[1] + b[1])
    return (nx, ny, nz)


def orient(faces, center=None):
    """Drop degenerate faces and wind the rest counter-clockwise seen from
    outside, for shapes that lie around `center` (convex, or shells)."""
    faces = [f for f in (_clean(f) for f in faces) if len(f) >= 3]
    if not faces:
        return []
    if center is None:
        pts = [p for f in faces for p in f]
        center = tuple(sum(p[i] for p in pts) / len(pts) for i in range(3))
    out = []
    for f in faces:
        n = _newell(f)
        if dot(n, n) < 1e-14:
            continue
        c = tuple(sum(p[i] for p in f) / len(f) for i in range(3))
        out.append(f if dot(n, sub(c, center)) >= 0 else f[::-1])
    return out


# ---------------------------------------------------------------- transforms


def move(faces, dx=0.0, dy=0.0, dz=0.0):
    return [[(p[0] + dx, p[1] + dy, p[2] + dz) for p in f] for f in faces]


def scale(faces, sx=1.0, sy=None, sz=None, pivot=(0, 0, 0)):
    sy = sx if sy is None else sy
    sz = sx if sz is None else sz
    return [[(pivot[0] + (p[0] - pivot[0]) * sx, pivot[1] + (p[1] - pivot[1]) * sy,
              pivot[2] + (p[2] - pivot[2]) * sz) for p in f] for f in faces]


def rotate(faces, rx=0.0, ry=0.0, rz=0.0, pivot=(0, 0, 0)):
    """Euler rotation in degrees, applied X then Y then Z around `pivot`."""
    ax, ay, az = math.radians(rx), math.radians(ry), math.radians(rz)
    cx, sx_, cy, sy_, cz, sz_ = math.cos(ax), math.sin(ax), math.cos(ay), math.sin(ay), math.cos(az), math.sin(az)

    def r(p):
        x, y, z = p[0] - pivot[0], p[1] - pivot[1], p[2] - pivot[2]
        y, z = y * cx - z * sx_, y * sx_ + z * cx
        x, z = x * cy + z * sy_, -x * sy_ + z * cy
        x, y = x * cz - y * sz_, x * sz_ + y * cz
        return (x + pivot[0], y + pivot[1], z + pivot[2])
    return [[r(p) for p in f] for f in faces]


def mirror_x(faces):
    return [[(-p[0], p[1], p[2]) for p in f][::-1] for f in faces]


def ring_copies(faces, count, around=(0, 0, 0)):
    """`count` copies spread evenly round the Y axis."""
    out = []
    for i in range(count):
        out += rotate(faces, 0, 360.0 * i / count, 0, around)
    return out


# ---------------------------------------------------------------- primitives


def frustum(x, y, z, bw, bd, tw, td, h, dx=0.0, dz=0.0, sides=4, rot=0.0):
    """Tapered prism. (x, y, z) is the centre of the base, bw/bd and tw/td
    the base and top width/depth, the top shifted by dx/dz. sides=4 gives a
    box-like shape, more gives a rough cylinder or cone (tw=td=0: a point)."""
    def ring(w, d, cx, cy, cz):
        if sides == 4:
            pts = [(-w / 2, -d / 2), (w / 2, -d / 2), (w / 2, d / 2), (-w / 2, d / 2)]
        else:
            pts = [(w / 2 * math.cos(rot + 2 * math.pi * i / sides),
                    d / 2 * math.sin(rot + 2 * math.pi * i / sides)) for i in range(sides)]
        return [(cx + px, cy, cz + pz) for px, pz in pts]
    b = ring(bw, bd, x, y, z)
    t = ring(tw, td, x + dx, y + h, z + dz)
    n = sides
    faces = [[b[i], b[(i + 1) % n], t[(i + 1) % n], t[i]] for i in range(n)]
    faces.append(b[::-1])
    faces.append(t)
    return orient(faces, (x + dx / 2, y + h / 2, z + dz / 2))


def box(cx, cy, cz, w, h, d):
    """Axis-aligned box centred on (cx, cy, cz)."""
    return frustum(cx, cy - h / 2, cz, w, d, w, d, h)


def box_base(x, y, z, w, h, d):
    """Axis-aligned box standing on y."""
    return frustum(x, y, z, w, d, w, d, h)


def cone(x, y, z, r, h, sides=6, rot=0.0):
    return frustum(x, y, z, r * 2, r * 2, 0, 0, h, sides=sides, rot=rot)


def cylinder(x, y, z, r, h, sides=8, rot=0.0):
    return frustum(x, y, z, r * 2, r * 2, r * 2, r * 2, h, sides=sides, rot=rot)


def sphere(cx, cy, cz, rx, ry=None, rz=None, seg=8, rings=4, th0=0.0, th1=math.pi,
           ph0=0.0, ph1=2 * math.pi, jitter=0.0, rng=None):
    """Low-poly ellipsoid or a patch of one. theta runs from the top pole
    (0) to the bottom pole (pi); phi runs round, 0 facing +Z. Give a
    partial theta/phi range for caps and domes. jitter roughens it (rocks)."""
    ry = rx if ry is None else ry
    rz = rx if rz is None else rz
    rng = rng or random.Random(1)
    full = abs((ph1 - ph0) - 2 * math.pi) < 1e-6
    cols = seg if full else seg + 1
    grid = {}
    for k in range(rings + 1):
        th = th0 + (th1 - th0) * k / rings
        for s in range(cols):
            ph = ph0 + (ph1 - ph0) * s / seg
            j = 1.0 + (rng.uniform(-jitter, jitter) if jitter else 0.0)
            if abs(math.sin(th)) < 1e-9:
                j = grid[(k, 0)][3] if (k, 0) in grid else j
            grid[(k, s)] = (cx + rx * j * math.sin(th) * math.sin(ph), cy + ry * j * math.cos(th),
                            cz + rz * j * math.sin(th) * math.cos(ph), j)
    faces = []
    for k in range(rings):
        for s in range(seg):
            s2 = (s + 1) % cols if full else s + 1
            faces.append([grid[(k, s)][:3], grid[(k, s2)][:3], grid[(k + 1, s2)][:3], grid[(k + 1, s)][:3]])
    return orient(faces, (cx, cy, cz))


def gem(cx, cy, cz, rx, ry=None, rz=None):
    """Octahedron - a shard or a diamond."""
    return sphere(cx, cy, cz, rx, ry, rz, seg=4, rings=2)


def _basis(axis):
    a = norm(axis)
    helper = (0, 1, 0) if abs(a[1]) < 0.95 else (1, 0, 0)
    u = norm(cross(a, helper))
    v = cross(a, u)
    return a, u, v


def taper_rod(p0, p1, w0, w1, sides=4):
    """Tapered prism between two points: a limb, a spike, a plank."""
    a, u, v = _basis(sub(p1, p0))
    rot = math.pi / 4 if sides == 4 else 0.0

    def ring(p, w):
        return [add(p, add(mul(u, w / 2 * math.cos(rot + 2 * math.pi * i / sides)),
                           mul(v, w / 2 * math.sin(rot + 2 * math.pi * i / sides)))) for i in range(sides)]
    b, t = ring(p0, w0 * (math.sqrt(2) if sides == 4 else 1)), ring(p1, w1 * (math.sqrt(2) if sides == 4 else 1))
    n = sides
    faces = [[b[i], b[(i + 1) % n], t[(i + 1) % n], t[i]] for i in range(n)]
    faces.append(b[::-1])
    faces.append(t)
    return orient(faces, mul(add(p0, p1), 0.5))


def spike(p0, p1, w):
    return taper_rod(p0, p1, w, 0.0)


def rod_ring(cx, cy, cz, radius, thick, n=8, plane="xz", rot=0.0):
    """A hollow ring made of straight bars (halo, crown band, hoop)."""
    pts = []
    for i in range(n):
        t = rot + 2 * math.pi * i / n
        if plane == "xz":
            pts.append((cx + radius * math.sin(t), cy, cz + radius * math.cos(t)))
        elif plane == "xy":
            pts.append((cx + radius * math.cos(t), cy + radius * math.sin(t), cz))
        else:
            pts.append((cx, cy + radius * math.sin(t), cz + radius * math.cos(t)))
    faces = []
    for i in range(n):
        a, b = pts[i], pts[(i + 1) % n]
        ext = mul(norm(sub(b, a)), thick * 0.5)
        faces += taper_rod(sub(a, ext), add(b, ext), thick, thick)
    return faces


def gable(x, y, z, w, d, h, overhang=0.0, ridge_along="x"):
    """Triangular-prism roof sitting on y, ridge along x (or z)."""
    ow, od = w / 2 + overhang, d / 2 + overhang
    if ridge_along == "x":
        c = [(-ow, 0, -od), (ow, 0, -od), (ow, 0, od), (-ow, 0, od)]
        r = [(-ow, h, 0), (ow, h, 0)]
        faces = [[c[0], c[1], r[1], r[0]], [c[2], c[3], r[0], r[1]],
                 [c[1], c[2], r[1]], [c[3], c[0], r[0]], c[::-1]]
    else:
        c = [(-ow, 0, -od), (ow, 0, -od), (ow, 0, od), (-ow, 0, od)]
        r = [(0, h, -od), (0, h, od)]
        faces = [[c[0], c[1], r[0]], [c[2], c[3], r[1]],
                 [c[1], c[2], r[1], r[0]], [c[3], c[0], r[0], r[1]], c[::-1]]
    return move(orient(faces, (0, h / 3, 0)), x, y, z)


def slab(x, y, z, w, d, thick=0.05):
    return box_base(x, y, z, w, thick, d)


# ---------------------------------------------------------------- objects and files


class Obj:
    """One named mesh: a list of (material, polygon)."""

    def __init__(self, name):
        self.name = name
        self.faces = []

    def add(self, mat, faces):
        for f in faces:
            self.faces.append((mat, [tuple(p) for p in f]))
        return self

    def tris(self):
        return sum(len(f) - 2 for _, f in self.faces)

    def bounds(self):
        pts = [p for _, f in self.faces for p in f]
        if not pts:
            return (0, 0, 0), (0, 0, 0)
        return (tuple(min(p[i] for p in pts) for i in range(3)),
                tuple(max(p[i] for p in pts) for i in range(3)))

    def copy_as(self, name, transform=None):
        o = Obj(name)
        for m, f in self.faces:
            o.faces.append((m, f))
        if transform:
            o.faces = [(m, f) for (m, _), f in zip(o.faces, transform([f for _, f in o.faces]))]
        return o


def write_obj(path, objs, comment="", mtllib="Materials.mtl"):
    """Write named objects to one OBJ. Vertices are shared inside the file;
    every face carries its own flat normal."""
    verts, vkey, norms, nkey = [], {}, [], {}

    def vi(p):
        k = (round(p[0], 5), round(p[1], 5), round(p[2], 5))
        if k not in vkey:
            vkey[k] = len(verts) + 1
            verts.append(k)
        return vkey[k]

    def ni(n):
        k = (round(n[0], 4), round(n[1], 4), round(n[2], 4))
        if k not in nkey:
            nkey[k] = len(norms) + 1
            norms.append(k)
        return nkey[k]

    body = []
    for o in objs:
        body.append("o %s" % o.name)
        last = None
        for m, f in o.faces:
            if m != last:
                body.append("usemtl %s" % m)
                last = m
            n = ni(norm(_newell(f)))
            body.append("f " + " ".join("%d//%d" % (vi(p), n) for p in f))
    lines = ["# The Darkest Light - low-poly model (Y up, +Z forward, metres)"]
    if comment:
        lines += ["# " + c for c in comment.splitlines()]
    lines.append("mtllib %s" % mtllib)
    lines += ["v %.5f %.5f %.5f" % v for v in verts]
    lines += ["vn %.4f %.4f %.4f" % n for n in norms]
    lines += body
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")
