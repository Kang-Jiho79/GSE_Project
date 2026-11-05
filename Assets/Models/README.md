# 3D 로우폴리 모델 에셋

`Tools/ModelGen/build.py`가 만든다 (`python Tools/ModelGen/build.py`, 미리보기 생략은 `--no-preview`).
모양을 고치려면 `Tools/ModelGen/`의 스크립트를 고치고 다시 돌린다. 이 폴더의 OBJ·JSON은 직접 고치지 않는다.

## 규약
- 오른손 좌표계, **Y 위, 모델 앞쪽 = +Z**, 단위 미터
- 플랫 셰이딩(면마다 법선), 텍스처·UV 없음. 색은 `usemtl` 슬롯 이름 → `Materials.mtl`
- NPC 파츠의 원점은 모두 같다 (두 발 사이 바닥점). 겹쳐 놓기만 하면 조립된다
- 월드 모델의 원점은 바닥 중심, 문이 있는 쪽이 +Z
- 오브젝트(`o`) 이름이 곧 메시 이름. 한 OBJ 파일에 같은 종류가 여러 오브젝트로 들어 있다

## 파일
| 파일 | 내용 |
|---|---|
| `NPC/NPC_Body.obj` | 남/여 몸 (Head, Torso, ArmL, ArmR, LegL, LegR) |
| `NPC/NPC_Hair.obj` | 짧은/긴 머리 × 남/여 |
| `NPC/NPC_Mark.obj` | 소속 표식: 빛(후광) / 어둠(뿔) / 잿빛(깨진 고리·파편) |
| `NPC/NPC_Outfit.obj` | 옷 / 갑옷 / 마법사옷 × 남/여 (Torso, ArmL, ArmR, LegL, LegR) |
| `NPC/NPC_Headgear.obj` | 후드, 투구, 주교관, 왕관, 모자, 머리띠 |
| `NPC/NPC_Beard.obj` | 짧은/긴 수염 |
| `NPC/NPC_Prop.obj` | 지팡이, 단장, 등불, 배낭, 망치 |
| `Env/*.obj` | 나무·바위·폐허·집·신전·제단·수도·알현실·마을 소품·볼거리·보스 자리 |
| `NpcParts.json` | 슬롯 목록(인덱스 순서), 메시 이름 규칙, 관절 위치, 팔레트·피부·머리색 |
| `NpcPresets.json` / `.md` | **NPC별 파츠 인덱스** (게임이 읽을 정보) |
| `EnvModels.json` | 월드 모델별 파일·크기·삼각형 수 |
| `Preview/*.png` | 소프트웨어 렌더 미리보기 (확인용) |

## NPC 조립
성별(몸) → 머리 → 소속 표식 → 옷, 그리고 선택으로 머리장식·수염·소품.
`NpcPresets.json`의 `parts` 값은 `NpcParts.json`의 `slots` 목록 인덱스이고 `-1`은 없음.
`meshes`에는 그 NPC가 쓰는 메시 이름이 그리는 순서대로 들어 있다.

색은 `npc_*` 슬롯을 NPC마다 바꿔 칠한다: `colors.palette`(빛/어둠/잿빛/경계인/섞임),
`colors.skin`, `colors.hairColor`. `npc_accent`만 발광. `height`는 몸 크기 배율(어린이 0.62, 노인 0.95).
몸·옷은 관절(`joints`)을 중심으로 부위별로 돌릴 수 있다. 소품은 `HandGrip`이 원점(배낭만 몸 원점).
