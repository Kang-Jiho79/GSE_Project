#include "stdafx.h"
#include "LightFormMoves.h"
#include "PlayerActor.h"
#include "ProjectileActor.h"
#include "BurstActor.h"
#include "BeamActor.h"
#include "LightningActor.h"
#include "PlayerStatus.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "SceneGraph.h"
#include "Tuning.h"
#include <cmath>

void LightFormMoves::Update(const ActorContext& ctx, PlayerActor& player)
{
	if (m_BoltCooldown > 0.f)
	{
		m_BoltCooldown -= ctx.deltaTime;
	}
}

void LightFormMoves::Attack(const ActorContext& ctx, PlayerActor& player)
{
	if (m_BoltCooldown > 0.f)
	{
		return;
	}

	float aimLength = sqrtf(ctx.input.aimX * ctx.input.aimX + ctx.input.aimY * ctx.input.aimY);
	if (aimLength < 0.001f)
	{
		return;
	}

	std::unique_ptr<ProjectileActor> bolt(new ProjectileActor(Alignment::Light,
		(ctx.input.aimX / aimLength) * Tuning::LightForm::kBoltSpeedPx,
		(ctx.input.aimY / aimLength) * Tuning::LightForm::kBoltSpeedPx));
	bolt->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(bolt));

	m_BoltCooldown = Tuning::LightForm::kBoltCooldown;
}

void LightFormMoves::Skill(const ActorContext& ctx, PlayerActor& player)
{
	if (!ctx.status->lightSkill.Ready())
	{
		return;
	}

	float dirCol, dirRow;
	if (!WorldGrid::ScreenToGroundDirection(ctx.input.aimX, ctx.input.aimY, dirCol, dirRow))
	{
		return;
	}

	Combat::HitLine(*ctx.graph, player.WorldX(), player.WorldY(), dirCol, dirRow,
		Tuning::LightForm::kBeamLength, Tuning::LightForm::kBeamWidth, Combat::BeamDamage(*ctx.status), true);

	std::unique_ptr<BeamActor> beam(new BeamActor(Alignment::Light, dirCol, dirRow, Tuning::LightForm::kBeamLength));
	beam->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(beam));

	ctx.status->lightSkill.Start(Tuning::LightForm::kBeamCooldown);
}

void LightFormMoves::Special(const ActorContext& ctx, PlayerActor& player)
{
	if (!ctx.status->lightSpecial.Ready())
	{
		return;
	}

	// The cursor marks the spot on the ground. Too far, and it lands at the
	// edge of his reach in that direction instead.
	float playerCol, playerRow, aimCol, aimRow;
	WorldGrid::InverseIso(player.WorldX(), player.WorldY(), playerCol, playerRow);
	WorldGrid::InverseIso(ctx.input.aimX, ctx.input.aimY, aimCol, aimRow);

	float distance = sqrtf(aimCol * aimCol + aimRow * aimRow);
	if (distance > Tuning::LightForm::kStrikeMaxRange)
	{
		aimCol *= Tuning::LightForm::kStrikeMaxRange / distance;
		aimRow *= Tuning::LightForm::kStrikeMaxRange / distance;
	}

	float col = playerCol + aimCol;
	float row = playerRow + aimRow;
	WorldGrid::ClampToMap(col, row);

	float x, y;
	WorldGrid::IsoPosition(col, row, x, y);

	std::unique_ptr<LightningActor> strike(new LightningActor(LightningActor::Target::Monsters,
		Tuning::LightForm::kStrikeRadius, Tuning::LightForm::kStrikeDelay, Combat::StrikeDamage(*ctx.status)));
	strike->SetLocalPosition(x, y);
	ctx.graph->Spawn(std::move(strike));

	ctx.status->lightSpecial.Start(Tuning::LightForm::kStrikeCooldown);
}

void LightFormMoves::Enter(const ActorContext& ctx, PlayerActor& player)
{
	Combat::HitRadius(*ctx.graph, player.WorldX(), player.WorldY(),
		Tuning::SwapSkill::kLightBurstRadius, 0.f, Tuning::SwapSkill::kLightBurstPush);
	ctx.status->GrantInvuln(Tuning::SwapSkill::kLightBurstInvuln);

	std::unique_ptr<BurstActor> burst(new BurstActor(Alignment::Light, Tuning::SwapSkill::kLightBurstRadius, false));
	burst->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(burst));
}
