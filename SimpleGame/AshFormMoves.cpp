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

	// The wave: a band of gray rolling out across the wedge and fading.
	const float kWaveShowTime = 0.22f;
	const int kWaveSegments = 10;
	const float kWaveInner = 0.4f;   // tiles
	const float kWaveBand = 0.55f;   // tiles deep
	const float kWaveLift = 8.f;     // pixels above the ground

	const int kEraseDust = 4;
	const float kEraseDustSpeed = 90.f;
	const float kEraseDustLift = 20.f;
	const float kEraseShake = 5.f;

	// Wipes every enemy shot within a circle on the ground out of the air.
	void EraseShots(SceneGraph& graph, float centerX, float centerY, float radiusTiles)
	{
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
			if (dc * dc + dr * dr > radiusTiles * radiusTiles)
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

	Combat::HitArc(*ctx.graph, player.WorldX(), player.WorldY(), dirCol, dirRow,
		Tuning::AshForm::kWaveRange, Tuning::AshForm::kWaveArcDegrees, Combat::AshWaveDamage(*ctx.status));

	m_WaveDirCol = dirCol;
	m_WaveDirRow = dirRow;
	m_WaveTimer = kWaveShowTime;
	player.Act(PlayerActor::Action::Thrust, ctx.input.aimX, ctx.input.aimY);
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
	Combat::HitRadius(*ctx.graph, player.WorldX(), player.WorldY(), radius, Combat::EraseDamage(*ctx.status), 0.f);
	ctx.status->GrantInvuln(Tuning::AshForm::kEraseInvuln);
	player.Act(PlayerActor::Action::Swap, 0.f, 0.f);
	ctx.impact->Shake(kEraseShake);

	std::unique_ptr<BurstActor> burst(new BurstActor(Alignment::Ashen, radius, false));
	burst->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(burst));

	ctx.status->ashSpecial.Start(Tuning::AshForm::kEraseCooldown);
}

void AshFormMoves::Enter(const ActorContext& ctx, PlayerActor& player)
{
	float radius = Tuning::AshForm::kVeilRadius;
	EraseShots(*ctx.graph, player.WorldX(), player.WorldY(), radius);
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
	float range = Tuning::AshForm::kWaveRange;
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
	out.AddGroundArc(player.WorldX(), player.WorldY(), screenX, groundY + kWaveLift, inner, outer,
		center - half, center + half, kWaveSegments, r, g, b, 0.75f * (1.f - age));
}
