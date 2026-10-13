#include "stdafx.h"
#include "GodMarkActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Palette.h"
#include <cmath>

namespace
{
	const float kGrowTime = 0.8f;
	const float kSpinRate = 1.2f;      // radians a second
	const float kNarrowest = 0.12f;    // of its width, edge-on, so it never vanishes
	const float kInnerShare = 0.45f;   // the brighter heart inside it
	const float kGlowRadius = 3.2f;    // of its height
	const float kShine = 0.7f;
}

GodMarkActor::GodMarkActor(float lift, float size)
	: m_Lift(lift)
	, m_Size(size)
{
}

void GodMarkActor::DrawMark(DrawList& out, float depth, float x, float y, float size, float spin, float alpha)
{
	if (alpha <= 0.f || size <= 0.f)
	{
		return;
	}

	// Turning about its upright axis: narrow edge-on, full face-on.
	float turn = fabsf(cosf(spin));
	if (turn < kNarrowest)
	{
		turn = kNarrowest;
	}
	float width = size * 0.75f * turn;

	const Palette::Color& bone = Palette::kUiBone;
	out.AddGlow(depth - 0.01f, x, y, size * kGlowRadius, bone.r, bone.g, bone.b, 0.5f * alpha);
	out.SetGlow(kShine);
	out.AddDiamond(depth, x, y, width, size, bone.r, bone.g, bone.b, alpha);
	out.AddDiamond(depth + 0.001f, x, y, width * kInnerShare, size * kInnerShare, 1.f, 1.f, 1.f, alpha);
	out.SetGlow(0.f);
}

void GodMarkActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;
}

void GodMarkActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, m_Size * 2.f, m_Lift + m_Size * 2.f))
	{
		return;
	}

	float grown = (m_Age < kGrowTime) ? m_Age / kGrowTime : 1.f;
	grown = 1.f - (1.f - grown) * (1.f - grown);
	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.9f;
	DrawMark(out, depth, x, groundY + m_Lift, m_Size * grown, m_Age * kSpinRate, grown);
}
