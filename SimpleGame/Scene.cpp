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

namespace
{
	const int kSpawnMargin = 15;
	const int kSpawnTries = 20;
	// Nothing grows this close to the waystone.
	const float kWaystoneClearing = 4.f;
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
	const char* const kTitleLine = "이명  ·  가장 어두운 빛이자 가장 밝은 어둠";

	// The boss bar, low in the middle above the controls line.
	const float kBossBarWidth = 420.f;
	const float kBossBarHeight = 10.f;
	const float kBossBarRise = 80.f; // above the bottom of the window

	const float kMinimapSize = 150.f;
	const float kMinimapBorder = 4.f;
	const float kMinimapRange = 42.f; // tiles from center to edge
}

Scene::Scene(Progress& progress)
	: m_Progress(progress)
{
	EnterRegion(Regions::Limina);
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

const Regions::Atmosphere& Scene::CurrentAtmosphere() const
{
	return Regions::AtmosphereOf(m_Region);
}

const bool* Scene::VisitedRegions() const
{
	return m_Progress.visited;
}

Scene::Interaction Scene::AvailableInteraction() const
{
	if (m_Fade != Fade::None)
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
	if (m_AtKingToTalk || m_Listener != nullptr)
	{
		return Interaction::Talk;
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
}

void Scene::NewGame()
{
	m_Fade = Fade::None;
	m_FadeElapsed = 0.f;
	m_Conversation = -1;
	EnterRegion(Regions::Limina);
}

void Scene::Continue(int region)
{
	m_Fade = Fade::None;
	m_FadeElapsed = 0.f;
	m_Conversation = -1;
	EnterRegion(region);

	float x, y;
	RespawnPoint(x, y);
	m_Player->Respawn(x, y);
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
	return m_Events.HoldsSave();
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
	if (m_Fade != Fade::None)
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
	float center = Regions::Get(m_Region).size * 0.5f;
	col = center;
	row = center;
}

void Scene::ArrivalPoint(float& x, float& y) const
{
	float col, row;
	WaystoneTile(col, row);
	WorldGrid::IsoPosition(col + kArrivalOffset, row + kArrivalOffset, x, y);
}

void Scene::RespawnPoint(float& x, float& y) const
{
	if (m_Progress.respawnAtAltar && m_Progress.respawnRegion == m_Region && m_Settlement.hasTemple)
	{
		WorldGrid::IsoPosition(m_Settlement.altarCol + kAltarOffset, m_Settlement.altarRow + kAltarOffset, x, y);
		return;
	}
	ArrivalPoint(x, y);
}

void Scene::BuildSettlement(const Regions::Info& info, Actor* environment)
{
	m_Settlement = Settlement::Layout();
	if (info.kind != Regions::Kind::Village && info.kind != Regions::Kind::Capital)
	{
		return;
	}

	float centerCol, centerRow;
	WaystoneTile(centerCol, centerRow);
	m_Settlement = Settlement::Generate(m_Region, centerCol, centerRow);

	for (size_t i = 0; i < m_Settlement.buildings.size(); i++)
	{
		const Settlement::Building& building = m_Settlement.buildings[i];
		if (Settlement::IsSolid(building))
		{
			float half = building.footprint * 0.5f;
			WorldGrid::AddBlocker(building.col - half, building.row - half, building.col + half, building.row + half);
		}
	}

	environment->AddChild(std::unique_ptr<Actor>(
		new SettlementActor(m_Settlement, info.style, info.kind == Regions::Kind::Capital)));
}

void Scene::EnterRegion(int region)
{
	m_Region = region;
	m_Progress.visited[region] = true;

	const Regions::Info& info = Regions::Get(region);
	WorldGrid::SetMap(info.size, info.size, Regions::Seed(region), info.terraceLevels);

	WorldGrid::ClearBlockers();
	m_Graph.Clear();

	float waystoneCol, waystoneRow;
	WaystoneTile(waystoneCol, waystoneRow);

	// Two groups under the root: the ground the region is made of, and the
	// things living on it. Everything spawned later joins the second one.
	Actor* environment = m_Graph.Root().AddChild(std::unique_ptr<Actor>(new Actor()));
	environment->AddChild(std::unique_ptr<Actor>(new TerrainActor()));
	BuildSettlement(info, environment);

	// Nothing wild grows inside a settlement, only round its edges.
	float clearing = (m_Settlement.radius > kWaystoneClearing) ? m_Settlement.radius : kWaystoneClearing;
	environment->AddChild(std::unique_ptr<Actor>(
		new PropsActor(info.propDensity, waystoneCol, waystoneRow, clearing)));

	Actor* entities = m_Graph.Root().AddChild(std::unique_ptr<Actor>(new Actor()));
	m_Graph.SetSpawnParent(entities);

	float x, y;
	if (!Regions::IsOffMap(region))
	{
		WorldGrid::IsoPosition(waystoneCol, waystoneRow, x, y);
		std::unique_ptr<WaystoneActor> waystone(new WaystoneActor());
		waystone->SetLocalPosition(x, y);
		entities->AddChild(std::move(waystone));
	}

	std::unique_ptr<PlayerActor> player(new PlayerActor());
	ArrivalPoint(x, y);
	player->SetLocalPosition(x, y);
	m_Player = player.get();
	entities->AddChild(std::move(player));

	SpawnMonsters(info);
	m_King = dynamic_cast<KingActor*>(Bosses::Spawn(region, m_Progress, m_Settlement, m_Graph));
	Npcs::Spawn(region, m_Progress, m_Settlement, m_Graph);
	m_Listener = nullptr;

	m_Events.Enter(region, m_Progress, m_Settlement, m_Graph, waystoneCol, waystoneRow);
	m_AtKingToTalk = false;

	if (region == Regions::Limina && !m_Progress.Has(Story::VillageRaided))
	{
		m_Prologue.Begin();
	}
	else
	{
		m_Prologue.Stop();
	}

	m_AtWaystone = false;
	m_AtAltar = false;
	m_AtKing = false;
	m_BannerTimer = kBannerTime;
}

void Scene::SpawnMonsters(const Regions::Info& info)
{
	int margin = kSpawnMargin;
	if (margin > info.size / 6)
	{
		margin = info.size / 6;
	}

	float waystoneCol, waystoneRow;
	WaystoneTile(waystoneCol, waystoneRow);

	// The side split is exact, so a region's leaning is what the player
	// meets; which of them are mages is left to chance.
	int lightCount = (int)(info.monsterCount * info.lightShare + 0.5f);

	for (int i = 0; i < info.monsterCount; i++)
	{
		int col = 0;
		int row = 0;
		for (int attempt = 0; attempt < kSpawnTries; attempt++)
		{
			col = Random::Range(margin, info.size - margin);
			row = Random::Range(margin, info.size - margin);

			float dc = col - waystoneCol;
			float dr = row - waystoneRow;
			if (dc * dc + dr * dr >= Tuning::Travel::kSpawnClearance * Tuning::Travel::kSpawnClearance
				&& !WorldGrid::IsBlocked((float)col, (float)row))
			{
				break;
			}
		}

		Alignment alignment = (i < lightCount) ? Alignment::Light : Alignment::Dark;
		MonsterActor::Kind kind = (Random::Unit() < info.mageShare) ? MonsterActor::Kind::Mage : MonsterActor::Kind::Knight;

		std::unique_ptr<MonsterActor> monster(new MonsterActor(kind, alignment));
		float x, y;
		WorldGrid::IsoPosition((float)col, (float)row, x, y);
		monster->SetLocalPosition(x, y);
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
	ctx.camX = ctx.playerX + m_Impact.OffsetX();
	ctx.camY = ctx.playerY + m_Impact.OffsetY();

	ctx.windowSizeX = m_WindowSizeX;
	ctx.windowSizeY = m_WindowSizeY;
	ctx.status = &status;
	ctx.graph = &m_Graph;
	ctx.impact = &m_Impact;
	ctx.progress = &m_Progress;

	return ctx;
}

void Scene::Update(const InputState& input, float deltaTime, PlayerStatus& status)
{
	UpdateFade(deltaTime, status);

	// A hit stop holds the world still while real time - the fades, the
	// shake dying away - carries on.
	float worldTime = m_Impact.Advance(deltaTime);
	ActorContext ctx = MakeContext(worldTime, status);

	// Under a fade he gets no say; the rest of the world carries on around him.
	if (m_Fade == Fade::None)
	{
		ctx.input = input;
	}

	{
		ProfileScope scope("update");
		m_Graph.Update(ctx);
	}

	float waystoneCol, waystoneRow;
	WaystoneTile(waystoneCol, waystoneRow);
	m_AtWaystone = !Regions::IsOffMap(m_Region) && IsNear(waystoneCol, waystoneRow, Tuning::Travel::kWaystoneReach);
	m_AtAltar = m_Settlement.hasTemple
		&& IsNear(m_Settlement.altarCol, m_Settlement.altarRow, Tuning::Travel::kWaystoneReach);

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

	// Having taken up the gray, he holds it.
	if (m_Progress.Has(Story::AcceptAsh))
	{
		status.ashUnlocked = true;
	}

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

	// A king that has yielded sends him on, to the old battlefield.
	if (m_King != nullptr && m_King->TakeSendOff())
	{
		BeginTravel(Regions::Bellum);
	}

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
			// He rises at the last temple he rested at, even if that is in
			// another region altogether.
			// In the god's place there is no going back: he rises at its
			// edge, and it begins again.
			if (Regions::IsOffMap(m_Region))
			{
				EnterRegion(m_Region);
			}
			else if (m_Progress.respawnRegion != m_Region)
			{
				EnterRegion(m_Progress.respawnRegion);
			}

			float x, y;
			RespawnPoint(x, y);
			m_Player->Respawn(x, y);
			status.Revive();
		}
		else
		{
			EnterRegion(m_TravelTarget);
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
	const Palette::Color& fog = CurrentAtmosphere().fog;
	renderer->DrawSolidRect(0.f, 0.f, (float)m_WindowSizeX, (float)m_WindowSizeY,
		fog.r, fog.g, fog.b, FadeAlpha());
}

void Scene::RenderText(TextRenderer* text) const
{
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
			prompt = "F  대화";
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
		// Yielded to by both kings, he has a name.
		if (m_King->HasYielded() && m_Progress.HasTitle())
		{
			lineY -= kRemarkSpacing;
			text->Draw(TextRenderer::Face::Body, kTitleLine, 0.f, lineY, Palette::kUiBone, alpha, TextRenderer::Align::Center);
		}
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
		const Regions::Atmosphere& mood = CurrentAtmosphere();
		m_DrawList.SetAtmosphere(mood.lightRadius, mood.fogStart, mood.fogEnd, mood.fog.r, mood.fog.g, mood.fog.b);
		m_Graph.Draw(m_DrawList, ctx);
	}

	{
		ProfileScope scope("sort");
		m_DrawList.Sort();
	}

	{
		ProfileScope scope("submit");
		m_DrawList.Submit(renderer);
	}

	// Over the world only; the HUD drawn after it stays readable, so the
	// health bar can be seen refilling.
	DrawErosionTint(renderer, status);
	DrawFade(renderer);

	Profiler::Get().SetGauge("world_quads", (int)m_DrawList.Count());
}

void Scene::RenderMinimap(Renderer* renderer)
{
	float left = -m_WindowSizeX / 2.f;
	float top = m_WindowSizeY / 2.f;

	// Sits directly under the vessel/health block in the same corner.
	float centerX = left + 95.f;
	float centerY = top - 220.f;

	renderer->DrawSolidRect(centerX, centerY,
		kMinimapSize + kMinimapBorder * 2.f, kMinimapSize + kMinimapBorder * 2.f,
		Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
	renderer->DrawSolidRect(centerX, centerY, kMinimapSize, kMinimapSize, Palette::kMapWell.r, Palette::kMapWell.g, Palette::kMapWell.b, 1.f);

	renderer->SetClip(centerX - kMinimapSize / 2.f, centerY - kMinimapSize / 2.f, kMinimapSize, kMinimapSize);

	// A plain linear col/row offset, not the iso projection - a minimap reads
	// better as a straightforward north-up map than a tilted diamond.
	float pixelsPerTile = (kMinimapSize / 2.f) / kMinimapRange;

	float playerCol, playerRow;
	WorldGrid::InverseIso(m_Player->WorldX(), m_Player->WorldY(), playerCol, playerRow);

	// The waystone, so the way out can always be found.
	float waystoneCol, waystoneRow;
	WaystoneTile(waystoneCol, waystoneRow);
	renderer->DrawSolidDiamond(centerX + (waystoneCol - playerCol) * pixelsPerTile,
		centerY - (waystoneRow - playerRow) * pixelsPerTile, 9.f, 9.f,
		Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b, 0.8f);

	m_Graph.ForEach<MonsterActor>([&](MonsterActor& monster)
	{
		float col, row;
		WorldGrid::InverseIso(monster.WorldX(), monster.WorldY(), col, row);

		float dotX = centerX + (col - playerCol) * pixelsPerTile;
		float dotY = centerY - (row - playerRow) * pixelsPerTile;

		renderer->DrawSolidRect(dotX, dotY, 5.f, 5.f, Palette::kMapEnemy.r, Palette::kMapEnemy.g, Palette::kMapEnemy.b, 1.f);
	});

	m_Graph.ForEach<BossActor>([&](BossActor& boss)
	{
		float col, row;
		WorldGrid::InverseIso(boss.WorldX(), boss.WorldY(), col, row);
		renderer->DrawSolidDiamond(centerX + (col - playerCol) * pixelsPerTile, centerY - (row - playerRow) * pixelsPerTile,
			11.f, 11.f, Palette::kMapEnemy.r, Palette::kMapEnemy.g, Palette::kMapEnemy.b, 1.f);
	});

	renderer->ClearClip();

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
