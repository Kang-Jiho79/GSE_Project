"""Tiny software renderer: flat-shaded z-buffered PNG previews, no libraries."""
import math
import struct
import zlib
from lowpoly import norm, dot, sub, cross


def write_png(path, w, h, pix):
    raw = b"".join(b"\x00" + bytes(pix[y * w * 3:(y + 1) * w * 3]) for y in range(h))

    def chunk(t, d):
        c = struct.pack(">I", len(d)) + t + d
        return c + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)
    with open(path, "wb") as fh:
        fh.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) +
                 chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


class Canvas:
    def __init__(self, w, h, bg=(0.16, 0.16, 0.19)):
        self.w, self.h = w, h
        self.pix = bytearray(int(bg[0] * 255).to_bytes(1, "big") + int(bg[1] * 255).to_bytes(1, "big") +
                             int(bg[2] * 255).to_bytes(1, "big")) * (w * h)
        self.z = [1e9] * (w * h)

    def tri(self, a, b, c, col):
        minx = max(int(min(a[0], b[0], c[0])), 0)
        maxx = min(int(max(a[0], b[0], c[0])) + 1, self.w - 1)
        miny = max(int(min(a[1], b[1], c[1])), 0)
        maxy = min(int(max(a[1], b[1], c[1])) + 1, self.h - 1)
        d = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
        if abs(d) < 1e-9:
            return
        r, g, bl = int(col[0] * 255), int(col[1] * 255), int(col[2] * 255)
        for y in range(miny, maxy + 1):
            for x in range(minx, maxx + 1):
                px, py = x + .5, y + .5
                l1 = ((b[1] - c[1]) * (px - c[0]) + (c[0] - b[0]) * (py - c[1])) / d
                l2 = ((c[1] - a[1]) * (px - c[0]) + (a[0] - c[0]) * (py - c[1])) / d
                l3 = 1 - l1 - l2
                if l1 < -1e-6 or l2 < -1e-6 or l3 < -1e-6:
                    continue
                zz = l1 * a[2] + l2 * b[2] + l3 * c[2]
                i = y * self.w + x
                if zz < self.z[i]:
                    self.z[i] = zz
                    o = i * 3
                    self.pix[o], self.pix[o + 1], self.pix[o + 2] = r, g, bl


def view(yaw=40.0, pitch=30.0):
    ya, pa = math.radians(yaw), math.radians(pitch)

    def f(p):
        x = p[0] * math.cos(ya) - p[2] * math.sin(ya)
        z = p[0] * math.sin(ya) + p[2] * math.cos(ya)
        y = p[1] * math.cos(pa) - z * math.sin(pa)
        d = p[1] * math.sin(pa) + z * math.cos(pa)   # depth: larger = nearer the camera
        return (x, y, d)
    return f


def draw_objs(cv, objs, colors, cx, cy, s, yaw=40.0, pitch=30.0, offset=(0, 0, 0), yscale=1.0):
    """objs: iterable of Obj; colors: material -> rgb. Centre of the model's
    ground point lands on pixel (cx, cy), `s` pixels per metre."""
    f = view(yaw, pitch)
    light = norm((-0.4, 0.8, 0.5))
    for o in objs:
        for m, face in o.faces:
            pts = [(p[0] + offset[0], p[1] + offset[1], p[2] + offset[2]) for p in face]
            n = norm(cross(sub(pts[1], pts[0]), sub(pts[2], pts[0])))
            vn = f((n[0], n[1], n[2]))
            vo = f((0, 0, 0))
            if vn[2] - vo[2] < -0.0:   # facing away from the camera
                continue
            base = colors.get(m, (0.8, 0.2, 0.8))
            sh = 0.45 + 0.55 * max(dot(n, light), 0.0)
            col = tuple(min(c * sh, 1.0) for c in base)
            sp = []
            for p in pts:
                q = f(p)
                sp.append((cx + q[0] * s, cy - q[1] * s, -q[2]))
            for i in range(1, len(sp) - 1):
                cv.tri(sp[0], sp[i], sp[i + 1], col)
