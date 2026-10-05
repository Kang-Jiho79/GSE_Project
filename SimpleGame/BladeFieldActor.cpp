#include "stdafx.h"
#include "BladeFieldActor.h"
#include "BladeActor.h"
#include "EnemyActor.h"
#include "PlayerStatus.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Impact.h"
#include "Random.h"
#include "Alignment.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kPi = 3.14159265f;
	const int kSegments = 32;
	const float kLift = 1.f;
	// The circle lingers this long after the last wave, fading.
	const float kFadeTime = 0.4f;
	// Each blade is a touch late after the last, so they ripple out.
	const float kRipple = 0.02f;
}

BladeFieldActor::BladeFieldActor(float radiusTiles, float damage, int waves)
	: m_Radius(radiusTiles)
	, m_Damage(damage)
	, m_WavesLeft(waves)
	, m_NextWave(Tuning::DarkForm::kEruptDelay)
{
}

void BladeFieldActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;
	if (m_WavesLeft > 0 && m_Age >= m_NextWave)
	{
		Erupt(ctx);
		m_WavesLeft--;
		m_NextWave += Tuning::DarkForm::kEruptWaveGap;
	}
	if (m_WavesLeft <= 0 && m_Age >= m_NextWave - Tuning::DarkForm::kEruptWaveGap + kFadeTime)
	{
		Destroy();
	}
}

void BladeFieldActor::Erupt(const ActorContext& ctx)
{
	float centerCol, centerRow;
	WorldGrid::InverseIso(WorldX(), WorldY(), centerCol, centerRow);

	// Under every enemy inside at once, up to a limit.
	float landed = 0.f;
	int struck = 0;
	ctx.graph->ForEach<EnemyActor>([&](EnemyActor& enemy)
	{
		if (enemy.IsDestroyed() || struck >= Tuning::DarkForm::kEruptMax)
		{
			return;
		}
		float col, row;
		WorldGrid::InverseIso(enemy.WorldX(), enemy.WorldY(), col, row);
		float dc = col - centerCol;
		float dr = row - centerRow;
		float reach = m_Radius + enemy.BodyRadius();
		if (dc * dc + dr * dr > reach * reach)
		{
			return;
		}

		landed += enemy.TakeHit(m_Damage);
		std::unique_ptr<BladeActor> blade(new BladeActor(struck * kRipple, (Random::Unit() - 0.5f) * 10.f, 1.2f));
		blade->SetLocalPosition(enemy.WorldX(), enemy.WorldY());
		ctx.graph->Spawn(std::move(blade));
		struck++;
	});

	// And a few where there is no one, scattered through the circle.
	for (int i = 0; i < Tuning::DarkForm::kEruptExtras; i++)
	{
		float angle = Random::Angle();
		float distance = sqrtf(Random::Unit()) * m_Radius;
		float x, y;
		WorldGrid::IsoPosition(centerCol + cosf(angle) * distance, centerRow + sinf(angle) * distance, x, y);
		std::unique_ptr<BladeActor> blade(new BladeActor(Random::Unit() * 0.12f, (Random::Unit() - 0.5f) * 10.f,
			0.6f + Random::Unit() * 0.3f));
		blade->SetLocalPosition(x, y);
		ctx.graph->Spawn(std::move(blade));
	}

	ctx.status->Heal(Combat::Lifesteal(*ctx.status, landed));
	ctx.impact->Shake(Tuning::Feel::kHeavyShake);
	if (landed > 0.f)
	{
		ctx.impact->HitStop(Tuning::Feel::kHeavyStop);
	}
}

void BladeFieldActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY()) + kLift;

	// Drawn out from the sword before the first wave, held through them, and
	// faded after the last.
	float spread = m_Age / Tuning::DarkForm::kEruptDelay;
	if (spread > 1.f)
	{
		spread = 1.f;
	}
	float fade = 1.f;
	if (m_WavesLeft <= 0)
	{
		float after = m_Age - (m_NextWave - Tuning::DarkForm::kEruptWaveGap);
		fade = 1.f - after / kFadeTime;
		if (fade < 0.f)
		{
			fade = 0.f;
		}
	}

	float r, g, b;
	AlignmentColor::Core(Alignment::Dark, r, g, b);
	float radius = m_Radius * spread;
	out.AddGroundArc(WorldX(), WorldY(), x, groundY, 0.f, radius, 0.f, 2.f * kPi, kSegments, r, g, b, 0.12f * fade);
	out.AddGroundArc(WorldX(), WorldY(), x, groundY, radius - 0.08f, radius, 0.f, 2.f * kPi, kSegments, r, g, b, 0.7f * fade);

	// Cracks running out from the sword to the rim.
	for (int i = 0; i < 6; i++)
	{
		float angle = i * kPi / 3.f + 0.3f;
		out.AddGroundArc(WorldX(), WorldY(), x, groundY, 0.2f, radius, angle - 0.02f, angle + 0.02f, 1,
			r * 0.6f, g * 0.6f, b * 0.6f, 0.6f * fade);
	}
}
