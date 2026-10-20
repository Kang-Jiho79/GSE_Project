#include "stdafx.h"
#include "Scene.h"
#include "Renderer.h"
#include "PlayerStatus.h"
#include "Profiler.h"
#include "WorldGrid.h"
#include "Random.h"
#include "Tuning.h"
#include "Palette.h"
#include "TextRenderer.h"
#include "Progress.h"
#include <cmath>
#include <string>

#include "TerrainActor.h"
#include "PropsActor.h"
#include "Traces.h"
#include "TracesActor.h"
#include "Props.h"
#include "PlayerActor.h"
#include "MonsterActor.h"
#include "ProjectileActor.h"
#include "ParticleActor.h"
#include "SparkActor.h"
#include "WaystoneActor.h"
#include "SettlementActor.h"
#include "BossActor.h"
#include "KingActor.h"
#include "Bosses.h"
#include "NpcActor.h"
#include "Npcs.h"
#include "Story.h"
#include "Titles.h"
#include "Cutscenes.h"
#include "GodMarkActor.h"
#include "ArenaActor.h"
#include "LandmarkActor.h"
#include "ThroneRoomActor.h"
#include "Quests.h"
#include "JsonWriter.h"
#include "Dialogue.h"
#include "Combat.h"

namespace
{
	const int kSpawnMargin = 15;
	const int kSpawnTries = 20;
	// Nothing grows this close to the waystone.
	const float kWaystoneClearing = 4.f;
	// Open ground round where a boss waits, so it has room to fight in.
	const float kArenaClearing = 7.f;
	// The squares laid out along the way into a walled board, past its walls.
	const int kApproachLength = 9;
	// Open ground round what stands in a square with no name.
	const float kLandmarkClearing = 9.f;
	// Out of a castle, he is set down this far before its door.
	const float kDoorOutside = 1.5f;
	// The window height the regions' light and fog distances were set for;
	// a taller window stretches them to match, so the larger view is seen
	// rather than fogged over.
	const float kAtmosphereWindowY = 640.f;
	// A cutscene's spray of light.
	const int kCueSparks = 24;
	const float kCueSparkSpeed = 220.f;
	const float kCueSparkLift = 40.f;
	// He arrives this far from the waystone along both axes, and rises this far
	// in front of an altar.
	const float kArrivalOffset = 1.5f;
	const float kAltarOffset = 0.9f;

	// A gray veil over the world: faint while he is warned, heavier and
	// breathing once the gray is eating him.
	const float kAshTintWarning = 0.07f;
	const float kAshTintEroding = 0.10f;
	const float kAshTintPulseRate = 6.f;

	// The region's name, shown as he arrives and fading over its last part.
	const float kBannerTime = 2.6f;
	const float kBannerFade = 0.8f;
	const float kBannerDrop = 120.f; // below the top of the window

	// A king's words, under the banner, and what is written under them.
	const float kRemarkDrop = 175.f;
	const float kRemarkSpacing = 30.f;
	const char* const kTitlePrefix = "이명  ·  ";

	// The boss bar, low in the middle above the controls line.
	const float kBossBarWidth = 420.f;
	const float kBossBarHeight = 10.f;
	const float kBossBarRise = 80.f; // above the bottom of the window

	const float kMinimapSize = 170.f;
	const float kMinimapBorder = 4.f;
	const float kMinimapMargin = 18.f;  // from the window's corner
	const float kMinimapRange = 30.f;   // tiles out from him it shows
	const float kMinimapScale = 4.f;    // pixels across a tile's diamond
	// The main road's next step: its marker on the minimap (kept this far
	// inside its edge when the place is out of reach), and its line under it.
	const float kGoalMarker = 10.f;
	const float kGoalInset = 8.f;
	const float kGoalDrop = 26.f;
	const float kGoalLabelRise = 20.f;
	const int kMinimapCell = 2;         // tiles to one diamond of ground
	const float kMinimapHeightShade = 0.012f; // lighter per pixel of rise
}

// Nothing is built here: the scene is a global, made before main, when
// other files' globals it would touch (the grid's blockers) may not exist
// yet. NewGame or Continue builds the first region.
Scene::Scene(Progress& progress)
	: m_Progress(progress)
{
}

void Scene::SetViewport(int windowSizeX, int windowSizeY)
{
	m_WindowSizeX = windowSizeX;
	m_WindowSizeY = windowSizeY;
}

int Scene::CurrentRegion() const
{
	return m_Region;
}

Regions::Atmosphere Scene::CurrentAtmosphere() const
{
	Regions::Atmosphere air = Regions::AtmosphereOf(m_Region);
	if (!m_InWorld || m_Player == nullptr)
	{
		return air;
	}

	// The nearest other place within reach, looked for outward in rings: as
	// he nears it, its air mixes in - half and half right at the border.
	float playerCol, playerRow;
	WorldGrid::InverseIso(m_Player->WorldX(), m_Player->WorldY(), playerCol, playerRow);
	const int kRings = 4;
	const int kSpokes = 8;
	for (int ring = 1; ring <= kRings; ring++)
	{
		float reach = Tuning::Travel::kBlendReach * ring / kRings;
		for (int spoke = 0; spoke < kSpokes; spoke++)
		{
			float angle = spoke * 6.2831853f / kSpokes;
			int col = (int)floorf(playerCol + cosf(angle) * reach + 0.5f);
			int row = (int)floorf(playerRow + sinf(angle) * reach + 0.5f);
			int other = Regions::RegionAt(col, row);
			if (other < 0 || other == m_Region)
			{
				continue;
			}
			const Regions::Atmosphere& next = Regions::AtmosphereOf(other);
			float t = 0.5f * (1.f - reach / (Tuning::Travel::kBlendReach + 1.f));
			air.fog.r += (next.fog.r - air.fog.r) * t;
			air.fog.g += (next.fog.g - air.fog.g) * t;
			air.fog.b += (next.fog.b - air.fog.b) * t;
			air.lightRadius += (next.lightRadius - air.lightRadius) * t;
			air.fogStart += (next.fogStart - air.fogStart) * t;
			air.fogEnd += (next.fogEnd - air.fogEnd) * t;
			air.saturation += (next.saturation - air.saturation) * t;
			air.contrast += (next.contrast - air.contrast) * t;
			air.tint.r += (next.tint.r - air.tint.r) * t;
			air.tint.g += (next.tint.g - air.tint.g) * t;
			air.tint.b += (next.tint.b - air.tint.b) * t;
			return air;
		}
	}
	return air;
}

const bool* Scene::VisitedRegions() const
{
	return m_Progress.visited;
}

void Scene::PlayerTile(float& col, float& row) const
{
	WorldGrid::InverseIso(m_Player->WorldX(), m_Player->WorldY(), col, row);
}

void Scene::MapLocation(int& region, float& col, float& row) const
{
	region = Regions::MapRegion(m_Region);
	if (region == m_Region)
	{
		PlayerTile(col, row);
		return;
	}
	// Inside a castle: at its door, in its capital.
	Settlement::Layout capital = Settlement::Generate(region, 0.f, 0.f);
	float centerCol, centerRow;
	Regions::Center(region, centerCol, centerRow);
	col = centerCol + capital.doorCol;
	row = centerRow + capital.doorRow;
}

void Scene::UseDoor()
{
	if (m_DoorTarget < 0)
	{
		return;
	}
	int target = m_DoorTarget;
	BeginTravel(target);
	m_DoorTravel = (m_Fade == Fade::Travel);
}

Scene::Interaction Scene::AvailableInteraction() const
{
	if (m_Fade != Fade::None || m_Cutscene.IsPlaying())
	{
		return Interaction::None;
	}
	// In a duel there is nothing to do but fight it - and no leaving it by
	// the waystone with his form still held.
	if (m_King != nullptr && m_King->IsDueling())
	{
		return Interaction::None;
	}
	if (m_AtKing)
	{
		return Interaction::Duel;
	}
	if (m_AtBoard)
	{
		return Interaction::Board;
	}
	if (m_AtKingToTalk || m_Listener != nullptr)
	{
		return Interaction::Talk;
	}
	if (m_DoorTarget >= 0)
	{
		return Interaction::Door;
	}
	if (m_AtAltar)
	{
		return Interaction::Temple;
	}
	// Not while the hidden village still needs him.
	if (m_AtWaystone && !m_Prologue.HoldsWaystone())
	{
		return Interaction::WorldMap;
	}
	return Interaction::None;
}

void Scene::UseTemple(PlayerStatus& status)
{
	status.Heal(status.maxHealth);
	m_Progress.respawnRegion = m_Region;
	m_Progress.respawnAtAltar = true;
	// Rested, the world fills again: everything near is set out afresh.
	RefillAll();
	UpdatePlaces();
}

void Scene::NewGame()
{
	m_Fade = Fade::None;
	m_FadeElapsed = 0.f;
	m_Conversation = -1;
	// A new journey: the world built afresh round the hidden village.
	m_InWorld = false;
	EnterRegion(Regions::Limina);
}

void Scene::Continue(int region)
{
	m_Fade = Fade::None;
	m_FadeElapsed = 0.f;
	m_Conversation = -1;
	m_InWorld = false;
	EnterRegion(region);

	float x, y;
	RestPoint(region, x, y);
	m_Player->Respawn(x, y);
	m_CamX = x;
	m_CamY = y;
	UpdatePlaces();
}

int Scene::TakeConversation()
{
	int conversation = m_Conversation;
	m_Conversation = -1;
	return conversation;
}

int Scene::TakeEnding()
{
	return m_Events.TakeEnding();
}

bool Scene::HoldsSave() const
{
	return m_Events.HoldsSave() || m_Cutscene.IsPlaying();
}

void Scene::PlayCutscene(int id)
{
	float col, row, x, y;
	WaystoneTile(col, row);
	WorldGrid::IsoPosition(col, row, x, y);
	PlayCutscene(id, x, y);
}

void Scene::PlayCutscene(int id, float anchorX, float anchorY)
{
	if (m_Fade != Fade::None || m_Cutscene.IsPlaying() || (m_King != nullptr && m_King->IsDueling()))
	{
		return;
	}
	m_Cutscene.Play(id, anchorX, anchorY, m_Player->LocalX(), m_Player->LocalY());
}

bool Scene::BeginLimit(PlayerStatus& status, float aimX, float aimY)
{
	if (!status.IsLimitReady() || status.health <= 0.f || m_Fade != Fade::None || m_Cutscene.IsPlaying()
		|| m_Conversation >= 0)
	{
		return false;
	}

	int id = Cutscenes::LimitLight;
	if (status.form == Form::Dark)
	{
		id = Cutscenes::LimitDark;
	}
	else if (status.form == Form::Ash)
	{
		id = Cutscenes::LimitAsh;
	}

	status.limit = 0.f;
	// The mouse is read against the window; the world may be zoomed.
	status.limitAimX = aimX / m_Zoom;
	status.limitAimY = aimY / m_Zoom;
	m_Cutscene.Play(id, m_Player->LocalX(), m_Player->LocalY(), m_Player->LocalX(), m_Player->LocalY());
	return true;
}

float Scene::Desaturation() const
{
	return m_Cutscene.IsGray() ? 1.f : 0.f;
}

bool Scene::AskedKing() const
{
	if (m_King == nullptr)
	{
		return false;
	}
	return m_Progress.Has((m_King->Power() == Form::Light) ? Story::AskedLucius : Story::AskedNoctis);
}

bool Scene::IsInCutscene() const
{
	return m_Cutscene.IsPlaying();
}

void Scene::SettleCamera()
{
	// His parent never moves, so where he was placed is where he is - even
	// in a region built this very frame, before anything was traversed.
	if (m_Cutscene.IsPlaying())
	{
		m_CamX = m_Cutscene.CameraX();
		m_CamY = m_Cutscene.CameraY();
		m_Zoom = m_Cutscene.Zoom();
		return;
	}
	m_CamX = m_Player->LocalX();
	m_CamY = m_Player->LocalY();
	// Drawn back in the god's place, so the giant fits on screen.
	m_Zoom = (m_Region == Regions::Realm) ? Tuning::God::kRealmZoom : 1.f;
}

void Scene::PlayCue(int cue, PlayerStatus& status)
{
	switch (cue)
	{
	case Cutscenes::CueTestBurst:
	{
		float col, row, x, y;
		WaystoneTile(col, row);
		WorldGrid::IsoPosition(col, row, x, y);
		SparkActor::Burst(m_Graph, x, y, SparkActor::Style::Light, kCueSparks, kCueSparkSpeed, kCueSparkLift);
		break;
	}
	case Cutscenes::CueGodMark:
	{
		if (m_King != nullptr)
		{
			std::unique_ptr<GodMarkActor> mark(new GodMarkActor(Tuning::Cutscene::kMarkLift, Tuning::Cutscene::kMarkSize));
			mark->SetLocalPosition(m_King->WorldX(), m_King->WorldY());
			m_Graph.Spawn(std::move(mark));
		}
		break;
	}
	case Cutscenes::CueIntervene:
	{
		m_Intervening = true;
		break;
	}
	case Cutscenes::CueLimit:
	{
		status.limitRelease = true;
		break;
	}
	default:
	{
		break;
	}
	}
}

int Scene::TalkScript() const
{
	if (m_AtKingToTalk)
	{
		return m_King->Conversation();
	}
	return (m_Listener != nullptr) ? m_Listener->Conversation() : -1;
}

void Scene::BeginDuel(PlayerStatus& status)
{
	if (m_King == nullptr || !m_King->CanChallenge())
	{
		return;
	}

	m_Player->SettleForm(status, m_King->Power());
	m_King->BeginDuel(status);
}

bool Scene::IsNear(float col, float row, float reach) const
{
	float playerCol, playerRow;
	WorldGrid::InverseIso(m_Player->WorldX(), m_Player->WorldY(), playerCol, playerRow);
	float dc = playerCol - col;
	float dr = playerRow - row;
	return dc * dc + dr * dr <= reach * reach;
}

void Scene::BeginTravel(int region)
{
	if (m_Fade != Fade::None || m_Cutscene.IsPlaying())
	{
		return;
	}
	// Not in the middle of a duel - he would leave with his form still held.
	if (m_King != nullptr && m_King->IsDueling())
	{
		return;
	}

	m_Fade = Fade::Travel;
	m_FadeElapsed = 0.f;
	m_TravelTarget = region;
}

void Scene::WaystoneTile(float& col, float& row) const
{
	Regions::Center(m_Region, col, row);
}

void Scene::ArrivalPoint(int region, float& x, float& y) const
{
	float col, row;
	// Into a castle: just inside its door.
	if (Regions::IsCastle(region))
	{
		ThroneRoomActor::Arrival(col, row);
		WorldGrid::IsoPosition(col, row, x, y);
		return;
	}
	Regions::Center(region, col, row);
	WorldGrid::IsoPosition(col + kArrivalOffset, row + kArrivalOffset, x, y);
}

void Scene::RestPoint(int region, float& x, float& y) const
{
	const Settlement::Layout& settlement = m_Settlements[region];
	if (m_Progress.respawnAtAltar && m_Progress.respawnRegion == region && settlement.hasTemple)
	{
		WorldGrid::IsoPosition(settlement.altarCol + kAltarOffset, settlement.altarRow + kAltarOffset, x, y);
		return;
	}
	ArrivalPoint(region, x, y);
}

void Scene::BuildSettlement(int region, Actor* environment)
{
	m_Settlements[region] = Settlement::Layout();
	const Regions::Info& info = Regions::Get(region);
	if (info.kind != Regions::Kind::Village && info.kind != Regions::Kind::Capital)
	{
		return;
	}

	float centerCol, centerRow;
	Regions::Center(region, centerCol, centerRow);
	Settlement::Layout& settlement = m_Settlements[region];
	settlement = Settlement::Generate(region, centerCol, centerRow);

	for (size_t i = 0; i < settlement.buildings.size(); i++)
	{
		const Settlement::Building& building = settlement.buildings[i];
		if (Settlement::IsSolid(building))
		{
			float half = building.footprint * 0.5f;
			WorldGrid::AddBlocker(building.col - half, building.row - half, building.col + half, building.row + half);
		}
	}

	environment->AddChild(std::unique_ptr<Actor>(
		new SettlementActor(settlement, info.style, info.kind == Regions::Kind::Capital)));
}

void Scene::AddPlayer(float x, float y)
{
	Actor* entities = m_Graph.Root().AddChild(std::unique_ptr<Actor>(new Actor()));
	m_Graph.SetSpawnParent(entities);

	std::unique_ptr<PlayerActor> player(new PlayerActor());
	player->SetLocalPosition(x, y);
	m_Player = player.get();
	entities->AddChild(std::move(player));
}

void Scene::BuildWorld()
{
	WorldGrid::SetMap(Regions::WorldSize(), Regions::WorldSize(), Regions::WorldSeed(), 1);
	int side, cell;
	const std::vector<int>& cells = Regions::RegionCells(side, cell);
	std::vector<int> levels(Regions::Count, 1);
	for (int region = 0; region < Regions::Count; region++)
	{
		levels[region] = Regions::Get(region).terraceLevels;
	}
	WorldGrid::SetZones(cells, side, cell, levels);

	WorldGrid::ClearBlockers();
	m_Graph.Clear();
	m_Events.Leave(m_Graph);
	m_Prologue.Stop();
	m_King = nullptr;
	m_Listener = nullptr;
	m_LiminaSealed = false;
	for (int region = 0; region < Regions::Count; region++)
	{
		m_Active[region] = false;
		m_Arenas[region] = nullptr;
		m_Settlements[region] = Settlement::Layout();
	}

	// Two groups under the root: the ground the world is made of, and the
	// things living on it. Everything spawned later joins the second one.
	// The ground goes in first, so everything laid flat on it - paving, a
	// carpet, the rift's pools - is drawn over it.
	Actor* environment = m_Graph.Root().AddChild(std::unique_ptr<Actor>(new Actor()));
	TerrainActor* terrain = static_cast<TerrainActor*>(environment->AddChild(std::unique_ptr<Actor>(new TerrainActor())));
	Props::Clear();
	Traces::Clear();
	std::vector<std::unique_ptr<ArenaActor>> arenas;

	for (int region = 0; region < Regions::Count; region++)
	{
		if (Regions::IsOffMap(region))
		{
			continue;
		}
		const Regions::Info& info = Regions::Get(region);
		BuildSettlement(region, environment);

		// Nothing wild grows inside a settlement, only round its edges, nor
		// round a waystone or where a boss waits.
		float centerCol, centerRow;
		Regions::Center(region, centerCol, centerRow);
		Props::SetDensity(region, info.propDensity);
		// What is left of light and dark on its ground, leaning as its
		// monsters do; on the battlefields, some of both at once.
		bool battlefield = (info.style == Regions::Style::Mixed || region == Regions::Divisio);
		Traces::SetPlace(region, info.traceDensity, info.lightShare, battlefield ? Tuning::Trace::kMixedShare : 0.f);
		float clearing = (m_Settlements[region].radius > kWaystoneClearing) ? m_Settlements[region].radius : kWaystoneClearing;
		Props::AddClearing(centerCol, centerRow, clearing);
		if (Bosses::HasBoard(region) || region == Regions::Kinera)
		{
			float arenaCol, arenaRow;
			Bosses::Arena(region, arenaCol, arenaRow);
			Props::AddClearing(arenaCol, arenaRow, kArenaClearing);
		}

		// Where one of the god's pieces waits: its board worked into the
		// ground and walled round, and a way in laid in squares too.
		if (Bosses::HasBoard(region))
		{
			int boardCol, boardRow, dirCol, dirRow;
			Bosses::BoardCenter(region, boardCol, boardRow);
			Bosses::Opening(region, dirCol, dirRow);
			int half = Tuning::TaintedPiece::kBoardHalf;
			terrain->AddBoard(boardCol, boardRow, -half, half, -half, half, false);
			int from = Bosses::WallReach();
			int to = from + kApproachLength;
			int fromCol = (dirCol != 0) ? ((dirCol > 0) ? from : -to) : -1;
			int toCol = (dirCol != 0) ? ((dirCol > 0) ? to : -from) : 1;
			int fromRow = (dirRow != 0) ? ((dirRow > 0) ? from : -to) : -1;
			int toRow = (dirRow != 0) ? ((dirRow > 0) ? to : -from) : 1;
			terrain->AddBoard(boardCol, boardRow, fromCol, toCol, fromRow, toRow, false);
			Props::AddClearing(boardCol + dirCol * (from + to) * 0.5f, boardRow + dirRow * (from + to) * 0.5f,
				(float)(to - from) * 0.5f + 2.f);

			std::unique_ptr<ArenaActor> arena(new ArenaActor(region));
			m_Arenas[region] = arena.get();
			arenas.push_back(std::move(arena));
		}

		// What stands in a square with no name of its own.
		if (info.feature != Regions::Feature::None && info.feature != Regions::Feature::Hamlet)
		{
			float spotCol, spotRow;
			LandmarkActor::Spot(region, spotCol, spotRow);
			Props::AddClearing(spotCol, spotRow, kLandmarkClearing);
			environment->AddChild(std::unique_ptr<Actor>(new LandmarkActor(region, info.feature)));
		}
	}
	environment->AddChild(std::unique_ptr<Actor>(new PropsActor()));
	environment->AddChild(std::unique_ptr<Actor>(new TracesActor()));
	for (size_t i = 0; i < arenas.size(); i++)
	{
		environment->AddChild(std::move(arenas[i]));
	}

	float x, y;
	AddPlayer(0.f, 0.f);
	for (int region = 0; region < Regions::Count; region++)
	{
		if (Regions::IsOffMap(region))
		{
			continue;
		}
		float col, row;
		Regions::Center(region, col, row);
		WorldGrid::IsoPosition(col, row, x, y);
		std::unique_ptr<WaystoneActor> waystone(new WaystoneActor());
		waystone->SetLocalPosition(x, y);
		m_Graph.Spawn(std::move(waystone));
		Npcs::Spawn(region, m_Progress, m_Settlements[region], m_Graph);
	}

	m_InWorld = true;
	m_Region = -1;
}

void Scene::BuildIsolated(int region)
{
	const Regions::Info& info = Regions::Get(region);
	WorldGrid::SetMap(info.size, info.size, Regions::Seed(region), info.terraceLevels);
	WorldGrid::ClearBlockers();
	m_Graph.Clear();
	m_Events.Leave(m_Graph);
	m_Prologue.Stop();
	m_King = nullptr;
	m_Listener = nullptr;
	m_LiminaSealed = false;
	for (int i = 0; i < Regions::Count; i++)
	{
		m_Active[i] = false;
		m_Arenas[i] = nullptr;
		m_Settlements[i] = Settlement::Layout();
	}

	float waystoneCol, waystoneRow;
	Regions::Center(region, waystoneCol, waystoneRow);

	Actor* environment = m_Graph.Root().AddChild(std::unique_ptr<Actor>(new Actor()));
	std::unique_ptr<TerrainActor> terrain(new TerrainActor());
	// The god's place is its board, walled round: nothing past the squares.
	if (region == Regions::Realm)
	{
		int boardCol = (int)waystoneCol;
		int boardRow = (int)waystoneRow;
		terrain->AddBoard(boardCol, boardRow, Tuning::God::kBoardFrom, Tuning::God::kBoardTo,
			Tuning::God::kBoardFrom, Tuning::God::kBoardTo, true);
		float from = Tuning::God::kBoardFrom - 0.5f;
		float to = Tuning::God::kBoardTo + 0.5f;
		float edge = (float)info.size;
		WorldGrid::AddBlocker(-1.f, -1.f, boardCol + from, edge);
		WorldGrid::AddBlocker(boardCol + to, -1.f, edge, edge);
		WorldGrid::AddBlocker(-1.f, -1.f, edge, boardRow + from);
		WorldGrid::AddBlocker(-1.f, boardRow + to, edge, edge);
	}
	// A throne room: its floor a board too, walled round, the throne at its
	// far end.
	if (Regions::IsCastle(region))
	{
		int reach = ThroneRoomActor::FloorReach();
		terrain->AddBoard((int)waystoneCol, (int)waystoneRow, -reach, reach, -reach, reach, true);
	}
	environment->AddChild(std::move(terrain));
	BuildSettlement(region, environment);
	if (Regions::IsCastle(region))
	{
		m_Settlements[region] = ThroneRoomActor::Layout();
		environment->AddChild(std::unique_ptr<Actor>(new ThroneRoomActor(info.style)));
	}
	Props::Clear();
	Props::SetDensity(0, info.propDensity);
	// Off the map there is nothing left lying of either.
	Traces::Clear();
	Props::AddClearing(waystoneCol, waystoneRow, kWaystoneClearing);
	environment->AddChild(std::unique_ptr<Actor>(new PropsActor()));

	float x, y;
	ArrivalPoint(region, x, y);
	AddPlayer(x, y);
	SpawnMonsters(region);

	// The king at the carpet's end, and the queen by the throne.
	if (Regions::IsCastle(region))
	{
		m_King = dynamic_cast<KingActor*>(Bosses::Spawn(region, m_Progress, m_Settlements[region], m_Graph));
		m_AskedKing = AskedKing();
		Npcs::Spawn(region, m_Progress, m_Settlements[region], m_Graph);
	}

	m_InWorld = false;
	m_Region = region;
	m_Progress.visited[region] = true;
	m_Events.Enter(region, m_Progress, m_Settlements[region], m_Graph, waystoneCol, waystoneRow);
	m_BannerTimer = kBannerTime;
}

void Scene::EnterRegion(int region)
{
	m_Cutscene = Cutscene();
	m_Intervening = false;
	m_AtWaystone = false;
	m_AtAltar = false;
	m_AtKing = false;
	m_AtKingToTalk = false;

	float x, y;
	if (Regions::IsOffMap(region))
	{
		BuildIsolated(region);
		ArrivalPoint(region, x, y);
	}
	else
	{
		if (!m_InWorld)
		{
			BuildWorld();
		}
		ArrivalPoint(region, x, y);
		m_Player->Respawn(x, y);
		SetCurrent(region);
		UpdatePlaces();
	}

	// The camera starts on him, even before the first update - the title
	// shows this place behind it without ever running it.
	m_CamX = x;
	m_CamY = y;
	m_Zoom = (region == Regions::Realm) ? Tuning::God::kRealmZoom : 1.f;
}

void Scene::SetCurrent(int region)
{
	if (region == m_Region)
	{
		return;
	}
	m_Events.Leave(m_Graph);
	m_Region = region;
	m_Progress.visited[region] = true;
	m_BannerTimer = kBannerTime;

	float centerCol, centerRow;
	Regions::Center(region, centerCol, centerRow);
	m_Events.Enter(region, m_Progress, m_Settlements[region], m_Graph, centerCol, centerRow);

	// The hidden village's last day, the first time he is there.
	if (region == Regions::Limina && !m_Progress.Has(Story::VillageRaided))
	{
		if (!m_Prologue.IsActive())
		{
			m_Prologue.Begin();
		}
	}
	else
	{
		m_Prologue.Stop();
	}
}

void Scene::UpdatePlaces()
{
	if (!m_InWorld)
	{
		return;
	}
	float playerCol, playerRow;
	WorldGrid::InverseIso(m_Player->WorldX(), m_Player->WorldY(), playerCol, playerRow);
	for (int region = 0; region < Regions::Count; region++)
	{
		if (Regions::IsOffMap(region))
		{
			continue;
		}
		float distance = Regions::DistanceOutside(region, playerCol, playerRow);
		if (!m_Active[region] && distance <= Tuning::Travel::kWakeReach)
		{
			Activate(region);
		}
		else if (m_Active[region] && distance > Tuning::Travel::kForgetReach)
		{
			Deactivate(region);
		}
	}
}

void Scene::Activate(int region)
{
	m_Active[region] = true;
	SpawnMonsters(region);
	Npcs::SpawnErrands(region, m_Progress, m_Settlements[region], m_Graph);

	BossActor* boss = Bosses::Spawn(region, m_Progress, m_Settlements[region], m_Graph);
	if (boss != nullptr)
	{
		boss->SetZone(region);
		KingActor* king = dynamic_cast<KingActor*>(boss);
		if (king != nullptr)
		{
			m_King = king;
			m_AskedKing = AskedKing();
		}
	}
}

void Scene::Deactivate(int region)
{
	m_Active[region] = false;
	m_Graph.ForEach<Actor>([&](Actor& actor)
	{
		if (actor.Zone() == region)
		{
			actor.Destroy();
		}
	});
	if (m_King != nullptr && m_King->Zone() == region)
	{
		m_King = nullptr;
		m_AtKing = false;
		m_AtKingToTalk = false;
	}
}

namespace
{
	const char* StyleName(Regions::Style style)
	{
		switch (style)
		{
		case Regions::Style::Light:
		{
			return "Light";
		}
		case Regions::Style::Dark:
		{
			return "Dark";
		}
		case Regions::Style::Ashen:
		{
			return "Ashen";
		}
		case Regions::Style::Borderborn:
		{
			return "Borderborn";
		}
		default:
		{
			return "Mixed";
		}
		}
	}

	const char* NpcKindName(NpcActor::Kind kind)
	{
		switch (kind)
		{
		case NpcActor::Kind::Adult:
		{
			return "Adult";
		}
		case NpcActor::Kind::Elder:
		{
			return "Elder";
		}
		case NpcActor::Kind::Child:
		{
			return "Child";
		}
		case NpcActor::Kind::Priest:
		{
			return "Priest";
		}
		case NpcActor::Kind::Kinis:
		{
			return "Kinis";
		}
		case NpcActor::Kind::Queen:
		{
			return "Queen";
		}
		case NpcActor::Kind::Merchant:
		{
			return "Merchant";
		}
		case NpcActor::Kind::Sign:
		{
			return "Sign";
		}
		case NpcActor::Kind::Note:
		{
			return "Note";
		}
		default:
		{
			return "Lantern";
		}
		}
	}
}

void Scene::WriteSnapshot(JsonWriter& json, float time, bool talking)
{
	json.Clear();
	json.BeginObject();
	json.Value("type", "snapshot");
	json.Value("time", time);
	json.BeginObject("region");
	json.Value("id", m_Region);
	json.Value("name", (m_Region >= 0) ? Regions::Get(m_Region).name : "");
	json.EndObject();

	// The gray, by kind and side - in all, and in each place.
	int byKind[2][2] = {};
	int monsters = 0;
	int monstersIn[Regions::Count] = {};
	m_Graph.ForEach<MonsterActor>([&](MonsterActor& monster)
	{
		if (monster.IsDestroyed())
		{
			return;
		}
		monsters++;
		byKind[monster.Side() == Alignment::Light ? 0 : 1][monster.IsKnight() ? 0 : 1]++;
		if (monster.Zone() >= 0 && monster.Zone() < Regions::Count)
		{
			monstersIn[monster.Zone()]++;
		}
	});

	// Everyone he could speak to, or read.
	bool escaped = m_Progress.Has(Story::EscapedGodInfluence);
	int people = 0;
	int listed = 0;
	int peopleIn[Regions::Count] = {};
	json.BeginArray("npcs");
	m_Graph.ForEach<NpcActor>([&](NpcActor& npc)
	{
		if (npc.IsDestroyed())
		{
			return;
		}
		float col, row;
		WorldGrid::InverseIso(npc.WorldX(), npc.WorldY(), col, row);
		int region = m_InWorld ? Regions::RegionAt((int)floorf(col + 0.5f), (int)floorf(row + 0.5f)) : m_Region;
		bool person = !npc.IsReadable();
		if (person)
		{
			people++;
			if (region >= 0 && region < Regions::Count)
			{
				peopleIn[region]++;
			}
		}

		json.BeginObject();
		json.Value("id", ++listed);
		json.Value("kind", NpcKindName(npc.Type()));
		json.Value("style", StyleName(npc.Dress()));
		json.Value("region", (region >= 0) ? Regions::Get(region).name : "");
		json.Value("col", col);
		json.Value("row", row);
		int script = (npc.Conversation() >= 0) ? Dialogue::Resolve(npc.Conversation(), m_Progress) : -1;
		json.Value("dialogue", script);
		const Dialogue::Script* lines = (script >= 0) ? &Dialogue::Get(script) : nullptr;
		json.Value("line", (lines != nullptr && lines->lineCount > 0) ? lines->lines[0].text : "");
		json.Value("talking", talking && &npc == m_Listener);
		// Everyone but Kinis, and things, is in the god's hold; once he is
		// out of it, a priest asked about the god can no longer hear it.
		json.Value("influenced", person && npc.Type() != NpcActor::Kind::Kinis);
		json.Value("errorReaction", npc.Type() == NpcActor::Kind::Priest && escaped);
		json.EndObject();
	});
	json.EndArray();

	json.BeginObject("counts");
	json.Value("monsters", monsters);
	json.BeginObject("monstersByKind");
	json.Value("lightKnight", byKind[0][0]);
	json.Value("lightMage", byKind[0][1]);
	json.Value("darkKnight", byKind[1][0]);
	json.Value("darkMage", byKind[1][1]);
	json.EndObject();
	json.Value("npcs", people);
	json.EndObject();

	// Each village and capital on the map.
	json.BeginArray("villages");
	for (int region = 0; region < Regions::Count; region++)
	{
		const Regions::Info& info = Regions::Get(region);
		if (Regions::IsOffMap(region) || (info.kind != Regions::Kind::Village && info.kind != Regions::Kind::Capital))
		{
			continue;
		}
		int requests[4];
		json.BeginObject();
		json.Value("name", info.name);
		json.Value("style", StyleName(info.style));
		json.Value("temple", info.temple);
		json.Value("npcs", peopleIn[region]);
		// Only a place set out has any monsters to count.
		if (m_InWorld && m_Active[region])
		{
			json.Value("monsters", monstersIn[region]);
		}
		else
		{
			json.Null("monsters");
		}
		json.Value("visited", m_Progress.visited[region]);
		json.Value("requests", Quests::OnBoard(region, m_Progress, requests, 4));
		json.EndObject();
	}
	json.EndArray();

	json.EndObject();
}

void Scene::RefreshRegion(int region)
{
	// Cleared now, and set out again as soon as he is near enough.
	if (m_InWorld && region >= 0 && m_Active[region])
	{
		Deactivate(region);
	}
}

void Scene::RefillAll()
{
	for (int region = 0; region < Regions::Count; region++)
	{
		if (m_Active[region])
		{
			Deactivate(region);
		}
	}
}

int Scene::AwakeBossRegion()
{
	int found = -1;
	m_Graph.ForEach<BossActor>([&](BossActor& boss)
	{
		if (!boss.IsDestroyed() && boss.IsAwake() && boss.HealthFraction() > 0.f && boss.Zone() >= 0
			&& Bosses::HasBoard(boss.Zone()))
		{
			found = boss.Zone();
		}
	});
	return found;
}

void Scene::UpdateSeals()
{
	if (!m_InWorld)
	{
		return;
	}

	// The hidden village, shut in on every side until the gray is gone.
	bool seal = !m_Progress.Has(Story::VillageRaided);
	if (seal != m_LiminaSealed)
	{
		if (seal)
		{
			float minCol, minRow, maxCol, maxRow;
			Regions::Bounds(Regions::Limina, minCol, minRow, maxCol, maxRow);
			float boxes[4][4] =
			{
				{ minCol - 1.f, minRow - 1.f, maxCol + 1.f, minRow },
				{ minCol - 1.f, maxRow, maxCol + 1.f, maxRow + 1.f },
				{ minCol - 1.f, minRow, minCol, maxRow },
				{ maxCol, minRow, maxCol + 1.f, maxRow },
			};
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					m_LiminaSeal[i][j] = boxes[i][j];
				}
				WorldGrid::AddBlocker(boxes[i][0], boxes[i][1], boxes[i][2], boxes[i][3]);
			}
		}
		else
		{
			for (int i = 0; i < 4; i++)
			{
				WorldGrid::RemoveBlocker(m_LiminaSeal[i][0], m_LiminaSeal[i][1], m_LiminaSeal[i][2], m_LiminaSeal[i][3]);
			}
		}
		m_LiminaSealed = seal;
	}

	// A board, shut while its boss is awake and he is on it.
	int awake = AwakeBossRegion();
	float playerCol, playerRow;
	WorldGrid::InverseIso(m_Player->WorldX(), m_Player->WorldY(), playerCol, playerRow);
	for (int region = 0; region < Regions::Count; region++)
	{
		ArenaActor* arena = m_Arenas[region];
		if (arena == nullptr)
		{
			continue;
		}
		bool shut = (awake == region) && arena->IsInside(playerCol, playerRow);
		// Once shut, it stays so while the fight goes on.
		if (!shut && awake == region)
		{
			continue;
		}
		arena->SetSealed(shut);
	}
}

void Scene::SpawnMonsters(int region)
{
	const Regions::Info& info = Regions::Get(region);
	float minCol, minRow, maxCol, maxRow;
	Regions::Bounds(region, minCol, minRow, maxCol, maxRow);
	float margin = (float)kSpawnMargin;
	if (margin > info.size / 6.f)
	{
		margin = info.size / 6.f;
	}

	float waystoneCol, waystoneRow;
	Regions::Center(region, waystoneCol, waystoneRow);
	const Settlement::Layout& settlement = m_Settlements[region];
	bool board = Bosses::HasBoard(region);
	int boardCol = 0;
	int boardRow = 0;
	if (board)
	{
		Bosses::BoardCenter(region, boardCol, boardRow);
	}

	// The side split is exact, so a region's leaning is what the player
	// meets; which of them are mages is left to chance.
	int lightCount = (int)(info.monsterCount * info.lightShare + 0.5f);

	for (int i = 0; i < info.monsterCount; i++)
	{
		// Each side keeps to its own patches of the ground, as long as there
		// are any to be found.
		Alignment alignment = (i < lightCount) ? Alignment::Light : Alignment::Dark;
		int col = 0;
		int row = 0;
		for (int attempt = 0; attempt < kSpawnTries; attempt++)
		{
			col = Random::Range((int)(minCol + margin), (int)(maxCol - margin));
			row = Random::Range((int)(minRow + margin), (int)(maxRow - margin));

			// Not at the waystone, not in a settlement, not in the quiet
			// before a board.
			float dc = col - waystoneCol;
			float dr = row - waystoneRow;
			bool clear = dc * dc + dr * dr >= Tuning::Travel::kSpawnClearance * Tuning::Travel::kSpawnClearance
				&& !WorldGrid::IsBlocked((float)col, (float)row);
			if (clear && settlement.radius > 0.f)
			{
				float sc = col - settlement.squareCol;
				float sr = row - settlement.squareRow;
				clear = sc * sc + sr * sr >= (settlement.radius + 2.f) * (settlement.radius + 2.f);
			}
			if (clear && board)
			{
				float bc = (float)(col - boardCol);
				float br = (float)(row - boardRow);
				clear = bc * bc + br * br >= Tuning::Travel::kArenaQuiet * Tuning::Travel::kArenaQuiet;
			}
			if (clear && attempt < kSpawnTries / 2)
			{
				float lean = Traces::LightLean(col, row);
				clear = (alignment == Alignment::Light) ? lean >= 0.5f : lean <= 0.5f;
			}
			if (clear)
			{
				break;
			}
		}

		MonsterActor::Kind kind = (Random::Unit() < info.mageShare) ? MonsterActor::Kind::Mage : MonsterActor::Kind::Knight;

		std::unique_ptr<MonsterActor> monster(new MonsterActor(kind, alignment));
		float x, y;
		WorldGrid::IsoPosition((float)col, (float)row, x, y);
		monster->SetLocalPosition(x, y);
		monster->SetZone(region);
		m_Graph.Spawn(std::move(monster));
	}
}

ActorContext Scene::MakeContext(float deltaTime, PlayerStatus& status)
{
	ActorContext ctx;
	ctx.deltaTime = deltaTime;

	// The camera sits exactly on the player, so he is always at the screen
	// origin and a screen offset is already a world offset.
	ctx.playerX = m_Player->WorldX();
	ctx.playerY = m_Player->WorldY();
	ctx.camX = m_CamX + m_Impact.OffsetX();
	ctx.camY = m_CamY + m_Impact.OffsetY();

	// Closed in, the window shows less of the world; drawn back, more - so
	// what is culled and how far the ground is walked follow the zoom.
	ctx.windowSizeX = (int)(m_WindowSizeX / m_Zoom);
	ctx.windowSizeY = (int)(m_WindowSizeY / m_Zoom);
	ctx.status = &status;
	ctx.graph = &m_Graph;
	ctx.impact = &m_Impact;
	ctx.progress = &m_Progress;

	return ctx;
}

void Scene::Update(const InputState& input, float deltaTime, PlayerStatus& status)
{
	UpdateFade(deltaTime, status);

	// A cutscene runs on real time and takes the camera; the cues and the
	// conversation its shots begin are acted on at once.
	m_Cutscene.Update(deltaTime, m_Player->LocalX(), m_Player->LocalY(), input.skip);
	for (int cue = m_Cutscene.TakeCue(); cue != Cutscenes::NoCue; cue = m_Cutscene.TakeCue())
	{
		PlayCue(cue, status);
	}
	// Under the white, the old battlefield is built and he comes to there.
	if (m_Intervening)
	{
		m_Intervening = false;
		EnterRegion(Regions::Bellum);
		float col, row, x, y;
		WaystoneTile(col, row);
		WorldGrid::IsoPosition(col, row, x, y);
		m_Cutscene.Play(Cutscenes::Intervened, x, y, m_Player->LocalX(), m_Player->LocalY());
	}
	int scripted = m_Cutscene.TakeConversation();
	if (scripted >= 0)
	{
		m_Conversation = scripted;
	}
	if (m_Cutscene.IsPlaying())
	{
		status.GrantInvuln(Tuning::Cutscene::kShield);
	}
	SettleCamera();

	// A hit stop holds the world still while real time - the fades, the
	// shake dying away - carries on. A cutscene may slow or stop it too.
	float worldTime = m_Impact.Advance(deltaTime) * m_Cutscene.TimeScale();
	ActorContext ctx = MakeContext(worldTime, status);

	// Under a fade or a cutscene he gets no say; the rest of the world
	// carries on around him.
	if (m_Fade == Fade::None && !m_Cutscene.IsPlaying())
	{
		ctx.input = input;
		// The mouse is read against the window; the world may be zoomed.
		ctx.input.aimX /= m_Zoom;
		ctx.input.aimY /= m_Zoom;
	}

	{
		ProfileScope scope("update");
		m_Graph.Update(ctx);
	}
	// What he landed this frame fills the special's gauge.
	status.ChargeLimit(Combat::TakeLanded() * Tuning::Limit::kPerDealt);
	// Again now that he has moved, so the frame is drawn with him at its
	// centre rather than where he stood a moment ago.
	SettleCamera();

	float waystoneCol, waystoneRow;
	WaystoneTile(waystoneCol, waystoneRow);
	m_AtWaystone = !Regions::IsOffMap(m_Region) && IsNear(waystoneCol, waystoneRow, Tuning::Travel::kWaystoneReach);
	// In the world: which place he has walked into, and the places near
	// enough to be set out or far enough to be cleared away.
	if (m_InWorld && m_Fade == Fade::None)
	{
		float playerCol, playerRow;
		WorldGrid::InverseIso(m_Player->WorldX(), m_Player->WorldY(), playerCol, playerRow);
		int here = Regions::RegionAt((int)floorf(playerCol + 0.5f), (int)floorf(playerRow + 0.5f));
		if (here >= 0)
		{
			SetCurrent(here);
		}
		UpdatePlaces();
	}
	UpdateSeals();

	const Settlement::Layout& settlement = m_Settlements[m_Region];
	m_AtAltar = settlement.hasTemple
		&& IsNear(settlement.altarCol, settlement.altarRow, Tuning::Travel::kWaystoneReach);

	// A castle's door: in from its capital, or back out.
	m_DoorTarget = -1;
	if (Regions::IsCastle(m_Region))
	{
		float doorCol, doorRow;
		ThroneRoomActor::ExitDoor(doorCol, doorRow);
		if (IsNear(doorCol, doorRow, Tuning::Travel::kWaystoneReach))
		{
			m_DoorTarget = Regions::MapRegion(m_Region);
		}
	}
	else if (settlement.hasCastle && IsNear(settlement.doorCol, settlement.doorRow, Tuning::Travel::kWaystoneReach))
	{
		m_DoorTarget = (m_Region == Regions::Lumen) ? Regions::LumenCastle : Regions::TenebraeCastle;
	}

	// A notice board, where one has requests on it.
	m_AtBoard = false;
	if (Quests::HasBoard(m_Region))
	{
		for (size_t i = 0; i < settlement.buildings.size(); i++)
		{
			const Settlement::Building& building = settlement.buildings[i];
			if (building.type == Settlement::BuildingType::NoticeBoard
				&& IsNear(building.col, building.row, Tuning::Quest::kBoardReach))
			{
				m_AtBoard = true;
			}
		}
	}

	m_AtKing = false;
	if (m_King != nullptr && m_King->CanChallenge())
	{
		float kingCol, kingRow;
		WorldGrid::InverseIso(m_King->WorldX(), m_King->WorldY(), kingCol, kingRow);
		m_AtKing = IsNear(kingCol, kingRow, Tuning::Duel::kReach);
	}
	m_AtKingToTalk = false;
	if (m_King != nullptr && m_King->CanTalk())
	{
		float kingCol, kingRow;
		WorldGrid::InverseIso(m_King->WorldX(), m_King->WorldY(), kingCol, kingRow);
		m_AtKingToTalk = IsNear(kingCol, kingRow, Tuning::Duel::kReach);
	}

	if (m_Prologue.IsActive() && m_Fade == Fade::None)
	{
		int conversation = m_Prologue.Update(ctx.deltaTime, m_Graph, m_Progress, m_Impact, waystoneCol, waystoneRow);
		if (conversation >= 0)
		{
			m_Conversation = conversation;
		}
	}
	if (m_Fade == Fade::None && m_Conversation < 0)
	{
		m_Conversation = m_Events.Update(m_Graph, m_Progress, status, m_Player->WorldX(), m_Player->WorldY());
	}

	// Having taken up the gray, he holds it - and nothing else.
	if (m_Progress.Has(Story::AcceptAsh))
	{
		status.ashUnlocked = true;
	}
	if (status.ashUnlocked)
	{
		status.form = Form::Ash;
	}
	// Off the board, or with light and dark given up, the balance no longer
	// binds him.
	status.balanceUnbound = Titles::IsUnbound(m_Progress) || status.ashUnlocked;

	// The story moving him on by itself - into the god's place.
	int destination = m_Events.TakeTravel();
	if (destination >= 0)
	{
		BeginTravel(destination);
	}

	// The nearest person within reach, if any - the prologue may just have
	// taken them.
	m_Listener = nullptr;
	float playerCol, playerRow;
	WorldGrid::InverseIso(m_Player->WorldX(), m_Player->WorldY(), playerCol, playerRow);
	float nearest = Tuning::Dialogue::kTalkReach * Tuning::Dialogue::kTalkReach;
	m_Graph.ForEach<NpcActor>([&](NpcActor& npc)
	{
		float col, row;
		WorldGrid::InverseIso(npc.WorldX(), npc.WorldY(), col, row);
		float distance = (col - playerCol) * (col - playerCol) + (row - playerRow) * (row - playerRow);
		if (!npc.IsDestroyed() && npc.Conversation() >= 0 && distance <= nearest)
		{
			nearest = distance;
			m_Listener = &npc;
		}
	});

	// The collusion put to a king - the conversation just over - and before
	// it can answer, the god does: its mark over the king, and the world
	// washing white.
	bool asked = AskedKing();
	if (asked && !m_AskedKing && m_King != nullptr)
	{
		PlayCutscene(Cutscenes::Intervention, m_King->WorldX(), m_King->WorldY());
	}
	m_AskedKing = asked;

	RecordPopulation();
}

void Scene::SetEffectsEnabled(bool enabled)
{
	m_Impact.SetEnabled(enabled);
	SparkActor::SetEnabled(enabled);
}

bool Scene::AreEffectsEnabled() const
{
	return m_Impact.IsEnabled();
}

void Scene::FadeTimes(float& fadeOut, float& hold, float& fadeIn) const
{
	if (m_Fade == Fade::Travel)
	{
		fadeOut = Tuning::Travel::kFadeOut;
		hold = Tuning::Travel::kHold;
		fadeIn = Tuning::Travel::kFadeIn;
		return;
	}

	fadeOut = Tuning::Death::kFadeOut;
	hold = Tuning::Death::kHold;
	fadeIn = Tuning::Death::kFadeIn;
}

void Scene::UpdateFade(float deltaTime, PlayerStatus& status)
{
	if (m_BannerTimer > 0.f)
	{
		m_BannerTimer -= deltaTime;
	}

	if (m_Fade == Fade::None)
	{
		// Falling in a duel is not dying; the king sees to it.
		bool dueling = (m_King != nullptr && m_King->IsDueling());
		if (status.health <= 0.f && !dueling)
		{
			m_Fade = Fade::Death;
			m_FadeElapsed = 0.f;
			m_DeathByGray = fabsf(status.BalanceRatio()) > 1.f;
		}
		return;
	}

	float fadeOut, hold, fadeIn;
	FadeTimes(fadeOut, hold, fadeIn);

	float before = m_FadeElapsed;
	m_FadeElapsed += deltaTime;

	// Whatever changes happens on the frame the screen goes fully dark, so the
	// jump is never seen.
	if (before < fadeOut && m_FadeElapsed >= fadeOut)
	{
		if (m_Fade == Fade::Death)
		{
			// In the god's place there is no going back: he rises at its
			// edge, and it begins again. Fallen to a boss on its board, he
			// gets up at its threshold, and it waits for him afresh.
			// Otherwise he rises at the last temple he rested at, wherever
			// that is.
			int boss = AwakeBossRegion();
			if (Regions::IsOffMap(m_Region))
			{
				EnterRegion(m_Region);
			}
			else if (boss >= 0)
			{
				Deactivate(boss);
				if (m_Arenas[boss] != nullptr)
				{
					m_Arenas[boss]->SetSealed(false);
				}
				float col, row, x, y;
				Bosses::Threshold(boss, col, row);
				WorldGrid::IsoPosition(col, row, x, y);
				m_Player->Respawn(x, y);
			}
			else
			{
				float x, y;
				RestPoint(m_Progress.respawnRegion, x, y);
				m_Player->Respawn(x, y);
			}
			status.Revive();
		}
		else
		{
			EnterRegion(m_TravelTarget);
			// Out of a castle: set down before its door, not at the waystone.
			if (m_DoorTravel && !Regions::IsOffMap(m_TravelTarget))
			{
				const Settlement::Layout& capital = m_Settlements[m_TravelTarget];
				float x, y;
				WorldGrid::IsoPosition(capital.doorCol, capital.doorRow + kDoorOutside, x, y);
				m_Player->Respawn(x, y);
				m_CamX = x;
				m_CamY = y;
			}
			m_DoorTravel = false;
		}
	}

	if (m_FadeElapsed >= fadeOut + hold + fadeIn)
	{
		m_Fade = Fade::None;
	}
}

float Scene::FadeAlpha() const
{
	if (m_Fade == Fade::None)
	{
		return 0.f;
	}

	float fadeOut, hold, fadeIn;
	FadeTimes(fadeOut, hold, fadeIn);

	if (m_FadeElapsed < fadeOut)
	{
		return m_FadeElapsed / fadeOut;
	}
	if (m_FadeElapsed > fadeOut + hold)
	{
		return 1.f - (m_FadeElapsed - fadeOut - hold) / fadeIn;
	}
	return 1.f;
}

void Scene::DrawFade(Renderer* renderer) const
{
	if (m_Fade == Fade::None)
	{
		return;
	}

	// The same dark the world already dissolves into at its edges, so dying
	// reads as the dark closing in rather than a screen going black.
	Palette::Color fog = CurrentAtmosphere().fog;
	renderer->DrawSolidRect(0.f, 0.f, (float)m_WindowSizeX, (float)m_WindowSizeY,
		fog.r, fog.g, fog.b, FadeAlpha());
}

void Scene::RenderText(TextRenderer* text) const
{
	// A cutscene has the screen to itself: its caption and nothing else.
	if (m_Cutscene.IsPlaying())
	{
		m_Cutscene.RenderText(text, m_WindowSizeX, m_WindowSizeY);
		return;
	}

	float top = m_WindowSizeY / 2.f;
	float bottom = -m_WindowSizeY / 2.f;

	if (m_BannerTimer > 0.f)
	{
		float alpha = (m_BannerTimer < kBannerFade) ? m_BannerTimer / kBannerFade : 1.f;
		text->Draw(TextRenderer::Face::Title, Regions::Get(m_Region).name, 0.f, top - kBannerDrop,
			Palette::kUiBone, alpha, TextRenderer::Align::Center);
	}

	Interaction interaction = AvailableInteraction();
	if (interaction != Interaction::None)
	{
		const char* prompt = "F  월드맵";
		if (interaction == Interaction::Temple)
		{
			prompt = "F  신전";
		}
		else if (interaction == Interaction::Duel)
		{
			prompt = "F  결투";
		}
		else if (interaction == Interaction::Talk)
		{
			bool reading = !m_AtKingToTalk && m_Listener != nullptr && m_Listener->IsReadable();
			prompt = reading ? "F  읽기" : "F  대화";
		}
		else if (interaction == Interaction::Board)
		{
			prompt = "F  게시판";
		}
		else if (interaction == Interaction::Door)
		{
			prompt = Regions::IsCastle(m_Region) ? "F  나가기" : "F  성으로";
		}
		text->Draw(TextRenderer::Face::Body, prompt, 0.f, bottom + 44.f,
			Palette::kUiBone, 0.9f, TextRenderer::Align::Center);
	}

	int raiders = m_Prologue.RaidersLeft();
	int besiegers = m_Events.RescueLeft();
	if (m_Fade == Fade::None && (raiders > 0 || besiegers > 0))
	{
		std::string goal = (raiders > 0)
			? "잿빛의 존재들을 물리쳐라  ·  남은 수 " + std::to_string(raiders)
			: "잿빛에 둘러싸인 이를 구하라  ·  남은 수 " + std::to_string(besiegers);
		text->Draw(TextRenderer::Face::Body, goal.c_str(), 0.f, top - kRemarkDrop, Palette::kUiBone, 0.9f,
			TextRenderer::Align::Center);
	}

	if (m_King != nullptr && m_King->Remark() != nullptr)
	{
		float alpha = m_King->RemarkAlpha();
		float lineY = top - kRemarkDrop;
		text->Draw(TextRenderer::Face::Body, m_King->Remark(), 0.f, lineY, Palette::kUiBone, alpha, TextRenderer::Align::Center);
		if (m_King->Aside() != nullptr)
		{
			lineY -= kRemarkSpacing;
			text->Draw(TextRenderer::Face::Body, m_King->Aside(), 0.f, lineY, Palette::kUiNotch, alpha, TextRenderer::Align::Center);
		}
		// Each king that yields gives him a name of its own.
		if (m_King->HasYielded())
		{
			lineY -= kRemarkSpacing;
			const char* title = (m_Region == Regions::LumenCastle) ? Titles::kDarkestLight : Titles::kBrightestDark;
			std::string line = std::string(kTitlePrefix) + title;
			text->Draw(TextRenderer::Face::Body, line.c_str(), 0.f, lineY, Palette::kUiBone, alpha, TextRenderer::Align::Center);
		}
	}

	// The main road's next step, under the minimap.
	const char* goal = Quests::MainGoal(m_Progress);
	if (goal != nullptr && m_Fade == Fade::None)
	{
		float right = m_WindowSizeX / 2.f - kMinimapMargin;
		float lineY = top - kMinimapMargin - kMinimapSize - kMinimapBorder * 2.f - kGoalDrop;
		text->Draw(TextRenderer::Face::Small, "목표", right, lineY + kGoalLabelRise, Palette::kMapGoal, 0.9f,
			TextRenderer::Align::Right);
		text->Draw(TextRenderer::Face::Body, goal, right, lineY, Palette::kUiBone, 0.9f, TextRenderer::Align::Right);
	}

	if (m_Fade == Fade::Death)
	{
		// Taken by the gray, he is told so - and in its colour.
		const char* line = m_DeathByGray ? "잿빛에 삼켜졌다" : "쓰러졌다";
		const Palette::Color& color = m_DeathByGray ? Palette::kAsh : Palette::kUiBone;
		text->Draw(TextRenderer::Face::Title, line, 0.f, 0.f, color, FadeAlpha(), TextRenderer::Align::Center);
	}
}

void Scene::DrawErosionTint(Renderer* renderer, const PlayerStatus& status) const
{
	float alpha = status.WarningLevel() * kAshTintWarning;
	if (status.IsEroding())
	{
		alpha += kAshTintEroding * (0.75f + 0.25f * sinf(status.erosionTime * kAshTintPulseRate));
	}

	if (alpha <= 0.f)
	{
		return;
	}

	renderer->DrawSolidRect(0.f, 0.f, (float)m_WindowSizeX, (float)m_WindowSizeY,
		Palette::kAshVeil.r, Palette::kAshVeil.g, Palette::kAshVeil.b, alpha);
}

void Scene::RecordPopulation()
{
	int monsters = 0;
	int projectiles = 0;
	int particles = 0;

	m_Graph.ForEach<MonsterActor>([&](MonsterActor&)
	{
		monsters++;
	});
	m_Graph.ForEach<ProjectileActor>([&](ProjectileActor&)
	{
		projectiles++;
	});
	m_Graph.ForEach<ParticleActor>([&](ParticleActor&)
	{
		particles++;
	});

	Profiler::Get().SetGauge("monsters", monsters);
	Profiler::Get().SetGauge("projectiles", projectiles);
	Profiler::Get().SetGauge("particles", particles);
}

void Scene::Render(Renderer* renderer, PlayerStatus& status)
{
	ActorContext ctx = MakeContext(0.f, status);

	// collect: actors deciding what is visible and pushing quads. This is the
	// per-frame terrain walk - it is meant to be cheap, and this is where to
	// look if it ever is not.
	{
		ProfileScope scope("collect");
		m_DrawList.Clear();
		// A small pool of ground around him at full value, then a long fall-off
		// into the region's own dark - the same the window clears to - so the
		// world dissolves instead of ending at a visible edge.
		Regions::Atmosphere mood = CurrentAtmosphere();
		float reach = m_WindowSizeY / kAtmosphereWindowY;
		m_DrawList.SetAtmosphere(mood.lightRadius * reach, mood.fogStart * reach, mood.fogEnd * reach,
			mood.fog.r, mood.fog.g, mood.fog.b);
		m_Graph.Draw(m_DrawList, ctx);
	}

	{
		ProfileScope scope("sort");
		m_DrawList.Sort();
	}

	{
		ProfileScope scope("submit");
		renderer->SetWorldZoom(m_Zoom);
		m_DrawList.Submit(renderer);
		renderer->SetWorldZoom(1.f);
	}

	// Over the world only; the HUD drawn after it stays readable, so the
	// health bar can be seen refilling.
	DrawErosionTint(renderer, status);
	DrawFade(renderer);
	m_Cutscene.Render(renderer, m_WindowSizeX, m_WindowSizeY);

	Profiler::Get().SetGauge("world_quads", (int)m_DrawList.Count());
}

void Scene::RenderMinimap(Renderer* renderer)
{
	float right = m_WindowSizeX / 2.f;
	float top = m_WindowSizeY / 2.f;

	// In the top right corner, out of the way of everything else.
	float centerX = right - kMinimapSize / 2.f - kMinimapMargin;
	float centerY = top - kMinimapSize / 2.f - kMinimapMargin;

	renderer->DrawSolidRect(centerX, centerY,
		kMinimapSize + kMinimapBorder * 2.f, kMinimapSize + kMinimapBorder * 2.f,
		Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
	renderer->DrawSolidRect(centerX, centerY, kMinimapSize, kMinimapSize, Palette::kMapWell.r, Palette::kMapWell.g, Palette::kMapWell.b, 1.f);

	renderer->SetClip(centerX - kMinimapSize / 2.f, centerY - kMinimapSize / 2.f, kMinimapSize, kMinimapSize);

	// Turned the way the world is seen on screen, so up on the map is up on
	// the screen: a tile is a little diamond, kMinimapScale across.
	float playerCol, playerRow;
	WorldGrid::InverseIso(m_Player->WorldX(), m_Player->WorldY(), playerCol, playerRow);
	auto toMap = [&](float col, float row, float& x, float& y)
	{
		float dc = col - playerCol;
		float dr = row - playerRow;
		x = centerX + (dc - dr) * kMinimapScale * 0.5f;
		y = centerY - (dc + dr) * kMinimapScale * 0.25f;
	};

	// The ground: a coarse diamond for every few tiles, in the colour of
	// whose land it is, lighter where it stands higher. Cliffs and fog are
	// left dark.
	int cell = kMinimapCell;
	int reach = (int)kMinimapRange / cell + 1;
	int baseCol = (int)floorf(playerCol / cell);
	int baseRow = (int)floorf(playerRow / cell);
	for (int j = baseRow - reach; j <= baseRow + reach; j++)
	{
		for (int i = baseCol - reach; i <= baseCol + reach; i++)
		{
			int col = i * cell + cell / 2;
			int row = j * cell + cell / 2;
			if (col < 0 || row < 0 || col >= WorldGrid::MapCols() || row >= WorldGrid::MapRows())
			{
				continue;
			}
			int zone = WorldGrid::ZoneAt(col, row);
			if (zone < 0)
			{
				continue;
			}
			int region = m_InWorld ? zone : m_Region;
			Palette::Color ground = Palette::kMapMixed;
			switch (Regions::Get(region).style)
			{
			case Regions::Style::Light:
			{
				ground = Palette::kMapLight;
				break;
			}
			case Regions::Style::Dark:
			{
				ground = Palette::kMapDark;
				break;
			}
			case Regions::Style::Ashen:
			{
				ground = Palette::kMapAshen;
				break;
			}
			case Regions::Style::Borderborn:
			{
				ground = Palette::kMapBorder;
				break;
			}
			default:
			{
				break;
			}
			}
			float lift = 1.f + WorldGrid::Elevation(col, row) * kMinimapHeightShade;
			float x, y;
			toMap((float)col, (float)row, x, y);
			renderer->DrawSolidDiamond(x, y, cell * kMinimapScale + 1.f, cell * kMinimapScale * 0.5f + 1.f,
				ground.r * lift, ground.g * lift, ground.b * lift, 1.f);
		}
	}

	// What stands on it: the settlements' buildings, the walled boards, and
	// every waystone and altar.
	for (int region = 0; region < Regions::Count; region++)
	{
		const Settlement::Layout& settlement = m_Settlements[region];
		for (size_t i = 0; i < settlement.buildings.size(); i++)
		{
			const Settlement::Building& building = settlement.buildings[i];
			if (fabsf(building.col - playerCol) > kMinimapRange * 1.5f || fabsf(building.row - playerRow) > kMinimapRange * 1.5f)
			{
				continue;
			}
			float x, y;
			toMap(building.col, building.row, x, y);
			renderer->DrawSolidDiamond(x, y, building.footprint * kMinimapScale, building.footprint * kMinimapScale * 0.5f,
				Palette::kMapBuilding.r, Palette::kMapBuilding.g, Palette::kMapBuilding.b, 1.f);
		}
		if (settlement.hasTemple)
		{
			float x, y;
			toMap(settlement.altarCol, settlement.altarRow, x, y);
			renderer->DrawSolidDiamond(x, y, 7.f, 9.f, Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b, 0.9f);
		}
		if (m_Arenas[region] != nullptr)
		{
			int boardCol, boardRow;
			Bosses::BoardCenter(region, boardCol, boardRow);
			float size = (Bosses::WallReach() * 2.f + 1.f) * kMinimapScale;
			float x, y;
			toMap((float)boardCol, (float)boardRow, x, y);
			renderer->DrawSolidDiamond(x, y, size + 4.f, size * 0.5f + 2.f,
				Palette::kMapWall.r, Palette::kMapWall.g, Palette::kMapWall.b, 1.f);
			renderer->DrawSolidDiamond(x, y, size - 2.f, size * 0.5f - 1.f,
				Palette::kMapWell.r, Palette::kMapWell.g, Palette::kMapWell.b, 1.f);
		}
		if (m_InWorld && !Regions::IsOffMap(region))
		{
			float col, row, x, y;
			Regions::Center(region, col, row);
			toMap(col, row, x, y);
			renderer->DrawSolidDiamond(x, y, 9.f, 11.f, Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
			renderer->DrawSolidDiamond(x, y, 6.f, 8.f, Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b, 1.f);
		}
	}

	// Who is about: people he can talk to, the gray, and its bosses.
	m_Graph.ForEach<NpcActor>([&](NpcActor& npc)
	{
		if (npc.IsDestroyed() || npc.Conversation() < 0)
		{
			return;
		}
		float col, row, x, y;
		WorldGrid::InverseIso(npc.WorldX(), npc.WorldY(), col, row);
		toMap(col, row, x, y);
		renderer->DrawSolidRect(x, y, 4.f, 4.f, Palette::kMapPerson.r, Palette::kMapPerson.g, Palette::kMapPerson.b, 1.f);
	});

	m_Graph.ForEach<MonsterActor>([&](MonsterActor& monster)
	{
		float col, row, x, y;
		WorldGrid::InverseIso(monster.WorldX(), monster.WorldY(), col, row);
		toMap(col, row, x, y);
		renderer->DrawSolidRect(x, y, 4.f, 4.f, Palette::kMapEnemy.r, Palette::kMapEnemy.g, Palette::kMapEnemy.b, 1.f);
	});

	m_Graph.ForEach<BossActor>([&](BossActor& boss)
	{
		float col, row, x, y;
		WorldGrid::InverseIso(boss.WorldX(), boss.WorldY(), col, row);
		toMap(col, row, x, y);
		renderer->DrawSolidDiamond(x, y, 11.f, 11.f, Palette::kMapEnemy.r, Palette::kMapEnemy.g, Palette::kMapEnemy.b, 1.f);
	});

	renderer->ClearClip();

	// Where the main road leads: on the map if it is in reach, or pinned to
	// its edge in that direction if not.
	if (m_InWorld)
	{
		int targets[2];
		int count = Quests::MainTargets(m_Progress, targets);
		float half = kMinimapSize / 2.f - kGoalInset;
		for (int i = 0; i < count; i++)
		{
			float col, row, x, y;
			Regions::Center(targets[i], col, row);
			toMap(col, row, x, y);
			float dx = x - centerX;
			float dy = y - centerY;
			float outward = (fabsf(dx) > fabsf(dy)) ? fabsf(dx) : fabsf(dy);
			if (outward > half)
			{
				dx *= half / outward;
				dy *= half / outward;
			}
			renderer->DrawSolidDiamond(centerX + dx, centerY + dy, kGoalMarker + 4.f, kGoalMarker + 4.f,
				Palette::kUiInk.r, Palette::kUiInk.g, Palette::kUiInk.b, 1.f);
			renderer->DrawSolidDiamond(centerX + dx, centerY + dy, kGoalMarker, kGoalMarker,
				Palette::kMapGoal.r, Palette::kMapGoal.g, Palette::kMapGoal.b, 1.f);
		}
	}

	// The player is always exactly at the center and always on top.
	renderer->DrawSolidRect(centerX, centerY, 8.f, 8.f, Palette::kMapPlayerRim.r, Palette::kMapPlayerRim.g, Palette::kMapPlayerRim.b, 1.f);
	renderer->DrawSolidRect(centerX, centerY, 6.f, 6.f, Palette::kMapPlayer.r, Palette::kMapPlayer.g, Palette::kMapPlayer.b, 1.f);
}

void Scene::RenderBossBar(Renderer* renderer, TextRenderer* text)
{
	BossActor* awake = nullptr;
	m_Graph.ForEach<BossActor>([&](BossActor& boss)
	{
		if (boss.IsAwake() && !boss.IsDestroyed() && boss.HealthFraction() > 0.f)
		{
			awake = &boss;
		}
	});
	if (awake == nullptr || m_Fade != Fade::None)
	{
		return;
	}

	float barY = -m_WindowSizeY / 2.f + kBossBarRise;
	float fill = kBossBarWidth * awake->HealthFraction();

	renderer->DrawSolidRect(0.f, barY, kBossBarWidth + 6.f, kBossBarHeight + 6.f,
		Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
	renderer->DrawSolidRect(0.f, barY, kBossBarWidth, kBossBarHeight,
		Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 1.f);
	// Past its time, the bar burns and says so.
	bool furious = awake->FuryLevel() > 0;
	const Palette::Color& barColor = furious ? Palette::kFury : Palette::kHealth;
	if (fill > 0.f)
	{
		renderer->DrawSolidRect(-kBossBarWidth / 2.f + fill / 2.f, barY, fill, kBossBarHeight,
			barColor.r, barColor.g, barColor.b, 1.f);
	}

	std::string name = awake->Name();
	if (awake->Hint() != nullptr)
	{
		name += "  ·  ";
		name += awake->Hint();
	}
	if (furious)
	{
		name += "  ·  광폭";
	}
	text->Draw(TextRenderer::Face::Body, name.c_str(), 0.f, barY + 14.f,
		furious ? Palette::kFury : Palette::kUiBone, 1.f, TextRenderer::Align::Center);
}
