#include "stdafx.h"
#include "LightFormMoves.h"
#include "PlayerActor.h"
#include "ProjectileActor.h"
#include "BurstActor.h"
#include "BeamActor.h"
#include "LightningActor.h"
#include "SparkActor.h"
#include "Impact.h"
#include "PlayerStatus.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "SceneGraph.h"
#include "Tuning.h"
#include "Growth.h"
#include <cmath>

namespace
{
	// A spark off his hand with every bolt.
	const int kCastSparks = 3;
	const float kCastSparkSpeed = 110.f;
	const float kCastSparkLift = 22.f;

	// Light scattering along the beam once it has passed.
	const int kBeamSparkPoints = 4;
	const int kBeamSparks = 5;
	const float kBeamSparkSpeed = 90.f;
	const float kBeamSparkLift = 14.f;

	// The spray as the form turns over to light.
	const int kSwapSparks = 14;
	const float kSwapSparkSpeed = 220.f;
	const float kSwapSparkLift = 16.f;
}

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

	// One bolt at first; two, then three, fanned about the aim as it grows.
	int bolts = 1;
	if (Growth::Has(*ctx.status, Tuning::Growth::kAttackII))
	{
		bolts = 3;
	}
	else if (Growth::Has(*ctx.status, Tuning::Growth::kAttackI))
	{
		bolts = 2;
	}
	float aimAngle = atan2f(ctx.input.aimY, ctx.input.aimX);
	float spread = Tuning::Growth::kBoltSpread * 3.14159265f / 180.f;
	for (int i = 0; i < bolts; i++)
	{
		float angle = aimAngle + (i - (bolts - 1) * 0.5f) * spread;
		std::unique_ptr<ProjectileActor> bolt(new ProjectileActor(Alignment::Light,
			cosf(angle) * Tuning::LightForm::kBoltSpeedPx, sinf(angle) * Tuning::LightForm::kBoltSpeedPx));
		bolt->SetLocalPosition(player.WorldX(), player.WorldY());
		ctx.graph->Spawn(std::move(bolt));
	}

	player.Act(PlayerActor::Action::Cast, ctx.input.aimX, ctx.input.aimY);
	SparkActor::Burst(*ctx.graph, player.WorldX(), player.WorldY(), SparkActor::Style::Light,
		kCastSparks, kCastSparkSpeed, kCastSparkLift);

	m_BoltCooldown = Tuning::LightForm::kBoltCooldown;
}

void LightFormMoves::Skill(const ActorContext& ctx, PlayerActor& player)
{
	if (!Growth::Has(*ctx.status, Tuning::Growth::kSkills) || !ctx.status->lightSkill.Ready())
	{
		return;
	}

	float dirCol, dirRow;
	if (!WorldGrid::ScreenToGroundDirection(ctx.input.aimX, ctx.input.aimY, dirCol, dirRow))
	{
		return;
	}

	float width = Tuning::LightForm::kBeamWidth;
	if (Growth::Has(*ctx.status, Tuning::Growth::kSkillsII))
	{
		width *= Tuning::Growth::kBeamWidthScale;
	}
	float landed = Combat::HitLine(*ctx.graph, player.WorldX(), player.WorldY(), dirCol, dirRow,
		Tuning::LightForm::kBeamLength, width, Combat::BeamDamage(*ctx.status), true);
	if (landed > 0.f)
	{
		ctx.impact->Shake(Tuning::Feel::kLightShake);
	}

	player.Act(PlayerActor::Action::Beam, ctx.input.aimX, ctx.input.aimY);

	// Light left hanging along where it went, scattering as it fades.
	for (int i = 1; i <= kBeamSparkPoints; i++)
	{
		float along = Tuning::LightForm::kBeamLength * i / (kBeamSparkPoints + 1);
		float offsetX, offsetY;
		WorldGrid::IsoPosition(dirCol * along, dirRow * along, offsetX, offsetY);
		SparkActor::Burst(*ctx.graph, player.WorldX() + offsetX, player.WorldY() + offsetY, SparkActor::Style::Light,
			kBeamSparks, kBeamSparkSpeed, kBeamSparkLift);
	}

	std::unique_ptr<BeamActor> beam(new BeamActor(Alignment::Light, dirCol, dirRow, Tuning::LightForm::kBeamLength));
	beam->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(beam));

	ctx.status->lightSkill.Start(Tuning::LightForm::kBeamCooldown);
}

void LightFormMoves::Special(const ActorContext& ctx, PlayerActor& player)
{
	if (!Growth::Has(*ctx.status, Tuning::Growth::kSkills) || !ctx.status->lightSpecial.Ready())
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

	// Grown stronger, it strikes the same spot a second time.
	if (Growth::Has(*ctx.status, Tuning::Growth::kSkillsII))
	{
		std::unique_ptr<LightningActor> again(new LightningActor(LightningActor::Target::Monsters,
			Tuning::LightForm::kStrikeRadius, Tuning::LightForm::kStrikeDelay + Tuning::Growth::kSecondStrikeDelay,
			Combat::StrikeDamage(*ctx.status)));
		again->SetLocalPosition(x, y);
		ctx.graph->Spawn(std::move(again));
	}
	player.Act(PlayerActor::Action::Call, ctx.input.aimX, ctx.input.aimY);

	ctx.status->lightSpecial.Start(Tuning::LightForm::kStrikeCooldown);
}

void LightFormMoves::Enter(const ActorContext& ctx, PlayerActor& player)
{
	float scale = Growth::Has(*ctx.status, Tuning::Growth::kSwapII) ? Tuning::Growth::kSwapScale : 1.f;
	float radius = Tuning::SwapSkill::kLightBurstRadius * scale;
	Combat::HitRadius(*ctx.graph, player.WorldX(), player.WorldY(), radius, 0.f, Tuning::SwapSkill::kLightBurstPush * scale);
	SparkActor::Burst(*ctx.graph, player.WorldX(), player.WorldY(), SparkActor::Style::Light,
		kSwapSparks, kSwapSparkSpeed, kSwapSparkLift);
	ctx.status->GrantInvuln(Tuning::SwapSkill::kLightBurstInvuln * scale);
	player.Act(PlayerActor::Action::Swap, 0.f, 0.f);

	std::unique_ptr<BurstActor> burst(new BurstActor(Alignment::Light, radius, false));
	burst->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(burst));
}
