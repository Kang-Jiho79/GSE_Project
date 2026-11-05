"""World models: nature, stonework, villages, temples, capitals, landmarks.

Styles follow the game: Light = tall, straight, white stone and gold;
Dark = low, heavy, black stone and violet thorns; Ashen = grey ruin;
Border = timber and thatch of the borderborn. Every object is named
Env_<Group>_<Name>[_<Variant>] and lies in the file of its group.
1 unit = 1 metre, Y up, the front (door side) faces +Z, the origin is the
centre of the footprint on the ground.
"""
import math
import random
from lowpoly import *

STY = {
    "Light": dict(stone="stone_light", roof="roof_light", trim="trim_gold", glow="glow_window", cloth="cloth_light"),
    "Dark": dict(stone="stone_dark", roof="roof_dark", trim="trim_violet", glow="glow_violet", cloth="cloth_dark"),
    "Ashen": dict(stone="stone_ash", roof="roof_ash", trim="trim_silver", glow="stone_ash_dark", cloth="stone_ash_dark"),
    "Border": dict(stone="wood", roof="thatch", trim="cloth_border", glow="glow_window", cloth="cloth_border"),
}


def O(name, *items):
    o = Obj(name)
    for m, f in items:
        o.add(m, f)
    return o


def scaled(o, name, s):
    return o.copy_as(name, lambda fs: scale(fs, s))


# ---------------------------------------------------------------- trees


def _branch(rng, p, ang, up, length, w, depth, out_faces, thorn=None):
    dx, dz = math.cos(ang), math.sin(ang)
    q = (p[0] + dx * length * (1 - up), p[1] + length * up, p[2] + dz * length * (1 - up))
    out_faces += taper_rod(p, q, w, max(w * .35, .015), 5)
    if thorn:
        for t in (.4, .75):
            b = (p[0] + (q[0] - p[0]) * t, p[1] + (q[1] - p[1]) * t, p[2] + (q[2] - p[2]) * t)
            thorn.extend(spike(b, (b[0] - dz * .25, b[1] + .12, b[2] + dx * .25), .06))
    if depth > 0:
        for k in range(2):
            _branch(rng, q, ang + rng.uniform(-1.1, 1.1), max(.3, up - .1), length * .62, w * .55, depth - 1, out_faces, thorn)
    return q


def tree(kind, seed):
    rng = random.Random(seed)
    trunk, thorns, leaves = [], [], []
    h = {"dead": 3.6, "light": 2.4, "dark": 3.0}[kind]
    p = (0.0, 0.0, 0.0)
    pts = [p]
    for i in range(4):
        q = (p[0] + rng.uniform(-.14, .14), p[1] + h / 4, p[2] + rng.uniform(-.14, .14))
        w0 = .42 * (1 - i / 4.5)
        trunk += taper_rod(p, q, w0, w0 * .78, 6)
        p = q
        pts.append(p)
    for i in range(3):   # roots
        a = i * 2.1 + rng.uniform(0, .6)
        trunk += spike((math.cos(a) * .12, .25, math.sin(a) * .12), (math.cos(a) * .55, 0, math.sin(a) * .55), .16)
    th = thorns if kind == "dark" else None
    for i in (2, 3, 4):
        for _ in range(2 if kind != "light" else 1):
            _branch(rng, pts[i], rng.uniform(0, 6.28), rng.uniform(.35, .6), rng.uniform(.9, 1.4) * (1 if kind != "light" else .8),
                    .15 - (i - 2) * .03, 1 if kind != "light" else 0, trunk, th)
    if kind == "light":
        for c, r in (((0, h + .55, 0), .95), ((.8, h + .1, .3), .7), ((-.7, h, -.3), .65)):
            leaves += sphere(c[0], c[1], c[2], r, r * .8, r, seg=6, rings=3, jitter=.12, rng=rng)
    name = {"dead": "Tree_Dead", "light": "Tree_Light", "dark": "Tree_Dark"}[kind]
    o = O("Env_Trees_%s_%s" % (name.split("_")[1], "ABC"[seed % 3]),
          ("bark_dark" if kind == "dark" else "bark_ash", trunk))
    if thorns:
        o.add("thorn_dark", thorns)
    if leaves:
        o.add("leaf_light", leaves)
    return o


def stump():
    return O("Env_Trees_Stump", ("bark_ash", frustum(0, 0, 0, .7, .7, .55, .55, .5, sides=7)),
             ("bark_ash", spike((.1, .5, 0), (.12, .85, .05), .16)))


def fallen_log():
    return O("Env_Trees_Log", ("bark_ash", taper_rod((-1.3, .28, 0), (1.3, .26, .1), .6, .5, 7)),
             ("bark_ash", spike((.6, .4, .1), (.9, .85, .3), .12)))


def bush(kind):
    rng = random.Random(7 if kind == "dead" else 9)
    f = []
    for i in range(7):
        a = rng.uniform(0, 6.28)
        f += spike((0, 0, 0), (math.cos(a) * .55, rng.uniform(.5, .9), math.sin(a) * .55), .09)
    return O("Env_Trees_Bush_%s" % ("Dead" if kind == "dead" else "Thorn"),
             ("bark_ash" if kind == "dead" else "thorn_dark", f))


# ---------------------------------------------------------------- rocks


def boulder(size, var):
    r = {"S": .45, "M": .85, "L": 1.5}[size]
    rng = random.Random(ord(size) * 7 + var)
    return O("Env_Rocks_Boulder_%s_%s" % (size, "AB"[var]),
             ("rock_dark" if var else "rock_ash",
              sphere(0, r * .35, 0, r, r * .75, r * (.85 + .2 * var), seg=7, rings=4, jitter=.2, rng=rng, th1=math.radians(160))))


def rock_spire():
    rng = random.Random(3)
    f = taper_rod((0, 0, 0), (.1, 3.4, 0), 1.0, .25, 6) + taper_rod((.55, 0, .2), (.7, 1.8, .2), .6, .1, 5)
    return O("Env_Rocks_Spire", ("rock_ash", f))


def rock_slab():
    return O("Env_Rocks_Slab", ("rock_ash", frustum(0, 0, 0, 2.2, 1.5, 1.9, 1.3, .35, dx=.1, dz=.05, sides=6, rot=.3)))


def pebbles():
    rng = random.Random(5)
    f = []
    for i in range(6):
        f += sphere(rng.uniform(-.5, .5), .08, rng.uniform(-.5, .5), rng.uniform(.08, .16), .09, rng.uniform(.08, .16),
                    seg=5, rings=2, jitter=.15, rng=rng)
    return O("Env_Rocks_Pebbles", ("rock_ash", f))


def crystals(kind):
    rng = random.Random(11)
    f = []
    for i in range(6):
        a = rng.uniform(0, 6.28)
        d = rng.uniform(0, .35)
        base = (math.cos(a) * d, 0, math.sin(a) * d)
        tip = (base[0] + math.cos(a) * .25, rng.uniform(.6, 1.5), base[2] + math.sin(a) * .25)
        f += taper_rod(base, tip, .22, .0, 6)
    return O("Env_Rocks_Crystal_%s" % kind, ("crystal_%s" % kind.lower(), f), ("rock_dark", frustum(0, 0, 0, .9, .9, .6, .6, .18, sides=6)))


# ---------------------------------------------------------------- stonework


def rubble_pile():
    rng = random.Random(21)
    f = []
    for i in range(9):
        b = box_base(rng.uniform(-.7, .7), 0, rng.uniform(-.7, .7), rng.uniform(.25, .6), rng.uniform(.15, .45), rng.uniform(.25, .5))
        f += rotate(b, rng.uniform(-12, 12), rng.uniform(0, 90), rng.uniform(-12, 12), (0, .2, 0))
    return O("Env_Ruins_Rubble_Pile", ("stone_ash", f))


def wall_segment(style, broken=False):
    s = STY[style]
    h = {"Light": 3.0, "Dark": 2.2, "Ashen": 2.4}[style]
    f = box_base(0, 0, 0, 4.0, h, .7)
    o = O("Env_Wall_%s_%s" % ("Broken" if broken else "Segment", style), (s["stone"], f if not broken else
          box_base(-1.0, 0, 0, 2.0, h, .7) + frustum(1.0, 0, 0, 2.0, .7, 1.0, .5, h * .55, dx=.3)))
    if not broken:
        for i in range(4):
            o.add(s["stone"], box_base(-1.5 + i, h, 0, .5, .45, .72))
        o.add(s["trim"], box_base(0, h - .1, 0, 4.02, .1, .74))
        if style == "Dark":
            for i in range(4):
                o.add(s["trim"], spike((-1.5 + i, h + .45, 0), (-1.5 + i, h + .95, 0), .2))
    else:
        o.add("stone_ash", box_base(2.2, 0, .8, .5, .3, .4))
    return o


def pillar(style, broken=False):
    s = STY[style]
    h = {"Light": 4.6, "Dark": 3.0, "Ashen": 3.8}[style]
    w = {"Light": .6, "Dark": .95, "Ashen": .8}[style]
    o = O("Env_Pillar_%s_%s" % ("Broken" if broken else "Standing", style),
          (s["stone"], box_base(0, 0, 0, w + .4, .35, w + .4)))
    if broken:
        o.add(s["stone"], frustum(0, .35, 0, w, w, w * .9, w * .9, h * .45, sides=8, rot=.2))
        o.add(s["stone"], spike((0, .35 + h * .45, 0), (.15, .35 + h * .45 + .55, .1), w * .8))
    else:
        o.add(s["stone"], frustum(0, .35, 0, w, w, w * .85, w * .85, h, sides=8, rot=.2))
        o.add(s["stone"], box_base(0, h + .35, 0, w + .35, .3, w + .35))
        o.add(s["trim"], box_base(0, .35, 0, w + .1, .06, w + .1))
        if style == "Dark":
            o.add(s["trim"], spike((0, h + .65, 0), (0, h + 1.3, 0), .35))
        if style == "Light":
            o.add(s["trim"], spike((0, h + .65, 0), (0, h + 1.1, 0), .22))
    return o


def column_fallen():
    return O("Env_Ruins_Column_Fallen", ("stone_ash", taper_rod((-1.6, .4, 0), (1.6, .4, .1), .8, .7, 8)),
             ("stone_ash", box_base(-1.8, 0, 0, .3, .8, .8)))


def _battlements(style, w, d, y, n, mat):
    f = []
    for i in range(n):
        a = 2 * math.pi * i / n
        f += box_base(math.sin(a) * w / 2, y, math.cos(a) * d / 2, .45, .4, .45)
    return f


def tower(style, ruined=False):
    s = STY[style]
    h = {"Light": 9.0, "Dark": 6.5, "Ashen": 7.0}[style]
    w = {"Light": 3.0, "Dark": 4.2, "Ashen": 3.6}[style]
    o = O("Env_Tower_%s%s" % (style, "_Ruined" if ruined else ""), (s["stone"], box_base(0, 0, 0, w + .6, .5, w + .6)))
    if ruined:
        o.add(s["stone"], frustum(0, .5, 0, w, w, w * .95, w * .95, h * .55, sides=8, rot=.4))
        o.add(s["stone"], frustum(0, .5 + h * .55, 0, w * .95, w * .95, w * .6, w * .6, 1.4, dx=.5, sides=8, rot=.4))
        o.add("stone_ash", box_base(2.1, 0, 1.0, .9, .6, .8))
        return o
    o.add(s["stone"], frustum(0, .5, 0, w, w, w * .88, w * .88, h, sides=8, rot=.4))
    o.add(s["trim"], frustum(0, .5 + h * .7, 0, w * .9, w * .9, w * .9, w * .9, .15, sides=8, rot=.4))
    o.add(s["stone"], frustum(0, .5 + h, 0, w * 1.05, w * 1.05, w * 1.05, w * 1.05, .5, sides=8, rot=.4))
    if style == "Dark":
        o.add(s["roof"], _battlements(style, w, w, 1.0 + h, 8, s["roof"]))
        for i in range(6):
            a = 2 * math.pi * i / 6
            o.add(s["trim"], spike((math.sin(a) * w * .45, 1.0 + h, math.cos(a) * w * .45),
                                   (math.sin(a) * w * .55, 2.4 + h, math.cos(a) * w * .55), .3))
    elif style == "Light":
        o.add(s["roof"], cone(0, 1.0 + h, 0, w * .65, 3.6, 8, .4))
        o.add(s["trim"], spike((0, 4.6 + h, 0), (0, 5.8 + h, 0), .22))
    else:
        o.add(s["roof"], _battlements(style, w, w, 1.0 + h, 8, s["roof"]))
    o.add("door", box(0, 1.0, w * .44 + .02, 1.0, 2.0, .1))
    for k in range(3):
        o.add(s["glow"], box(0, 3.0 + k * 1.8, w * .43, .3, .7, .1))
    return o


# ---------------------------------------------------------------- houses


def house(style, variant):
    s = STY[style]
    big = variant == "B"
    k = 1.4 if big else 1.0
    w, d = {"Light": (3.2, 2.8), "Dark": (3.8, 3.2), "Border": (3.4, 3.0)}.get(style, (3.4, 3.0))
    w, d = w * k, d * k
    if style == "Ruin":
        return house_ruin(variant)
    h = {"Light": 3.2, "Dark": 2.1, "Border": 2.3}[style]
    name = "Env_House_%s_%s" % (style, variant)
    o = O(name, (s["stone"], box_base(0, 0, 0, w, h, d)),
          ("stone_ash_dark" if style != "Border" else "stone_ash", box_base(0, 0, 0, w + .12, .25, d + .12)))
    rh = {"Light": w * .62, "Dark": .85, "Border": 1.3}[style]
    oh = {"Light": .25, "Dark": .5, "Border": .55}[style]
    o.add(s["roof"], gable(0, h, 0, w, d, rh, oh))
    o.add("door", box(0, .95, d / 2 + .03, .95, 1.9, .08))
    o.add(s["trim"], box(0, 2.0, d / 2 + .04, 1.2, .12, .08))
    for sx in (-1, 1):
        o.add(s["glow"], box(sx * w * .3, h * .6, d / 2 + .03, .5, .6 if style != "Dark" else .25, .06))
        o.add(s["glow"], box(sx * (w / 2 + .03), h * .6, 0, .06, .55 if style != "Dark" else .22, .5))
    if style == "Light":
        for sx in (-1, 1):
            for sz in (-1, 1):
                o.add(s["trim"], box_base(sx * w / 2, 0, sz * d / 2, .22, h, .22))
        o.add(s["trim"], spike((-w / 2 - oh, h + .02, 0), (-w / 2 - oh - .3, h + .3, 0), .12))
        o.add(s["trim"], spike((0, h + rh, 0), (0, h + rh + .8, 0), .12))
    if style == "Dark":
        for i in range(4):
            x = -w * .4 + i * w * .8 / 3
            o.add(s["trim"], spike((x, h + .5, 0), (x, h + 1.25, 0), .22))
        o.add(s["stone"], box_base(w * .3, h, -d * .2, .6, 1.0, .6))
    if style == "Border":
        for i in range(5):
            o.add("wood_dark", box_base(0, i * .46, d / 2 + .02, w + .04, .06, .02))
        o.add("stone_ash", box_base(w * .3, h, -d * .2, .5, 1.5, .5))
    if big:
        ax = w / 2 + .9
        o.add(s["stone"], box_base(ax, 0, .4, 1.8, h * .65, d * .7))
        o.add(s["roof"], gable(ax, h * .65, .4, 1.8, d * .7, rh * .55, oh, "z"))
    return o


def house_ruin(variant):
    rng = random.Random(40 + ord(variant))
    k = 1.3 if variant == "B" else 1.0
    f = box_base(0, 0, -1.4 * k, 3.4 * k, 2.4, .5)
    f += frustum(-1.6 * k, 0, 0, .5, 2.8 * k, .45, 2.2 * k, 1.5, dz=.2)
    f += frustum(1.6 * k, 0, 0, .5, 2.8 * k, .45, 1.0, .9)
    f += frustum(0, 0, 1.4 * k, 1.2, .5, 1.0, .45, .5)
    o = O("Env_House_Ruin_%s" % variant, ("stone_ash", f), ("stone_ash_dark", box_base(0, 0, 0, 3.6 * k, .2, 3.2 * k)))
    for i in range(3):   # fallen roof beams
        a = rng.uniform(-.8, .8)
        o.add("wood_dark", taper_rod((-1.5 + i * 1.4, 2.0, -1.2), (-.8 + i * 1.4 + a, .35, .5), .22, .2))
    for i in range(6):
        o.add("stone_ash", rotate(box_base(rng.uniform(-1.5, 1.5), 0, rng.uniform(-1.3, 1.3), rng.uniform(.3, .5), .3, .4), 0, rng.uniform(0, 90), 0))
    return o


# ---------------------------------------------------------------- temples and altar


def temple(style):
    s = STY.get(style)
    if style == "Light":
        o = O("Env_Temple_Light", ("stone_light", box_base(0, 0, 0, 9, .3, 12.5)), ("stone_light", box_base(0, .3, 0, 8.4, .25, 12)))
        o.add(s["stone"], box_base(0, .55, -.6, 6.0, 4.0, 9.6))
        o.add(s["roof"], gable(0, 4.55, -.6, 6.0, 9.6, 3.0, .3, "z"))
        o.add(s["stone"], box_base(0, .55, 4.6, 3.0, 5.5, 3.0))
        o.add(s["stone"], frustum(0, 6.05, 4.6, 3.0, 3.0, 2.0, 2.0, 2.8))
        o.add(s["roof"], frustum(0, 8.85, 4.6, 2.4, 2.4, 0, 0, 5.2))
        o.add(s["trim"], spike((0, 14.0, 4.6), (0, 15.6, 4.6), .25))
        for sx in (-1, 1):
            o.add(s["stone"], box_base(sx * 3.5, .55, 4.6, .9, 4.6, .9))
            o.add(s["roof"], frustum(sx * 3.5, 5.15, 4.6, .9, .9, 0, 0, 2.0))
            for z in (-2.5, .5, 3.0):
                o.add(s["trim"], box_base(sx * 3.05, .55, z, .35, 4.0, .35))
        o.add("door", box(0, 1.9, 6.15, 1.6, 2.7, .12))
        o.add(s["glow"], rotate(box(0, 5.2, 6.15, .9, .9, .1), 0, 0, 45, (0, 5.2, 6.15)))
        return o
    if style == "Dark":
        o = O("Env_Temple_Dark", ("stone_dark", box_base(0, 0, 0, 10, .35, 13)))
        o.add(s["stone"], box_base(0, .35, -.5, 8.2, 2.9, 10.2))
        o.add(s["roof"], gable(0, 3.25, -.5, 8.2, 10.2, 1.4, .4, "z"))
        for sx in (-1, 1):
            o.add(s["stone"], box_base(sx * 4.2, .35, 4.2, 2.2, 4.4, 2.2))
            o.add(s["roof"], frustum(sx * 4.2, 4.75, 4.2, 2.6, 2.6, 1.0, 1.0, .6))
            for i in range(3):
                a = i * 2.1
                o.add(s["trim"], spike((sx * 4.2 + math.sin(a) * .7, 5.35, 4.2 + math.cos(a) * .7),
                                       (sx * 4.2 + math.sin(a) * 1.0, 7.0 + i * .3, 4.2 + math.cos(a) * 1.0), .3))
        for i in range(8):
            z = -4.5 + i * 1.3
            for sx in (-1, 1):
                o.add(s["trim"], spike((sx * 4.0, 3.5 + 1.0, z), (sx * 4.15, 5.0, z), .3))
        o.add("door", box(0, 1.5, 5.15, 1.7, 2.3, .12))
        o.add(s["trim"], box(0, 2.85, 5.18, 2.2, .18, .1))
        for sx in (-1, 1):
            o.add("iron", cylinder(sx * 2.2, .35, 5.8, .35, .9, 6))
            o.add(s["glow"], gem(sx * 2.2, 1.6, 5.8, .22, .3, .22))
        return o
    o = O("Env_Temple_Ruined", ("stone_ash", box_base(0, 0, 0, 9, .3, 12)))
    o.add("stone_ash", box_base(-2.6, .3, -1.0, .8, 3.4, 8.0) + box_base(2.6, .3, -2.5, .8, 2.0, 5.0) +
          frustum(0, .3, -5.0, 5.8, .8, 4.5, .6, 2.6))
    for x, z, hh in ((-3, 4, 3.8), (3, 4, 2.0), (-3, 1.2, 1.2), (3, 1.4, 3.1)):
        o.add("stone_ash", frustum(x, .3, z, .8, .8, .7, .7, hh, sides=8))
    o.add("stone_ash", taper_rod((-1.2, .5, 2.5), (1.8, .5, 3.5), .7, .65, 8))
    for i in range(7):
        o.add("stone_ash", box_base(-3.5 + i * 1.1, 0, 4.8 + (i % 2) * .4, .5 + (i % 3) * .1, .25, .4))
    return o


def altar():
    """The same shape everywhere (the same god): stepped plinth, slab, floating white mark."""
    base = (box_base(0, 0, 0, 3.0, .25, 3.0) + box_base(0, .25, 0, 2.3, .25, 2.3) + box_base(0, .5, 0, 1.6, .6, 1.6) +
            box_base(0, 1.1, 0, 2.0, .2, 2.0))
    a = O("Env_Altar_Altar", ("altar_stone", base), ("trim_silver", box_base(0, 1.3, 0, 1.2, .05, 1.2)))
    m = O("Env_Altar_Mark", ("mark_white", gem(0, 2.5, 0, .35, .65, .35)))
    big = scaled(a, "Env_Altar_Altar_Large", 1.7)
    bigm = O("Env_Altar_Mark_Large", ("mark_white", gem(0, 4.2, 0, .6, 1.1, .6)))
    return [a, m, big, bigm]


# ---------------------------------------------------------------- capital


def castle(style):
    s = STY[style]
    light = style == "Light"
    kh = 12 if light else 9
    o = O("Env_Capital_Castle_%s" % style, (s["stone"], box_base(0, 0, 0, 26, .8, 22)))
    o.add(s["stone"], box_base(0, .8, -2, 14, kh, 12))
    o.add(s["trim"], box_base(0, .8 + kh * .55, 4.02, 14.1, .25, .1))
    if light:
        o.add(s["roof"], gable(0, .8 + kh, -2, 14, 12, 5.5, .5, "z"))
        o.add(s["stone"], box_base(0, .8 + kh, -2, 5, 3, 5) + frustum(0, .8 + kh + 3, -2, 5, 5, 3, 3, 4))
        o.add(s["roof"], frustum(0, .8 + kh + 7, -2, 3.6, 3.6, 0, 0, 7))
        o.add(s["trim"], spike((0, .8 + kh + 14, -2), (0, .8 + kh + 16.5, -2), .3))
    else:
        o.add(s["roof"], gable(0, .8 + kh, -2, 14, 12, 2.0, .6, "z"))
        o.add(s["stone"], box_base(0, .8 + kh, -2, 5, 3, 5))
        for i in range(10):
            a = 2 * math.pi * i / 10
            o.add(s["trim"], spike((math.sin(a) * 2.2, .8 + kh + 3, -2 + math.cos(a) * 2.2),
                                   (math.sin(a) * 2.6, .8 + kh + 6.5 + (i % 2), -2 + math.cos(a) * 2.6), .45))
    for sx in (-1, 1):
        for sz in (-1, 1):
            tx, tz = sx * 11, sz * 8 - 1
            th = 10 if light else 7
            o.add(s["stone"], frustum(tx, .8, tz, 4, 4, 3.5, 3.5, th, sides=8, rot=.4))
            if light:
                o.add(s["roof"], cone(tx, .8 + th, tz, 2.6, 5.5, 8, .4))
                o.add(s["trim"], spike((tx, .8 + th + 5.5, tz), (tx, .8 + th + 7.0, tz), .2))
            else:
                o.add(s["roof"], _battlements(style, 3.6, 3.6, .8 + th, 8, s["roof"]))
                for i in range(5):
                    a = 2 * math.pi * i / 5
                    o.add(s["trim"], spike((tx + math.sin(a) * 1.2, .8 + th, tz + math.cos(a) * 1.2),
                                           (tx + math.sin(a) * 1.4, .8 + th + 2.4, tz + math.cos(a) * 1.4), .35))
    o.add("door", box(0, 3.3, 4.05, 3.2, 5.0, .2))
    o.add(s["trim"], box(0, 6.0, 4.1, 4.0, .4, .2))
    for z in (-6.0 + 12 * 0, ):
        pass
    for sx in (-1, 1):
        o.add(s["cloth"], box_base(sx * 4.5, 4.0, 4.08, 1.0, 5.0, .08))
        o.add(s["glow"], box(sx * 4.5, 5.2, 4.14, .4, 1.0, .06))
    return o


def gate_tower(style):
    s = STY[style]
    h = 8.0 if style == "Light" else 6.0
    o = O("Env_Capital_GateTower_%s" % style)
    for sx in (-1, 1):
        o.add(s["stone"], box_base(sx * 2.6, 0, 0, 2.6, h, 4.2))
    o.add(s["stone"], box_base(0, h - 2.2, 0, 8.0, 2.2, 4.2))
    o.add(s["trim"], box_base(0, h - 2.25, 0, 3.0, .15, 4.3))
    o.add(s["stone"], _battlements(style, 8, 4.2, h, 8, s["stone"]))
    for i in range(5):
        o.add(s["stone"], box_base(-3.4 + i * 1.7, h, 2.0, .9, .6, .5))
    o.add(s["glow"], box(-2.6, h * .6, 2.12, .35, .9, .06) + box(2.6, h * .6, 2.12, .35, .9, .06))
    if style == "Dark":
        for sx in (-1, 1):
            o.add(s["trim"], spike((sx * 2.6, h + .6, 0), (sx * 2.6, h + 3.2, 0), .5))
    else:
        for sx in (-1, 1):
            o.add(s["roof"], cone(sx * 2.6, h + .6, 0, 1.3, 3.4, 4, math.pi / 4))
    return o


def carpet(style):
    return O("Env_Capital_Carpet_%s" % style, ("carpet_light" if style == "Light" else "carpet_dark", box_base(0, 0, 0, 2.4, .04, 6.0)),
             (STY[style]["trim"], box_base(-1.15, .02, 0, .1, .03, 6.0) + box_base(1.15, .02, 0, .1, .03, 6.0)))


# ---------------------------------------------------------------- throne room


def throne(style):
    s = STY[style]
    o = O("Env_ThroneRoom_Throne_%s" % style, (s["stone"], box_base(0, 0, 0, 2.0, .9, 1.8)),
          (s["stone"], box_base(0, .9, -.6, 1.6, 2.4 if style == "Light" else 1.8, .35)),
          (s["trim"], box_base(0, .9, .0, 1.4, .1, 1.1)))
    for sx in (-1, 1):
        o.add(s["stone"], box_base(sx * .8, .9, 0, .25, .9, 1.3))
    if style == "Light":
        o.add(s["trim"], spike((0, 3.3, -.6), (0, 4.4, -.6), .5))
        o.add(s["trim"], spike((-.6, 3.3, -.6), (-.9, 4.0, -.6), .3) + spike((.6, 3.3, -.6), (.9, 4.0, -.6), .3))
    else:
        for sx in (-1, 0, 1):
            o.add(s["trim"], spike((sx * .65, 2.7, -.6), (sx * .8, 3.9 + (0 if sx else .5), -.6), .4))
    return o


def hall_objects():
    out = [O("Env_ThroneRoom_Dais", ("stone_ash", box_base(0, 0, 0, 7, .3, 5) + box_base(0, .3, -.3, 5.6, .3, 4)),
             ("trim_silver", box_base(0, .6, 1.9, 5.6, .05, .1)))]
    for st in ("Light", "Dark"):
        s = STY[st]
        h = 6.5 if st == "Light" else 5.0
        w = .8 if st == "Light" else 1.2
        out.append(O("Env_ThroneRoom_HallPillar_%s" % st, (s["stone"], box_base(0, 0, 0, w + .6, .4, w + .6)),
                     (s["stone"], frustum(0, .4, 0, w, w, w * .9, w * .9, h, sides=8, rot=.2)),
                     (s["trim"], box_base(0, h + .4, 0, w + .4, .3, w + .4))))
    out.append(O("Env_ThroneRoom_Banner_Light", ("wood", taper_rod((0, 0, 0), (0, 4.2, 0), .12, .1, 6)),
                 ("trim_gold", box_base(-.6, 3.0, 0, 1.2, .08, .06)), ("cloth_light", box_base(-.55, 1.2, 0, 1.1, 1.9, .05)),
                 ("trim_gold", gem(0, 4.35, 0, .15, .25, .15))))
    out.append(O("Env_ThroneRoom_Brazier_Dark", ("iron", frustum(0, 0, 0, .9, .9, .5, .5, 1.1, sides=6)),
                 ("iron", frustum(0, 1.1, 0, .5, .5, 1.1, 1.1, .4, sides=6)), ("glow_violet", gem(0, 1.8, 0, .4, .6, .4))))
    out.append(O("Env_ThroneRoom_SpearRack_Light", ("wood", box_base(0, 0, 0, 2.0, .15, .5) + box_base(0, 1.3, 0, 2.0, .1, .5)),
                 ("trim_silver", spike((-.7, .1, 0), (-.7, 2.3, 0), .08) + spike((0, .1, 0), (0, 2.5, 0), .08) + spike((.7, .1, 0), (.7, 2.3, 0), .08))))
    return out


# ---------------------------------------------------------------- village props


def village_props():
    out = []
    out.append(O("Env_Props_NoticeBoard", ("wood_dark", box_base(-.9, 0, 0, .15, 2.2, .15) + box_base(.9, 0, 0, .15, 2.2, .15)),
                 ("wood", box_base(0, 1.0, 0, 2.2, 1.2, .1)), ("canvas", box_base(-.6, 1.3, .07, .5, .6, .02) + box_base(.2, 1.15, .07, .45, .5, .02) +
                                                                  box_base(.7, 1.4, .07, .3, .5, .02)),
                 ("wood_dark", gable(0, 2.2, 0, 2.0, .7, .5, .1))))
    out.append(O("Env_Props_Well", ("stone_ash", frustum(0, 0, 0, 1.6, 1.6, 1.5, 1.5, .9, sides=8)), ("wood_dark", box_base(-.8, .9, 0, .12, 1.6, .12) + box_base(.8, .9, 0, .12, 1.6, .12)),
                 ("wood", gable(0, 2.5, 0, 1.8, 1.2, .55, .2)), ("wood_dark", taper_rod((-.8, 2.0, 0), (.8, 2.0, 0), .08, .08, 6))))
    out.append(O("Env_Props_Bench", ("wood", box_base(0, .45, 0, 1.8, .1, .5) + box_base(-.7, 0, 0, .12, .45, .45) + box_base(.7, 0, 0, .12, .45, .45) +
                                    box_base(0, .55, -.22, 1.8, .45, .08))))
    for st, nm in (("Light", "Light"), ("Dark", "Dark")):
        s = STY[st]
        out.append(O("Env_Props_LampPost_%s" % nm, ("iron", frustum(0, 0, 0, .3, .3, .15, .15, .25, sides=6) + taper_rod((0, .2, 0), (0, 2.8, 0), .1, .08, 6)),
                     (s["glow"], gem(0, 3.1, 0, .22, .32, .22)), (s["trim"], box_base(0, 2.8, 0, .4, .08, .4) +
                                                                  (spike((0, 3.45, 0), (0, 3.9, 0), .16) if st == "Dark" else cone(0, 3.4, 0, .26, .4, 4, math.pi / 4)))))
    out.append(O("Env_Props_Fence_Segment", ("wood", box_base(-1.0, 0, 0, .12, 1.0, .12) + box_base(1.0, 0, 0, .12, 1.0, .12) + box_base(0, .35, 0, 2.0, .09, .06) +
                                            box_base(0, .7, 0, 2.0, .09, .06))))
    out.append(O("Env_Props_Crate", ("wood", box_base(0, 0, 0, .8, .8, .8)), ("wood_dark", box_base(0, .35, 0, .84, .08, .84))))
    out.append(O("Env_Props_Barrel", ("wood", frustum(0, 0, 0, .7, .7, .6, .6, .5, sides=8) + frustum(0, .5, 0, .6, .6, .7, .7, .5, sides=8)),
                 ("iron", frustum(0, .2, 0, .72, .72, .72, .72, .05, sides=8) + frustum(0, .75, 0, .72, .72, .72, .72, .05, sides=8))))
    out.append(O("Env_Props_Cart", ("wood", box_base(0, .55, 0, 1.8, .15, 1.0) + box_base(0, .7, .5, 1.8, .4, .08) + box_base(0, .7, -.5, 1.8, .4, .08)),
                 ("wood_dark", frustum(.9, .5, .0, .1, .1, .1, .1, .1) + rod_ring(0, .45, .6, .42, .07, 8, "yz") + rod_ring(0, .45, -.6, .42, .07, 8, "yz")),
                 ("wood", taper_rod((0, .6, .5), (0, .6, 2.1), .08, .08, 4)),
                 ("canvas", box_base(0, .7, 0, 1.4, .3, .8))))
    out.append(O("Env_Props_Waystone", ("stone_ash", box_base(0, 0, 0, 1.6, .3, 1.6) + frustum(0, .3, 0, 1.0, 1.0, .7, .7, 2.2, sides=6)),
                 ("trim_silver", box_base(0, 1.3, 0, 1.06, .08, 1.06)), ("mark_white", gem(0, 3.4, 0, .28, .45, .28))))
    out.append(O("Env_Props_Signpost", ("wood", taper_rod((0, 0, 0), (.05, 2.6, 0), .18, .14, 5)),
                 ("wood", box_base(-.55, 2.0, .1, 1.3, .3, .06) + box_base(.45, 1.55, .1, 1.1, .28, .06)),
                 ("wood_dark", spike((.7, 2.0, .1), (1.0, 2.15, .1), .3))))
    out.append(O("Env_Props_NoteStone", ("stone_ash", frustum(0, 0, 0, 1.0, .4, .8, .3, 1.4)), ("stone_ash_dark", box_base(0, .75, .2, .55, .35, .02))))
    out.append(O("Env_Props_LostLantern", ("iron", frustum(0, 0, 0, .3, .3, .26, .26, .06) + frustum(0, .36, 0, .28, .28, .02, .02, .1) + rod_ring(0, .5, 0, .12, .025, 6, "yz")),
                 ("glow_window", box_base(0, .06, 0, .22, .3, .22))))
    out.append(O("Env_Props_Campfire", ("rock_ash", ring_copies(box_base(.55, 0, 0, .3, .22, .3), 7)),
                 ("wood_dark", taper_rod((-.35, .1, -.2), (.3, .35, .25), .1, .1, 5) + taper_rod((.3, .1, -.3), (-.2, .4, .3), .1, .1, 5)),
                 ("fire", cone(0, .15, 0, .3, .8, 5) + cone(.15, .1, .1, .18, .55, 4))))
    out.append(O("Env_Props_Tent", ("canvas", gable(0, 0, 0, 2.4, 2.6, 1.8, .1, "x") if False else gable(0, 0, 0, 2.2, 2.8, 1.7, .05, "z")),
                 ("wood_dark", spike((-1.1, 0, 1.4), (-1.1, 1.9, 1.4), .06)), ("door", box_base(0, 0, 1.38, .8, 1.0, .03))))
    for st in ("Light", "Dark"):
        s = STY[st]
        out.append(O("Env_Props_FlagPole_%s" % st, ("wood", taper_rod((0, 0, 0), (0, 4.5, 0), .1, .08, 6)),
                     (s["cloth"], box_base(.05, 2.8, 0, .08, 1.5, 1.0)), (s["trim"], gem(0, 4.65, 0, .12, .2, .12))))
    out.append(O("Env_Props_Gravestone", ("stone_ash", box_base(0, 0, 0, .8, 1.0, .25) + sphere(0, 1.0, 0, .4, .25, .125, seg=6, rings=2, th1=math.pi / 2))))
    return out


# ---------------------------------------------------------------- landmarks and arena


def chess_piece(kind, colour):
    mat = "white_stone" if colour == "White" else "black_stone"
    f = cylinder(0, 0, 0, .55, .3, 8)
    if kind == "Pawn":
        f += frustum(0, .3, 0, .6, .6, .3, .3, .8, sides=8) + sphere(0, 1.35, 0, .3, .3, .3, seg=8, rings=4)
    elif kind == "Bishop":
        f += frustum(0, .3, 0, .6, .6, .3, .3, 1.4, sides=8) + sphere(0, 2.05, 0, .32, .5, .32, seg=8, rings=4) + spike((0, 2.5, 0), (0, 2.85, 0), .16)
        f += box_base(0, 1.7, .24, .08, .6, .06)
    elif kind == "Rook":
        f += frustum(0, .3, 0, .62, .62, .46, .46, 1.3, sides=8) + frustum(0, 1.6, 0, .46, .46, .8, .8, .25, sides=8) + cylinder(0, 1.85, 0, .4, .3, 8)
        for i in range(4):
            a = i * math.pi / 2
            f += box_base(math.sin(a) * .34, 2.15, math.cos(a) * .34, .22, .25, .22)
    elif kind == "King":
        f += frustum(0, .3, 0, .7, .7, .35, .35, 1.7, sides=8) + frustum(0, 2.0, 0, .35, .35, .75, .75, .3, sides=8) + cylinder(0, 2.3, 0, .38, .35, 8)
        f += box_base(0, 2.65, 0, .16, .7, .16) + box_base(0, 3.0, 0, .5, .16, .16)
    elif kind == "Knight":
        f += frustum(0, .3, 0, .6, .6, .4, .4, .8, sides=8)
        f += taper_rod((0, 1.0, 0), (0, 1.9, .2), .4, .34, 4) + taper_rod((0, 1.9, .2), (0, 1.95, .75), .34, .22, 4)
        f += spike((-.1, 2.05, .1), (-.1, 2.4, -.05), .1) + spike((.1, 2.05, .1), (.1, 2.4, -.05), .1)
    return O("Env_Arena_Chess_%s_%s" % (kind, colour), (mat, f))


def arena_objects():
    out = [O("Env_Arena_Wall_Segment", ("stone_ash", box_base(0, 0, 0, 4.0, 3.2, .9)), ("stone_ash_dark", box_base(0, 3.2, 0, 4.1, .3, 1.0)),
             ("stone_ash", box_base(-1.7, 3.5, 0, .5, .6, .6) + box_base(0, 3.5, 0, .5, .6, .6) + box_base(1.7, 3.5, 0, .5, .6, .6))),
           O("Env_Arena_Threshold_Pillar", ("stone_ash_dark", box_base(0, 0, 0, 1.8, .6, 1.8)),
             ("stone_ash", frustum(0, .6, 0, 1.2, 1.2, 1.0, 1.0, 7.0, sides=8, rot=.2)), ("stone_ash_dark", box_base(0, 7.6, 0, 1.6, .5, 1.6)),
             ("trim_silver", spike((0, 8.1, 0), (0, 9.0, 0), .5))),
           O("Env_Arena_Board_Tile_White", ("white_stone", box_base(0, 0, 0, 2.0, .12, 2.0))),
           O("Env_Arena_Board_Tile_Black", ("black_stone", box_base(0, 0, 0, 2.0, .12, 2.0)))]
    for c in ("White", "Black"):
        for k in ("Pawn", "Bishop", "Rook", "King", "Knight"):
            out.append(chess_piece(k, c))
    king = chess_piece("King", "White").copy_as("Env_Arena_KingStatue_Giant", lambda fs: scale(fs, 3.2))
    out.append(king)
    return out


def landmark_objects():
    out = []
    f = box_base(0, 0, 0, 3.0, .8, 8.0)
    sta = (frustum(0, 1.0, -1.4, 1.2, .9, 1.6, 1.0, 2.6) if False else taper_rod((0, 1.4, -3.0), (0, 1.7, 1.0), 1.4, 1.1, 6))
    out.append(O("Env_Landmark_FallenKing", ("stone_ash_dark", f), ("stone_ash", sta),
                 ("stone_ash", sphere(0, 1.9, 2.0, .8, .85, .8, seg=7, rings=4)),
                 ("stone_ash", taper_rod((-.7, 1.3, -.4), (-1.4, .9, 1.8), .55, .4, 5)),
                 ("trim_silver", rod_ring(0, 2.5, 2.0, .6, .1, 6) if False else spike((-.3, 2.6, 2.0), (-.3, 3.1, 2.0), .12) + spike((.3, 2.6, 2.0), (.3, 3.1, 2.0), .12) + spike((0, 2.65, 2.0), (0, 3.4, 2.0), .12))))
    for st in ("Light", "Dark"):
        s = STY[st]
        blade = taper_rod((0, 0, 0), (.1, 1.7, 0), .22, .12, 4) if st == "Light" else taper_rod((0, 0, 0), (-.1, 1.5, 0), .26, .1, 4)
        out.append(O("Env_Landmark_BrokenSword_%s" % st, ("trim_silver" if st == "Light" else "iron", blade),
                     (s["trim"], box_base(.1, 1.7, 0, .7, .1, .15)), ("rock_ash", rubble_pile().faces and [f for _, f in rubble_pile().faces[:3]])))
    out.append(O("Env_Landmark_BrokenBanner", ("wood", taper_rod((0, 0, 0), (.6, 2.2, 0), .1, .08, 5)), ("cloth_light", box_base(.4, 1.0, 0, .08, .8, .6)),
                 ("cloth_dark", box_base(.2, .4, .0, .08, .6, .5))))
    out.append(O("Env_Landmark_BrokenShield", ("iron", rotate(box(0, .45, 0, .9, 1.0, .08), 70, 0, 20, (0, .45, 0))), ("trim_silver", rotate(box(0, .5, .06, .3, .4, .05), 70, 0, 20, (0, .45, 0)))))
    out.append(O("Env_Landmark_RiftPit", ("ground_scar", frustum(0, -.05, 0, 7.0, 6.0, 6.0, 5.0, .06, sides=9, rot=.2)),
                 ("ash_mist", frustum(0, .06, 0, 5.0, 4.5, 4.0, 3.5, .1, sides=9)),
                 ("rock_dark", [f for i in range(9) for f in spike((math.sin(i * .7) * 3.4, 0, math.cos(i * .7) * 2.9), (math.sin(i * .7) * 3.6, .6 + (i % 3) * .35, math.cos(i * .7) * 3.1), .6)])))
    mound = sphere(0, 0, 0, 4.2, 2.6, 3.6, seg=8, rings=4, jitter=.14, rng=random.Random(2), th1=math.pi / 2)
    out.append(O("Env_Landmark_HideoutMound", ("rock_ash", mound), ("door", box(0, .9, 3.3, 1.7, 1.8, .6)),
                 ("rock_dark", box(0, 1.95, 3.1, 2.4, .5, .8) + box_base(-1.1, 0, 3.1, .45, 1.7, .6) + box_base(1.1, 0, 3.1, .45, 1.7, .6))))
    out.append(O("Env_Landmark_PilgrimStone", ("stone_light", box_base(-.8, 0, 0, 1.0, 1.9, .4)), ("stone_dark", box_base(.8, 0, 0, 1.0, 1.9, .4)),
                 ("mark_white", gem(0, 1.5, 0, .18, .32, .18)), ("altar_stone", box_base(0, 0, 0, .8, .35, .5))))
    return out


def build_all():
    trees = [tree("dead", i) for i in range(3)] + [tree("light", 0), tree("dark", 0), stump(), fallen_log(), bush("dead"), bush("thorn")]
    rocks = [boulder(sz, v) for sz in "SML" for v in (0, 1)] + [rock_spire(), rock_slab(), pebbles(), crystals("Dark"), crystals("Light")]
    ruins = [rubble_pile(), column_fallen()]
    for st in ("Light", "Dark", "Ashen"):
        ruins += [wall_segment(st), wall_segment(st, True), pillar(st), pillar(st, True), tower(st)]
    ruins.append(tower("Ashen", True))
    houses = [house(st, v) for st in ("Light", "Dark", "Border") for v in "AB"] + [house_ruin("A"), house_ruin("B")]
    temples = [temple("Light"), temple("Dark"), temple("Ruined")]
    for st in ("Light", "Dark"):
        temples.append(scaled(temple(st), "Env_Temple_Great_%s" % st, 1.5))
    capital = [castle("Light"), castle("Dark"), gate_tower("Light"), gate_tower("Dark"), carpet("Light"), carpet("Dark")]
    room = [throne("Light"), throne("Dark")] + hall_objects()
    return {
        "Env_Trees": trees, "Env_Rocks": rocks, "Env_Ruins": ruins, "Env_Houses": houses, "Env_Temples": temples,
        "Env_Altar": altar(), "Env_Capital": capital, "Env_ThroneRoom": room, "Env_Props": village_props(),
        "Env_Landmarks": landmark_objects(), "Env_Arena": arena_objects(),
    }
