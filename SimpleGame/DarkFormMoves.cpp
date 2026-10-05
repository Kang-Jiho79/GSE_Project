#include "stdafx.h"
#include "DarkFormMoves.h"
#include "PlayerActor.h"
#include "PlayerStatus.h"
#include "BurstActor.h"
#include "BeamActor.h"
#include "BladeFieldActor.h"
#include "BladeActor.h"
#include "Random.h"
#include "SparkActor.h"
#include "Impact.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Tuning.h"
#include "Growth.h"
#include <cmath>

namespace
{
	const float kDegToRad = 3.14159265f / 180.f;

	// The slash trail: a fan of quads on the ground around him, swept out over
	// the first part of its life and faded over the rest.
	const float kSlashShowTime = 0.16f;
	const float kSlashSweepShare = 0.45f;
	const int kSlashSegments = 7;
	const float kSlashLift = 12.f;    // pixels above the ground, about waist height

	// The spray as the form turns over to dark.
	const int kSwapSparks = 14;
	const float kSwapSparkSpeed = 220.f;
	const float kSwapSparkLift = 16.f;

	// Grit thrown off where the point of each cut passes.
	const int kCutSparks = 5;
	const float kCutSparkSpeed = 150.f;
	const float kCutSparkLift = 12.f;

	// The sweep throws grit all along its arc.
	const int kSweepSparkPoints = 3;
	const int kSweepSparks = 4;

	// The last cut's shockwave where it comes down, the grit it throws up,
	// and the crater it leaves for a moment.
	const int kShockSparks = 14;
	const float kShockSparkSpeed = 220.f;
	const float kShockSparkLift = 16.f;
	const float kSmashShowTime = 0.35f;
	const int kSmashCracks = 7;
	const float kSmashBladeRing = 0.7f; // tiles out from where it lands

	// How each cut is drawn: where its trail starts inside, and how much of
	// it is the dark smear before the band of the blade.
	const float kCutInner[4] = { 0.75f, 0.35f, 0.6f, 0.f };
	const float kCutBlade[4] = { 0.7f, 0.45f, 0.6f, 0.f };
	// The edge of every cut: a thin line far brighter than the trail.
	const float kSlashEdgeWidth = 0.07f;

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

	if (m_PlantTimer > 0.f)
	{
		m_PlantTimer -= ctx.deltaTime;
	}
}

bool DarkFormMoves::IsPlanting() const
{
	return m_PlantTimer > 0.f;
}

void DarkFormMoves::Attack(const ActorContext& ctx, PlayerActor& player)
{
	if (m_Cooldown > 0.f || IsPlanting())
	{
		return;
	}

	float dirCol, dirRow;
	if (!WorldGrid::ScreenToGroundDirection(ctx.input.aimX, ctx.input.aimY, dirCol, dirRow))
	{
		return;
	}

	// Three cuts until the basic attack grows, then four. With three, the
	// sweep goes straight into the last.
	int length = Tuning::DarkForm::kComboLength;
	if (!Growth::Has(*ctx.status, Tuning::Growth::kAttackII))
	{
		length--;
	}
	if (m_ComboTimer <= 0.f || m_ComboStep >= length)
	{
		m_ComboStep = 0;
	}

	int step = m_ComboStep;
	if (m_ComboStep == length - 1)
	{
		step = Tuning::DarkForm::kComboLength - 1;
	}
	bool finisher = (step == Tuning::DarkForm::kComboLength - 1);
	float damage = Combat::SlashDamage(*ctx.status, step);
	float landed = 0.f;

	if (finisher)
	{
		// Brought down on a spot in front of him: everything round it is
		// struck, the ground cracks, and blades of darkness spring up about it.
		float offsetX, offsetY;
		WorldGrid::IsoPosition(dirCol * Tuning::DarkForm::kSmashReach, dirRow * Tuning::DarkForm::kSmashReach,
			offsetX, offsetY);
		m_SmashX = player.WorldX() + offsetX;
		m_SmashY = player.WorldY() + offsetY;
		bool grown = Growth::Has(*ctx.status, Tuning::Growth::kAttackI);
		m_SmashRadius = grown ? Tuning::DarkForm::kSmashRadius : Tuning::DarkForm::kSmashRadiusPlain;
		landed = Combat::HitRadius(*ctx.graph, m_SmashX, m_SmashY, m_SmashRadius, damage, 0.f);

		std::unique_ptr<BurstActor> shock(new BurstActor(Alignment::Dark, m_SmashRadius + 0.4f, false));
		shock->SetLocalPosition(m_SmashX, m_SmashY);
		ctx.graph->Spawn(std::move(shock));
		SparkActor::Burst(*ctx.graph, m_SmashX, m_SmashY, SparkActor::Style::Dark,
			kShockSparks, kShockSparkSpeed, kShockSparkLift);

		float smashCol, smashRow;
		WorldGrid::InverseIso(m_SmashX, m_SmashY, smashCol, smashRow);
		for (int i = 0; grown && i < Tuning::DarkForm::kSmashBlades; i++)
		{
			float angle = Random::Angle();
			float bladeX, bladeY;
			WorldGrid::IsoPosition(smashCol + cosf(angle) * kSmashBladeRing, smashRow + sinf(angle) * kSmashBladeRing,
				bladeX, bladeY);
			std::unique_ptr<BladeActor> blade(new BladeActor(i * 0.03f, (Random::Unit() - 0.5f) * 10.f, 0.75f));
			blade->SetLocalPosition(bladeX, bladeY);
			ctx.graph->Spawn(std::move(blade));
		}

		ctx.impact->Shake(Tuning::Feel::kHeavyShake);
		if (landed > 0.f)
		{
			ctx.impact->HitStop(Tuning::Feel::kHeavyStop);
		}
	}
	else
	{
		float range = Tuning::DarkForm::kSlashRange[step];
		float arc = Tuning::DarkForm::kSlashArcDegrees[step];
		landed = Combat::HitArc(*ctx.graph, player.WorldX(), player.WorldY(), dirCol, dirRow, range, arc, damage);

		// Grit thrown off where the point passes: at the tip of the quick
		// cut, and all along the arc of the sweep.
		int points = (step == 0) ? 1 : kSweepSparkPoints;
		float center = atan2f(dirRow, dirCol);
		float half = arc * 0.5f * kDegToRad;
		for (int i = 0; i < points; i++)
		{
			float angle = (points == 1) ? center : center - half + 2.f * half * (i + 0.5f) / points;
			float tipX, tipY;
			WorldGrid::IsoPosition(cosf(angle) * range, sinf(angle) * range, tipX, tipY);
			SparkActor::Burst(*ctx.graph, player.WorldX() + tipX, player.WorldY() + tipY, SparkActor::Style::Dark,
				(step == 0) ? kCutSparks : kSweepSparks, kCutSparkSpeed, kCutSparkLift);
		}
		if (landed > 0.f && step == 1)
		{
			ctx.impact->Shake(Tuning::Feel::kLightShake);
		}
	}
	ctx.status->Heal(Combat::Lifesteal(*ctx.status, landed));

	m_SlashStep = m_ComboStep;
	m_SlashTimer = finisher ? kSmashShowTime : kSlashShowTime;
	m_SlashDirCol = dirCol;
	m_SlashDirRow = dirRow;

	// He throws himself into it: a quick cut, a sweep back the other way
	// turning his whole body, and a leap into the last.
	PlayerActor::Action action = PlayerActor::Action::Slash;
	if (step == 1)
	{
		action = PlayerActor::Action::Sweep;
	}
	else if (finisher)
	{
		action = PlayerActor::Action::Finisher;
	}
	player.Act(action, ctx.input.aimX, ctx.input.aimY);
	player.Lunge(dirCol, dirRow, Tuning::DarkForm::kSlashStep[step], Tuning::DarkForm::kSlashStepTime, false);
	m_Cooldown = finisher ? Tuning::DarkForm::kFinisherRecovery : Tuning::DarkForm::kSlashInterval;
	m_ComboStep = finisher ? 0 : m_ComboStep + 1;
	m_ComboTimer = Tuning::DarkForm::kComboWindow;
}

void DarkFormMoves::Skill(const ActorContext& ctx, PlayerActor& player)
{
	if (!Growth::Has(*ctx.status, Tuning::Growth::kSkills) || !ctx.status->darkSkill.Ready() || IsPlanting())
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
	float distance = Tuning::DarkForm::kDashDistance;
	if (Growth::Has(*ctx.status, Tuning::Growth::kSkillsII))
	{
		distance *= Tuning::Growth::kDashScale;
	}
	float landed = Combat::HitLine(*ctx.graph, player.WorldX(), player.WorldY(), dirCol, dirRow,
		distance, Tuning::DarkForm::kDashWidth, Combat::DashDamage(*ctx.status), false);
	ctx.status->Heal(Combat::Lifesteal(*ctx.status, landed));
	if (landed > 0.f)
	{
		ctx.impact->HitStop(Tuning::Feel::kHeavyStop);
		ctx.impact->Shake(Tuning::Feel::kHeavyShake);
	}

	std::unique_ptr<BeamActor> streak(new BeamActor(Alignment::Dark, dirCol, dirRow, distance));
	streak->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(streak));

	player.Act(PlayerActor::Action::Thrust, ctx.input.aimX, ctx.input.aimY);
	player.Lunge(dirCol, dirRow, distance, Tuning::DarkForm::kDashDuration);
	ctx.status->GrantInvuln(Tuning::DarkForm::kDashDuration);
	ctx.status->darkSkill.Start(Tuning::DarkForm::kDashCooldown);
}

void DarkFormMoves::Special(const ActorContext& ctx, PlayerActor& player)
{
	if (!Growth::Has(*ctx.status, Tuning::Growth::kSkills) || !ctx.status->darkSpecial.Ready() || IsPlanting())
	{
		return;
	}

	// Grown stronger, it reaches further and the blades come up twice.
	bool grown = Growth::Has(*ctx.status, Tuning::Growth::kSkillsII);
	std::unique_ptr<BladeFieldActor> field(new BladeFieldActor(
		grown ? Tuning::Growth::kEruptRadiusII : Tuning::DarkForm::kEruptRadius,
		Combat::EruptDamage(*ctx.status), grown ? Tuning::Growth::kEruptWavesII : 1));
	field->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(field));

	m_PlantTimer = Tuning::DarkForm::kPlantTime;
	m_ComboStep = 0;
	m_ComboTimer = 0.f;
	ctx.status->darkSpecial.Start(Tuning::DarkForm::kEruptCooldown);
}

void DarkFormMoves::Enter(const ActorContext& ctx, PlayerActor& player)
{
	// Not a blow in reach, so it feeds no lifesteal.
	float scale = Growth::Has(*ctx.status, Tuning::Growth::kSwapII) ? Tuning::Growth::kSwapScale : 1.f;
	float radius = Tuning::SwapSkill::kDarkBurstRadius * scale;
	float landed = Combat::HitRadius(*ctx.graph, player.WorldX(), player.WorldY(),
		radius, Combat::DarkBurstDamage(*ctx.status), -Tuning::SwapSkill::kDarkBurstPull * scale);
	if (landed > 0.f)
	{
		ctx.impact->Shake(Tuning::Feel::kHeavyShake);
	}
	SparkActor::Burst(*ctx.graph, player.WorldX(), player.WorldY(), SparkActor::Style::Dark,
		kSwapSparks, kSwapSparkSpeed, kSwapSparkLift);
	player.Act(PlayerActor::Action::Swap, 0.f, 0.f);

	std::unique_ptr<BurstActor> burst(new BurstActor(Alignment::Dark, radius, true));
	burst->SetLocalPosition(player.WorldX(), player.WorldY());
	ctx.graph->Spawn(std::move(burst));
}

void DarkFormMoves::Interrupt()
{
	m_ComboStep = 0;
	m_ComboTimer = 0.f;
	m_PlantTimer = 0.f;
}

void DarkFormMoves::Draw(DrawList& out, const PlayerActor& player, float screenX, float groundY) const
{
	if (m_SlashTimer <= 0.f)
	{
		return;
	}
	if (m_SlashStep == Tuning::DarkForm::kComboLength - 1)
	{
		DrawSmash(out, player, screenX, groundY);
		return;
	}
	DrawCut(out, player, screenX, groundY);
}

void DarkFormMoves::DrawCut(DrawList& out, const PlayerActor& player, float screenX, float groundY) const
{
	int step = m_SlashStep;
	bool sweeping = (step == 1);

	float progress = 1.f - m_SlashTimer / kSlashShowTime;
	float sweep = Clamp(progress / kSlashSweepShare, 0.f, 1.f);
	float fade = 1.f;
	if (progress > kSlashSweepShare)
	{
		fade = 1.f - (progress - kSlashSweepShare) / (1.f - kSlashSweepShare);
	}
	int visible = (int)ceilf(sweep * kSlashSegments);
	if (visible <= 0)
	{
		return;
	}

	float r, g, b;
	AlignmentColor::Core(Alignment::Dark, r, g, b);
	float alpha = (sweeping ? 0.85f : 0.7f) * fade;

	float center = atan2f(m_SlashDirRow, m_SlashDirCol);
	float half = Tuning::DarkForm::kSlashArcDegrees[step] * 0.5f * kDegToRad;
	float segmentAngle = half * 2.f / kSlashSegments;

	// The sweep comes back the other way, so the two read as to and fro.
	float from = center - half;
	float to = from + visible * segmentAngle;
	if (sweeping)
	{
		to = center + half;
		from = to - visible * segmentAngle;
	}

	// The quick cut is a thin bright crescent far out; the sweep a heavy
	// band from close in, with a faint echo of it rolling on past its reach.
	float outer = Tuning::DarkForm::kSlashRange[step];
	float inner = kCutInner[step];
	float blade = inner + (outer - inner) * kCutBlade[step];
	float baseY = groundY + kSlashLift;

	out.AddGroundArc(player.WorldX(), player.WorldY(), screenX, baseY, inner, blade, from, to, visible,
		r * 0.35f, g * 0.35f, b * 0.35f, alpha * 0.6f);
	out.AddGroundArc(player.WorldX(), player.WorldY(), screenX, baseY, blade, outer - kSlashEdgeWidth, from, to, visible,
		r, g, b, alpha);
	float edgeR = r + (1.f - r) * 0.6f;
	float edgeG = g + (1.f - g) * 0.6f;
	float edgeB = b + (1.f - b) * 0.6f;
	out.AddGroundArc(player.WorldX(), player.WorldY(), screenX, baseY, outer - kSlashEdgeWidth, outer, from, to, visible,
		edgeR, edgeG, edgeB, alpha);

	if (sweeping)
	{
		float echo = outer + 0.25f + progress * 0.4f;
		out.AddGroundArc(player.WorldX(), player.WorldY(), screenX, baseY, echo - 0.12f, echo, from, to, visible,
			r, g, b, alpha * 0.35f);
	}
}

void DarkFormMoves::DrawSmash(DrawList& out, const PlayerActor& player, float screenX, float groundY) const
{
	// Where the blade came down, placed relative to his feet on screen: a
	// dark crater spreading and fading, cracks running out of it.
	float progress = 1.f - m_SlashTimer / kSmashShowTime;
	float fade = 1.f - progress;
	float x = screenX + (m_SmashX - player.WorldX());
	float y = groundY + (m_SmashY - player.WorldY()) + 1.f;

	float r, g, b;
	AlignmentColor::Core(Alignment::Dark, r, g, b);
	float radius = m_SmashRadius * (0.5f + 0.5f * Clamp(progress * 4.f, 0.f, 1.f));
	const float kFull = 6.2831853f;
	out.AddGroundArc(m_SmashX, m_SmashY, x, y, 0.f, radius, 0.f, kFull, 24, r * 0.3f, g * 0.3f, b * 0.3f, 0.7f * fade);
	out.AddGroundArc(m_SmashX, m_SmashY, x, y, radius - 0.1f, radius, 0.f, kFull, 24, r, g, b, 0.9f * fade);
	for (int i = 0; i < kSmashCracks; i++)
	{
		float angle = i * kFull / kSmashCracks + 0.4f;
		out.AddGroundArc(m_SmashX, m_SmashY, x, y, 0.1f, radius + 0.35f, angle - 0.03f, angle + 0.03f, 1,
			r * 0.7f, g * 0.7f, b * 0.7f, 0.8f * fade);
	}
}
