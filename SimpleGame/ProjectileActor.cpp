#include "stdafx.h"
#include "ProjectileActor.h"
#include "EnemyActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "PlayerStatus.h"
#include "Tuning.h"
#include "SparkActor.h"
#include <cmath>

namespace
{
	const float kGlowRadius = 26.f;
	const float kPixelsPerTile = 36.f;
	// The bolt: a shard of light long along its flight, and a tail of fading
	// sparks behind it.
	const float kShardLength = 18.f;
	const float kShardWidth = 6.f;
	const int kTailSparks = 5;
	const float kTailSpacing = 7.f;
	const float kFlightHeight = 14.f;

	const int kShotDust = 4;
	const float kShotDustSpeed = 80.f;
	const float kShotDustLift = 14.f;
}

ProjectileActor::ProjectileActor(Alignment alignment, float velX, float velY)
	: m_Alignment(alignment)
	, m_VelX(velX)
	, m_VelY(velY)
	, m_Life(Tuning::LightForm::kBoltLifetime)
{
}

void ProjectileActor::OnUpdate(const ActorContext& ctx)
{
	SetLocalPosition(LocalX() + m_VelX * ctx.deltaTime, LocalY() + m_VelY * ctx.deltaTime);

	m_Life -= ctx.deltaTime;
	if (m_Life <= 0.f)
	{
		Destroy();
		return;
	}

	float x = WorldX();
	float y = WorldY();

	// Stopped by anything standing in the way, in a puff of grit.
	float col, row;
	WorldGrid::InverseIso(x, y, col, row);
	if (WorldGrid::BlocksShot(col, row))
	{
		SparkActor::Burst(*ctx.graph, x, y, SparkActor::Style::Dust, kShotDust, kShotDustSpeed, kShotDustLift);
		Destroy();
		return;
	}

	bool spent = false;

	ctx.graph->ForEach<EnemyActor>([&](EnemyActor& enemy)
	{
		if (spent)
		{
			return;
		}

		float dx = enemy.WorldX() - x;
		float dy = enemy.WorldY() - y;
		// A body a tile across is roughly this many pixels on screen.
		if (sqrtf(dx * dx + dy * dy) > Tuning::LightForm::kBoltHitRadiusPx + enemy.BodyRadius() * kPixelsPerTile)
		{
			return;
		}

		// Both colours hurt a gray thing equally - gray is opposed to colour
		// itself, not to one side of it. A shield can still catch it, judged
		// from the side the bolt came in on.
		enemy.TakeHit(Combat::BoltDamage(*ctx.status), x - m_VelX, y - m_VelY);
		spent = true;
	});

	if (spent)
	{
		Destroy();
	}
}

void ProjectileActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);

	if (!ctx.IsOnScreen(x, y, kGlowRadius, kGlowRadius))
	{
		return;
	}

	float baseY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY()) + kFlightHeight;
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.3f;

	float glowR, glowG, glowB;
	AlignmentColor::Glow(m_Alignment, glowR, glowG, glowB);
	out.AddGlow(depth - 0.01f, x, baseY, kGlowRadius, glowR, glowG, glowB, 1.f);

	float coreR, coreG, coreB;
	AlignmentColor::Core(m_Alignment, coreR, coreG, coreB);
	out.SetGlow(1.f);

	float speed = sqrtf(m_VelX * m_VelX + m_VelY * m_VelY);
	float dirX = (speed > 0.001f) ? m_VelX / speed : 1.f;
	float dirY = (speed > 0.001f) ? m_VelY / speed : 0.f;
	float sideX = -dirY * kShardWidth * 0.5f;
	float sideY = dirX * kShardWidth * 0.5f;
	float shard[8] =
	{
		x + dirX * kShardLength * 0.6f, baseY + dirY * kShardLength * 0.6f,
		x + sideX, baseY + sideY,
		x - dirX * kShardLength * 0.4f, baseY - dirY * kShardLength * 0.4f,
		x - sideX, baseY - sideY,
	};
	out.AddCorners(depth, shard, coreR + (1.f - coreR) * 0.5f, coreG + (1.f - coreG) * 0.5f, coreB + (1.f - coreB) * 0.5f, 1.f);

	for (int i = 1; i <= kTailSparks; i++)
	{
		float back = i * kTailSpacing;
		float shrink = 1.f - i / (float)(kTailSparks + 1);
		out.AddDiamond(depth - 0.002f * i, x - dirX * back, baseY - dirY * back, 5.f * shrink, 5.f * shrink,
			coreR, coreG, coreB, 0.8f * shrink);
	}
}
