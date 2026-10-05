#include "stdafx.h"
#include "BladeActor.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Alignment.h"
#include "Palette.h"
#include <cmath>

namespace
{
	// Up fast, held a moment, then down slower.
	const float kRiseTime = 0.08f;
	const float kHoldTime = 0.18f;
	const float kSinkTime = 0.22f;

	const float kHeight = 34.f;   // pixels, at full height
	const float kWidth = 7.f;     // pixels across at the ground

	const int kBurstSparks = 6;
	const float kBurstSpeed = 120.f;
	const float kBurstLift = 10.f;
	const int kSegments = 12;
}

BladeActor::BladeActor(float delay, float lean, float scale)
	: m_Delay(delay)
	, m_Lean(lean)
	, m_Scale(scale)
{
}

void BladeActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;
	if (!m_Burst && m_Age >= m_Delay)
	{
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dark, kBurstSparks, kBurstSpeed, kBurstLift);
		m_Burst = true;
	}
	if (m_Age >= m_Delay + kRiseTime + kHoldTime + kSinkTime)
	{
		Destroy();
	}
}

void BladeActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float t = m_Age - m_Delay;
	if (t < 0.f)
	{
		return;
	}

	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 30.f, 60.f))
	{
		return;
	}
	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.5f;

	float rise = 1.f;
	if (t < kRiseTime)
	{
		rise = t / kRiseTime;
	}
	else if (t > kRiseTime + kHoldTime)
	{
		rise = 1.f - (t - kRiseTime - kHoldTime) / kSinkTime;
	}
	if (rise <= 0.f)
	{
		return;
	}

	float r, g, b;
	AlignmentColor::Core(Alignment::Dark, r, g, b);

	// The ground split open round it, and the dark welling out of the split.
	float gr, gg, gb;
	AlignmentColor::Glow(Alignment::Dark, gr, gg, gb);
	out.AddGlow(depth - 0.02f, x, groundY, 26.f * m_Scale, gr, gg, gb, 0.8f * rise);
	out.AddGroundArc(WorldX(), WorldY(), x, groundY + 1.f, 0.f, 0.32f * m_Scale, 0.f, 6.2831853f, kSegments,
		r * 0.4f, g * 0.4f, b * 0.4f, 0.7f * rise);

	// The blade: black, leaning off true, with a violet edge.
	float height = kHeight * m_Scale * rise;
	float half = kWidth * m_Scale * 0.5f;
	float tipX = x + m_Lean * rise;
	float tipY = groundY + height;
	float blade[8] =
	{
		x - half, groundY,
		tipX, tipY,
		tipX, tipY,
		x + half, groundY,
	};
	out.SetFigure(true);
	out.AddCorners(depth, blade, Palette::kShade.r, Palette::kShade.g, Palette::kShade.b, 1.f);
	float edge[8] =
	{
		x + half * 0.2f, groundY,
		tipX, tipY,
		tipX, tipY,
		x + half, groundY,
	};
	out.AddCorners(depth + 0.001f, edge, r, g, b, 0.95f);
}
