"""Named material slots shared by every model, and the NPC palettes.

The OBJ files only carry slot names (usemtl). Materials.mtl holds neutral
defaults; the game recolours the `npc_*` slots per NPC from NpcParts.json
(palette, skin tone, hair colour) so a few meshes make many people.
"""

# slot -> (diffuse, emissive or None)
NPC_SLOTS = {
    "npc_main": ((0.50, 0.50, 0.52), None),     # outer cloth: robe, tunic, sleeves
    "npc_under": ((0.36, 0.36, 0.38), None),    # under layer: trousers, body suit
    "npc_trim": ((0.78, 0.80, 0.83), None),     # belt, collar, hem band
    "npc_metal": ((0.62, 0.64, 0.68), None),    # armour plates, crown, helm
    "npc_leather": ((0.34, 0.31, 0.28), None),  # boots, straps, packs
    "npc_accent": ((0.85, 0.88, 0.92), (0.85, 0.88, 0.92)),  # glowing mark, gems
    "npc_skin": ((0.93, 0.78, 0.66), None),
    "npc_hair": ((0.30, 0.20, 0.15), None),
    "npc_eye": ((0.08, 0.08, 0.10), None),
    "npc_wood": ((0.40, 0.28, 0.18), None),     # staves, handles
}

# Palette per look. Index = `palette` in NpcPresets.json.
PALETTES = [
    ("Light", {   # gold and white
        "npc_main": (0.92, 0.90, 0.82), "npc_under": (0.78, 0.72, 0.58), "npc_trim": (0.95, 0.78, 0.30),
        "npc_metal": (0.86, 0.85, 0.80), "npc_leather": (0.55, 0.42, 0.28), "npc_accent": (1.00, 0.85, 0.35)}),
    ("Dark", {    # violet and ink
        "npc_main": (0.20, 0.17, 0.27), "npc_under": (0.12, 0.10, 0.16), "npc_trim": (0.55, 0.32, 0.85),
        "npc_metal": (0.28, 0.27, 0.33), "npc_leather": (0.20, 0.15, 0.14), "npc_accent": (0.65, 0.40, 1.00)}),
    ("Ashen", {   # grey and silver
        "npc_main": (0.50, 0.50, 0.52), "npc_under": (0.36, 0.36, 0.38), "npc_trim": (0.78, 0.80, 0.83),
        "npc_metal": (0.62, 0.64, 0.68), "npc_leather": (0.34, 0.31, 0.28), "npc_accent": (0.85, 0.88, 0.92)}),
    ("Borderborn", {  # worn green-brown, belonging to neither side
        "npc_main": (0.38, 0.45, 0.32), "npc_under": (0.30, 0.27, 0.20), "npc_trim": (0.70, 0.60, 0.35),
        "npc_metal": (0.45, 0.45, 0.42), "npc_leather": (0.40, 0.30, 0.20), "npc_accent": (0.75, 0.80, 0.50)}),
    ("Mixed", {   # travellers: earth tones with a gold and violet thread
        "npc_main": (0.55, 0.45, 0.38), "npc_under": (0.35, 0.30, 0.28), "npc_trim": (0.80, 0.60, 0.55),
        "npc_metal": (0.55, 0.52, 0.50), "npc_leather": (0.38, 0.28, 0.20), "npc_accent": (0.90, 0.70, 0.80)}),
]

SKIN_TONES = [(0.93, 0.78, 0.66), (0.80, 0.62, 0.48), (0.55, 0.38, 0.28), (0.75, 0.74, 0.76)]
SKIN_NAMES = ["Fair", "Tan", "Brown", "AshPale"]
HAIR_COLORS = [(0.90, 0.80, 0.50), (0.10, 0.09, 0.12), (0.60, 0.60, 0.62), (0.40, 0.25, 0.15),
               (0.85, 0.87, 0.90), (0.55, 0.20, 0.15), (0.30, 0.20, 0.45)]
HAIR_NAMES = ["Blonde", "Black", "Gray", "Brown", "SilverWhite", "Auburn", "VioletBlack"]

# World materials: name -> (diffuse, emissive or None)
ENV = {
    "stone_light": ((0.88, 0.86, 0.78), None), "stone_dark": ((0.17, 0.15, 0.22), None),
    "stone_ash": ((0.45, 0.45, 0.46), None), "stone_ash_dark": ((0.30, 0.30, 0.32), None),
    "altar_stone": ((0.80, 0.80, 0.84), None),
    "roof_light": ((0.93, 0.80, 0.42), None), "roof_dark": ((0.22, 0.16, 0.30), None),
    "roof_ash": ((0.32, 0.32, 0.34), None),
    "wood": ((0.40, 0.28, 0.18), None), "wood_dark": ((0.25, 0.17, 0.11), None),
    "thatch": ((0.58, 0.52, 0.34), None), "door": ((0.20, 0.13, 0.09), None),
    "trim_gold": ((0.95, 0.78, 0.30), (0.45, 0.35, 0.10)), "trim_violet": ((0.50, 0.30, 0.80), (0.20, 0.10, 0.35)),
    "trim_silver": ((0.75, 0.77, 0.80), None), "cloth_border": ((0.45, 0.50, 0.35), None),
    "glow_window": ((1.00, 0.85, 0.50), (1.00, 0.80, 0.40)),
    "glow_violet": ((0.70, 0.45, 1.00), (0.55, 0.30, 0.90)),
    "bark_ash": ((0.33, 0.32, 0.31), None), "bark_dark": ((0.20, 0.15, 0.20), None),
    "leaf_light": ((0.88, 0.80, 0.50), None), "thorn_dark": ((0.35, 0.18, 0.45), None),
    "rock_ash": ((0.42, 0.42, 0.43), None), "rock_dark": ((0.22, 0.20, 0.26), None),
    "rock_light": ((0.70, 0.68, 0.62), None),
    "crystal_dark": ((0.55, 0.30, 0.90), (0.35, 0.15, 0.60)),
    "crystal_light": ((1.00, 0.88, 0.45), (0.70, 0.55, 0.15)),
    "mark_white": ((1.00, 1.00, 1.00), (1.00, 1.00, 1.00)),
    "fire": ((1.00, 0.60, 0.20), (1.00, 0.50, 0.10)),
    "iron": ((0.30, 0.30, 0.33), None), "cloth_light": ((0.95, 0.92, 0.80), None),
    "cloth_dark": ((0.25, 0.15, 0.35), None), "canvas": ((0.62, 0.58, 0.48), None),
    "white_stone": ((0.92, 0.92, 0.94), None), "black_stone": ((0.10, 0.10, 0.12), None),
    "carpet_light": ((0.85, 0.70, 0.30), None), "carpet_dark": ((0.30, 0.15, 0.40), None),
    "ash_mist": ((0.55, 0.55, 0.57), None), "ground_scar": ((0.20, 0.20, 0.21), None),
}


def write_mtl(path):
    lines = ["# The Darkest Light - material slots (neutral defaults).",
             "# npc_* slots are recoloured per NPC by the game (see NpcParts.json)."]
    for name, (kd, ke) in list(NPC_SLOTS.items()) + list(ENV.items()):
        lines += ["", "newmtl %s" % name, "Kd %.3f %.3f %.3f" % kd, "Ka 0 0 0", "Ks 0 0 0", "d 1.0"]
        if ke:
            lines.append("Ke %.3f %.3f %.3f" % ke)
    with open(path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")


def palette_colors(palette, skin, hair):
    """Resolved slot colours for one NPC (used by the preview renderer)."""
    cols = {k: v[0] for k, v in NPC_SLOTS.items()}
    cols.update(PALETTES[palette][1])
    cols["npc_skin"] = SKIN_TONES[skin]
    cols["npc_hair"] = HAIR_COLORS[hair]
    return cols
