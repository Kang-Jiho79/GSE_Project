#include "stdafx.h"
#include "WitnessLampActor.h"
#include "KingActor.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Palette.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kPi = 3.14159265f;
	const float kHang = 46.f;     // pixels off the ground it hangs
	const float kOutShow = 0.4f;  // seconds it is seen going out
	const int kSegments = 24;
	const int kOutSparks = 10;
	const float kOutSparkSpeed = 150.f;
	const float kOutSparkLift = 46.f;
}

WitnessLampActor::WitnessLampActor(const KingActor* owner, float lifetime)
	: m_Owner(owner)
	, m_Lifetime(lifetime)
	, m_Health(Tuning::Lucius::kWitnessHealth)
{
}

float WitnessLampActor::BodyRadius() const
{
	return 0.5f;
}

bool WitnessLampActor::IsLit() const
{
	return m_Lit;
}

void WitnessLampActor::Snuff()
{
	m_Lit = false;
}

float WitnessLampActor::TakeHit(float damage)
{
	if (!m_Lit)
	{
		return 0.f;
	}
	m_Health -= damage;
	if (m_Health <= 0.f)
	{
		m_Lit = false;
		m_SparksPending = true;
	}
	return 0.f;
}

void WitnessLampActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;
	if (m_SparksPending)
	{
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Light, kOutSparks, kOutSparkSpeed, kOutSparkLift);
		m_SparksPending = false;
	}
	if (m_Lit && (!m_Owner->IsDueling() || m_Age >= m_Lifetime))
	{
		m_Lit = false;
	}
	if (!m_Lit)
	{
		m_OutTime += ctx.deltaTime;
		if (m_OutTime >= kOutShow)
		{
			Destroy();
		}
	}
}

void WitnessLampActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 80.f, 120.f))
	{
		return;
	}
	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;
	const Palette::Color& gold = Palette::kLightCore;
	float fade = m_Lit ? 1.f : 1.f - m_OutTime / kOutShow;
	float flicker = 0.85f + 0.15f * sinf(m_Age * 9.f);

	// Under it, the ring of its time, running out.
	float left = 1.f - m_Age / m_Lifetime;
	if (m_Lit && left > 0.f)
	{
		out.SetGlow(0.5f);
		out.AddGroundArc(WorldX(), WorldY(), x, groundY + 2.f, 0.55f, 0.7f, -kPi * 0.5f, -kPi * 0.5f + 2.f * kPi * left,
			kSegments, gold.r, gold.g, gold.b, 0.8f);
		out.SetGlow(0.f);
	}

	// The lamp itself, hanging in the air, and its light.
	out.AddShadow(depth - 0.01f, x, groundY, 20.f, 10.f);
	out.SetGlow(m_Lit ? 0.9f * flicker : 0.f);
	out.AddGlow(depth + 0.01f, x, groundY + kHang, 34.f * fade, gold.r, gold.g, gold.b, 0.6f * fade * flicker);
	out.AddDiamond(depth + 0.02f, x, groundY + kHang, 10.f, 16.f, gold.r * fade, gold.g * fade, gold.b * fade, 1.f);
	out.SetGlow(0.f);
	out.AddBox(depth + 0.015f, x, groundY + kHang + 8.f, 0.14f, 0.14f, 4.f, 0.5f, 0.45f, 0.35f);
}
