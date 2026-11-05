"""NPC parts: three heads tall, built to be mixed.

Pick one of each slot and stack them on the same origin (feet centre):
  gender (Body) -> hair -> alignment mark -> outfit, plus optional
  headgear, beard and a hand prop. Colour comes from the npc_* slots.

Body and outfits are split into Head/Torso/ArmL/ArmR/LegL/LegR so the game
can swing arms and legs on the joints listed in JOINTS.
"""
import math
from lowpoly import *

GENDERS = ["Male", "Female"]
HAIRS = ["Short", "Long"]
MARKS = ["Light", "Dark", "Ashen"]
OUTFITS = ["Clothes", "Armor", "MageRobe"]
HEADGEARS = ["Hood", "Helm", "Mitre", "Crown", "Cap", "Circlet"]
BEARDS = ["Short", "Long"]
PROPS = ["Staff", "Cane", "Lantern", "Backpack", "Hammer"]
PARTS = ["Head", "Torso", "ArmL", "ArmR", "LegL", "LegR"]

HEAD_Y = 1.50   # head centre; 1.8 m tall in all, the head 0.6 m = three heads
G = {
    "Male": dict(legX=.11, legW=.17, legD=.19, tbw=.38, ttw=.50, td=.26, shX=.285, armW=.12, armD=.13,
                 hr=(.285, .30, .28)),
    "Female": dict(legX=.095, legW=.15, legD=.17, tbw=.34, ttw=.40, td=.22, shX=.235, armW=.10, armD=.11,
                   hr=(.265, .295, .265)),
}

JOINTS = {   # rotate parts around these (model space), rest pose = standing
    "Head": (0.0, 1.14, 0.0),
    "ArmR": lambda g: (g["shX"], 1.10, 0.0),
    "ArmL": lambda g: (-g["shX"], 1.10, 0.0),
    "LegR": lambda g: (g["legX"], 0.62, 0.0),
    "LegL": lambda g: (-g["legX"], 0.62, 0.0),
    "Torso": (0.0, 0.60, 0.0),
}
HAND_GRIP = lambda g: (g["shX"], 0.54, 0.02)   # a hand prop's origin sits here (right hand)


def _surface_z(x, y, r):
    rx, ry, rz = r
    t = 1.0 - (x / rx) ** 2 - ((y - HEAD_Y) / ry) ** 2
    return rz * math.sqrt(max(t, 0.0))


def _side(obj_right, mats_faces):
    """Build the left limb by mirroring the right."""
    return [(m, mirror_x(f)) for m, f in mats_faces]


def _named(prefix, items):
    """items: {part: [(mat, faces), ...]} -> list of Obj."""
    out = []
    for part in PARTS:
        if part in items:
            o = Obj("%s_%s" % (prefix, part))
            for m, f in items[part]:
                o.add(m, f)
            out.append(o)
    return out


def _with_left(d):
    d = dict(d)
    d["ArmL"] = _side(None, d["ArmR"])
    d["LegL"] = _side(None, d["LegR"])
    return d


# ---------------------------------------------------------------- bodies


def body(gender):
    g = G[gender]
    rx, ry, rz = g["hr"]
    f_head = [("npc_skin", sphere(0, HEAD_Y, 0, rx, ry, rz, seg=8, rings=4)),
              ("npc_skin", cylinder(0, 1.10, 0, .065, .16, 6))]
    ex = .105 * rx / .285
    for s in (-1, 1):   # eyes, ears
        ey = HEAD_Y + (.02 if gender == "Male" else .01)
        z = _surface_z(ex, ey, g["hr"])
        ew, eh = (.055, .07) if gender == "Male" else (.06, .085)
        f_head.append(("npc_eye", box(s * ex, ey, z - .004, ew, eh, .04)))
        f_head.append(("npc_skin", box(s * (rx - .004), HEAD_Y, 0, .035, .08, .06)))
    f_head.append(("npc_skin", box(0, HEAD_Y - .045, _surface_z(0, HEAD_Y - .045, g["hr"]) + .004, .05, .06, .06)))
    f_head.append(("npc_eye", box(0, HEAD_Y - .125, _surface_z(0, HEAD_Y - .125, g["hr"]) - .004, .07, .012, .03)))

    torso = [("npc_under", frustum(0, .58, 0, g["tbw"], g["td"], g["ttw"], g["td"], .56))]
    arm = [("npc_under", frustum(g["shX"], .60, 0, g["armW"] * .85, g["armD"] * .85, g["armW"], g["armD"], .52)),
           ("npc_skin", box(g["shX"], .54, 0, .10, .12, .10))]
    leg = [("npc_under", frustum(g["legX"], .10, 0, g["legW"] * .9, g["legD"] * .9, g["legW"], g["legD"], .52)),
           ("npc_skin", box(g["legX"], .05, .04, g["legW"] + .01, .10, g["legD"] + .10))]
    d = _with_left({"Head": f_head, "Torso": torso, "ArmR": arm, "LegR": leg})
    return _named("NPC_Body_%s" % gender, d)


# ---------------------------------------------------------------- hair


def hair(style, gender):
    g = G[gender]
    rx, ry, rz = [r * 1.1 for r in g["hr"]]
    cy = HEAD_Y + .01
    f = sphere(0, cy, 0, rx, ry, rz, seg=8, rings=3, th1=math.radians(75))
    th_end = {("Short", "Male"): 105, ("Short", "Female"): 135, ("Long", "Male"): 115, ("Long", "Female"): 115}
    f += sphere(0, cy, 0, rx, ry, rz, seg=6, rings=2, th0=math.radians(75), th1=math.radians(th_end[(style, gender)]),
                ph0=math.pi - 2.2, ph1=math.pi + 2.2)
    f += box(0, cy + ry * .30, rz * .90, rx * 1.4, .10, .08)   # fringe
    if style == "Short" and gender == "Male":
        f += cone(.03, cy + ry * .92, .02, .06, .12, 5)       # tuft
    if style == "Short" and gender == "Female":
        for s in (-1, 1):
            f += box(s * rx * .93, cy - ry * .45, .02, .06, .30, .26)
    if style == "Long":
        if gender == "Male":
            f += frustum(0, 1.05, -rz * .9, .42, .13, .56, .13, .60)
        else:
            f += frustum(0, .72, -rz * .9, .40, .13, .58, .13, .96)
            for s in (-1, 1):
                f += box(s * .315, 1.22, .02, .07, .55, .12)
    o = Obj("NPC_Hair_%s_%s" % (style, gender))
    return [o.add("npc_hair", f)]


def beard(style):
    f = box(0, 1.385, .262, .17, .035, .04)    # moustache
    if style == "Short":
        f += frustum(0, 1.20, .17, .24, .12, .20, .12, .14)
    else:
        f += frustum(0, .98, .17, .10, .10, .26, .14, .26)
        f += frustum(0, 1.20, .17, .26, .14, .22, .13, .10)
    return [Obj("NPC_Beard_%s" % style).add("npc_hair", f)]


# ---------------------------------------------------------------- headgear


def headgear(kind):
    o = Obj("NPC_Head_%s" % kind)
    if kind == "Hood":
        o.add("npc_main", sphere(0, 1.50, 0, .35, .37, .34, seg=8, rings=4, th1=math.radians(125), ph0=.9, ph1=2 * math.pi - .9))
        o.add("npc_main", frustum(0, 1.0, -.20, .50, .20, .28, .20, .55))
        o.add("npc_under", cone(0, 1.80, -.22, .08, .20, 5))
    elif kind == "Helm":
        o.add("npc_metal", sphere(0, 1.51, 0, .33, .34, .32, seg=8, rings=3, th1=math.radians(100)))
        o.add("npc_trim", rod_ring(0, 1.55, 0, .325, .035, 8))
        o.add("npc_metal", box(0, 1.47, .31, .04, .20, .03))
        for s in (-1, 1):
            o.add("npc_metal", box(s * .31, 1.42, .0, .05, .20, .24))
        o.add("npc_accent", box(0, 1.85, -.02, .05, .12, .40))
    elif kind == "Mitre":
        o.add("npc_trim", frustum(0, 1.75, 0, .38, .30, .36, .28, .07))
        o.add("npc_main", frustum(0, 1.81, 0, .35, .26, .08, .06, .46))
        o.add("npc_accent", rotate(box(0, 1.95, .125, .09, .09, .02), 0, 0, 45, (0, 1.95, .125)))
    elif kind == "Crown":
        o.add("npc_metal", rod_ring(0, 1.80, 0, .27, .05, 8))
        for i in range(6):
            a = 2 * math.pi * i / 6
            p = (.27 * math.sin(a), 1.80, .27 * math.cos(a))
            o.add("npc_metal", spike(p, (p[0], 1.80 + (.20 if i % 2 == 0 else .14), p[2]), .07))
        o.add("npc_accent", gem(0, 1.83, .27, .035))
    elif kind == "Cap":
        o.add("npc_leather", sphere(0, 1.53, 0, .33, .30, .32, seg=8, rings=3, th1=math.radians(85)))
        o.add("npc_leather", box(0, 1.62, .30, .36, .03, .20))
        o.add("npc_trim", cylinder(0, 1.83, 0, .03, .04, 5))
    elif kind == "Circlet":
        o.add("npc_metal", rod_ring(0, 1.70, 0, .285, .025, 8))
        o.add("npc_accent", gem(0, 1.70, .29, .04))
    return [o]


# ---------------------------------------------------------------- alignment marks


def mark(kind):
    o = Obj("NPC_Mark_%s" % kind)
    if kind == "Light":   # straight and angular: a halo with rays
        o.add("npc_accent", rod_ring(0, 1.97, 0, .24, .035, 8))
        for i in range(4):
            a = math.pi / 2 * i + math.pi / 4
            o.add("npc_accent", spike((.27 * math.sin(a), 1.97, .27 * math.cos(a)),
                                      (.40 * math.sin(a), 1.97, .40 * math.cos(a)), .05))
    elif kind == "Dark":   # low and sharp: swept-back horns
        horn = (taper_rod((.17, 1.72, -.02), (.27, 1.90, -.09), .07, .05) +
                taper_rod((.27, 1.90, -.09), (.24, 2.06, -.22), .05, 0.0))
        o.add("npc_accent", horn)
        o.add("npc_accent", mirror_x(horn))
        for s in (-1, 1):
            o.add("npc_accent", spike((s * .12, 1.80, -.10), (s * .12, 1.98, -.16), .05))
    else:   # Ashen: broken ring and drifting shards
        for i in range(3):
            a0 = math.radians(20 + i * 110)
            p0 = (.25 * math.sin(a0), 1.96, .25 * math.cos(a0))
            a1 = a0 + math.radians(70)
            p1 = (.25 * math.sin(a1), 1.96, .25 * math.cos(a1))
            o.add("npc_accent", taper_rod(p0, p1, .03, .03))
        o.add("npc_accent", gem(.30, 1.84, .02, .04, .08, .04))
        o.add("npc_accent", gem(-.26, 2.04, -.12, .035, .07, .035))
        o.add("npc_accent", gem(.02, 2.12, -.20, .03, .06, .03))
    return [o]


# ---------------------------------------------------------------- outfits


def _leg_boots(g, tall=.20):
    lw, ld, x = g["legW"], g["legD"], g["legX"]
    return [("npc_leather", frustum(x, 0, .02, lw + .03, ld + .08, lw + .03, ld + .04, tall)),
            ("npc_leather", box(x, .05, .09, lw + .03, .10, .10))]


def _trousers(g):
    lw, ld, x = g["legW"], g["legD"], g["legX"]
    return ("npc_under", frustum(x, .14, 0, lw * .9 + .03, ld * .9 + .03, lw + .03, ld + .03, .50))


def outfit(kind, gender):
    g = G[gender]
    fem = gender == "Female"
    tbw, ttw, td = g["tbw"], g["ttw"], g["td"]
    sx, aw, ad, lx = g["shX"], g["armW"], g["armD"], g["legX"]
    belt_w = tbw + (ttw - tbw) * .21 + .06
    T, A, L = [], [], []
    if kind == "Clothes":
        if fem:
            T += [("npc_main", frustum(0, .58, 0, tbw + .04, td + .04, ttw + .03, td + .04, .56)),
                  ("npc_main", frustum(0, .30, 0, .52, .40, tbw + .05, td + .05, .30)),
                  ("npc_trim", frustum(0, .30, 0, .535, .415, .52, .40, .04))]
        else:
            T += [("npc_main", frustum(0, .50, 0, tbw + .06, td + .05, ttw + .04, td + .05, .64)),
                  ("npc_trim", frustum(0, .50, 0, tbw + .075, td + .06, tbw + .07, td + .055, .04))]
        T += [("npc_trim", box(0, .70, 0, belt_w, .06, td + .08)),
              ("npc_trim", frustum(0, 1.12, 0, .20, .18, .17, .16, .07, sides=6))]
        A += [("npc_main", frustum(sx, .64, 0, aw * .9 + .03, ad * .9 + .03, aw + .03, ad + .03, .50)),
              ("npc_trim", frustum(sx, .62, 0, aw * .9 + .05, ad * .9 + .05, aw * .9 + .04, ad * .9 + .04, .06))]
        L += [_trousers(g)] + _leg_boots(g)
    elif kind == "Armor":
        skirt = (.50, .38) if fem else (.44, .30)
        T += [("npc_main", frustum(0, .36, 0, skirt[0], skirt[1], tbw + .06, td + .06, .26)),
              ("npc_metal", frustum(0, .60, 0, tbw + .07, td + .07, ttw + .06, td + .07, .52)),
              ("npc_leather", box(0, .64, 0, tbw + .10, .06, td + .10)),
              ("npc_metal", box(0, .50, td / 2 + .04, .24, .22, .04)),
              ("npc_accent", rotate(box(0, .93, td / 2 + .05, .11, .11, .025), 0, 0, 45, (0, .93, td / 2 + .05))),
              ("npc_metal", frustum(0, 1.12, 0, .24, .22, .20, .19, .09, sides=6))]
        A += [("npc_main", frustum(sx, .62, 0, aw * .9 + .025, ad * .9 + .025, aw + .025, ad + .025, .50)),
              ("npc_metal", sphere(sx + .02, 1.10, 0, .17, .11, .16, seg=6, rings=2, th1=math.pi / 2)),
              ("npc_trim", frustum(sx + .02, 1.08, 0, .30, .30, .28, .28, .035, sides=6)),
              ("npc_metal", frustum(sx, .60, 0, aw * .9 + .05, ad * .9 + .05, aw * .95 + .05, ad * .95 + .05, .26))]
        lw, ld = g["legW"], g["legD"]
        L += [_trousers(g),
              ("npc_metal", box(lx, .40, ld / 2 + .03, lw * .8, .10, .05)),
              ("npc_metal", frustum(lx, .12, 0, lw * .9 + .05, ld * .9 + .05, lw * .95 + .05, ld * .95 + .05, .26)),
              ("npc_metal", box(lx, .05, .10, lw + .04, .09, .09))] + _leg_boots(g)[:1]
    else:   # MageRobe
        T += [("npc_main", frustum(0, .22, 0, .64, .50, .46, .34, .50, sides=6)),
              ("npc_main", frustum(0, .70, 0, .46, .34, ttw + .04, td + .04, .44, sides=6)),
              ("npc_trim", frustum(0, .22, 0, .65, .51, .635, .495, .05, sides=6)),
              ("npc_trim", frustum(0, .66, 0, .50, .38, .50, .38, .07, sides=6)),
              ("npc_trim", frustum(0, 1.10, 0, .26, .22, .20, .18, .10, sides=6)),
              ("npc_under", frustum(0, .98, 0, ttw + .18, td + .14, .30, .24, .20, sides=6))]
        A += [("npc_main", frustum(sx, .62, 0, .24, .24, aw + .04, ad + .04, .52, sides=6)),
              ("npc_trim", frustum(sx, .60, 0, .25, .25, .24, .24, .04, sides=6))]
        L += _leg_boots(g, .16)
    d = _with_left({"Torso": T, "ArmR": A, "LegR": L})
    return _named("NPC_Outfit_%s_%s" % (kind, gender), d)


# ---------------------------------------------------------------- hand props and back packs


def prop(kind):
    o = Obj("NPC_Prop_%s" % kind)
    g0 = -.55   # props are modelled on the ground and shifted so the origin is the grip
    if kind == "Staff":
        o.add("npc_wood", move(taper_rod((0, 0, 0), (0, 1.5, 0), .05, .04, 6), 0, g0, 0))
        o.add("npc_metal", move(rod_ring(0, 1.46, 0, .075, .025, 6), 0, g0, 0))
        o.add("npc_accent", move(gem(0, 1.64, 0, .07, .13, .07), 0, g0, 0))
        o.add("npc_metal", move(cone(0, -.06, 0, .03, .08, 5), 0, g0 + .06, 0))
    elif kind == "Cane":
        o.add("npc_wood", move(taper_rod((0, 0, 0), (0, .80, 0), .045, .04, 6), 0, g0, 0))
        o.add("npc_wood", move(taper_rod((0, .80, 0), (0, .88, .08), .04, .04, 6) +
                               taper_rod((0, .88, .08), (0, .80, .16), .04, .04, 6), 0, g0, 0))
    elif kind == "Lantern":
        o.add("npc_metal", taper_rod((0, 0, 0), (0, -.06, 0), .02, .02))
        o.add("npc_metal", rod_ring(0, -.07, 0, .05, .015, 6, "yz"))
        o.add("npc_metal", frustum(0, -.30, 0, .15, .15, .13, .13, .03))
        o.add("npc_accent", box(0, -.20, 0, .10, .14, .10))
        o.add("npc_metal", frustum(0, -.12, 0, .15, .15, .02, .02, .05, sides=4))
    elif kind == "Backpack":   # on the back; origin stays at the body origin
        o.add("npc_leather", box(0, .90, -.28, .40, .52, .24))
        o.add("npc_leather", box(0, 1.19, -.29, .42, .08, .26))
        o.add("npc_main", taper_rod((-.25, 1.30, -.30), (.25, 1.30, -.30), .17, .17, 8))
        o.add("npc_metal", cylinder(.19, .55, -.34, .09, .12, 6))
        for s in (-1, 1):
            o.add("npc_leather", box(s * .14, .90, .15, .045, .50, .02))
            o.add("npc_leather", box(s * .14, 1.14, -.02, .045, .02, .30))
    elif kind == "Hammer":
        o.add("npc_wood", move(taper_rod((0, 0, 0), (0, .85, 0), .05, .05, 6), 0, g0, 0))
        o.add("npc_metal", move(box(0, .88, 0, .30, .16, .16), 0, g0, 0))
    return [o]


def build_all():
    """{file: [Obj, ...]} for every NPC slot library."""
    return {
        "NPC_Body": body("Male") + body("Female"),
        "NPC_Hair": [h for gd in GENDERS for st in HAIRS for h in hair(st, gd)],
        "NPC_Beard": [b for s in BEARDS for b in beard(s)],
        "NPC_Headgear": [h for k in HEADGEARS for h in headgear(k)],
        "NPC_Mark": [m for k in MARKS for m in mark(k)],
        "NPC_Outfit": [o for k in OUTFITS for gd in GENDERS for o in outfit(k, gd)],
        "NPC_Prop": [p for k in PROPS for p in prop(k)],
    }
