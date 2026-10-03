#include "stdafx.h"
#include "CurseFieldActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Alignment.h"
#include "Combat.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kFadeTime = 0.3f;
	const float kLift = 2.f;            // pixels off the ground, just clear of the tiles
	const float kEdgeWidth = 0.1f;      // tiles
	const float kSwirlRate = 2.5f;      // radians per second
	const float kSwirlAngle = 1.4f;     // radians each swirl covers
	const float kPixelsPerTile = 32.f;  // rough screen size of a tile, for the glow
	const int kSegments = 28;
}

void CurseFieldActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;

	float armed = Tuning::DarkMage::kCurseArmTime;
	float ends = armed + Tuning::DarkMage::kCurseDuration;

	if (m_Age >= ends + kFadeTime)
	{
		Destroy();
		return;
	}

	if (m_Age < armed || m_Age >= ends)
	{
		return;
	}

	// The first bite lands the moment it is armed.
	m_TickTimer -= ctx.deltaTime;
	if (m_TickTimer <= 0.f)
	{
		Combat::StrikePlayerRadius(ctx, WorldX(), WorldY(), Tuning::DarkMage::kCurseRadius,
			Tuning::DarkMage::kCurseTickDamage);
		m_TickTimer += Tuning::DarkMage::kCurseTickInterval;
	}
}

void CurseFieldActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);

	float radius = Tuning::DarkMage::kCurseRadius;
	float reach = radius * kPixelsPerTile * 2.f;
	if (!ctx.IsOnScreen(x, y, reach, reach))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float baseY = groundY + kLift;

	float r, g, b;
	AlignmentColor::Core(Alignment::Dark, r, g, b);

	float armed = Tuning::DarkMage::kCurseArmTime;
	float ends = armed + Tuning::DarkMage::kCurseDuration;

	// Forming: the edge drawn first, then a fill closing in from it. Once it
	// reaches the middle, it bites.
	if (m_Age < armed)
	{
		float progress = m_Age / armed;
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, radius - kEdgeWidth, radius,
			0.f, 6.2831853f, kSegments, r, g, b, 0.3f + 0.3f * progress);

		float filled = radius * (1.f - progress);
		if (filled < radius - kEdgeWidth)
		{
			out.AddGroundArc(WorldX(), WorldY(), x, baseY, filled, radius - kEdgeWidth,
				0.f, 6.2831853f, kSegments, r, g, b, 0.12f);
		}
		return;
	}

	float fade = 1.f;
	if (m_Age > ends)
	{
		fade = 1.f - (m_Age - ends) / kFadeTime;
	}

	// The ground under it sunk dark, a dim violet pool, two slow swirls
	// turning in it, and a hard edge.
	float glowR, glowG, glowB;
	AlignmentColor::Glow(Alignment::Dark, glowR, glowG, glowB);
	out.AddGlow(WorldGrid::TileDepth(WorldX(), WorldY()) + 0.5f, x, groundY,
		radius * kPixelsPerTile * 1.3f, glowR, glowG, glowB, fade);

	out.AddGroundArc(WorldX(), WorldY(), x, baseY, 0.f, radius,
		0.f, 6.2831853f, kSegments, r, g, b, 0.18f * fade);

	float turn = m_Age * kSwirlRate;
	for (int swirl = 0; swirl < 2; swirl++)
	{
		float from = turn + swirl * 3.14159265f;
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, radius * 0.35f, radius * 0.75f,
			from, from + kSwirlAngle, 6, r, g, b, 0.35f * fade);
	}

	out.AddGroundArc(WorldX(), WorldY(), x, baseY, radius - kEdgeWidth, radius,
		0.f, 6.2831853f, kSegments, r, g, b, 0.6f * fade);
}
