#include "stdafx.h"
#include "Scene.h"
#include "Renderer.h"
#include "PlayerStatus.h"
#include "WorldGrid.h"
#include "Random.h"

#include "TerrainActor.h"
#include "RuinsActor.h"
#include "PlayerActor.h"
#include "MonsterActor.h"

namespace
{
	const int kMonsterCount = 24;
	const int kSpawnMargin = 15;

	const float kPlayerStartCol = 60.f;
	const float kPlayerStartRow = 60.f;

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
	environment->AddChild(std::unique_ptr<Actor>(new RuinsActor()));

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
		std::unique_ptr<MonsterActor> monster(new MonsterActor(alignment));

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
	ActorContext ctx = MakeContext(deltaTime, status);
	ctx.input = input;

	m_Graph.Update(ctx);
}

void Scene::Render(Renderer* renderer, PlayerStatus& status)
{
	ActorContext ctx = MakeContext(0.f, status);

	m_DrawList.Clear();
	m_Graph.Draw(m_DrawList, ctx);
	m_DrawList.SubmitSorted(renderer);
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
