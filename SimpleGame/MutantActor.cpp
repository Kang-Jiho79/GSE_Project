#include "stdafx.h"
#include "MutantActor.h"
#include "MonsterActor.h"
#include "EnemyProjectileActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Impact.h"
#include "Palette.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kPi = 3.14159265f;
	const float kStrikeShowTime = 0.25f;

	const float kFlesh = 0.20f;
	const float kGuardSpread = 2.5f; // tiles to either side it calls them

	// The marked ground: a faint fill of the whole reach, a stronger one
	// growing out to it as the blow gathers, and a bright edge.
	const float kTelegraphLift = 2.f;
	const float kTelegraphBase = 0.14f;
	const float kTelegraphFill = 0.26f;
	const float kTelegraphEdge = 0.5f;
	const float kTelegraphEdgeWidth = 0.08f;
	const int kTelegraphSegments = 24;

	// The cracks across its bulk, where what it was still shows through.
	const int kCracks = 4;
	const float kCrackX[kCracks] = { -14.f, 6.f, 16.f, -4.f };
	const float kCrackY[kCracks] = { 6.f, 14.f, 4.f, 20.f };
	const float kCrackHeight[kCracks] = { 9.f, 7.f, 10.f, 6.f };
}

MutantActor::MutantActor(int region, Origin origin, float maxHealth)
	: BossActor("잿빛 변이체", maxHealth, region, Tuning::Mutant::kWakeRange, Tuning::Mutant::kLeashRange,
		Tuning::Mutant::kFuryTime)
	, m_Origin(origin)
{
}

float MutantActor::BodyRadius() const
{
	return Tuning::Mutant::kBodyRadius;
}

bool MutantActor::Raging() const
{
	return HealthFraction() < Tuning::Mutant::kRageShare;
}

float MutantActor::WindupTime(Blow blow) const
{
	float time = Tuning::Mutant::kSlamWindup;
	if (blow == Blow::Sweep)
	{
		time = Tuning::Mutant::kSweepWindup;
	}
	else if (blow == Blow::Burst)
	{
		time = Tuning::Mutant::kBurstWindup;
	}
	if (Raging())
	{
		time *= Tuning::Mutant::kRageWindupScale;
	}
	return time / FurySpeed();
}

Alignment MutantActor::ColorOf(int index) const
{
	if (m_Origin == Origin::Light)
	{
		return Alignment::Light;
	}
	if (m_Origin == Origin::Dark)
	{
		return Alignment::Dark;
	}
	return (index % 2 == 0) ? Alignment::Light : Alignment::Dark;
}

void MutantActor::Fight(const ActorContext& ctx)
{
	m_StateTime += ctx.deltaTime;
	if (m_Cooldown > 0.f)
	{
		m_Cooldown -= ctx.deltaTime;
	}
	if (m_StrikeFlash > 0.f)
	{
		m_StrikeFlash -= ctx.deltaTime / kStrikeShowTime;
	}

	float dirCol, dirRow;
	float distance = ToPlayer(ctx, dirCol, dirRow);

	switch (m_State)
	{
	case State::Stalk:
	{
		FaceAlong(dirCol, dirRow);

		if (!m_Summoned && Raging())
		{
			SummonGuards(ctx);
			m_Summoned = true;
		}

		if (m_Cooldown <= 0.f)
		{
			Blow blow = Blow::Slam;
			if (m_BlowCount % 3 == 2)
			{
				blow = Blow::Burst;
			}
			else if (distance <= Tuning::Mutant::kSweepRange)
			{
				blow = Blow::Sweep;
			}
			StartBlow(ctx, blow, dirCol, dirRow);
			break;
		}

		if (distance > Tuning::Mutant::kKeepRange)
		{
			float speed = Tuning::Mutant::kSpeed * (Raging() ? Tuning::Mutant::kRageSpeedScale : 1.f) * FurySpeed();
			float step = speed * ctx.deltaTime;
			if (step > distance - Tuning::Mutant::kKeepRange)
			{
				step = distance - Tuning::Mutant::kKeepRange;
			}
			StepAlong(dirCol, dirRow, step);
		}
		break;
	}
	case State::Windup:
	{
		if (m_StateTime >= WindupTime(m_Blow))
		{
			Release(ctx);
		}
		break;
	}
	case State::Recover:
	{
		if (m_StateTime >= Tuning::Mutant::kRecover / FurySpeed())
		{
			m_State = State::Stalk;
			m_StateTime = 0.f;
		}
		break;
	}
	}
}

void MutantActor::StartBlow(const ActorContext& ctx, Blow blow, float dirCol, float dirRow)
{
	m_Blow = blow;
	m_DirCol = dirCol;
	m_DirRow = dirRow;
	FaceAlong(dirCol, dirRow);
	m_State = State::Windup;
	m_StateTime = 0.f;

	// A slam is aimed where he stands as it begins - stay, and it lands.
	if (blow == Blow::Slam)
	{
		WorldGrid::InverseIso(ctx.playerX, ctx.playerY, m_TargetCol, m_TargetRow);
	}
}

void MutantActor::Release(const ActorContext& ctx)
{
	switch (m_Blow)
	{
	case Blow::Slam:
	{
		float x, y;
		WorldGrid::IsoPosition(m_TargetCol, m_TargetRow, x, y);
		Combat::StrikePlayerRadius(ctx, x, y, Tuning::Mutant::kSlamRadius, Tuning::Mutant::kSlamDamage * FuryDamage());
		ctx.impact->Shake(Tuning::Mutant::kSlamShake);
		break;
	}
	case Blow::Sweep:
	{
		Combat::StrikePlayerArc(ctx, WorldX(), WorldY(), m_DirCol, m_DirRow,
			Tuning::Mutant::kSweepRange, 180.f, Tuning::Mutant::kSweepDamage * FuryDamage());
		break;
	}
	case Blow::Burst:
	{
		// Each burst turned half a step from the last, so the same gap does
		// not keep him safe twice.
		int count = Raging() ? Tuning::Mutant::kRageBurstCount : Tuning::Mutant::kBurstCount;
		float offset = (m_BlowCount % 2 == 0) ? 0.f : 0.5f;
		for (int i = 0; i < count; i++)
		{
			float angle = (i + offset) * 2.f * kPi / count;
			std::unique_ptr<EnemyProjectileActor> shot(new EnemyProjectileActor(ColorOf(i), cosf(angle), sinf(angle),
				Tuning::Mutant::kBurstSpeed * FurySpeed(), 0.f, Tuning::Mutant::kBurstLifetime,
				Tuning::Mutant::kBurstDamage * FuryDamage()));
			shot->SetLocalPosition(WorldX(), WorldY());
			ctx.graph->Spawn(std::move(shot));
		}
		break;
	}
	}

	m_StrikeFlash = 1.f;
	m_Cooldown = Tuning::Mutant::kCooldown * (Raging() ? Tuning::Mutant::kRageCooldownScale : 1.f) / FurySpeed();
	m_BlowCount++;
	m_State = State::Recover;
	m_StateTime = 0.f;
}

void MutantActor::SummonGuards(const ActorContext& ctx)
{
	float col, row;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);

	for (int i = 0; i < Tuning::Mutant::kGuards; i++)
	{
		float side = (i % 2 == 0) ? 1.f : -1.f;
		float guardCol = col + side * kGuardSpread;
		float guardRow = row - side * kGuardSpread;
		WorldGrid::ClampToMap(guardCol, guardRow);
		if (WorldGrid::IsBlocked(guardCol, guardRow))
		{
			continue;
		}

		std::unique_ptr<MonsterActor> guard(new MonsterActor(MonsterActor::Kind::Knight, ColorOf(i)));
		float x, y;
		WorldGrid::IsoPosition(guardCol, guardRow, x, y);
		guard->SetLocalPosition(x, y);
		ctx.graph->Spawn(std::move(guard));
	}

	ctx.impact->Shake(Tuning::Boss::kWakeShake);
}

void MutantActor::OnReset(const ActorContext& ctx)
{
	m_State = State::Stalk;
	m_StateTime = 0.f;
	m_Cooldown = 1.f;
	m_Summoned = false;
}

void MutantActor::Reward(const ActorContext& ctx)
{
	if (m_Origin == Origin::Mixed)
	{
		ScatterMotes(ctx, Alignment::Light, Tuning::Mutant::kRewardMotes / 2);
		ScatterMotes(ctx, Alignment::Dark, Tuning::Mutant::kRewardMotes / 2);
		return;
	}
	ScatterMotes(ctx, ColorOf(0), Tuning::Mutant::kRewardMotes);
}

void MutantActor::DrawTelegraph(DrawList& out, float x, float groundY) const
{
	float progress = m_StateTime / WindupTime(m_Blow);
	if (progress > 1.f)
	{
		progress = 1.f;
	}

	float r, g, b;
	AlignmentColor::Warning(ColorOf(0), Palette::kWarningWash, r, g, b);

	if (m_Blow == Blow::Burst)
	{
		// Swelling: a ring pushing out from its body as it fills with gray.
		float radius = Tuning::Mutant::kBodyRadius + progress * 1.5f;
		out.AddGroundArc(WorldX(), WorldY(), x, groundY + kTelegraphLift, radius - 0.15f, radius,
			0.f, 2.f * kPi, kTelegraphSegments, r, g, b, 0.4f * progress);
		return;
	}

	float centerX = WorldX();
	float centerY = WorldY();
	float screenX = x;
	float screenY = groundY;
	float radius = Tuning::Mutant::kSweepRange;
	float from = 0.f;
	float to = 2.f * kPi;

	if (m_Blow == Blow::Slam)
	{
		// The ground it will strike, where he stood: placed relative to its
		// own feet, so it stays put on screen as he moves.
		float offsetX, offsetY, col, row;
		WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
		WorldGrid::IsoPosition(m_TargetCol - col, m_TargetRow - row, offsetX, offsetY);
		centerX = WorldX() + offsetX;
		centerY = WorldY() + offsetY;
		screenX = x + offsetX;
		screenY = groundY + offsetY;
		radius = Tuning::Mutant::kSlamRadius;
	}
	else
	{
		float center = atan2f(m_DirRow, m_DirCol);
		from = center - kPi * 0.5f;
		to = center + kPi * 0.5f;
	}

	screenY += kTelegraphLift;
	out.AddGroundArc(centerX, centerY, screenX, screenY, 0.f, radius, from, to, kTelegraphSegments, r, g, b, kTelegraphBase);
	if (progress > 0.f)
	{
		out.AddGroundArc(centerX, centerY, screenX, screenY, 0.f, radius * progress, from, to, kTelegraphSegments,
			r, g, b, kTelegraphFill);
	}
	out.AddGroundArc(centerX, centerY, screenX, screenY, radius - kTelegraphEdgeWidth, radius, from, to, kTelegraphSegments,
		r, g, b, kTelegraphEdge);
}

void MutantActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 160.f, 200.f))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	if (m_State == State::Windup)
	{
		DrawTelegraph(out, x, groundY);
	}

	out.AddShadow(depth - 0.01f, x, groundY, 96.f, 48.f);
	DrawFury(out, depth - 0.005f, x, groundY, 90.f);

	out.SetFigure(true);
	if (HitFlash() > 0.f)
	{
		out.SetFlash(HitFlash() * 0.75f);
	}

	float f = Facing();
	bool dormant = IsDormant();
	float windup = 0.f;
	if (m_State == State::Windup)
	{
		windup = m_StateTime / WindupTime(m_Blow);
		if (windup > 1.f)
		{
			windup = 1.f;
		}
	}
	float strike = (m_StrikeFlash > 0.f) ? m_StrikeFlash : 0.f;

	// Asleep it slumps low and barely breathes.
	float hunch = dormant ? 0.8f : 1.f;
	float breathe = dormant ? sinf(Time() * 0.8f) * 0.8f : sinf(Time() * 1.6f) * 1.6f;
	float raise = (m_Blow == Blow::Slam) ? windup : 0.f;
	float pullBack = (m_Blow == Blow::Sweep) ? windup : 0.f;
	float swell = (m_Blow == Blow::Burst) ? windup * 0.18f : 0.f;
	float lean = f * (3.f - raise * 4.f + strike * 6.f - pullBack * 3.f);

	// A broad base sunk into the ground, and a hunched bulk on top of it
	// flaring out to the shoulders.
	out.AddTaperedBox(depth, x, groundY, 1.5f, 1.4f, 22.f * hunch, 0.8f, lean * 0.3f, 0.f,
		kFlesh * 0.8f, kFlesh * 0.8f, kFlesh * 0.8f);
	float upperBase = groundY + 20.f * hunch + breathe;
	float upperHeight = 24.f * hunch * (1.f + swell);
	out.AddTaperedBox(depth + 0.01f, x + lean * 0.4f, upperBase, 1.25f * (1.f + swell), 1.15f * (1.f + swell), upperHeight,
		1.25f, lean, 0.f, kFlesh, kFlesh, kFlesh);
	float shoulderY = upperBase + upperHeight - 2.f;

	// The head, sunk low between the shoulders and pushed forward.
	float headX = x + f * 12.f + lean * 1.4f;
	out.AddTaperedBox(depth + 0.03f, headX, shoulderY - 6.f, 0.32f, 0.32f, 10.f, 0.6f, f * 2.f, 0.f,
		kFlesh * 1.1f, kFlesh * 1.1f, kFlesh * 1.1f);

	// Two thin arms behind, two great ones in front: hanging to the ground at
	// rest, thrown up overhead for a slam, dragged back for a sweep.
	for (int side = -1; side <= 1; side += 2)
	{
		float backX = x + side * 18.f + lean * 0.5f;
		out.AddTaperedBox(depth - 0.005f, backX, groundY + 6.f, 0.12f, 0.12f, shoulderY - groundY - 10.f,
			1.5f, -side * 3.f, 0.f, kFlesh * 0.85f, kFlesh * 0.85f, kFlesh * 0.85f);

		float shoulderX = x + side * 30.f + lean - f * 10.f * pullBack;
		float armDepth = depth + ((side == (int)f) ? 0.025f : 0.005f);
		if (raise > 0.f)
		{
			out.AddTaperedBox(armDepth, shoulderX, shoulderY - 4.f, 0.24f, 0.24f, 8.f + 30.f * raise,
				0.55f, (side * 5.f + f * 4.f) * raise, 0.f, kFlesh * 0.95f, kFlesh * 0.95f, kFlesh * 0.95f);
		}
		else
		{
			out.AddTaperedBox(armDepth, shoulderX + side * 4.f, groundY + 2.f, 0.24f, 0.24f, shoulderY - groundY - 2.f,
				1.7f, -side * 4.f, 0.f, kFlesh * 0.95f, kFlesh * 0.95f, kFlesh * 0.95f);
		}
	}

	// The cracks, and three eyes: what it was, still showing through. Light
	// shines; dark does not. Dim while it sleeps, pulsing once awake.
	float pulse = dormant ? 0.5f : 0.8f + 0.2f * sinf(Time() * 4.f);
	for (int i = 0; i < kCracks; i++)
	{
		Alignment alignment = ColorOf(i);
		float r, g, b;
		AlignmentColor::Core(alignment, r, g, b);
		out.SetGlow(alignment == Alignment::Light ? 0.8f * pulse : 0.f);
		out.AddRect(depth + 0.02f, x + lean * 0.5f + kCrackX[i], upperBase + kCrackY[i] * hunch, 2.f, kCrackHeight[i],
			r * pulse, g * pulse, b * pulse, 1.f);
	}

	for (int i = 0; i < 3; i++)
	{
		Alignment alignment = ColorOf(i + 1);
		float r, g, b;
		AlignmentColor::Core(alignment, r, g, b);
		out.SetGlow(alignment == Alignment::Light ? 0.6f * pulse : 0.f);
		out.AddRect(depth + 0.04f, headX + f * 3.f + (i - 1) * 3.f, shoulderY - 1.f + (i == 1 ? 2.f : 0.f), 1.6f, 1.6f,
			r * pulse, g * pulse, b * pulse, 1.f);
	}
	out.SetGlow(0.f);
}
