#include "stdafx.h"
#include "WaystoneActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Palette.h"
#include <cmath>

namespace
{
	const float kStone = 0.30f;
	const float kRingRadius = 0.9f; // tiles
	const float kRingWidth = 0.08f;
	const float kPulseRate = 1.6f;
}

void WaystoneActor::OnUpdate(const ActorContext& ctx)
{
	m_Time += ctx.deltaTime;
}

void WaystoneActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 60.f, 80.f))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.5f;

	// A worn ring cut into the ground around it, faintly breathing, so it can
	// be found from a distance.
	float pulse = 0.25f + 0.12f * sinf(m_Time * kPulseRate);
	out.AddGroundArc(WorldX(), WorldY(), x, groundY + 1.f, kRingRadius - kRingWidth, kRingRadius,
		0.f, 6.2831853f, 24, Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b, pulse);

	out.AddShadow(depth - 0.01f, x, groundY, 30.f, 15.f);

	// A rough plinth, then the stone itself, narrowing and leaning a little,
	// with a cap stone set on top.
	out.AddTaperedBox(depth, x, groundY, 0.42f, 0.42f, 5.f, 0.9f, 0.f, 0.f, kStone * 0.85f, kStone * 0.85f, kStone * 0.85f);
	out.AddTaperedBox(depth + 0.01f, x, groundY + 5.f, 0.24f, 0.22f, 30.f, 0.7f, 1.5f, 0.f, kStone, kStone, kStone);
	out.AddTaperedBox(depth + 0.02f, x + 1.5f, groundY + 35.f, 0.2f, 0.2f, 4.f, 0.8f, 0.f, 0.f,
		kStone * 1.15f, kStone * 1.15f, kStone * 1.15f);

	// A cut mark down its face, worn pale.
	out.AddRect(depth + 0.015f, x + 3.f, groundY + 22.f, 1.6f, 12.f,
		Palette::kUiBone.r * 0.6f, Palette::kUiBone.g * 0.6f, Palette::kUiBone.b * 0.6f, 1.f);
}
