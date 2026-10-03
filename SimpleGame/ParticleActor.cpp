#include "stdafx.h"
#include "ParticleActor.h"
#include "DrawList.h"
#include "PlayerStatus.h"
#include "WorldGrid.h"
#include <cmath>

namespace
{
	const float kFriction = 3.f; // velocity decay per second
	const float kMagnetRadius = 90.f;
	const float kMagnetAccel = 260.f;
	const float kAbsorbRadius = 22.f;
	const float kLifetime = 20.f;

	const float kMarkWidth = 7.f;
	const float kMarkHeight = 5.f;
	const float kHoverHeight = 10.f;
}

ParticleActor::ParticleActor(Alignment alignment, float velX, float velY)
	: m_Alignment(alignment)
	, m_VelX(velX)
	, m_VelY(velY)
	, m_Life(kLifetime)
{
}

void ParticleActor::OnUpdate(const ActorContext& ctx)
{
	float friction = 1.f - kFriction * ctx.deltaTime;
	if (friction < 0.f)
	{
		friction = 0.f;
	}
	m_VelX *= friction;
	m_VelY *= friction;

	float toPlayerX = ctx.playerX - WorldX();
	float toPlayerY = ctx.playerY - WorldY();
	float distance = sqrtf(toPlayerX * toPlayerX + toPlayerY * toPlayerY);

	if (distance < kMagnetRadius && distance > 0.001f)
	{
		m_VelX += (toPlayerX / distance) * kMagnetAccel * ctx.deltaTime;
		m_VelY += (toPlayerY / distance) * kMagnetAccel * ctx.deltaTime;
	}

	SetLocalPosition(LocalX() + m_VelX * ctx.deltaTime, LocalY() + m_VelY * ctx.deltaTime);
	m_Life -= ctx.deltaTime;

	if (distance < kAbsorbRadius)
	{
		if (m_Alignment == Alignment::Light)
		{
			ctx.status->AddLight(1.f);
		}
		else
		{
			ctx.status->AddDark(1.f);
		}
		Destroy();
		return;
	}

	if (m_Life <= 0.f)
	{
		Destroy();
	}
}

void ParticleActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);

	if (!ctx.IsOnScreen(x, y, 16.f, 16.f))
	{
		return;
	}

	float baseY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY()) + kHoverHeight;
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.3f;

	float r, g, b;
	AlignmentColor::Core(m_Alignment, r, g, b);

	// Motes of light shine; motes of dark do not - dark gives off no light.
	out.SetGlow(m_Alignment == Alignment::Light ? 1.f : 0.f);
	out.AddDiamond(depth, x, baseY, kMarkWidth, kMarkHeight, r, g, b, 1.f);
}
