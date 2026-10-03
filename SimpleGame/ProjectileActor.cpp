#include "stdafx.h"
#include "ProjectileActor.h"
#include "MonsterActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "PlayerStatus.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kGlowRadius = 26.f;
	const float kMarkWidth = 12.f;
	const float kMarkHeight = 8.f;
	const float kFlightHeight = 14.f;
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
	bool spent = false;

	ctx.graph->ForEach<MonsterActor>([&](MonsterActor& monster)
	{
		if (spent)
		{
			return;
		}

		float dx = monster.WorldX() - x;
		float dy = monster.WorldY() - y;
		if (sqrtf(dx * dx + dy * dy) > Tuning::LightForm::kBoltHitRadiusPx)
		{
			return;
		}

		// Both colours hurt a gray thing equally - gray is opposed to colour
		// itself, not to one side of it. A shield can still catch it, judged
		// from the side the bolt came in on.
		monster.TakeHit(Combat::BoltDamage(*ctx.status), x - m_VelX, y - m_VelY);
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
	out.AddDiamond(depth, x, baseY, kMarkWidth, kMarkHeight, coreR, coreG, coreB, 1.f);
}
