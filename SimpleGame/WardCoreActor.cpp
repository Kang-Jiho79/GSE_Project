#include "stdafx.h"
#include "WardCoreActor.h"
#include "BurstActor.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Impact.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kPi = 3.14159265f;

	// A shard hanging over a low plinth, turning, with a ring on the ground.
	const float kHang = 46.f;        // pixels above the ground
	const float kBob = 4.f;
	const float kBobRate = 2.2f;
	const float kSpinRate = 1.6f;
	const float kShardWidth = 18.f;
	const float kShardHeight = 34.f;
	const float kRingRadius = 0.75f; // tiles
	const float kRingWidth = 0.1f;
	const int kRingSegments = 20;
	const float kGlanceTime = 0.2f;

	const int kBreakSparks = 18;
	const float kBreakSparkSpeed = 220.f;
	const float kBreakSparkLift = 40.f;
	const float kBreakShake = 4.f;

	SparkActor::Style SparkStyleFor(Alignment alignment)
	{
		return (alignment == Alignment::Dark) ? SparkActor::Style::Dark : SparkActor::Style::Light;
	}
}

WardCoreActor::WardCoreActor(Alignment alignment)
	: m_Alignment(alignment)
{
}

float WardCoreActor::TakeHit(float damage)
{
	if (!m_Broken)
	{
		m_Glance = kGlanceTime;
	}
	return 0.f;
}

void WardCoreActor::Pierce()
{
	if (m_Broken)
	{
		return;
	}
	m_Broken = true;
	m_BurstPending = true;
}

float WardCoreActor::BodyRadius() const
{
	return Tuning::God::kCoreRadius;
}

bool WardCoreActor::IsBroken() const
{
	return m_Broken;
}

void WardCoreActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;
	if (m_Glance > 0.f)
	{
		m_Glance -= ctx.deltaTime;
	}

	if (m_BurstPending)
	{
		m_BurstPending = false;
		std::unique_ptr<BurstActor> burst(new BurstActor(m_Alignment, Tuning::God::kCoreRadius * 2.5f, false));
		burst->SetLocalPosition(WorldX(), WorldY());
		ctx.graph->Spawn(std::move(burst));
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkStyleFor(m_Alignment),
			kBreakSparks, kBreakSparkSpeed, kBreakSparkLift);
		ctx.impact->Shake(kBreakShake);
	}
}

void WardCoreActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	if (m_Broken)
	{
		return;
	}

	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 40.f, kHang + kShardHeight + 20.f))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;
	float r, g, b;
	AlignmentColor::Core(m_Alignment, r, g, b);
	float glowR, glowG, glowB;
	AlignmentColor::Glow(m_Alignment, glowR, glowG, glowB);

	// The plinth, and the ring on the ground round it.
	out.AddShadow(depth - 0.01f, x, groundY, 30.f, 15.f);
	out.AddBox(depth, x, groundY, 0.45f, 0.45f, 8.f, 0.30f, 0.30f, 0.32f);
	out.SetGlow(m_Alignment == Alignment::Dark ? 0.f : 0.6f);
	out.AddGroundArc(WorldX(), WorldY(), x, groundY + 2.f, kRingRadius - kRingWidth, kRingRadius, 0.f, 2.f * kPi,
		kRingSegments, r, g, b, 0.7f);

	// The shard, turning about its upright axis, flaring when a blow glances
	// off it.
	float shardY = groundY + kHang + sinf(m_Age * kBobRate) * kBob;
	float turn = fabsf(cosf(m_Age * kSpinRate));
	float width = kShardWidth * (0.25f + 0.75f * turn);
	float glance = (m_Glance > 0.f) ? m_Glance / kGlanceTime : 0.f;
	out.AddGlow(depth + 0.01f, x, shardY, 40.f + 20.f * glance, glowR, glowG, glowB, 0.8f);
	out.SetFlash(glance * 0.6f);
	out.AddDiamond(depth + 0.02f, x, shardY, width, kShardHeight, r, g, b, 1.f);
	out.AddDiamond(depth + 0.021f, x, shardY, width * 0.4f, kShardHeight * 0.5f, 1.f, 1.f, 1.f, 0.8f);
	out.SetFlash(0.f);
	out.SetGlow(0.f);
}
