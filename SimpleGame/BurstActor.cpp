#include "stdafx.h"
#include "BurstActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include <cmath>

namespace
{
	const float kLifetime = 0.3f;
	const int kSegments = 24;
	const float kRingWidth = 0.35f;   // tiles
	const float kSmallest = 0.3f;     // tiles; where an inward ring ends and an outward one starts
	const float kLift = 3.f;          // pixels off the ground, just clear of the tiles
	const float kPixelsPerTile = 32.f; // rough screen size of a tile, for the glow
}

BurstActor::BurstActor(Alignment alignment, float radiusTiles, bool inward)
	: m_Alignment(alignment)
	, m_Radius(radiusTiles)
	, m_Inward(inward)
{
}

void BurstActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;
	if (m_Age >= kLifetime)
	{
		Destroy();
	}
}

void BurstActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);

	float reachPx = m_Radius * kPixelsPerTile * 2.f;
	if (!ctx.IsOnScreen(x, y, reachPx, reachPx))
	{
		return;
	}

	// Fast at first and settling, like a shock wave rather than a timer.
	float t = m_Age / kLifetime;
	float eased = 1.f - (1.f - t) * (1.f - t);
	float fade = 1.f - t;

	float radius = m_Inward
		? m_Radius + (kSmallest - m_Radius) * eased
		: kSmallest + (m_Radius - kSmallest) * eased;
	float inner = radius - kRingWidth * 0.5f;
	float outer = radius + kRingWidth * 0.5f;
	if (inner < 0.f)
	{
		inner = 0.f;
	}

	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float baseY = groundY + kLift;

	// Light brightens the ground it washes over; dark, a light that gives off
	// darkness, sinks it.
	float glowR, glowG, glowB;
	AlignmentColor::Glow(m_Alignment, glowR, glowG, glowB);
	out.AddGlow(WorldGrid::TileDepth(WorldX(), WorldY()) + 0.5f, x, groundY,
		radius * kPixelsPerTile * 1.4f, glowR, glowG, glowB, fade);

	float r, g, b;
	AlignmentColor::Core(m_Alignment, r, g, b);

	out.AddGroundArc(WorldX(), WorldY(), x, baseY, inner, outer, 0.f, 6.2831853f, kSegments,
		r, g, b, 0.8f * fade);
}
