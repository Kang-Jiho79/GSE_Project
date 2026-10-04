#include "stdafx.h"
#include "SnareActor.h"
#include "KingActor.h"
#include "Alignment.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Impact.h"
#include "Random.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kPi = 3.14159265f;
	const int kSegments = 18;
	const float kLift = 2.f;

	const float kMarkRadius = 0.45f;
	const float kHiddenAlpha = 0.05f;
	const float kLaidAlpha = 0.5f;

	// Every so often it catches the light, just for a moment.
	const float kGlintEvery = 2.6f;
	const float kGlintTime = 0.25f;
	const float kGlintAlpha = 0.3f;

	const float kBurstShow = 0.3f;
	const int kBurstSparks = 10;
	const float kBurstSparkSpeed = 160.f;
	const float kBurstSparkLift = 20.f;
	const float kBurstShake = 4.f;
}

SnareActor::SnareActor(const KingActor* owner, float damage)
	: m_Owner(owner)
	, m_Damage(damage)
{
	m_GlintPhase = Random::Unit() * kGlintEvery;
}

const KingActor* SnareActor::Owner() const
{
	return m_Owner;
}

void SnareActor::OnUpdate(const ActorContext& ctx)
{
	if (!m_Owner->IsDueling())
	{
		Destroy();
		return;
	}

	m_Age += ctx.deltaTime;

	if (m_Sprung)
	{
		m_SprungTime += ctx.deltaTime;
		if (!m_Struck && m_SprungTime >= Tuning::Noctis::kSnareDelay)
		{
			Combat::StrikePlayerRadius(ctx, WorldX(), WorldY(), Tuning::Noctis::kSnareRadius, m_Damage);
			SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dark, kBurstSparks, kBurstSparkSpeed, kBurstSparkLift);
			ctx.impact->Shake(kBurstShake);
			m_Struck = true;
		}
		if (m_SprungTime >= Tuning::Noctis::kSnareDelay + kBurstShow)
		{
			Destroy();
		}
		return;
	}

	if (m_Age >= Tuning::Noctis::kSnareLifetime)
	{
		Destroy();
		return;
	}

	// Harmless until it is set.
	if (m_Age < Tuning::Noctis::kSnareArming)
	{
		return;
	}

	float col, row, playerCol, playerRow;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
	float dc = playerCol - col;
	float dr = playerRow - row;
	float trigger = Tuning::Noctis::kSnareTrigger;
	if (dc * dc + dr * dr <= trigger * trigger)
	{
		m_Sprung = true;
		m_SprungTime = 0.f;
	}
}

void SnareActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 80.f, 50.f))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY()) + kLift;
	float r, g, b;
	AlignmentColor::Warning(Alignment::Dark, Palette::kHostileSpellWash, r, g, b);

	if (m_Struck)
	{
		// The burst: dark welling up out of the ground and draining away.
		float fade = 1.f - (m_SprungTime - Tuning::Noctis::kSnareDelay) / kBurstShow;
		AlignmentColor::Core(Alignment::Dark, r, g, b);
		out.AddGroundArc(WorldX(), WorldY(), x, groundY, 0.f, Tuning::Noctis::kSnareRadius, 0.f, 2.f * kPi, kSegments,
			r, g, b, 0.6f * fade);
		return;
	}

	if (m_Sprung)
	{
		// Sprung: what it will catch, marked plainly for the moment he has.
		float progress = m_SprungTime / Tuning::Noctis::kSnareDelay;
		float radius = Tuning::Noctis::kSnareRadius;
		out.AddGroundArc(WorldX(), WorldY(), x, groundY, 0.f, radius * progress, 0.f, 2.f * kPi, kSegments, r, g, b, 0.3f);
		out.AddGroundArc(WorldX(), WorldY(), x, groundY, radius - 0.08f, radius, 0.f, 2.f * kPi, kSegments, r, g, b, 0.7f);
		return;
	}

	// Seen as it is laid, fading to almost nothing as it sets.
	float alpha = kHiddenAlpha;
	float arming = Tuning::Noctis::kSnareArming;
	if (m_Age < arming)
	{
		alpha = kLaidAlpha + (kHiddenAlpha - kLaidAlpha) * (m_Age / arming);
	}
	else if (fmodf(m_Age + m_GlintPhase, kGlintEvery) < kGlintTime)
	{
		alpha = kGlintAlpha;
	}

	out.AddGroundArc(WorldX(), WorldY(), x, groundY, kMarkRadius - 0.07f, kMarkRadius, 0.f, 2.f * kPi, kSegments, r, g, b, alpha);
	out.AddDiamond(WorldGrid::TileDepth(WorldX(), WorldY()) + 0.02f, x, groundY, 8.f, 4.f, r, g, b, alpha);
}
