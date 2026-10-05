#include "stdafx.h"
#include "AfterimageActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include <cmath>

namespace
{
	const float kLifetime = 0.28f;
	const float kStartAlpha = 0.55f;

	// A flat quad between two heights, narrowing from one width to another.
	void Band(DrawList& out, float depth, float x, float bottomY, float topY, float bottomHalf, float topHalf,
		float lean, float r, float g, float b, float a)
	{
		float corners[8] =
		{
			x - bottomHalf, bottomY,
			x + lean - topHalf, topY,
			x + lean + topHalf, topY,
			x + bottomHalf, bottomY,
		};
		out.AddCorners(depth, corners, r, g, b, a);
	}
}

AfterimageActor::AfterimageActor(Alignment alignment, float side)
	: m_Alignment(alignment)
	, m_Side(side)
{
}

void AfterimageActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;
	if (m_Age >= kLifetime)
	{
		Destroy();
	}
}

void AfterimageActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 30.f, 60.f))
	{
		return;
	}
	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.55f;

	float fade = 1.f - m_Age / kLifetime;
	float alpha = kStartAlpha * fade;
	float r, g, b;
	AlignmentColor::Core(m_Alignment, r, g, b);
	// The dark's is its colour sunk nearly to black; the light's shines.
	if (m_Alignment == Alignment::Dark)
	{
		r *= 0.55f;
		g *= 0.55f;
		b *= 0.55f;
	}
	out.SetGlow(m_Alignment == Alignment::Light ? 0.7f : 0.f);

	// Legs, the body and the cloak behind it, and the hood: his shape, flat.
	Band(out, depth, x, groundY, groundY + 12.f, 7.f, 6.f, 0.f, r, g, b, alpha * 0.7f);
	Band(out, depth - 0.001f, x - m_Side * 4.f, groundY + 3.f, groundY + 27.f, 11.f, 8.f, m_Side * 3.f, r, g, b, alpha * 0.6f);
	Band(out, depth + 0.001f, x, groundY + 12.f, groundY + 27.f, 7.f, 9.f, 0.f, r, g, b, alpha);
	Band(out, depth + 0.002f, x + m_Side, groundY + 26.f, groundY + 39.f, 6.f, 2.f, -m_Side * 1.5f, r, g, b, alpha);
}
