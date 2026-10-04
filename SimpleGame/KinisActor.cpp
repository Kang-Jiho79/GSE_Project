#include "stdafx.h"
#include "KinisActor.h"
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
	const float kDegToRad = kPi / 180.f;

	const float kStrikeShowTime = 0.25f;
	const float kKneelTime = 0.8f;

	const float kTelegraphLift = 2.f;
	const float kTelegraphBase = 0.14f;
	const float kTelegraphFill = 0.26f;
	const float kTelegraphEdge = 0.5f;
	const float kTelegraphEdgeWidth = 0.08f;
	const int kTelegraphSegments = 24;

	// Taller than any man, robed in the gray from hood to hem.
	const float kScale = 1.3f;
}

KinisActor::KinisActor(int region)
	: BossActor("키니스", Tuning::Kinis::kMaxHealth, region, Tuning::Kinis::kWakeRange, Tuning::Kinis::kLeashRange,
		Tuning::Kinis::kFuryTime)
{
}

float KinisActor::BodyRadius() const
{
	return Tuning::Kinis::kBodyRadius;
}

float KinisActor::TakeHit(float damage)
{
	if (!m_Provoked)
	{
		return 0.f;
	}
	return BossActor::TakeHit(damage);
}

void KinisActor::Provoke()
{
	m_Provoked = true;
	RequestWake();
}

bool KinisActor::HasFallen() const
{
	return m_Fallen;
}

bool KinisActor::Worn() const
{
	return HealthFraction() < Tuning::Kinis::kPhaseShare;
}

int KinisActor::LanceCount() const
{
	return Worn() ? 3 : 1;
}

float KinisActor::LanceAngle(int index) const
{
	// The middle one straight at him, the others to either side.
	return (index - (LanceCount() - 1) * 0.5f) * Tuning::Kinis::kLanceSpread * kDegToRad;
}

void KinisActor::OnUpdate(const ActorContext& ctx)
{
	m_Breath += ctx.deltaTime;
	if (m_Fallen)
	{
		m_FallenTime += ctx.deltaTime;
		return;
	}
	if (!m_Provoked)
	{
		return;
	}
	BossActor::OnUpdate(ctx);
}

void KinisActor::Defeated(const ActorContext& ctx)
{
	m_Fallen = true;
	m_FallenTime = 0.f;
	ctx.impact->HitStop(Tuning::Boss::kDefeatStop);
	ctx.impact->Shake(Tuning::Boss::kDefeatShake);
}

void KinisActor::Reward(const ActorContext& ctx)
{
}

void KinisActor::OnReset(const ActorContext& ctx)
{
	m_State = State::Stalk;
	m_StateTime = 0.f;
	m_Cooldown = 1.f;
	m_BlowCount = 0;
	m_StrikeFlash = 0.f;
}

void KinisActor::Fight(const ActorContext& ctx)
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
	m_Moving = false;

	switch (m_State)
	{
	case State::Stalk:
	{
		FaceAlong(dirCol, dirRow);
		if (m_Cooldown <= 0.f)
		{
			if (m_BlowCount % 3 == 2)
			{
				Start(Blow::Ring, Tuning::Kinis::kRingWindup, dirCol, dirRow);
			}
			else if (distance <= Tuning::Kinis::kCleaveReach)
			{
				Start(Blow::Cleave, Tuning::Kinis::kCleaveWindup, dirCol, dirRow);
			}
			else
			{
				Start(Blow::Lance, Tuning::Kinis::kLanceWindup, dirCol, dirRow);
			}
			break;
		}

		if (distance > Tuning::Kinis::kKeepRange)
		{
			float step = Tuning::Kinis::kSpeed * FurySpeed() * ctx.deltaTime;
			if (step > distance - Tuning::Kinis::kKeepRange)
			{
				step = distance - Tuning::Kinis::kKeepRange;
			}
			StepAlong(dirCol, dirRow, step);
			m_Moving = true;
		}
		break;
	}
	case State::Windup:
	{
		if (m_StateTime >= m_Windup)
		{
			Release(ctx);
		}
		break;
	}
	case State::Recover:
	{
		if (m_StateTime >= Tuning::Kinis::kRecover / FurySpeed())
		{
			m_State = State::Stalk;
			m_StateTime = 0.f;
		}
		break;
	}
	}
}

void KinisActor::Start(Blow blow, float windup, float dirCol, float dirRow)
{
	m_Blow = blow;
	m_DirCol = dirCol;
	m_DirRow = dirRow;
	FaceAlong(dirCol, dirRow);

	if (Worn())
	{
		windup *= Tuning::Kinis::kPhaseWindupScale;
	}
	m_Windup = windup / FurySpeed();
	m_State = State::Windup;
	m_StateTime = 0.f;
}

void KinisActor::Release(const ActorContext& ctx)
{
	switch (m_Blow)
	{
	case Blow::Lance:
	{
		float center = atan2f(m_DirRow, m_DirCol);
		for (int i = 0; i < LanceCount(); i++)
		{
			float angle = center + LanceAngle(i);
			Combat::StrikePlayerLine(ctx, WorldX(), WorldY(), cosf(angle), sinf(angle), Tuning::Kinis::kLanceLength,
				Tuning::Kinis::kLanceWidth, Tuning::Kinis::kLanceDamage * FuryDamage());
		}
		ctx.impact->Shake(Tuning::Kinis::kLanceShake);
		break;
	}
	case Blow::Cleave:
	{
		Combat::StrikePlayerArc(ctx, WorldX(), WorldY(), m_DirCol, m_DirRow, Tuning::Kinis::kCleaveRange,
			Tuning::Kinis::kCleaveArc, Tuning::Kinis::kCleaveDamage * FuryDamage());
		break;
	}
	case Blow::Ring:
	{
		// Light and dark by turns: the gray, coming apart into what it was.
		int count = Worn() ? Tuning::Kinis::kPhaseRingCount : Tuning::Kinis::kRingCount;
		float offset = (m_BlowCount % 2 == 0) ? 0.f : 0.5f;
		for (int i = 0; i < count; i++)
		{
			float angle = (i + offset) * 2.f * kPi / count;
			Alignment alignment = (i % 2 == 0) ? Alignment::Light : Alignment::Dark;
			std::unique_ptr<EnemyProjectileActor> shot(new EnemyProjectileActor(alignment, cosf(angle), sinf(angle),
				Tuning::Kinis::kRingSpeed * FurySpeed(), 0.f, Tuning::Kinis::kRingLifetime,
				Tuning::Kinis::kRingDamage * FuryDamage()));
			shot->SetLocalPosition(WorldX(), WorldY());
			ctx.graph->Spawn(std::move(shot));
		}
		break;
	}
	}

	m_StrikeFlash = 1.f;
	m_Cooldown = Tuning::Kinis::kCooldown / FurySpeed();
	m_BlowCount++;
	m_State = State::Recover;
	m_StateTime = 0.f;
}

void KinisActor::DrawTelegraph(DrawList& out, float x, float groundY) const
{
	float progress = m_StateTime / m_Windup;
	if (progress > 1.f)
	{
		progress = 1.f;
	}
	float baseY = groundY + kTelegraphLift;
	float r, g, b;

	switch (m_Blow)
	{
	case Blow::Lance:
	{
		AlignmentColor::Warning(Alignment::Light, Palette::kWarningWash, r, g, b);
		float center = atan2f(m_DirRow, m_DirCol);
		float length = Tuning::Kinis::kLanceLength;
		float width = Tuning::Kinis::kLanceWidth;
		for (int i = 0; i < LanceCount(); i++)
		{
			float angle = center + LanceAngle(i);
			out.AddGroundStrip(WorldX(), WorldY(), x, baseY, cosf(angle), sinf(angle), 0.f, length, width,
				r, g, b, kTelegraphBase);
			out.AddGroundStrip(WorldX(), WorldY(), x, baseY, cosf(angle), sinf(angle), 0.f, length * progress, width,
				r, g, b, kTelegraphFill);
		}
		break;
	}
	case Blow::Cleave:
	{
		AlignmentColor::Warning(Alignment::Dark, Palette::kWarningWash, r, g, b);
		float radius = Tuning::Kinis::kCleaveRange;
		float center = atan2f(m_DirRow, m_DirCol);
		float half = Tuning::Kinis::kCleaveArc * 0.5f * kDegToRad;
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, 0.f, radius, center - half, center + half, kTelegraphSegments,
			r, g, b, kTelegraphBase);
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, 0.f, radius * progress, center - half, center + half,
			kTelegraphSegments, r, g, b, kTelegraphFill);
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, radius - kTelegraphEdgeWidth, radius, center - half, center + half,
			kTelegraphSegments, r, g, b, kTelegraphEdge);
		break;
	}
	default:
	{
		// The gray gathering round him before it bursts.
		const Palette::Color& ash = Palette::kAshWarning;
		float radius = Tuning::Kinis::kBodyRadius + progress * 1.6f;
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, radius - 0.15f, radius, 0.f, 2.f * kPi, kTelegraphSegments,
			ash.r, ash.g, ash.b, 0.45f * progress);
		break;
	}
	}
}

void KinisActor::DrawBody(DrawList& out, float depth, float x, float groundY) const
{
	const Palette::Color& ash = Palette::kAsh;
	float s = kScale;
	float f = Facing();
	float windup = 0.f;
	if (m_State == State::Windup && m_Windup > 0.f && !m_Fallen)
	{
		windup = m_StateTime / m_Windup;
		if (windup > 1.f)
		{
			windup = 1.f;
		}
	}
	float strike = (m_StrikeFlash > 0.f && !m_Fallen) ? m_StrikeFlash : 0.f;
	float kneel = m_Fallen ? ((m_FallenTime < kKneelTime) ? m_FallenTime / kKneelTime : 1.f) : 0.f;

	float breathe = sinf(m_Breath * 1.1f) * 0.6f;
	float sway = m_Moving ? sinf(m_Breath * 6.f) * 2.f : sinf(m_Breath * 0.7f) * 1.5f;
	float lean = f * (-windup * 3.f + strike * 5.f) + kneel * f * 4.f;
	float drop = kneel * 14.f;
	float swell = (m_Blow == Blow::Ring) ? windup * 0.15f : 0.f;

	// The gray pooled about his feet - thinner as he falls.
	out.AddGlow(depth - 0.02f, x, groundY, 70.f, Palette::kAshVeil.r, Palette::kAshVeil.g, Palette::kAshVeil.b,
		0.55f * (1.f - kneel * 0.5f));
	out.AddShadow(depth - 0.01f, x, groundY, 36.f, 18.f);
	DrawFury(out, depth - 0.005f, x, groundY, 70.f);
	out.SetFigure(true);

	float robeHeight = (42.f + breathe) * s - drop;
	out.AddTaperedBox(depth, x, groundY, 0.38f * (1.f + swell), 0.32f * (1.f + swell), robeHeight, 0.5f, sway + lean, 0.f,
		ash.r * 0.55f, ash.g * 0.55f, ash.b * 0.57f);
	float shoulderY = groundY + robeHeight;

	float headX = x + f * (1.5f + kneel * 4.f) + sway + lean * 1.3f;
	out.AddTaperedBox(depth + 0.03f, headX, shoulderY - 2.f, 0.26f, 0.26f, 15.f * s, 0.55f, 0.f, 0.f,
		ash.r * 0.45f, ash.g * 0.45f, ash.b * 0.47f);
	out.AddRect(depth + 0.035f, headX + f * 3.f, shoulderY + 6.f, 8.f, 7.f,
		Palette::kShade.r, Palette::kShade.g, Palette::kShade.b, 1.f);

	// One eye gold, one violet; dimming as he falls.
	float dim = 1.f - kneel * 0.6f;
	float lr, lg, lb, dr, dg, db;
	AlignmentColor::Core(Alignment::Light, lr, lg, lb);
	AlignmentColor::Core(Alignment::Dark, dr, dg, db);
	out.SetGlow(0.8f * dim);
	out.AddRect(depth + 0.04f, headX + f * 1.5f, shoulderY + 7.f, 2.f, 2.f, lr * dim, lg * dim, lb * dim, 1.f);
	out.SetGlow(0.f);
	out.AddRect(depth + 0.04f, headX + f * 5.f, shoulderY + 7.f, 2.f, 2.f, dr * dim, dg * dim, db * dim, 1.f);

	// The cracks down him, brightest as a blow gathers.
	float pulse = (0.7f + 0.3f * sinf(m_Breath * 2.3f)) * dim + windup * 0.4f;
	out.SetGlow(0.6f * pulse);
	out.AddRect(depth + 0.02f, x - 7.f + sway * 0.6f, groundY + 26.f - drop, 2.f, 16.f, lr * pulse, lg * pulse, lb * pulse, 1.f);
	out.SetGlow(0.f);
	out.AddRect(depth + 0.02f, x + 8.f + sway * 0.6f, groundY + 18.f - drop, 2.f, 13.f, dr * pulse, dg * pulse, db * pulse, 1.f);

	if (m_Fallen)
	{
		return;
	}

	// A blade of the gray in one hand: held low, drawn back to cleave, and
	// the other hand raised and burning as a lance gathers.
	float handX = x + f * 16.f + lean;
	float handY = shoulderY - 18.f;
	float bladeLean = f * 10.f;
	if (m_State == State::Windup && m_Blow == Blow::Cleave)
	{
		handX -= f * windup * 18.f;
		handY += windup * 14.f;
		bladeLean = -f * 12.f * windup;
	}
	else if (strike > 0.f && m_Blow == Blow::Cleave)
	{
		bladeLean = f * (10.f + strike * 14.f);
	}
	out.AddTaperedBox(depth + 0.05f, handX, handY, 0.06f, 0.06f, 34.f, 0.25f, bladeLean, 0.f,
		ash.r * 0.85f, ash.g * 0.85f, ash.b * 0.88f);

	if (m_State == State::Windup && m_Blow == Blow::Lance)
	{
		out.SetGlow(1.f);
		out.AddGlow(depth + 0.1f, x - f * 14.f, shoulderY + 8.f + windup * 10.f, 26.f * windup,
			Palette::kLightGlow.r, Palette::kLightGlow.g, Palette::kLightGlow.b, windup);
		out.SetGlow(0.f);
	}
}

void KinisActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	// A lance reaches far from him; its mark is drawn even when he is not on
	// screen.
	if (m_Provoked && !m_Fallen && IsAwake() && m_State == State::Windup)
	{
		DrawTelegraph(out, x, groundY);
	}

	if (!ctx.IsOnScreen(x, y, 160.f, 220.f))
	{
		return;
	}

	if (HitFlash() > 0.f)
	{
		out.SetFlash(HitFlash() * 0.75f);
	}
	DrawBody(out, depth, x, groundY);
}
