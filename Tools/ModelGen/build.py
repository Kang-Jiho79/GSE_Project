"""Builds everything under Assets/Models:

  Materials.mtl            named material slots
  NPC/NPC_<Slot>.obj       one library per part slot, every part a named object
  Env/Env_<Group>.obj      world models, every model a named object
  NpcParts.json            slot lists, mesh-name rules, joints, palettes
  NpcPresets.json          per-NPC part indices (what the game will read)
  EnvModels.json           every world model: file, name, size, triangles
  Preview/*.png            software-rendered contact sheets

Run:  python Tools/ModelGen/build.py [--no-preview]
"""
import json
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(ROOT, "Assets", "Models")

import env_models
import materials as M
import npc_parts as NP
import npc_presets as PR
import render as R
from lowpoly import move, write_obj


def mesh_names(p):
    """The mesh objects one preset is built from, in draw order."""
    g, h = p["gender"], p["hair"]
    names = ["NPC_Body_%s_%s" % (g, part) for part in NP.PARTS]
    names.append("NPC_Hair_%s_%s" % (h, g))
    if p["mark"]:
        names.append("NPC_Mark_%s" % p["mark"])
    names += ["NPC_Outfit_%s_%s_%s" % (p["outfit"], g, part) for part in NP.PARTS if part != "Head"]
    if p["head"]:
        names.append("NPC_Head_%s" % p["head"])
    if p["beard"]:
        names.append("NPC_Beard_%s" % p["beard"])
    if p["prop"]:
        names.append("NPC_Prop_%s" % p["prop"])
    return names


def index_of(lst, v):
    return -1 if v is None else lst.index(v)


def preset_record(p):
    return {
        "id": p["id"], "name": p["name"], "kind": p["kind"], "region": p["region"], "dialogue": p["dialogue"],
        "parts": {   # indices into the lists of NpcParts.json; -1 = none
            "gender": index_of(NP.GENDERS, p["gender"]),
            "hair": index_of(NP.HAIRS, p["hair"]),
            "mark": index_of(NP.MARKS, p["mark"]),
            "outfit": index_of(NP.OUTFITS, p["outfit"]),
            "headgear": index_of(NP.HEADGEARS, p["head"]),
            "beard": index_of(NP.BEARDS, p["beard"]),
            "prop": index_of(NP.PROPS, p["prop"]),
        },
        "colors": {"palette": [x[0] for x in M.PALETTES].index(p["palette"]), "skin": p["skin"], "hairColor": p["hairColor"]},
        "height": p["height"],
        "meshes": mesh_names(p),
    }


def check_unique(presets):
    seen = {}
    for p in presets:
        key = (p["gender"], p["hair"], p["mark"], p["outfit"], p["head"], p["beard"], p["prop"], p["palette"], p["skin"],
               p["hairColor"], p["height"])
        if key in seen:
            raise SystemExit("NPCs look identical: %s and %s" % (seen[key], p["id"]))
        seen[key] = p["id"]


def parts_manifest():
    g = NP.G["Male"]
    joints = {}
    for gd in NP.GENDERS:
        gg = NP.G[gd]
        joints[gd] = {k: list(v(gg) if callable(v) else v) for k, v in NP.JOINTS.items()}
        joints[gd]["HandGrip"] = list(NP.HAND_GRIP(gg))
    return {
        "convention": "Y up, +Z forward, metres, origin = ground point between the feet; every part shares that origin",
        "heightMetres": 1.8, "headHeadsTall": 3,
        "slots": {
            "gender": NP.GENDERS, "hair": NP.HAIRS, "mark": NP.MARKS, "outfit": NP.OUTFITS,
            "headgear": NP.HEADGEARS, "beard": NP.BEARDS, "prop": NP.PROPS,
        },
        "slotNote": "A part index of -1 means none. mark is the alignment part (-1 for borderborn and travellers).",
        "parts": NP.PARTS,
        "meshNames": {
            "body": "NPC_Body_{gender}_{part}",
            "hair": "NPC_Hair_{hair}_{gender}",
            "mark": "NPC_Mark_{mark}",
            "outfit": "NPC_Outfit_{outfit}_{gender}_{part}",
            "headgear": "NPC_Head_{headgear}",
            "beard": "NPC_Beard_{beard}",
            "prop": "NPC_Prop_{prop}",
        },
        "files": {"body": "NPC/NPC_Body.obj", "hair": "NPC/NPC_Hair.obj", "mark": "NPC/NPC_Mark.obj",
                  "outfit": "NPC/NPC_Outfit.obj", "headgear": "NPC/NPC_Headgear.obj", "beard": "NPC/NPC_Beard.obj",
                  "prop": "NPC/NPC_Prop.obj"},
        "propAttach": {"Staff": "HandGrip", "Cane": "HandGrip", "Lantern": "HandGrip", "Hammer": "HandGrip",
                       "Backpack": "Torso (origin stays at the body origin)"},
        "joints": joints,
        "materialSlots": list(M.NPC_SLOTS.keys()),
        "palettes": [{"name": n, "colors": {k: list(v) for k, v in c.items()}} for n, c in M.PALETTES],
        "skinTones": [{"name": n, "color": list(c)} for n, c in zip(M.SKIN_NAMES, M.SKIN_TONES)],
        "hairColors": [{"name": n, "color": list(c)} for n, c in zip(M.HAIR_NAMES, M.HAIR_COLORS)],
        "emissiveSlots": ["npc_accent"],
        "heightScale": {"Adult": 1.0, "Elder": 0.95, "Child": 0.62},
    }


def write_all(npc_lib, env_lib):
    os.makedirs(os.path.join(OUT, "NPC"), exist_ok=True)
    os.makedirs(os.path.join(OUT, "Env"), exist_ok=True)
    M.write_mtl(os.path.join(OUT, "Materials.mtl"))
    for name, objs in npc_lib.items():
        write_obj(os.path.join(OUT, "NPC", name + ".obj"), objs,
                  "NPC part library: %s. Objects: %s" % (name, ", ".join(o.name for o in objs)), "../Materials.mtl")
    env_json = []
    for name, objs in env_lib.items():
        write_obj(os.path.join(OUT, "Env", name + ".obj"), objs,
                  "World models: %s." % name, "../Materials.mtl")
        for o in objs:
            lo, hi = o.bounds()
            env_json.append({"mesh": o.name, "file": "Env/%s.obj" % name, "triangles": o.tris(),
                             "size": [round(hi[i] - lo[i], 2) for i in range(3)],
                             "min": [round(lo[i], 2) for i in range(3)], "max": [round(hi[i], 2) for i in range(3)]})
    with open(os.path.join(OUT, "NpcParts.json"), "w", encoding="utf-8", newline="\n") as fh:
        json.dump(parts_manifest(), fh, ensure_ascii=False, indent=1)
    recs = [preset_record(p) for p in PR.PRESETS]
    with open(os.path.join(OUT, "NpcPresets.json"), "w", encoding="utf-8", newline="\n") as fh:
        json.dump({"note": "indices into NpcParts.json lists; -1 = none. things = NpcActor kinds that are objects, not people",
                   "npcs": recs, "things": PR.THINGS}, fh, ensure_ascii=False, indent=1)
    rows = ["# NPC 파츠 인덱스 표 (자동 생성 — Tools/ModelGen/build.py)", "",
            "인덱스 목록은 `NpcParts.json`의 slots 순서. `-` = 없음.", "",
            "| id | 이름 | 종류 | 지역 | 성별 | 머리 | 소속 | 옷 | 머리장식 | 수염 | 소품 | 팔레트 | 피부 | 머리색 | 키 |",
            "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|"]
    for r in recs:
        q, c = r["parts"], r["colors"]
        def nm(lst, i):
            return "-" if i < 0 else "%d %s" % (i, lst[i])
        rows.append("| %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %d %s | %d | %d | %.2f |" % (
            r["id"], r["name"], r["kind"], r["region"], nm(NP.GENDERS, q["gender"]), nm(NP.HAIRS, q["hair"]),
            nm(NP.MARKS, q["mark"]), nm(NP.OUTFITS, q["outfit"]), nm(NP.HEADGEARS, q["headgear"]),
            nm(NP.BEARDS, q["beard"]), nm(NP.PROPS, q["prop"]), c["palette"], M.PALETTES[c["palette"]][0],
            c["skin"], c["hairColor"], r["height"]))
    with open(os.path.join(OUT, "NpcPresets.md"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(rows) + "\n")
    with open(os.path.join(OUT, "EnvModels.json"), "w", encoding="utf-8", newline="\n") as fh:
        json.dump({"convention": "Y up, +Z forward (door side), metres, origin = footprint centre on the ground",
                   "models": env_json}, fh, ensure_ascii=False, indent=1)
    return recs, env_json


def preview(npc_lib, env_lib):
    os.makedirs(os.path.join(OUT, "Preview"), exist_ok=True)
    lib = {o.name: o for objs in npc_lib.values() for o in objs}

    def figure(p):
        objs = [lib[n] for n in mesh_names(p)]
        out = []
        sc = p["height"]
        gg = NP.G[p["gender"]]
        for o in objs:
            if o.name.startswith("NPC_Prop_") and o.name != "NPC_Prop_Backpack":
                o = o.copy_as(o.name, lambda fs: move(fs, gg["shX"], .54, .02))
            out.append(o)
        return out, sc

    cols = 10
    cell_w, cell_h = 150, 250
    rows = math.ceil(len(PR.PRESETS) / cols)
    cv = R.Canvas(cols * cell_w, rows * cell_h)
    for i, p in enumerate(PR.PRESETS):
        objs, sc = figure(p)
        colors = M.palette_colors([x[0] for x in M.PALETTES].index(p["palette"]), p["skin"], p["hairColor"])
        colors.update({k: v[0] for k, v in M.ENV.items()})
        cx = (i % cols) * cell_w + cell_w // 2
        cy = (i // cols) * cell_h + cell_h - 20
        R.draw_objs(cv, objs, colors, cx, cy, 105 * sc, 25, 22)
    R.write_png(os.path.join(OUT, "Preview", "NPC_Presets.png"), cv.w, cv.h, cv.pix)
    print("preview NPC_Presets.png")

    allc = {k: v[0] for k, v in M.ENV.items()}
    allc.update({k: v[0] for k, v in M.NPC_SLOTS.items()})
    for gname, objs in env_lib.items():
        n = len(objs)
        cols = min(n, 6)
        rows = math.ceil(n / cols)
        cw = ch = 230
        cv = R.Canvas(cols * cw, rows * ch)
        for i, o in enumerate(objs):
            lo, hi = o.bounds()
            ext = max(hi[0] - lo[0], hi[2] - lo[2], (hi[1] - lo[1]) * 1.1, 0.1)
            s = (cw - 50) / (ext * 1.25)
            cx = (i % cols) * cw + cw // 2
            cy = (i // cols) * ch + ch - 40
            R.draw_objs(cv, [o], allc, cx, cy, s, 35, 28, offset=(-(lo[0] + hi[0]) / 2, 0, -(lo[2] + hi[2]) / 2))
        R.write_png(os.path.join(OUT, "Preview", gname + ".png"), cv.w, cv.h, cv.pix)
        print("preview", gname)


def main():
    check_unique(PR.PRESETS)
    npc_lib = NP.build_all()
    env_lib = env_models.build_all()
    # every mesh a preset names must exist
    have = {o.name for objs in npc_lib.values() for o in objs}
    for p in PR.PRESETS:
        for n in mesh_names(p):
            if n not in have:
                raise SystemExit("%s: missing mesh %s" % (p["id"], n))
    names = [o.name for objs in list(npc_lib.values()) + list(env_lib.values()) for o in objs]
    dup = {n for n in names if names.count(n) > 1}
    if dup:
        raise SystemExit("duplicate mesh names: %s" % sorted(dup))
    recs, env_json = write_all(npc_lib, env_lib)
    print("NPC part meshes: %d (%d tris)" % (sum(len(v) for v in npc_lib.values()),
                                              sum(o.tris() for v in npc_lib.values() for o in v)))
    print("World models: %d (%d tris)" % (len(env_json), sum(e["triangles"] for e in env_json)))
    print("NPC presets: %d" % len(recs))
    if "--no-preview" not in sys.argv:
        preview(npc_lib, env_lib)


if __name__ == "__main__":
    main()
