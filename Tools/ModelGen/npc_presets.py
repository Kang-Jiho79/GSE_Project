"""Which parts each NPC of the game wears.

Fields (names, not indices - build.py turns them into the index record):
  gender  Male | Female           hair    Short | Long
  mark    Light | Dark | Ashen | None   (the alignment part; None for borderborn and travellers)
  outfit  Clothes | Armor | MageRobe
  head    headgear name or None   beard   Short | Long | None   prop  name or None
  palette Light | Dark | Ashen | Borderborn | Mixed   (the npc_* colours)
  skin    0..3                    hairColor 0..6
  height  body scale (1 = adult); the head is scaled with it
"""

N = dict  # shorthand


def P(id, ko, kind, region, dialogue, gender, hair, mark, outfit, palette, skin, hairColor, head=None, beard=None,
      prop=None, height=1.0):
    return N(id=id, name=ko, kind=kind, region=region, dialogue=dialogue, gender=gender, hair=hair, mark=mark,
             outfit=outfit, head=head, beard=beard, prop=prop, palette=palette, skin=skin, hairColor=hairColor,
             height=height)


PRESETS = [
    # ---- Limina and the borderborn
    P("limina_elder", "리미나 장로", "Elder", "Limina", "LiminaElder", "Male", "Short", None, "Clothes", "Borderborn", 1, 2,
      None, "Long", "Cane", .95),
    P("limina_child", "리미나 아이", "Child", "Limina", "LiminaChild", "Female", "Short", None, "Clothes", "Borderborn", 1, 3,
      None, None, None, .62),
    P("wayfarer", "나그네", "Adult", "WayfarerCamp", "Wayfarer", "Male", "Long", None, "Clothes", "Borderborn", 2, 1,
      "Hood", None, "Lantern"),
    P("merchant", "행상", "Merchant", "Wreckage", "MerchantBesieged", "Male", "Short", None, "Clothes", "Mixed", 1, 3,
      "Cap", "Short", "Backpack"),
    # ---- pilgrims at the rest stop
    P("pilgrim_rest_light", "순례자 쉼터의 빛 노인", "Elder", "PilgrimRest", "PilgrimRest", "Female", "Long", "Light", "MageRobe",
      "Light", 0, 4, "Circlet", None, "Cane", .95),
    P("pilgrim_rest_dark", "순례자 쉼터의 어둠 순례자", "Adult", "PilgrimRest", "PilgrimRest", "Male", "Short", "Dark", "Clothes",
      "Dark", 2, 1, "Hood", None, None),
    # ---- light side
    P("rumor_lucius_alba", "알바의 소문꾼", "Adult", "Alba", "RumorLucius", "Male", "Short", "Light", "Clothes", "Light", 0, 0,
      "Cap"),
    P("rumor_lucius_aura", "아우라의 소문꾼", "Adult", "Aura", "RumorLucius", "Female", "Long", "Light", "Clothes", "Light", 1, 5,
      "Circlet"),
    P("alba_mother", "알바의 어머니", "Elder", "Alba", "AlbaMother", "Female", "Short", "Light", "Clothes", "Light", 0, 2,
      None, None, "Cane", .95),
    P("citizen_lumen", "루멘 시민", "Adult", "Lumen", "CitizenLumen", "Male", "Long", "Light", "Armor", "Light", 0, 4, "Helm"),
    P("pilgrim_candida", "칸디다의 순례자", "Adult", "Candida", "PilgrimCandida", "Female", "Long", "Light", "MageRobe", "Light", 1, 0,
      "Hood", None, "Staff"),
    P("hamlet_mother", "이름 없는 마을(빛)의 어머니", "Adult", "LightHamlet", "HamletMother", "Female", "Short", "Light", "Clothes",
      "Light", 1, 3),
    P("treuga_light_neighbour", "트레우가의 빛 이웃", "Adult", "Treuga", "TreugaNeighbours", "Male", "Short", "Light", "Clothes",
      "Light", 0, 5),
    # ---- dark side
    P("rumor_noctis_umbra", "움브라의 소문꾼", "Adult", "Umbra", "RumorNoctis", "Male", "Long", "Dark", "Armor", "Dark", 2, 1, "Cap"),
    P("rumor_noctis_crypta", "크립타의 소문꾼", "Adult", "Crypta", "RumorNoctis", "Female", "Short", "Dark", "Clothes", "Dark", 3, 6),
    P("citizen_tenebrae", "테네브라에 시민", "Adult", "Tenebrae", "CitizenTenebrae", "Male", "Long", "Dark", "MageRobe", "Dark", 1, 1,
      "Hood"),
    P("pilgrim_obscura", "오브스쿠라의 순례자", "Adult", "Obscura", "PilgrimObscura", "Male", "Short", "Dark", "MageRobe", "Dark", 2, 6,
      "Hood", None, "Staff"),
    P("hamlet_smith", "이름 없는 마을(어둠)의 대장장이", "Adult", "DarkHamlet", "HamletSmith", "Male", "Short", "Dark", "Clothes", "Dark", 2, 1,
      None, "Short", "Hammer"),
    P("treuga_dark_neighbour", "트레우가의 어둠 이웃", "Adult", "Treuga", "TreugaNeighbours", "Female", "Long", "Dark", "Clothes",
      "Dark", 1, 6),
    # ---- the villages between, and the ashen ones
    P("vespera_keeper", "베스페라의 저녁지기", "Elder", "Vespera", "EveningKeeper", "Male", "Long", None, "MageRobe", "Mixed", 1, 2,
      "Hood", "Long", "Lantern", .95),
    P("vespera_child", "베스페라의 아이", "Child", "Vespera", "VesperaChild", "Female", "Long", None, "Clothes", "Mixed", 0, 5,
      None, None, None, .62),
    P("treuga_guide", "트레우가의 길잡이", "Adult", "Treuga", "TreugaGuide", "Female", "Short", None, "Clothes", "Mixed", 2, 3,
      "Cap", None, "Backpack"),
    P("refugee_widow", "레푸기움의 미망인", "Adult", "Refugium", "RefugeeWidow", "Female", "Long", "Ashen", "Clothes", "Ashen", 3, 2,
      "Hood"),
    P("refugee_elder", "레푸기움의 노인", "Elder", "Refugium", "RefugeeElder", "Male", "Short", "Ashen", "Clothes", "Ashen", 3, 4,
      None, "Short", "Cane", .95),
    # ---- temple priests (Mitre + robe + staff, told apart by the rest)
    P("priest_alba", "알바의 사제", "Priest", "Alba", "PriestLight", "Male", "Long", "Light", "MageRobe", "Light", 0, 4, "Mitre", "Long", "Staff"),
    P("priest_aura", "아우라의 사제", "Priest", "Aura", "PriestLight", "Female", "Long", "Light", "MageRobe", "Light", 0, 0, "Mitre", None, "Staff"),
    P("priest_lumen", "루멘 대신전의 사제", "Priest", "Lumen", "PriestLight", "Male", "Short", "Light", "MageRobe", "Light", 1, 2, "Mitre", "Short", "Staff"),
    P("priest_candida", "칸디다의 사제", "Priest", "Candida", "PriestLight", "Female", "Short", "Light", "MageRobe", "Light", 1, 5, "Mitre", None, "Staff"),
    P("priest_light_hamlet", "이름 없는 마을(빛)의 사제", "Priest", "LightHamlet", "PriestLight", "Male", "Short", "Light", "MageRobe", "Light", 0, 3, "Mitre", None, "Staff"),
    P("priest_umbra", "움브라의 사제", "Priest", "Umbra", "PriestDark", "Male", "Short", "Dark", "MageRobe", "Dark", 2, 1, "Mitre", "Short", "Staff"),
    P("priest_crypta", "크립타의 사제", "Priest", "Crypta", "PriestDark", "Female", "Long", "Dark", "MageRobe", "Dark", 3, 6, "Mitre", None, "Staff"),
    P("priest_tenebrae", "테네브라에 대신전의 사제", "Priest", "Tenebrae", "PriestDark", "Male", "Long", "Dark", "MageRobe", "Dark", 1, 1, "Mitre", "Long", "Staff"),
    P("priest_obscura", "오브스쿠라의 사제", "Priest", "Obscura", "PriestDark", "Female", "Short", "Dark", "MageRobe", "Dark", 2, 6, "Mitre", None, "Staff"),
    P("priest_dark_hamlet", "이름 없는 마을(어둠)의 사제", "Priest", "DarkHamlet", "PriestDark", "Male", "Long", "Dark", "MageRobe", "Dark", 3, 1, "Mitre", None, "Staff"),
    # ---- the named ones
    P("queen_solena", "빛의 왕비 솔레나", "Queen", "LumenCastle", "(none)", "Female", "Long", "Light", "MageRobe", "Light", 0, 0, "Crown"),
    P("queen_nocta", "어둠의 왕비 녹타", "Queen", "TenebraeCastle", "(none)", "Female", "Short", "Dark", "MageRobe", "Dark", 3, 1, "Crown"),
    P("kinis", "키니스 (임시 외형 — 확정 필요)", "Kinis", "Belum / Kinera", "KinisFirst", "Male", "Long", "Ashen", "MageRobe", "Ashen", 3, 4, "Hood"),
]

# What the game's `NpcActor::Kind` things (not people) use.
THINGS = [
    dict(kind="Sign", mesh="Env_Props_Signpost", file="Env_Props"),
    dict(kind="Note", mesh="Env_Props_NoteStone", file="Env_Props"),
    dict(kind="Lantern", mesh="Env_Props_LostLantern", file="Env_Props"),
]
