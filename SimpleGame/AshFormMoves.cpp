#include "stdafx.h"
#include "AshFormMoves.h"
#include "PlayerActor.h"
#include "PlayerStatus.h"
#include "EnemyActor.h"
#include "EnemyProjectileActor.h"
#include "AfterimageActor.h"
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

	// A cut's trail: a band of gray swept out across its arc over the first
	// part of its life and faded over the rest, its outer edge bright.
	const float kCutShowTime = 0.22f;
	const float kCutSweepShare = 0.45f;
	const int kCutSegments = 12;
	const int kRingSegments = 28;
	const float kCutInnerShare = 0.4f;  // of its reach, where the band starts
	const float kCutEdge = 0.18f;       // tiles, the bright rim
	const float kCutLift = 12.f;        // pixels above the ground, about waist height
	const float kCutAlpha = 0.55f;
	const float kCutEdgeAlpha = 0.95f;
	const float kCutShine = 0.5f;
	const int kCutDust = 4;
	const float kCutDustSpeed = 150.f;
	const float kCutDustLift = 14.f;

	// The last cut's trail as it waits, faint, and as it bursts.
	const float kEchoWaitAlpha = 0.3f;
	const float kEchoShowTime = 0.25f;
	const int kEchoDust = 10;

	// The dash and its blast.
	const int kDashDust = 8;
	const float kDashDustSpeed = 170.f;
	const float kDashDustLift = 18.f;

	// The parry's stance: a ring close round him, tightening.
	const float kStanceInner = 0.7f;
	const float kStanceWidth = 0.15f;
	const float kStanceAlpha = 0.8f;
	const int kCounterDust = 14;
	const float kCounterDustSpeed = 220.f;

	const int kEraseDust = 4;
	const float kEraseDustSpeed = 90.f;
	const float kEraseDustLift = 20.f;
	const float kEraseShake = 5.f;

	// Wipes every enemy shot out of the air within a circle on the ground.
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

	// A wedge of ground struck from a point - a full turn struck all round.
	float Cut(SceneGraph& graph, float x, float y, float dirCol, float dirRow, float range, float arcDegrees, float damage)
	{
		if (arcDegrees >= 360.f)
		{
			return Combat::HitRadius(graph, x, y, range, damage, 0.f);
		}
		return Combat::HitArc(graph, x, y, dirCol, dirRow, range, arcDegrees, damage);
	}

	// A cut's band on the ground, swept across by sweep (0..1) of its arc.
	// side: which way it sweeps.
	void DrawBand(DrawList& out, float worldX, float worldY, float screenX, float screenY,
		float dirCol, float dirRow, float range, float arcDegrees, float side, float sweep, float alpha)
	{
		if (alpha <= 0.f || sweep <= 0.f)
		{
			return;
		}
		float r, g, b;
		AlignmentColor::Core(Alignment::Ashen, r, g, b);
		float center = atan2f(dirRow, dirCol);
		float half = arcDegrees * 0.5f * kDegToRad;
		int segments = (arcDegrees >= 360.f) ? kRingSegments : kCutSegments;
		// From the side it starts on, as far round as it has got.
		float from = center + side * half;
		float to = from - side * 2.f * half * sweep;
		if (to < from)
		{
			float swap = from;
			from = to;
			to = swap;
		}
		float inner = range * kCutInnerShare;
		out.AddGroundArc(worldX, worldY, screenX, screenY, inner, range - kCutEdge, from, to, segments,
			r, g, b, kCutAlpha * alpha);
		out.SetGlow(kCutShine);
		out.AddGroundArc(worldX, worldY, screenX, screenY, range - kCutEdge, range, from, to, segments,
			1.f, 1.f, 1.f, kCutEdgeAlpha * alpha);
		out.SetGlow(0.f);
	}

	// Which way each cut sweeps: the first one way, the second back, the
	// turn and the last the first way again.
	float CutSide(int step)
	{
		return (step == 1) ? -1.f : 1.f;
	}
}

void AshFormMoves::Update(const ActorContext& ctx, PlayerActor& player)
{
	if (m_Cooldown > 0.f)
	{
		m_Cooldown -= ctx.deltaTime;
	}
	if (m_DashCooldown > 0.f)
	{
		m_DashCooldown -= ctx.deltaTime;
	}
	if (m_ComboTimer > 0.f)
	{
		m_ComboTimer -= ctx.deltaTime;
	}
	if (m_CutTimer > 0.f)
	{
		m_CutTimer -= ctx.deltaTime;
	}
	if (m_EchoTimer > 0.f)
	{
		m_EchoTimer -= ctx.deltaTime;
	}
	if (m_StanceTime > 0.f)
	{
		m_StanceTime -= ctx.deltaTime;
	}

	// The last cut's trail bursts again where it was cut.
	if (m_EchoDelay > 0.f)
	{
		m_EchoDelay -= ctx.deltaTime;
		if (m_EchoDelay <= 0.f)
		{
			int last = Tuning::AshForm::kComboLength - 1;
			if (Cut(*ctx.graph, m_EchoX, m_EchoY, m_EchoDirCol, m_EchoDirRow, Tuning::AshForm::kCutRange[last],
				Tuning::AshForm::kCutArcDegrees[last], Combat::ScytheEchoDamage()) > 0.f)
			{
				ctx.impact->HitStop(Tuning::Feel::kHeavyStop);
			}
			ctx.impact->Shake(Tuning::Feel::kLightShake);
			float aheadX, aheadY;
			WorldGrid::IsoPosition(m_EchoDirCol * 1.5f, m_EchoDirRow * 1.5f, aheadX, aheadY);
			SparkActor::Burst(*ctx.graph, m_EchoX + aheadX, m_EchoY + aheadY, SparkActor::Style::Dust,
				kEchoDust, kCutDustSpeed, kCutDustLift);
			m_EchoTimer = kEchoShowTime;
		}
	}

	// The image left where a dash set off, bursting.
	if (m_BlastDelay > 0.f)
	{
		m_BlastDelay -= ctx.deltaTime;
		if (m_BlastDelay <= 0.f)
		{
			float radius = Tuning::AshForm::kDashBlastRadius;
			Combat::HitRadius(*ctx.graph, m_BlastX, m_BlastY, radius, Combat::DashBlastDamage(), 0.f);
			std::unique_ptr<BurstActor> burst(new BurstActor(Alignment::Ashen, radius, false));
			burst->SetLocalPosition(m_BlastX, m_BlastY);
			ctx.graph->Spawn(std::move(burst));
			SparkActor::Burst(*ctx.graph, m_BlastX, m_BlastY, SparkActor::Style::Dust,
				kDashDust, kDashDustSpeed, kDashDustLift);
		}
	}

	if (ctx.status->parried)
	{
		ctx.status->parried = false;
		Counter(ctx, player);
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

	if (m_ComboTimer <= 0.f)
	{
		m_ComboStep = 0;
	}
	int step = m_ComboStep;
	bool last = (step == Tuning::AshForm::kComboLength - 1);
	float range = Tuning::AshForm::kCutRange[step];
	float arc = Tuning::AshForm::kCutArcDegrees[step];

	float landed = Cut(*ctx.graph, player.WorldX(), player.WorldY(), dirCol, dirRow, range, arc,
		Combat::ScytheDamage(step));
	if (landed > 0.f)
	{
		if (last)
		{
			ctx.impact->HitStop(Tuning::Feel::kHeavyStop);
		}
		ctx.impact->Shake(last ? Tuning::Feel::kHeavyShake : Tuning::Feel::kLightShake);
	}

	// Gray grit thrown off where the point of the blade passes.
	float tipX, tipY;
	WorldGrid::IsoPosition(dirCol * range, dirRow * range, tipX, tipY);
	SparkActor::Burst(*ctx.graph, player.WorldX() + tipX, player.WorldY() + tipY, SparkActor::Style::Dust,
		kCutDust, kCutDustSpeed, kCutDustLift);

	// The last cut leaves its trail hanging, to burst again in a moment.
	if (last)
	{
		m_EchoDelay = Tuning::AshForm::kEchoDelay;
		m_EchoX = player.WorldX();
		m_EchoY = player.WorldY();
		m_EchoDirCol = dirCol;
		m_EchoDirRow = dirRow;
	}

	PlayerActor::Action action = PlayerActor::Action::Slash;
	if (step == 1)
	{
		action = PlayerActor::Action::Sweep;
	}
	else if (step == 2)
	{
		action = PlayerActor::Action::Spin;
	}
	else if (last)
	{
		action = PlayerActor::Action::Finisher;
	}
	player.Act(action, ctx.input.aimX, ctx.input.aimY);
	player.Lunge(dirCol, dirRow, Tuning::AshForm::kCutStep[step], Tuning::AshForm::kCutStepTime, false);

	m_CutStep = step;
	m_CutTimer = kCutShowTime;
	m_CutDirCol = dirCol;
	m_CutDirRow = dirRow;
	m_Cooldown = last ? Tuning::AshForm::kLastCutRecovery : Tuning::AshForm::kCutInterval;
	m_ComboStep = last ? 0 : step + 1;
	m_ComboTimer = Tuning::AshForm::kComboWindow;
}

EnemyActor* AshFormMoves::Nearest(const ActorContext& ctx, const PlayerActor& player, float reachTiles,
	float dirCol, float dirRow, float spreadDegrees) const
{
	float minDot = cosf(spreadDegrees * 0.5f * kDegToRad);
	float playerCol, playerRow;
	WorldGrid::InverseIso(player.WorldX(), player.WorldY(), playerCol, playerRow);

	EnemyActor* nearest = nullptr;
	float best = reachTiles;
	ctx.graph->ForEach<EnemyActor>([&](EnemyActor& enemy)
	{
		if (enemy.IsDestroyed())
		{
			return;
		}
		float col, row;
		WorldGrid::InverseIso(enemy.WorldX(), enemy.WorldY(), col, row);
		float dc = col - playerCol;
		float dr = row - playerRow;
		float distance = sqrtf(dc * dc + dr * dr) - enemy.BodyRadius();
		if (distance > best)
		{
			return;
		}
		float span = sqrtf(dc * dc + dr * dr);
		if (spreadDegrees < 360.f && span > 0.001f && (dc * dirCol + dr * dirRow) / span < minDot)
		{
			return;
		}
		best = distance;
		nearest = &enemy;
	});
	return nearest;
}

void AshFormMoves::GetBehind(PlayerActor& player, const EnemyActor& enemy, float& dirCol, float& dirRow) const
{
	float playerCol, playerRow, col, row;
	WorldGrid::InverseIso(player.WorldX(), player.WorldY(), playerCol, playerRow);
	WorldGrid::InverseIso(enemy.WorldX(), enemy.WorldY(), col, row);
	float dc = col - playerCol;
	float dr = row - playerRow;
	float span = sqrtf(dc * dc + dr * dr);
	if (span > 0.001f)
	{
		dirCol = dc / span;
		dirRow = dr / span;
	}
	player.Flash(dirCol, dirRow, span + enemy.BodyRadius() + Tuning::AshForm::kDashBehind);
}

bool AshFormMoves::DashCut(const ActorContext& ctx, PlayerActor& player)
{
	if (m_DashCooldown > 0.f)
	{
		return false;
	}

	float dirCol, dirRow;
	if (!WorldGrid::ScreenToGroundDirection(ctx.input.aimX, ctx.input.aimY, dirCol, dirRow))
	{
		return false;
	}

	// An image of him stays where he set off, and bursts a moment later.
	float fromX = player.WorldX();
	float fromY = player.WorldY();
	std::unique_ptr<AfterimageActor> image(new AfterimageActor(Alignment::Ashen, (ctx.input.aimX >= 0.f) ? 1.f : -1.f));
	image->SetLocalPosition(fromX, fromY);
	ctx.graph->Spawn(std::move(image));
	m_BlastDelay = Tuning::AshForm::kDashBlastDelay;
	m_BlastX = fromX;
	m_BlastY = fromY;

	EnemyActor* target = Nearest(ctx, player, Tuning::AshForm::kDashSearch, dirCol, dirRow,
		Tuning::AshForm::kDashSearchDegrees);
	if (target != nullptr)
	{
		float targetX = target->WorldX();
		float targetY = target->WorldY();
		GetBehind(player, *target, dirCol, dirRow);
		if (Combat::HitRadius(*ctx.graph, targetX, targetY, Tuning::AshForm::kDashCutRadius,
			Combat::DashCutDamage(), 0.f) > 0.f)
		{
			ctx.impact->HitStop(Tuning::Feel::kHeavyStop);
			ctx.impact->Shake(Tuning::Feel::kHeavyShake);
		}
		SparkActor::Burst(*ctx.graph, targetX, targetY, SparkActor::Style::Dust, kDashDust, kDashDustSpeed, kDashDustLift);
	}
	else
	{
		player.Flash(dirCol, dirRow, Tuning::AshForm::kDashDistance);
	}

	// A streak of the gray along the way he went.
	float travelCol, travelRow, fromCol, fromRow;
	WorldGrid::InverseIso(player.WorldX(), player.WorldY(), travelCol, travelRow);
	WorldGrid::InverseIso(fromX, fromY, fromCol, fromRow);
	float travelled = sqrtf((travelCol - fromCol) * (travelCol - fromCol) + (travelRow - fromRow) * (travelRow - fromRow));
	if (travelled > 0.1f)
	{
		std::unique_ptr<BeamActor> streak(new BeamActor(Alignment::Ashen, dirCol, dirRow, travelled));
		streak->SetLocalPosition(fromX, fromY);
		ctx.graph->Spawn(std::move(streak));
	}

	// Turned back on what he cut, the blade coming round.
	float facingX, facingY;
	WorldGrid::IsoPosition(dirCol, dirRow, facingX, facingY);
	player.Act(PlayerActor::Action::Sweep, facingX, facingY);
	ctx.status->GrantInvuln(Tuning::AshForm::kDashInvuln);
	m_DashCooldown = Tuning::AshForm::kDashCooldown;
	m_ComboStep = 0;
	m_ComboTimer = 0.f;
	return true;
}

void AshFormMoves::Parry(const ActorContext& ctx, PlayerActor& player)
{
	if (!ctx.status->ashParry.Ready())
	{
		return;
	}
	ctx.status->parryTimer = Tuning::AshForm::kParryWindow;
	ctx.status->ashParry.Start(Tuning::AshForm::kParryCooldown);
	m_StanceTime = Tuning::AshForm::kParryWindow;
	player.Act(PlayerActor::Action::Guard, ctx.input.aimX, ctx.input.aimY);
}

void AshFormMoves::Counter(const ActorContext& ctx, PlayerActor& player)
{
	m_StanceTime = 0.f;
	ctx.status->GrantInvuln(Tuning::AshForm::kCounterInvuln);
	ctx.impact->HitStop(Tuning::Feel::kHeavyStop);
	ctx.impact->Shake(Tuning::Feel::kHeavyShake);

	// Behind whoever is nearest, and through them.
	EnemyActor* target = Nearest(ctx, player, Tuning::AshForm::kParryReach, 1.f, 0.f, 360.f);
	if (target != nullptr)
	{
		float targetX = target->WorldX();
		float targetY = target->WorldY();
		std::unique_ptr<AfterimageActor> image(new AfterimageActor(Alignment::Ashen, 1.f));
		image->SetLocalPosition(player.WorldX(), player.WorldY());
		ctx.graph->Spawn(std::move(image));

		float dirCol = 1.f;
		float dirRow = 0.f;
		GetBehind(player, *target, dirCol, dirRow);
		float radius = Tuning::AshForm::kCounterRadius;
		Combat::HitRadius(*ctx.graph, targetX, targetY, radius, Combat::CounterDamage(true), 0.f);
		std::unique_ptr<BurstActor> burst(new BurstActor(Alignment::Ashen, radius, true));
		burst->SetLocalPosition(targetX, targetY);
		ctx.graph->Spawn(std::move(burst));
		SparkActor::Burst(*ctx.graph, targetX, targetY, SparkActor::Style::Dust, kCounterDust, kCounterDustSpeed, kDashDustLift);

		float facingX, facingY;
		WorldGrid::IsoPosition(-dirCol, -dirRow, facingX, facingY);
		player.Act(PlayerActor::Action::Spin, facingX, facingY);
		return;
	}

	// No one in reach: he lashes out all round.
	float radius = Tuning::AshForm::kCounterRingRadius;
	Combat::HitRadius(*ctx.graph, player.WorldX(), player.WorldY(), radius, Combat::CounterDamage(false), 0.f);
	std::unique_ptr<BurstActor> burst(new BurstActor(Alignment::Ashen, radius, false));
	burst->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(burst));
	player.Act(PlayerActor::Action::Spin, 0.f, 0.f);
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
	// It runs straight through, so whatever ward it passes comes undone.
	float landed = Combat::HitLine(*ctx.graph, player.WorldX(), player.WorldY(), dirCol, dirRow,
		Tuning::AshForm::kUnravelLength, Tuning::AshForm::kUnravelWidth, Combat::UnravelDamage(), false);
	if (landed > 0.f)
	{
		ctx.impact->Shake(Tuning::Feel::kLightShake);
	}

	// The scythe thrown out long along it.
	player.Act(PlayerActor::Action::Thrust, ctx.input.aimX, ctx.input.aimY);
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
	if (Combat::HitRadius(*ctx.graph, player.WorldX(), player.WorldY(), radius, Combat::EraseDamage(), 0.f) > 0.f)
	{
		ctx.impact->HitStop(Tuning::Feel::kHeavyStop);
	}
	ctx.status->GrantInvuln(Tuning::AshForm::kEraseInvuln);
	// The scythe swung all the way round him.
	player.Act(PlayerActor::Action::Spin, 0.f, 0.f);
	ctx.impact->Shake(kEraseShake);

	// A second ring closing in as the first spreads, so it reads heavier
	// than a cut.
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
}

void AshFormMoves::Interrupt()
{
	m_ComboStep = 0;
	m_ComboTimer = 0.f;
}

void AshFormMoves::Draw(DrawList& out, const PlayerActor& player, float screenX, float groundY) const
{
	// The cut just made, sweeping out and fading.
	if (m_CutTimer > 0.f)
	{
		float progress = 1.f - m_CutTimer / kCutShowTime;
		float sweep = progress / kCutSweepShare;
		if (sweep > 1.f)
		{
			sweep = 1.f;
		}
		float fade = (progress < kCutSweepShare) ? 1.f : 1.f - (progress - kCutSweepShare) / (1.f - kCutSweepShare);
		DrawBand(out, player.WorldX(), player.WorldY(), screenX, groundY + kCutLift, m_CutDirCol, m_CutDirRow,
			Tuning::AshForm::kCutRange[m_CutStep], Tuning::AshForm::kCutArcDegrees[m_CutStep], CutSide(m_CutStep),
			sweep, fade);
	}

	// The last cut's trail where it was cut: hanging faintly, then bursting.
	if (m_EchoDelay > 0.f || m_EchoTimer > 0.f)
	{
		int last = Tuning::AshForm::kComboLength - 1;
		float echoX = screenX + (m_EchoX - player.WorldX());
		float echoY = groundY + kCutLift + (m_EchoY - player.WorldY());
		float alpha = (m_EchoDelay > 0.f) ? kEchoWaitAlpha : m_EchoTimer / kEchoShowTime;
		DrawBand(out, m_EchoX, m_EchoY, echoX, echoY, m_EchoDirCol, m_EchoDirRow,
			Tuning::AshForm::kCutRange[last], Tuning::AshForm::kCutArcDegrees[last], CutSide(last), 1.f, alpha);
	}

	// The parry's stance: a ring close round him, tightening as it runs out.
	if (m_StanceTime > 0.f)
	{
		float r, g, b;
		AlignmentColor::Core(Alignment::Ashen, r, g, b);
		float left = m_StanceTime / Tuning::AshForm::kParryWindow;
		float inner = kStanceInner * (0.6f + 0.4f * left);
		out.SetGlow(kCutShine);
		out.AddGroundArc(player.WorldX(), player.WorldY(), screenX, groundY, inner, inner + kStanceWidth,
			0.f, 2.f * kPi, kRingSegments, r, g, b, kStanceAlpha);
		out.SetGlow(0.f);
	}
}
