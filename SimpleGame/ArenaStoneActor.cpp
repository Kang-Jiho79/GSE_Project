#include "stdafx.h"
#include "ArenaStoneActor.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Impact.h"
#include "Palette.h"
#include <cmath>

namespace
{
	// What it blocks: most of its square.
	const float kHalfBlock = 0.4f;

	const Palette::Color kStone = { 0.36f, 0.35f, 0.37f };
	const float kPillarHeight = 64.f;
	const float kWallHeight = 30.f;
	const float kRiseTime = 0.25f;     // a wall pushing up out of the board
	const float kCrumbleTime = 0.4f;   // a wall sinking back into it
	const float kRubbleHeight = 10.f;

	const int kBreakDust = 14;
	const float kBreakDustSpeed = 150.f;
	const float kBreakDustLift = 24.f;
	const float kBreakShake = 5.f;
}

ArenaStoneActor::ArenaStoneActor(int col, int row, bool pillar, float lifetime)
	: m_Col(col)
	, m_Row(row)
	, m_Pillar(pillar)
	, m_Lifetime(lifetime)
{
	float x, y;
	WorldGrid::IsoPosition((float)col, (float)row, x, y);
	SetLocalPosition(x, y);
	Block(true);
}

void ArenaStoneActor::Block(bool block)
{
	if (block == m_Blocking)
	{
		return;
	}
	if (block)
	{
		WorldGrid::AddBlocker(m_Col - kHalfBlock, m_Row - kHalfBlock, m_Col + kHalfBlock, m_Row + kHalfBlock);
	}
	else
	{
		WorldGrid::RemoveBlocker(m_Col - kHalfBlock, m_Row - kHalfBlock, m_Col + kHalfBlock, m_Row + kHalfBlock);
	}
	m_Blocking = block;
}

void ArenaStoneActor::Break()
{
	if (m_Broken)
	{
		return;
	}
	m_Broken = true;
	m_BrokenTime = 0.f;
	m_DustPending = true;
	Block(false);
}

void ArenaStoneActor::Restore()
{
	if (!m_Pillar || !m_Broken)
	{
		return;
	}
	m_Broken = false;
	Block(true);
}

bool ArenaStoneActor::IsPillar() const
{
	return m_Pillar;
}

bool ArenaStoneActor::IsBroken() const
{
	return m_Broken;
}

bool ArenaStoneActor::IsGone() const
{
	return !m_Pillar && m_Broken && m_BrokenTime >= kCrumbleTime;
}

int ArenaStoneActor::Col() const
{
	return m_Col;
}

int ArenaStoneActor::Row() const
{
	return m_Row;
}

void ArenaStoneActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;
	if (m_Broken)
	{
		m_BrokenTime += ctx.deltaTime;
	}
	if (!m_Pillar && !m_Broken && m_Age >= m_Lifetime)
	{
		Break();
	}

	if (m_DustPending)
	{
		m_DustPending = false;
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dust, kBreakDust, kBreakDustSpeed,
			kBreakDustLift);
		ctx.impact->Shake(kBreakShake);
	}
}

void ArenaStoneActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 50.f, 90.f))
	{
		return;
	}
	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.5f;

	float height = m_Pillar ? kPillarHeight : kWallHeight;
	if (!m_Pillar)
	{
		// Pushing up out of the board, and sinking back into it.
		float rise = (m_Age < kRiseTime) ? m_Age / kRiseTime : 1.f;
		float sink = m_Broken ? 1.f - m_BrokenTime / kCrumbleTime : 1.f;
		if (sink <= 0.f)
		{
			return;
		}
		height *= rise * sink;
	}
	else if (m_Broken)
	{
		height = kRubbleHeight;
	}

	out.AddShadow(depth - 0.01f, x, groundY, 50.f, 25.f);
	if (m_Pillar && !m_Broken)
	{
		// A plinth, a shaft, and a capital.
		out.AddBox(depth, x, groundY, 0.8f, 0.8f, 6.f, kStone.r * 0.9f, kStone.g * 0.9f, kStone.b * 0.9f);
		out.AddTaperedBox(depth + 0.005f, x, groundY + 6.f, 0.6f, 0.6f, height - 12.f, 0.9f, 0.f, 0.f,
			kStone.r, kStone.g, kStone.b);
		out.AddBox(depth + 0.01f, x, groundY + height - 6.f, 0.75f, 0.75f, 6.f,
			kStone.r * 1.1f, kStone.g * 1.1f, kStone.b * 1.1f);
		return;
	}
	if (m_Pillar)
	{
		// The stump of it, and the broken top lying beside it.
		out.AddTaperedBox(depth, x, groundY, 0.7f, 0.7f, height, 0.8f, 2.f, 0.f, kStone.r * 0.85f, kStone.g * 0.85f,
			kStone.b * 0.85f);
		out.AddBox(depth + 0.2f, x + 18.f, groundY - 6.f, 0.5f, 0.3f, 6.f, kStone.r * 0.8f, kStone.g * 0.8f, kStone.b * 0.8f);
		return;
	}
	// A length of battlemented wall.
	out.AddBox(depth, x, groundY, 0.9f, 0.9f, height, kStone.r, kStone.g, kStone.b);
	if (height > 12.f)
	{
		out.AddBox(depth + 0.01f, x - 10.f, groundY + height, 0.25f, 0.25f, 6.f, kStone.r, kStone.g, kStone.b);
		out.AddBox(depth + 0.01f, x + 10.f, groundY + height, 0.25f, 0.25f, 6.f, kStone.r, kStone.g, kStone.b);
	}
}
