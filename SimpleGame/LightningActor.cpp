#include "stdafx.h"
#include "LightningActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Alignment.h"
#include "Combat.h"
#include "Impact.h"
#include "Tuning.h"
#include "Random.h"
#include "SparkActor.h"
#include "BurstActor.h"
#include <cmath>

namespace
{
	// His own lightning, striking: light thrown out across the ground.
	const float kImpactRing = 1.3f;   // times its radius
	const int kImpactSparks = 18;
	const float kImpactSparkSpeed = 260.f;
	const float kImpactSparkLift = 30.f;

	const float kStrikeShowTime = 0.3f;
	const int kRingSegments = 32;
	const float kRingWidth = 0.15f;     // tiles; the warning's outline
	const float kLift = 2.f;            // pixels off the ground, just clear of the tiles
	const float kBoltHeight = 260.f;    // pixels; tall enough to come from off screen
	const float kBoltWidth = 7.f;
	const float kKinkSpread = 14.f;     // pixels each kink may wander sideways
	const float kPixelsPerTile = 32.f;  // rough screen size of a tile, for the glow
}

LightningActor::LightningActor(Target target, float radiusTiles, float delay, float damage)
	: m_Target(target)
	, m_Radius(radiusTiles)
	, m_Delay(delay)
	, m_Damage(damage)
{
	for (int i = 0; i < kKinks; i++)
	{
		m_Kinks[i] = (Random::Unit() * 2.f - 1.f) * kKinkSpread;
	}
}

void LightningActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;

	if (!m_Struck && m_Age >= m_Delay)
	{
		if (m_Target == Target::Monsters)
		{
			// His own lightning: the ground shakes either way, and the world
			// catches if it struck anything.
			float landed = Combat::HitRadius(*ctx.graph, WorldX(), WorldY(), m_Radius, m_Damage, 0.f);
			ctx.impact->Shake(Tuning::Feel::kStrikeShake);

			// Light thrown out across the ground where it struck.
			std::unique_ptr<BurstActor> ring(new BurstActor(Alignment::Light, m_Radius * kImpactRing, false));
			ring->SetLocalPosition(WorldX(), WorldY());
			ctx.graph->Spawn(std::move(ring));
			SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Light, kImpactSparks, kImpactSparkSpeed,
				kImpactSparkLift);
			if (landed > 0.f)
			{
				ctx.impact->HitStop(Tuning::Feel::kHeavyStop);
			}
		}
		else
		{
			Combat::StrikePlayerRadius(ctx, WorldX(), WorldY(), m_Radius, m_Damage);
		}
		m_Struck = true;
	}

	if (m_Age >= m_Delay + kStrikeShowTime)
	{
		Destroy();
	}
}

void LightningActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);

	float reach = m_Radius * kPixelsPerTile * 2.f;
	if (!ctx.IsOnScreen(x, y, reach, kBoltHeight))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());

	if (m_Struck)
	{
		DrawStrike(out, x, groundY);
	}
	else
	{
		DrawWarning(out, x, groundY);
	}
}

void LightningActor::DrawWarning(DrawList& out, float x, float groundY) const
{
	float radius = m_Radius;
	float progress = m_Age / m_Delay;

	// Called on the player, the mark is washed toward ash, so it reads as a
	// threat rather than his own.
	float r, g, b;
	AlignmentColor::Warning(Alignment::Light, (m_Target == Target::Player) ? Palette::kWarningWash : 0.f, r, g, b);

	// The edge of where it will land, pulsing faster as it nears.
	float pulse = 0.45f + 0.25f * sinf(progress * progress * 40.f);
	out.AddGroundArc(WorldX(), WorldY(), x, groundY + kLift, radius - kRingWidth, radius,
		0.f, 6.2831853f, kRingSegments, r, g, b, pulse);

	// A fill closing in from the edge: when it reaches the middle, it lands.
	float filled = radius * (1.f - progress);
	if (filled < radius - kRingWidth)
	{
		out.AddGroundArc(WorldX(), WorldY(), x, groundY + kLift, filled, radius - kRingWidth,
			0.f, 6.2831853f, kRingSegments, r, g, b, 0.16f);
	}
}

void LightningActor::DrawStrike(DrawList& out, float x, float groundY) const
{
	float t = (m_Age - m_Delay) / kStrikeShowTime;
	float fade = 1.f - t;
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.8f;

	float r, g, b;
	AlignmentColor::Glow(Alignment::Light, r, g, b);
	out.AddGlow(depth - 0.3f, x, groundY, m_Radius * kPixelsPerTile * 1.6f,
		r, g, b, fade);

	AlignmentColor::Core(Alignment::Light, r, g, b);
	out.SetGlow(1.f);
	out.AddGroundArc(WorldX(), WorldY(), x, groundY + kLift, 0.f, m_Radius,
		0.f, 6.2831853f, kRingSegments, r, g, b, 0.35f * fade);

	// The bolt: a jagged column from above the screen down to the ground,
	// pinned straight at both ends so it lands exactly on its mark.
	float segmentHeight = kBoltHeight / (kKinks + 1);
	float prevX = x;
	float prevY = groundY + kBoltHeight;
	for (int i = 0; i <= kKinks; i++)
	{
		float nextY = prevY - segmentHeight;
		float nextX = (i < kKinks) ? x + m_Kinks[i] : x;

		float corners[8] =
		{
			prevX - kBoltWidth * 0.5f, prevY,
			prevX + kBoltWidth * 0.5f, prevY,
			nextX + kBoltWidth * 0.5f, nextY,
			nextX - kBoltWidth * 0.5f, nextY,
		};
		out.AddCorners(depth, corners, r, g, b, 0.75f * fade);

		float core[8] =
		{
			prevX - 1.5f, prevY,
			prevX + 1.5f, prevY,
			nextX + 1.5f, nextY,
			nextX - 1.5f, nextY,
		};
		out.AddCorners(depth + 0.01f, core, 1.f, 0.98f, 0.92f, fade);

		prevX = nextX;
		prevY = nextY;
	}
}
