#include "stdafx.h"
#include "DarkFormMoves.h"
#include "PlayerActor.h"
#include "PlayerStatus.h"
#include "BurstActor.h"
#include "BeamActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kDegToRad = 3.14159265f / 180.f;

	// The slash trail: a fan of quads on the ground around him, swept out over
	// the first part of its life and faded over the rest.
	const float kSlashShowTime = 0.16f;
	const float kSlashSweepShare = 0.45f;
	const int kSlashSegments = 7;
	const float kSlashInner = 0.4f;   // tiles
	const float kSlashBlade = 0.6f;   // where the bright outer band starts, 0..1 across the trail
	const float kSlashLift = 12.f;    // pixels above the ground, about waist height

	// The spin: two blades sweeping round him inside a faint circle of reach.
	const float kSpinTurnRate = 14.f;      // radians per second
	const float kSpinBladeAngle = 1.6f;    // radians each blade covers
	const int kSpinBladeSegments = 6;
	const int kSpinRingSegments = 24;
	const float kSpinFadeTime = 0.12f;

	float Clamp(float value, float low, float high)
	{
		if (value < low)
		{
			return low;
		}
		if (value > high)
		{
			return high;
		}
		return value;
	}
}

void DarkFormMoves::Update(const ActorContext& ctx, PlayerActor& player)
{
	if (m_Cooldown > 0.f)
	{
		m_Cooldown -= ctx.deltaTime;
	}
	if (m_ComboTimer > 0.f)
	{
		m_ComboTimer -= ctx.deltaTime;
	}
	if (m_SlashTimer > 0.f)
	{
		m_SlashTimer -= ctx.deltaTime;
	}

	if (m_SpinTimer <= 0.f)
	{
		return;
	}

	// The first cut lands the moment the spin starts, then one every interval
	// for as long as it lasts.
	m_SpinTickTimer -= ctx.deltaTime;
	if (m_SpinTickTimer <= 0.f)
	{
		float landed = Combat::HitRadius(*ctx.graph, player.WorldX(), player.WorldY(),
			Tuning::DarkForm::kSpinRadius, Combat::SpinDamage(*ctx.status), 0.f);
		ctx.status->Heal(Combat::Lifesteal(landed, Tuning::DarkForm::kSpinLifestealScale));
		m_SpinTickTimer += Tuning::DarkForm::kSpinInterval;
	}

	m_SpinTimer -= ctx.deltaTime;
}

bool DarkFormMoves::IsSpinning() const
{
	return m_SpinTimer > 0.f;
}

void DarkFormMoves::Attack(const ActorContext& ctx, PlayerActor& player)
{
	if (m_Cooldown > 0.f || IsSpinning())
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

	float landed = Combat::HitArc(*ctx.graph, player.WorldX(), player.WorldY(), dirCol, dirRow,
		Tuning::DarkForm::kSlashRange, Tuning::DarkForm::kSlashArcDegrees,
		Combat::SlashDamage(*ctx.status, m_ComboStep));
	ctx.status->Heal(Combat::Lifesteal(landed));

	m_SlashStep = m_ComboStep;
	m_SlashTimer = kSlashShowTime;
	m_SlashDirCol = dirCol;
	m_SlashDirRow = dirRow;

	bool finisher = (m_ComboStep == Tuning::DarkForm::kComboLength - 1);
	m_Cooldown = finisher ? Tuning::DarkForm::kFinisherRecovery : Tuning::DarkForm::kSlashInterval;
	m_ComboStep = finisher ? 0 : m_ComboStep + 1;
	m_ComboTimer = Tuning::DarkForm::kComboWindow;
}

void DarkFormMoves::Skill(const ActorContext& ctx, PlayerActor& player)
{
	if (!ctx.status->darkSkill.Ready() || IsSpinning())
	{
		return;
	}

	float dirCol, dirRow;
	if (!WorldGrid::ScreenToGroundDirection(ctx.input.aimX, ctx.input.aimY, dirCol, dirRow))
	{
		return;
	}

	// Everything along the path is cut the moment he sets off; the dash itself
	// then carries him through where they stand. A blow in reach, so it feeds
	// lifesteal like any other cut.
	float landed = Combat::HitLine(*ctx.graph, player.WorldX(), player.WorldY(), dirCol, dirRow,
		Tuning::DarkForm::kDashDistance, Tuning::DarkForm::kDashWidth, Combat::DashDamage(*ctx.status), false);
	ctx.status->Heal(Combat::Lifesteal(landed));

	std::unique_ptr<BeamActor> streak(new BeamActor(Alignment::Dark, dirCol, dirRow, Tuning::DarkForm::kDashDistance));
	streak->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(streak));

	player.Lunge(dirCol, dirRow, Tuning::DarkForm::kDashDistance, Tuning::DarkForm::kDashDuration);
	ctx.status->GrantInvuln(Tuning::DarkForm::kDashDuration);
	ctx.status->darkSkill.Start(Tuning::DarkForm::kDashCooldown);
}

void DarkFormMoves::Special(const ActorContext& ctx, PlayerActor& player)
{
	if (!ctx.status->darkSpecial.Ready() || IsSpinning())
	{
		return;
	}

	m_SpinTimer = Tuning::DarkForm::kSpinDuration;
	m_SpinTickTimer = 0.f;
	m_ComboStep = 0;
	m_ComboTimer = 0.f;
	ctx.status->darkSpecial.Start(Tuning::DarkForm::kSpinCooldown);
}

void DarkFormMoves::Enter(const ActorContext& ctx, PlayerActor& player)
{
	// Not a blow in reach, so it feeds no lifesteal.
	Combat::HitRadius(*ctx.graph, player.WorldX(), player.WorldY(),
		Tuning::SwapSkill::kDarkBurstRadius, Combat::DarkBurstDamage(*ctx.status), -Tuning::SwapSkill::kDarkBurstPull);

	std::unique_ptr<BurstActor> burst(new BurstActor(Alignment::Dark, Tuning::SwapSkill::kDarkBurstRadius, true));
	burst->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(burst));
}

void DarkFormMoves::Interrupt()
{
	m_ComboStep = 0;
	m_ComboTimer = 0.f;
	m_SpinTimer = 0.f;
}

void DarkFormMoves::Draw(DrawList& out, const PlayerActor& player, float screenX, float groundY) const
{
	if (IsSpinning())
	{
		DrawSpin(out, player, screenX, groundY);
	}

	if (m_SlashTimer <= 0.f)
	{
		return;
	}

	bool finisher = (m_SlashStep == Tuning::DarkForm::kComboLength - 1);

	float progress = 1.f - m_SlashTimer / kSlashShowTime;
	float sweep = Clamp(progress / kSlashSweepShare, 0.f, 1.f);
	float fade = 1.f;
	if (progress > kSlashSweepShare)
	{
		fade = 1.f - (progress - kSlashSweepShare) / (1.f - kSlashSweepShare);
	}

	// The finisher lands all at once and brighter, so the third cut reads as
	// the heavy one.
	if (finisher)
	{
		sweep = 1.f;
	}
	int visible = (int)ceilf(sweep * kSlashSegments);
	if (visible <= 0)
	{
		return;
	}

	float r, g, b;
	AlignmentColor::Core(Alignment::Dark, r, g, b);
	if (finisher)
	{
		r += (1.f - r) * 0.35f;
		g += (1.f - g) * 0.35f;
		b += (1.f - b) * 0.35f;
	}
	float alpha = (finisher ? 0.9f : 0.7f) * fade;

	float center = atan2f(m_SlashDirRow, m_SlashDirCol);
	float half = Tuning::DarkForm::kSlashArcDegrees * 0.5f * kDegToRad;
	float segmentAngle = half * 2.f / kSlashSegments;

	// The middle cut sweeps back the other way, so the combo reads as swings
	// to and fro rather than the same stroke three times.
	float from = center - half;
	float to = from + visible * segmentAngle;
	if (m_SlashStep == 1)
	{
		to = center + half;
		from = to - visible * segmentAngle;
	}

	float inner = kSlashInner;
	float outer = Tuning::DarkForm::kSlashRange;
	float blade = inner + (outer - inner) * kSlashBlade;
	float baseY = groundY + kSlashLift;

	// Dim trail on the inside, the bright edge of the blade on the outside.
	out.AddGroundArc(player.WorldX(), player.WorldY(), screenX, baseY, inner, blade, from, to, visible,
		r, g, b, alpha * 0.45f);
	out.AddGroundArc(player.WorldX(), player.WorldY(), screenX, baseY, blade, outer, from, to, visible,
		r, g, b, alpha);
}

void DarkFormMoves::DrawSpin(DrawList& out, const PlayerActor& player, float screenX, float groundY) const
{
	float age = Tuning::DarkForm::kSpinDuration - m_SpinTimer;
	float turn = age * kSpinTurnRate;

	// Eases in and out at the ends of the spin rather than popping.
	float fade = 1.f;
	if (age < kSpinFadeTime)
	{
		fade = age / kSpinFadeTime;
	}
	else if (m_SpinTimer < kSpinFadeTime)
	{
		fade = m_SpinTimer / kSpinFadeTime;
	}

	float r, g, b;
	AlignmentColor::Core(Alignment::Dark, r, g, b);

	float baseY = groundY + kSlashLift;
	float inner = kSlashInner;
	float outer = Tuning::DarkForm::kSpinRadius;

	// A faint full circle marks the reach; two blades sweep round inside it.
	out.AddGroundArc(player.WorldX(), player.WorldY(), screenX, baseY, outer - 0.1f, outer,
		0.f, 6.2831853f, kSpinRingSegments, r, g, b, 0.3f * fade);
	for (int blade = 0; blade < 2; blade++)
	{
		float from = turn + blade * 3.14159265f;
		out.AddGroundArc(player.WorldX(), player.WorldY(), screenX, baseY, inner, outer,
			from, from + kSpinBladeAngle, kSpinBladeSegments, r, g, b, 0.6f * fade);
	}
}
