#include "stdafx.h"
#include "NoctisLikenessActor.h"
#include "NoctisActor.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "PlayerStatus.h"
#include "Palette.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kGlideSpeed = 6.f;   // tiles a second to where the king wants it
	const float kRushLength = 5.f;
	const float kRushWidth = 1.f;
	const float kRushTime = 0.22f;
	const float kLifetime = 14.f;    // gone by now whatever happened

	const int kBurstSparks = 12;
	const float kBurstSpeed = 160.f;
	const float kSparkLift = 30.f;

	const float kTelegraphLift = 2.f;
	const float kTelegraphBase = 0.14f;
	const float kTelegraphFill = 0.26f;
}

NoctisLikenessActor::NoctisLikenessActor(NoctisActor* owner)
	: m_Owner(owner)
{
}

float NoctisLikenessActor::BodyRadius() const
{
	return Tuning::Noctis::kBodyRadius;
}

float NoctisLikenessActor::TakeHit(float damage)
{
	if (m_State == State::Circle && !m_Burst)
	{
		m_Burst = true;
	}
	return 0.f;
}

void NoctisLikenessActor::MoveTo(float col, float row)
{
	m_ToCol = col;
	m_ToRow = row;
	m_HasTarget = true;
}

void NoctisLikenessActor::Rush(float dirCol, float dirRow)
{
	if (m_State != State::Circle)
	{
		return;
	}
	m_State = State::Rush;
	m_StateTime = 0.f;
	m_DirCol = dirCol;
	m_DirRow = dirRow;
}

void NoctisLikenessActor::Dissolve()
{
	m_State = State::Gone;
}

void NoctisLikenessActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;
	m_StateTime += ctx.deltaTime;

	if (m_State == State::Gone || m_Age >= kLifetime || !m_Owner->IsDueling())
	{
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dark, kBurstSparks / 2, kBurstSpeed * 0.6f, kSparkLift);
		Destroy();
		return;
	}

	// Struck: it bursts on him, holding him fast, and the king drinks.
	if (m_Burst)
	{
		Combat::StrikePlayerRadius(ctx, WorldX(), WorldY(), Tuning::Noctis::kLikenessBurst,
			Tuning::Noctis::kLikenessBurstDamage);
		ctx.status->rootTimer = Tuning::Noctis::kLikenessRoot;
		m_Owner->Drink(Tuning::Noctis::kLikenessHeal);
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dark, kBurstSparks, kBurstSpeed, kSparkLift);
		Destroy();
		return;
	}

	float col, row;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);

	if (m_State == State::Rush)
	{
		float windup = Tuning::Noctis::kReckonRush;
		if (m_StateTime >= windup && !m_Struck)
		{
			Combat::StrikePlayerLine(ctx, WorldX(), WorldY(), m_DirCol, m_DirRow, kRushLength, kRushWidth,
				Tuning::Noctis::kReckonRushDamage);
			m_Struck = true;
		}
		if (m_StateTime >= windup)
		{
			float x, y;
			float along = kRushLength / kRushTime * ctx.deltaTime;
			WorldGrid::IsoPosition(col + m_DirCol * along, row + m_DirRow * along, x, y);
			SetLocalPosition(x, y);
		}
		if (m_StateTime >= windup + kRushTime)
		{
			m_State = State::Gone;
		}
		return;
	}

	if (m_HasTarget)
	{
		float dc = m_ToCol - col;
		float dr = m_ToRow - row;
		float distance = sqrtf(dc * dc + dr * dr);
		float step = kGlideSpeed * ctx.deltaTime;
		if (distance > 0.01f)
		{
			float move = (step < distance) ? step : distance;
			float x, y;
			WorldGrid::IsoPosition(col + dc / distance * move, row + dr / distance * move, x, y);
			SetLocalPosition(x, y);
		}
	}
}

void NoctisLikenessActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 160.f, 200.f))
	{
		return;
	}
	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	if (m_State == State::Rush && m_StateTime < Tuning::Noctis::kReckonRush)
	{
		float r, g, b;
		AlignmentColor::Warning(Alignment::Dark, Palette::kWarningWash, r, g, b);
		float progress = m_StateTime / Tuning::Noctis::kReckonRush;
		out.AddGroundStrip(WorldX(), WorldY(), x, groundY + kTelegraphLift, m_DirCol, m_DirRow, 0.f, kRushLength, kRushWidth,
			r, g, b, kTelegraphBase);
		out.AddGroundStrip(WorldX(), WorldY(), x, groundY + kTelegraphLift, m_DirCol, m_DirRow, 0.f, kRushLength * progress,
			kRushWidth, r, g, b, kTelegraphFill);
	}

	// The king to the life - but no shadow, and a hair off the ground.
	m_Owner->DrawLikeness(out, depth, x, groundY + Tuning::Noctis::kLikenessLift);
}
