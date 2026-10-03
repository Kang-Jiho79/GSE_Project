#include "stdafx.h"
#include "Scene.h"
#include "Renderer.h"
#include "PlayerStatus.h"
#include "Profiler.h"
#include "WorldGrid.h"
#include "Random.h"
#include "Tuning.h"
#include <cmath>

#include "TerrainActor.h"
#include "PropsActor.h"
#include "PlayerActor.h"
#include "MonsterActor.h"
#include "ProjectileActor.h"
#include "ParticleActor.h"

namespace
{
	const int kMonsterCount = 24;
	const int kSpawnMargin = 15;

	const float kPlayerStartCol = 60.f;
	const float kPlayerStartRow = 60.f;

	// Dark fantasy: a small pool of ground around the player at full value, then
	// a long fall-off into the same near-black the window clears to, so the
	// world dissolves into the dark instead of ending at a visible edge.
	const float kLightRadius = 150.f;
	const float kFogStart = 210.f;
	const float kFogEnd = 620.f;
	const float kFogR = 0.045f;
	const float kFogG = 0.043f;
	const float kFogB = 0.055f;

	// A gray veil over the world: faint while he is warned, heavier and
	// breathing once the gray is eating him.
	const float kAshTintR = 0.42f;
	const float kAshTintG = 0.42f;
	const float kAshTintB = 0.44f;
	const float kAshTintWarning = 0.07f;
	const float kAshTintEroding = 0.10f;
	const float kAshTintPulseRate = 6.f;

	const float kMinimapSize = 150.f;
	const float kMinimapBorder = 4.f;
	const float kMinimapRange = 42.f; // tiles from center to edge
}

Scene::Scene()
{
	BuildLevel();
}

void Scene::SetViewport(int windowSizeX, int windowSizeY)
{
	m_WindowSizeX = windowSizeX;
	m_WindowSizeY = windowSizeY;
}

void Scene::BuildLevel()
{
	// Two groups under the root: the ground the level is made of, and the things
	// living on it. Everything spawned later joins the second one.
	Actor* environment = m_Graph.Root().AddChild(std::unique_ptr<Actor>(new Actor()));
	environment->AddChild(std::unique_ptr<Actor>(new TerrainActor()));
	environment->AddChild(std::unique_ptr<Actor>(new PropsActor()));

	Actor* entities = m_Graph.Root().AddChild(std::unique_ptr<Actor>(new Actor()));
	m_Graph.SetSpawnParent(entities);

	std::unique_ptr<PlayerActor> player(new PlayerActor());
	float startX, startY;
	WorldGrid::IsoPosition(kPlayerStartCol, kPlayerStartRow, startX, startY);
	player->SetLocalPosition(startX, startY);
	m_Player = player.get();
	entities->AddChild(std::move(player));

	for (int i = 0; i < kMonsterCount; i++)
	{
		int col = Random::Range(kSpawnMargin, WorldGrid::kMapCols - kSpawnMargin);
		int row = Random::Range(kSpawnMargin, WorldGrid::kMapRows - kSpawnMargin);

		Alignment alignment = (Random::Range(0, 2) == 0) ? Alignment::Light : Alignment::Dark;

		// Rolled separately from alignment, so the build never hints at the side.
		MonsterActor::Kind kind = MonsterActor::Kind::Crawler;
		int kindRoll = Random::Range(0, 3);
		if (kindRoll == 0)
		{
			kind = MonsterActor::Kind::Brute;
		}
		else if (kindRoll == 1)
		{
			kind = MonsterActor::Kind::Stalker;
		}

		std::unique_ptr<MonsterActor> monster(new MonsterActor(alignment, kind));

		float x, y;
		WorldGrid::IsoPosition((float)col, (float)row, x, y);
		monster->SetLocalPosition(x, y);

		entities->AddChild(std::move(monster));
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
	ctx.camX = ctx.playerX;
	ctx.camY = ctx.playerY;

	ctx.windowSizeX = m_WindowSizeX;
	ctx.windowSizeY = m_WindowSizeY;
	ctx.status = &status;
	ctx.graph = &m_Graph;

	return ctx;
}

void Scene::Update(const InputState& input, float deltaTime, PlayerStatus& status)
{
	UpdateDeath(deltaTime, status);

	ActorContext ctx = MakeContext(deltaTime, status);

	// A fallen player gets no say until he is back on his feet; the rest of
	// the world carries on around him.
	if (m_DeathElapsed < 0.f)
	{
		ctx.input = input;
	}

	{
		ProfileScope scope("update");
		m_Graph.Update(ctx);
	}

	RecordPopulation();
}

void Scene::UpdateDeath(float deltaTime, PlayerStatus& status)
{
	if (m_DeathElapsed < 0.f)
	{
		if (status.health <= 0.f)
		{
			m_DeathElapsed = 0.f;
		}
		return;
	}

	float before = m_DeathElapsed;
	m_DeathElapsed += deltaTime;

	// Moved on the frame the screen goes fully dark, so the jump is never seen.
	if (before < Tuning::Death::kFadeOut && m_DeathElapsed >= Tuning::Death::kFadeOut)
	{
		float x, y;
		WorldGrid::IsoPosition(kPlayerStartCol, kPlayerStartRow, x, y);
		m_Player->Respawn(x, y);
		status.Revive();
	}

	float total = Tuning::Death::kFadeOut + Tuning::Death::kHold + Tuning::Death::kFadeIn;
	if (m_DeathElapsed >= total)
	{
		m_DeathElapsed = -1.f;
	}
}

void Scene::DrawDeathFade(Renderer* renderer) const
{
	if (m_DeathElapsed < 0.f)
	{
		return;
	}

	float fadeInStart = Tuning::Death::kFadeOut + Tuning::Death::kHold;

	float alpha = 1.f;
	if (m_DeathElapsed < Tuning::Death::kFadeOut)
	{
		alpha = m_DeathElapsed / Tuning::Death::kFadeOut;
	}
	else if (m_DeathElapsed > fadeInStart)
	{
		alpha = 1.f - (m_DeathElapsed - fadeInStart) / Tuning::Death::kFadeIn;
	}

	// The same near-black the world already dissolves into at its edges, so
	// dying reads as the dark closing in rather than a screen going black.
	renderer->DrawSolidRect(0.f, 0.f, (float)m_WindowSizeX, (float)m_WindowSizeY,
		kFogR, kFogG, kFogB, alpha);
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
		kAshTintR, kAshTintG, kAshTintB, alpha);
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
		m_DrawList.SetAtmosphere(kLightRadius, kFogStart, kFogEnd, kFogR, kFogG, kFogB);
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
	DrawDeathFade(renderer);

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
		0.32f, 0.31f, 0.34f, 1.f);
	renderer->DrawSolidRect(centerX, centerY, kMinimapSize, kMinimapSize, 0.08f, 0.075f, 0.095f, 1.f);

	renderer->SetClip(centerX - kMinimapSize / 2.f, centerY - kMinimapSize / 2.f, kMinimapSize, kMinimapSize);

	// A plain linear col/row offset, not the iso projection - a minimap reads
	// better as a straightforward north-up map than a tilted diamond.
	float pixelsPerTile = (kMinimapSize / 2.f) / kMinimapRange;

	float playerCol, playerRow;
	WorldGrid::InverseIso(m_Player->WorldX(), m_Player->WorldY(), playerCol, playerRow);

	m_Graph.ForEach<MonsterActor>([&](MonsterActor& monster)
	{
		float col, row;
		WorldGrid::InverseIso(monster.WorldX(), monster.WorldY(), col, row);

		float dotX = centerX + (col - playerCol) * pixelsPerTile;
		float dotY = centerY - (row - playerRow) * pixelsPerTile;

		renderer->DrawSolidRect(dotX, dotY, 5.f, 5.f, 0.74f, 0.22f, 0.20f, 1.f);
	});

	renderer->ClearClip();

	// The player is always exactly at the center and always on top.
	renderer->DrawSolidRect(centerX, centerY, 8.f, 8.f, 0.30f, 0.30f, 0.34f, 1.f);
	renderer->DrawSolidRect(centerX, centerY, 6.f, 6.f, 0.94f, 0.92f, 0.88f, 1.f);
}
