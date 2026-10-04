#include "stdafx.h"
#include "ShadeActor.h"
#include "KingActor.h"
#include "Alignment.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Palette.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kPi = 3.14159265f;
	const float kRiseTime = 0.4f;
	const float kFadeTime = 0.3f;
	const float kStrikeArc = 140.f;
	const float kGaitPerTile = 3.5f;

	const int kDustSparks = 6;
	const float kDustSpeed = 110.f;
	const float kDustLift = 24.f;

	const float kTelegraphLift = 2.f;
	const int kTelegraphSegments = 16;
}

ShadeActor::ShadeActor(const KingActor* owner, float damage)
	: m_Owner(owner)
	, m_Damage(damage)
{
}

float ShadeActor::BodyRadius() const
{
	return 0.35f;
}

float ShadeActor::TakeHit(float damage)
{
	if (m_State != State::Fade)
	{
		m_Dispersed = true;
		m_DustPending = true;
		Enter(State::Fade);
	}
	return 0.f;
}

void ShadeActor::Enter(State state)
{
	m_State = state;
	m_StateTime = 0.f;
}

void ShadeActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;
	m_StateTime += ctx.deltaTime;

	if (m_DustPending)
	{
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dark, kDustSparks, kDustSpeed, kDustLift);
		m_DustPending = false;
	}

	if (m_State != State::Fade && (!m_Owner->IsDueling() || m_Age >= Tuning::Noctis::kShadeLifetime))
	{
		Enter(State::Fade);
	}

	float col, row, playerCol, playerRow;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
	float toCol = playerCol - col;
	float toRow = playerRow - row;
	float distance = sqrtf(toCol * toCol + toRow * toRow);

	switch (m_State)
	{
	case State::Rise:
	{
		if (m_StateTime >= kRiseTime)
		{
			Enter(State::Stalk);
		}
		break;
	}
	case State::Stalk:
	{
		if (distance > 0.001f)
		{
			m_DirCol = toCol / distance;
			m_DirRow = toRow / distance;
		}
		float screenX, screenY;
		WorldGrid::IsoPosition(m_DirCol, m_DirRow, screenX, screenY);
		if (fabsf(screenX) > 0.01f)
		{
			m_Facing = (screenX < 0.f) ? -1.f : 1.f;
		}

		// Close enough, it stops and draws back to cut along the way it faces.
		if (distance <= Tuning::Noctis::kShadeReach)
		{
			Enter(State::Windup);
			break;
		}

		float step = Tuning::Noctis::kShadeSpeed * ctx.deltaTime;
		WorldGrid::Move(col, row, m_DirCol * step, m_DirRow * step);
		float x, y;
		WorldGrid::IsoPosition(col, row, x, y);
		SetLocalPosition(x, y);
		m_Gait += step * kGaitPerTile;
		break;
	}
	case State::Windup:
	{
		if (m_StateTime >= Tuning::Noctis::kShadeStrikeWindup)
		{
			Combat::StrikePlayerArc(ctx, WorldX(), WorldY(), m_DirCol, m_DirRow,
				Tuning::Noctis::kShadeReach, kStrikeArc, m_Damage);
			Enter(State::Fade);
		}
		break;
	}
	case State::Fade:
	{
		if (m_StateTime >= kFadeTime)
		{
			Destroy();
		}
		break;
	}
	}
}

void ShadeActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 60.f, 90.f))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	// Up out of the ground as it rises, back down into it as it goes - fast
	// when struck apart.
	float height = 1.f;
	if (m_State == State::Rise)
	{
		height = m_StateTime / kRiseTime;
	}
	else if (m_State == State::Fade)
	{
		height = 1.f - m_StateTime / kFadeTime * (m_Dispersed ? 1.6f : 1.f);
	}
	if (height <= 0.05f)
	{
		return;
	}

	float windup = 0.f;
	if (m_State == State::Windup)
	{
		windup = m_StateTime / Tuning::Noctis::kShadeStrikeWindup;
		if (windup > 1.f)
		{
			windup = 1.f;
		}

		float r, g, b;
		AlignmentColor::Warning(Alignment::Dark, Palette::kWarningWash, r, g, b);
		float center = atan2f(m_DirRow, m_DirCol);
		float half = kStrikeArc * 0.5f * kPi / 180.f;
		float reach = Tuning::Noctis::kShadeReach;
		out.AddGroundArc(WorldX(), WorldY(), x, groundY + kTelegraphLift, 0.f, reach, center - half, center + half,
			kTelegraphSegments, r, g, b, 0.12f);
		out.AddGroundArc(WorldX(), WorldY(), x, groundY + kTelegraphLift, 0.f, reach * windup, center - half, center + half,
			kTelegraphSegments, r, g, b, 0.24f);
	}

	// A darkness pooled under it, darkening the ground.
	float gr, gg, gb;
	AlignmentColor::Glow(Alignment::Dark, gr, gg, gb);
	out.AddGlow(depth - 0.02f, x, groundY, 34.f, gr, gg, gb, 0.7f * height);

	out.SetFigure(true);
	const Palette::Color& c = Palette::kShade;
	float f = m_Facing;
	float stride = (m_State == State::Stalk) ? sinf(m_Gait) * 3.f : 0.f;
	float lean = f * (2.f - windup * 3.f);
	float flicker = 0.85f + 0.15f * sinf(m_Age * 23.f);

	// His own lean outline, all of it shadow: legs, a long body, the head,
	// and the curved blade drawn back to cut.
	out.AddTaperedBox(depth, x - 4.f + stride, groundY, 0.12f, 0.12f, 15.f * height, 0.8f, 0.f, 0.f, c.r, c.g, c.b);
	out.AddTaperedBox(depth, x + 4.f - stride, groundY, 0.12f, 0.12f, 15.f * height, 0.8f, 0.f, 0.f, c.r, c.g, c.b);
	float torsoBase = groundY + 15.f * height;
	out.AddTaperedBox(depth + 0.01f, x, torsoBase, 0.26f, 0.2f, 22.f * height, 1.25f, lean, 0.f, c.r, c.g, c.b);
	float headBase = torsoBase + 22.f * height;
	out.AddTaperedBox(depth + 0.02f, x + f + lean, headBase, 0.17f, 0.17f, 10.f * height, 0.7f, 0.f, 0.f, c.r, c.g, c.b);

	float er, eg, eb;
	AlignmentColor::Core(Alignment::Dark, er, eg, eb);
	out.AddRect(depth + 0.03f, x + f * 4.f + lean, headBase + 6.f * height, 4.f, 1.4f,
		er * flicker, eg * flicker, eb * flicker, 1.f);

	float handX = x + f * (10.f - windup * 14.f) + lean;
	float handY = torsoBase + 12.f * height + windup * 6.f;
	out.AddTaperedBox(depth + 0.025f, handX, handY, 0.05f, 0.05f, 22.f * height, 0.3f, f * (8.f + windup * 10.f), 0.f,
		c.r * 1.6f, c.g * 1.6f, c.b * 1.6f);
}
