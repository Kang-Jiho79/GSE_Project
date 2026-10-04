#include "stdafx.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Alignment.h"
#include "Palette.h"
#include "Random.h"
#include <cmath>

namespace
{
	bool s_Enabled = true;

	const float kStreakLifetime = 0.25f;
	const float kDustLifetime = 0.35f;
	const float kStreakLength = 7.f;  // pixels at the start, shrinking as it dies
	const float kStreakWidth = 2.f;
	const float kDustSize = 5.f;      // pixels at the start, growing as it fades
	const float kDustSpread = 0.5f;   // dust keeps low: its rise is squashed to this
}

SparkActor::Style SparkActor::StyleFor(Form form)
{
	switch (form)
	{
	case Form::Light:
	{
		return Style::Light;
	}
	case Form::Dark:
	{
		return Style::Dark;
	}
	default:
	{
		return Style::Dust;
	}
	}
}

void SparkActor::Burst(SceneGraph& graph, float worldX, float worldY, Style style,
	int count, float speed, float lift)
{
	if (!s_Enabled)
	{
		return;
	}

	std::unique_ptr<SparkActor> spray(new SparkActor(style, count, speed, lift));
	spray->SetLocalPosition(worldX, worldY);
	graph.Spawn(std::move(spray));
}

void SparkActor::SetEnabled(bool enabled)
{
	s_Enabled = enabled;
}

bool SparkActor::IsEnabled()
{
	return s_Enabled;
}

SparkActor::SparkActor(Style style, int count, float speed, float lift)
	: m_Style(style)
	, m_Count(count < kMaxSparks ? count : kMaxSparks)
	, m_Lift(lift)
{
	for (int i = 0; i < m_Count; i++)
	{
		float angle = Random::Angle();
		m_DirX[i] = cosf(angle);
		m_DirY[i] = sinf(angle);
		m_Speed[i] = speed * (0.5f + Random::Unit() * 0.5f);
	}
}

float SparkActor::Lifetime() const
{
	return (m_Style == Style::Dust) ? kDustLifetime : kStreakLifetime;
}

void SparkActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;
	if (m_Age >= Lifetime())
	{
		Destroy();
	}
}

void SparkActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 60.f, 60.f))
	{
		return;
	}

	float t = m_Age / Lifetime();
	float fade = 1.f - t;
	float baseY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY()) + m_Lift;
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.8f;

	// Fast at first, slowing as it flies.
	float travel = m_Age * (1.f - 0.5f * t);

	if (m_Style == Style::Dust)
	{
		float size = kDustSize * (1.f + t);
		for (int i = 0; i < m_Count; i++)
		{
			float px = x + m_DirX[i] * m_Speed[i] * travel;
			float py = baseY + m_DirY[i] * m_Speed[i] * travel * kDustSpread;
			out.AddDiamond(depth, px, py, size, size * 0.6f,
				Palette::kAsh.r * 0.8f, Palette::kAsh.g * 0.8f, Palette::kAsh.b * 0.8f, 0.5f * fade);
		}
		return;
	}

	float r, g, b;
	if (m_Style == Style::Light)
	{
		AlignmentColor::Core(Alignment::Light, r, g, b);
		out.SetGlow(1.f);
	}
	else
	{
		// Lighter than the dark's own core, so a violet spark still shows on a
		// near-black ground.
		AlignmentColor::Core(Alignment::Dark, r, g, b);
		r += (1.f - r) * 0.35f;
		g += (1.f - g) * 0.35f;
		b += (1.f - b) * 0.35f;
	}

	float length = kStreakLength * fade;
	for (int i = 0; i < m_Count; i++)
	{
		float headX = x + m_DirX[i] * m_Speed[i] * travel;
		float headY = baseY + m_DirY[i] * m_Speed[i] * travel;
		float tailX = headX - m_DirX[i] * length;
		float tailY = headY - m_DirY[i] * length;

		float sideX = -m_DirY[i] * kStreakWidth * 0.5f;
		float sideY = m_DirX[i] * kStreakWidth * 0.5f;

		float corners[8] =
		{
			tailX - sideX, tailY - sideY,
			headX - sideX, headY - sideY,
			headX + sideX, headY + sideY,
			tailX + sideX, tailY + sideY,
		};
		out.AddCorners(depth, corners, r, g, b, fade);
	}
}
