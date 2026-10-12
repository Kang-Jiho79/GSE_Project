#include "stdafx.h"
#include "AshFormMoves.h"
#include "PlayerActor.h"
#include "PlayerStatus.h"
#include "EnemyProjectileActor.h"
#include "BurstActor.h"
#include "BeamActor.h"
#include "SparkActor.h"
#include "Impact.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kDegToRad = 3.14159265f / 180.f;

	const float kPi = 3.14159265f;

	// The wave: a band of gray rolling out across the wedge and fading, a
	// fainter wash filling the ground it has crossed.
	const float kWaveShowTime = 0.26f;
	const int kWaveSegments = 14;
	const int kRingSegments = 32;
	const float kWaveInner = 0.4f;   // tiles
	const float kWaveBand = 0.8f;    // tiles deep
	const float kWaveLift = 8.f;     // pixels above the ground
	const float kWaveAlpha = 0.9f;
	const float kWaveWashAlpha = 0.25f;
	const float kWaveShine = 0.35f;
	const int kWaveDust = 5;
	const float kWaveDustSpeed = 150.f;
	const float kWaveDustLift = 18.f;

	const int kEraseDust = 4;
	const float kEraseDustSpeed = 90.f;
	const float kEraseDustLift = 20.f;
	const float kEraseShake = 5.f;

	// Wipes every enemy shot out of the air within a circle on the ground -
	// or, given a direction and a spread, within a wedge of it.
	void EraseShots(SceneGraph& graph, float centerX, float centerY, float radiusTiles,
		float dirCol = 0.f, float dirRow = 0.f, float arcDegrees = 360.f)
	{
		float minDot = cosf(arcDegrees * 0.5f * kDegToRad);
		float centerCol, centerRow;
		WorldGrid::InverseIso(centerX, centerY, centerCol, centerRow);

		graph.ForEach<EnemyProjectileActor>([&](EnemyProjectileActor& shot)
		{
			if (shot.IsDestroyed())
			{
				return;
			}
			float col, row;
			WorldGrid::InverseIso(shot.WorldX(), shot.WorldY(), col, row);
			float dc = col - centerCol;
			float dr = row - centerRow;
			float distance = sqrtf(dc * dc + dr * dr);
			if (distance > radiusTiles)
			{
				return;
			}
			if (arcDegrees < 360.f && distance > 0.001f && (dc * dirCol + dr * dirRow) / distance < minDot)
			{
				return;
			}
			SparkActor::Burst(graph, shot.WorldX(), shot.WorldY(), SparkActor::Style::Dust,
				kEraseDust, kEraseDustSpeed, kEraseDustLift);
			shot.Destroy();
		});
	}
}

void AshFormMoves::Update(const ActorContext& ctx, PlayerActor& player)
{
	if (m_Cooldown > 0.f)
	{
		m_Cooldown -= ctx.deltaTime;
	}
	if (m_WaveTimer > 0.f)
	{
		m_WaveTimer -= ctx.deltaTime;
	}
	if (m_ChainTimer > 0.f)
	{
		m_ChainTimer -= ctx.deltaTime;
	}
}

void AshFormMoves::Attack(const ActorContext& ctx, PlayerActor& player)
{
	if (m_Cooldown > 0.f)
	{
		return;
	}

	float dirCol, dirRow;
	if (!WorldGrid::ScreenToGroundDirection(ctx.input.aimX, ctx.input.aimY, dirCol, dirRow))
	{
		return;
	}

	if (m_ChainTimer <= 0.f)
	{
		m_WaveCount = 0;
	}
	m_WaveCount++;
	m_ChainTimer = Tuning::AshForm::kWaveChainTime;
	m_WaveRing = (m_WaveCount % Tuning::AshForm::kWaveRingEvery == 0);

	float damage = Combat::AshWaveDamage(*ctx.status);
	float range = Tuning::AshForm::kWaveRange;
	if (m_WaveRing)
	{
		range += Tuning::AshForm::kWaveRingReach;
		damage *= Tuning::AshForm::kWaveRingScale;
		EraseShots(*ctx.graph, player.WorldX(), player.WorldY(), range);
		if (Combat::HitRadius(*ctx.graph, player.WorldX(), player.WorldY(), range, damage, 0.f) > 0.f)
		{
			ctx.impact->HitStop(Tuning::Feel::kHeavyStop);
		}
		ctx.impact->Shake(Tuning::Feel::kHeavyShake);
		player.Act(PlayerActor::Action::Swap, 0.f, 0.f);
	}
	else
	{
		EraseShots(*ctx.graph, player.WorldX(), player.WorldY(), range, dirCol, dirRow, Tuning::AshForm::kWaveArcDegrees);
		if (Combat::HitArc(*ctx.graph, player.WorldX(), player.WorldY(), dirCol, dirRow,
			range, Tuning::AshForm::kWaveArcDegrees, damage) > 0.f)
		{
			ctx.impact->Shake(Tuning::Feel::kLightShake);
		}
		player.Act(PlayerActor::Action::Thrust, ctx.input.aimX, ctx.input.aimY);
	}

	// Gray grit thrown off ahead of it.
	float aheadX, aheadY;
	WorldGrid::IsoPosition(dirCol, dirRow, aheadX, aheadY);
	SparkActor::Burst(*ctx.graph, player.WorldX() + aheadX * 0.8f, player.WorldY() + aheadY * 0.8f,
		SparkActor::Style::Dust, kWaveDust, kWaveDustSpeed, kWaveDustLift);

	m_WaveDirCol = dirCol;
	m_WaveDirRow = dirRow;
	m_WaveTimer = kWaveShowTime;
	m_Cooldown = Tuning::AshForm::kWaveInterval;
}

void AshFormMoves::Skill(const ActorContext& ctx, PlayerActor& player)
{
	if (!ctx.status->ashSkill.Ready())
	{
		return;
	}

	float dirCol, dirRow;
	if (!WorldGrid::ScreenToGroundDirection(ctx.input.aimX, ctx.input.aimY, dirCol, dirRow))
	{
		return;
	}

	// Not blockable: a shield is only light and dark, and this is neither.
	float landed = Combat::HitLine(*ctx.graph, player.WorldX(), player.WorldY(), dirCol, dirRow,
		Tuning::AshForm::kUnravelLength, Tuning::AshForm::kUnravelWidth, Combat::UnravelDamage(*ctx.status), false);
	if (landed > 0.f)
	{
		ctx.impact->Shake(Tuning::Feel::kLightShake);
	}

	player.Act(PlayerActor::Action::Beam, ctx.input.aimX, ctx.input.aimY);
	std::unique_ptr<BeamActor> beam(new BeamActor(Alignment::Ashen, dirCol, dirRow, Tuning::AshForm::kUnravelLength));
	beam->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(beam));

	ctx.status->ashSkill.Start(Tuning::AshForm::kUnravelCooldown);
}

void AshFormMoves::Special(const ActorContext& ctx, PlayerActor& player)
{
	if (!ctx.status->ashSpecial.Ready())
	{
		return;
	}

	float radius = Tuning::AshForm::kEraseRadius;
	EraseShots(*ctx.graph, player.WorldX(), player.WorldY(), radius);
	if (Combat::HitRadius(*ctx.graph, player.WorldX(), player.WorldY(), radius, Combat::EraseDamage(*ctx.status), 0.f) > 0.f)
	{
		ctx.impact->HitStop(Tuning::Feel::kHeavyStop);
	}
	ctx.status->GrantInvuln(Tuning::AshForm::kEraseInvuln);
	player.Act(PlayerActor::Action::Swap, 0.f, 0.f);
	ctx.impact->Shake(kEraseShake);

	// A second ring closing in as the first spreads, so it reads heavier
	// than the waves.
	std::unique_ptr<BurstActor> inner(new BurstActor(Alignment::Ashen, radius * 0.6f, true));
	inner->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(inner));

	std::unique_ptr<BurstActor> burst(new BurstActor(Alignment::Ashen, radius, false));
	burst->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(burst));

	ctx.status->ashSpecial.Start(Tuning::AshForm::kEraseCooldown);
}

void AshFormMoves::Enter(const ActorContext& ctx, PlayerActor& player)
{
	float radius = Tuning::AshForm::kVeilRadius;
	EraseShots(*ctx.graph, player.WorldX(), player.WorldY(), radius);
	Combat::HitRadius(*ctx.graph, player.WorldX(), player.WorldY(), radius, Combat::VeilDamage(*ctx.status), 0.f);
	ctx.status->GrantInvuln(Tuning::AshForm::kVeilInvuln);
	player.Act(PlayerActor::Action::Swap, 0.f, 0.f);

	std::unique_ptr<BurstActor> burst(new BurstActor(Alignment::Ashen, radius, true));
	burst->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(burst));
}

void AshFormMoves::Draw(DrawList& out, const PlayerActor& player, float screenX, float groundY) const
{
	if (m_WaveTimer <= 0.f)
	{
		return;
	}

	// Rolling outward over its life, and thinning as it goes.
	float age = 1.f - m_WaveTimer / kWaveShowTime;
	float range = Tuning::AshForm::kWaveRange + (m_WaveRing ? Tuning::AshForm::kWaveRingReach : 0.f);
	float outer = kWaveInner + (range - kWaveInner) * age;
	float inner = outer - kWaveBand;
	if (inner < kWaveInner)
	{
		inner = kWaveInner;
	}

	float r, g, b;
	AlignmentColor::Core(Alignment::Ashen, r, g, b);
	float center = atan2f(m_WaveDirRow, m_WaveDirCol);
	float half = Tuning::AshForm::kWaveArcDegrees * 0.5f * kDegToRad;
	int segments = kWaveSegments;
	if (m_WaveRing)
	{
		half = kPi;
		segments = kRingSegments;
	}
	float fade = 1.f - age;
	out.AddGroundArc(player.WorldX(), player.WorldY(), screenX, groundY + kWaveLift, kWaveInner, inner,
		center - half, center + half, segments, r, g, b, kWaveWashAlpha * fade);
	out.SetGlow(kWaveShine);
	out.AddGroundArc(player.WorldX(), player.WorldY(), screenX, groundY + kWaveLift, inner, outer,
		center - half, center + half, segments, r, g, b, kWaveAlpha * fade);
	out.SetGlow(0.f);
}
