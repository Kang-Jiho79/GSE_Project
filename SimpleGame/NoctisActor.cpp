#include "stdafx.h"
#include "NoctisActor.h"
#include "SnareActor.h"
#include "ShadeActor.h"
#include "EnemyProjectileActor.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Combat.h"
#include "Impact.h"
#include "PlayerStatus.h"
#include "Progress.h"
#include "Palette.h"
#include "Tuning.h"
#include "Dialogue.h"
#include <cmath>

namespace
{
	const float kPi = 3.14159265f;
	const float kDegToRad = kPi / 180.f;

	const float kStrikeShowTime = 0.25f;
	const float kGaitPerTile = 3.4f;
	const float kStrafeShare = 0.55f;
	const float kHoldSlack = 0.5f; // past his distance by this much, he comes on

	const float kTelegraphLift = 2.f;
	const float kTelegraphBase = 0.14f;
	const float kTelegraphFill = 0.26f;
	const float kTelegraphEdge = 0.5f;
	const float kTelegraphEdgeWidth = 0.08f;
	const int kTelegraphSegments = 24;

	const int kCastSparks = 6;
	const float kCastSparkSpeed = 120.f;
	const float kSparkLift = 30.f;

	// His face under the helm, and the edge of his blade.
	const Palette::Color kFace = { 0.42f, 0.40f, 0.44f };
	const Palette::Color kEdge = { 0.30f, 0.28f, 0.34f };

	const KingActor::Lines kLines =
	{
		"어둠만 쥐고 와라. 계산은 내가 하지.",
		"죽이는 건 언제든 할 수 있다. 다시 와라.",
		"물러서는 것도 수다. 나쁘지 않군.",
		"……좋다. 네 어둠을 인정하지.",
		"녹티스의 축복  ·  피흡이 늘어난다",
		Dialogue::KingNoctis,
	};

	// A straight length of blade from a point, at an angle on screen measured
	// from the way he faces - 0 straight ahead, 90 straight up.
	void Edge(DrawList& out, float depth, float fromX, float fromY, float facing, float angleDegrees,
		float length, float width, float& tipX, float& tipY)
	{
		float angle = angleDegrees * kDegToRad;
		float dirX = cosf(angle) * facing;
		float dirY = sinf(angle);
		tipX = fromX + dirX * length;
		tipY = fromY + dirY * length;

		float sideX = -dirY * width * 0.5f;
		float sideY = dirX * width * 0.5f;
		float corners[8] =
		{
			fromX - sideX, fromY - sideY,
			tipX - sideX * 0.5f, tipY - sideY * 0.5f,
			tipX + sideX * 0.5f, tipY + sideY * 0.5f,
			fromX + sideX, fromY + sideY,
		};
		out.AddCorners(depth, corners, kEdge.r, kEdge.g, kEdge.b, 1.f);
	}
}

NoctisActor::NoctisActor(int region, bool recognized)
	: KingActor("어둠의 왕 녹티스", region, Tuning::Noctis::kMaxHealth, Tuning::Noctis::kFuryTime, Form::Dark,
		kLines, recognized)
{
}

float NoctisActor::BodyRadius() const
{
	return Tuning::Noctis::kBodyRadius;
}

bool NoctisActor::Cunning() const
{
	return HealthFraction() < Tuning::Noctis::kPhaseShare;
}

float NoctisActor::Resist(float damage)
{
	// In the stance every blow is turned aside - and remembered.
	if (m_State == State::Counter)
	{
		m_Countered = true;
		return 0.f;
	}
	return damage;
}

void NoctisActor::Fight(const ActorContext& ctx)
{
	m_StateTime += ctx.deltaTime;
	if (m_Cooldown > 0.f)
	{
		m_Cooldown -= ctx.deltaTime;
	}
	if (m_ShadeCooldown > 0.f)
	{
		m_ShadeCooldown -= ctx.deltaTime;
	}
	if (m_StrikeFlash > 0.f)
	{
		m_StrikeFlash -= ctx.deltaTime / kStrikeShowTime;
	}

	float dirCol, dirRow;
	float distance = ToPlayer(ctx, dirCol, dirRow);
	m_Moving = false;

	// He keeps count of how long he has been let stand close.
	if (distance <= Tuning::Noctis::kCloseRange)
	{
		m_Pressure += ctx.deltaTime;
	}
	else
	{
		m_Pressure -= Tuning::Noctis::kPressureDecay * ctx.deltaTime;
		if (m_Pressure < 0.f)
		{
			m_Pressure = 0.f;
		}
	}

	switch (m_State)
	{
	case State::Stalk:
	{
		FaceAlong(dirCol, dirRow);
		if (m_Cooldown <= 0.f)
		{
			Choose(ctx, distance, dirCol, dirRow);
			break;
		}
		Stalk(ctx, distance, dirCol, dirRow);
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
	case State::Lunge:
	{
		float step = Tuning::Noctis::kLungeLength / Tuning::Noctis::kLungeTime * ctx.deltaTime;
		StepAlong(m_DirCol, m_DirRow, step);
		if (m_StateTime >= Tuning::Noctis::kLungeTime)
		{
			Finish();
		}
		break;
	}
	case State::Counter:
	{
		FaceAlong(dirCol, dirRow);
		if (m_Countered)
		{
			m_State = State::Riposte;
			m_StateTime = 0.f;
		}
		else if (m_StateTime >= Tuning::Noctis::kCounterTime)
		{
			Finish();
		}
		break;
	}
	case State::Riposte:
	{
		if (m_StateTime >= Tuning::Noctis::kCounterDelay)
		{
			Combat::StrikePlayerRadius(ctx, WorldX(), WorldY(), Tuning::Noctis::kCounterRange,
				Tuning::Noctis::kCounterDamage * FuryDamage());
			SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dark, kCastSparks * 2, kCastSparkSpeed * 1.5f,
				kSparkLift);
			ctx.impact->Shake(Tuning::Noctis::kCounterShake);
			m_StrikeFlash = 1.f;
			Finish();
		}
		break;
	}
	case State::Recover:
	{
		if (m_StateTime >= Tuning::Noctis::kRecover / FurySpeed())
		{
			m_State = State::Stalk;
			m_StateTime = 0.f;
		}
		break;
	}
	}
}

void NoctisActor::Stalk(const ActorContext& ctx, float distance, float dirCol, float dirRow)
{
	m_StrafeTimer -= ctx.deltaTime;
	if (m_StrafeTimer <= 0.f)
	{
		m_StrafeSign = -m_StrafeSign;
		m_StrafeTimer = Tuning::Noctis::kStrafeTime;
	}

	// He never comes all the way; he waits at his distance, circling, for him
	// to come instead.
	float step = Tuning::Noctis::kSpeed * FurySpeed() * ctx.deltaTime;
	if (distance > Tuning::Noctis::kHoldRange + kHoldSlack)
	{
		StepAlong(dirCol, dirRow, step);
	}
	else
	{
		step *= kStrafeShare;
		StepAlong(-dirRow * m_StrafeSign, dirCol * m_StrafeSign, step);
	}

	m_Moving = true;
	m_Gait += step * kGaitPerTile;
}

void NoctisActor::Choose(const ActorContext& ctx, float distance, float dirCol, float dirRow)
{
	int snareEvery = Cunning() ? 3 : 4;
	if (m_BlowCount % snareEvery == snareEvery - 1
		&& SnaresLaid(ctx) + Tuning::Noctis::kSnares <= Tuning::Noctis::kMaxSnares)
	{
		Start(Blow::Snares, Tuning::Noctis::kSnareWindup, dirCol, dirRow);
		return;
	}

	if (distance <= Tuning::Noctis::kCloseRange)
	{
		// Kept close long enough, he stops cutting and waits to be struck.
		if (m_Pressure >= Tuning::Noctis::kPressureForCounter && m_LastBlow != Blow::Counter)
		{
			m_Blow = Blow::Counter;
			m_LastBlow = Blow::Counter;
			m_State = State::Counter;
			m_StateTime = 0.f;
			m_Countered = false;
			m_Pressure = 0.f;
			return;
		}
		Start(Blow::Cut, Tuning::Noctis::kCutWindup, dirCol, dirRow);
		return;
	}

	if (distance >= Tuning::Noctis::kFarRange)
	{
		if (m_ShadeCooldown <= 0.f)
		{
			Start(Blow::Shades, Tuning::Noctis::kShadeWindup, dirCol, dirRow);
		}
		else
		{
			Start(Blow::Orbs, Tuning::Noctis::kOrbWindup, dirCol, dirRow);
		}
		return;
	}

	Start(Blow::Lunge, Tuning::Noctis::kLungeWindup, dirCol, dirRow);
}

void NoctisActor::Start(Blow blow, float windup, float dirCol, float dirRow)
{
	m_Blow = blow;
	m_LastBlow = blow;
	m_DirCol = dirCol;
	m_DirRow = dirRow;
	FaceAlong(dirCol, dirRow);

	if (Cunning())
	{
		windup *= Tuning::Noctis::kPhaseWindupScale;
	}
	m_Windup = windup / FurySpeed();
	m_State = State::Windup;
	m_StateTime = 0.f;
}

void NoctisActor::Release(const ActorContext& ctx)
{
	switch (m_Blow)
	{
	case Blow::Cut:
	{
		Combat::StrikePlayerArc(ctx, WorldX(), WorldY(), m_DirCol, m_DirRow, Tuning::Noctis::kCutRange, 180.f,
			Tuning::Noctis::kCutDamage * FuryDamage());
		break;
	}
	case Blow::Lunge:
	{
		// The whole marked line is struck as he goes; he arrives a moment later.
		Combat::StrikePlayerLine(ctx, WorldX(), WorldY(), m_DirCol, m_DirRow, Tuning::Noctis::kLungeLength,
			Tuning::Noctis::kLungeWidth, Tuning::Noctis::kLungeDamage * FuryDamage());
		m_StrikeFlash = 1.f;
		m_State = State::Lunge;
		m_StateTime = 0.f;
		return;
	}
	case Blow::Snares:
	{
		LaySnares(ctx);
		break;
	}
	case Blow::Shades:
	{
		RaiseShades(ctx);
		m_ShadeCooldown = Tuning::Noctis::kShadeCooldown;
		break;
	}
	case Blow::Orbs:
	{
		ThrowOrbs(ctx);
		break;
	}
	default:
	{
		break;
	}
	}

	m_StrikeFlash = 1.f;
	Finish();
}

void NoctisActor::LaySnares(const ActorContext& ctx)
{
	float col, row;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);

	// A ring round himself, turned a little each time so no path through it
	// stays safe.
	int count = Tuning::Noctis::kSnares;
	float turn = m_BlowCount * 0.7f;
	for (int i = 0; i < count; i++)
	{
		float angle = turn + i * 2.f * kPi / count;
		float snareCol = col + cosf(angle) * Tuning::Noctis::kSnareRing;
		float snareRow = row + sinf(angle) * Tuning::Noctis::kSnareRing;
		WorldGrid::ClampToMap(snareCol, snareRow);
		if (WorldGrid::IsBlocked(snareCol, snareRow))
		{
			continue;
		}

		std::unique_ptr<SnareActor> snare(new SnareActor(this, Tuning::Noctis::kSnareDamage * FuryDamage()));
		float x, y;
		WorldGrid::IsoPosition(snareCol, snareRow, x, y);
		snare->SetLocalPosition(x, y);
		ctx.graph->Spawn(std::move(snare));
	}

	SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dark, kCastSparks, kCastSparkSpeed, kSparkLift);
}

void NoctisActor::RaiseShades(const ActorContext& ctx)
{
	float playerCol, playerRow;
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);

	// Out of the ground to either side of him, across the way to the king.
	for (int i = 0; i < Tuning::Noctis::kShades; i++)
	{
		float side = (i % 2 == 0) ? 1.f : -1.f;
		float shadeCol = playerCol - m_DirRow * side * Tuning::Noctis::kShadeSpread;
		float shadeRow = playerRow + m_DirCol * side * Tuning::Noctis::kShadeSpread;
		WorldGrid::ClampToMap(shadeCol, shadeRow);
		if (WorldGrid::IsBlocked(shadeCol, shadeRow))
		{
			continue;
		}

		std::unique_ptr<ShadeActor> shade(new ShadeActor(this, Tuning::Noctis::kShadeDamage * FuryDamage()));
		float x, y;
		WorldGrid::IsoPosition(shadeCol, shadeRow, x, y);
		shade->SetLocalPosition(x, y);
		ctx.graph->Spawn(std::move(shade));
		SparkActor::Burst(*ctx.graph, x, y, SparkActor::Style::Dark, kCastSparks, kCastSparkSpeed, kSparkLift);
	}
}

void NoctisActor::ThrowOrbs(const ActorContext& ctx)
{
	int count = Tuning::Noctis::kOrbs;
	float spacing = Tuning::Noctis::kOrbSpread * kDegToRad / (count - 1);
	float center = atan2f(m_DirRow, m_DirCol);
	for (int i = 0; i < count; i++)
	{
		float angle = center + (i - (count - 1) * 0.5f) * spacing;
		std::unique_ptr<EnemyProjectileActor> orb(new EnemyProjectileActor(Alignment::Dark, cosf(angle), sinf(angle),
			Tuning::Noctis::kOrbSpeed * FurySpeed(), Tuning::Noctis::kOrbTurn, Tuning::Noctis::kOrbLifetime,
			Tuning::Noctis::kOrbDamage * FuryDamage()));
		orb->SetLocalPosition(WorldX(), WorldY());
		ctx.graph->Spawn(std::move(orb));
	}
}

int NoctisActor::SnaresLaid(const ActorContext& ctx) const
{
	int count = 0;
	ctx.graph->ForEach<SnareActor>([&](SnareActor& snare)
	{
		if (snare.Owner() == this && !snare.IsDestroyed())
		{
			count++;
		}
	});
	return count;
}

void NoctisActor::Finish()
{
	m_Cooldown = Tuning::Noctis::kCooldown / FurySpeed();
	m_BlowCount++;
	m_State = State::Recover;
	m_StateTime = 0.f;
}

void NoctisActor::ResetMoves()
{
	m_State = State::Stalk;
	m_Blow = Blow::Cut;
	m_LastBlow = Blow::Cut;
	m_StateTime = 0.f;
	m_Cooldown = 1.f;
	m_ShadeCooldown = 0.f;
	m_BlowCount = 0;
	m_Pressure = 0.f;
	m_Countered = false;
	m_StrikeFlash = 0.f;
}

void NoctisActor::Reward(const ActorContext& ctx)
{
	ctx.status->darkBlessing = true;
	ctx.progress->recognizedByNoctis = true;
}

void NoctisActor::DrawTelegraph(DrawList& out, float x, float groundY) const
{
	float r, g, b;
	AlignmentColor::Warning(Alignment::Dark, Palette::kWarningWash, r, g, b);
	float baseY = groundY + kTelegraphLift;

	// The stance: a ring at the reach of the answer, pulsing - strike him
	// now and this is where it lands.
	if (m_State == State::Counter || m_State == State::Riposte)
	{
		float radius = Tuning::Noctis::kCounterRange;
		float pulse = 0.7f + 0.3f * sinf(Time() * 9.f);
		float fill = (m_State == State::Riposte) ? m_StateTime / Tuning::Noctis::kCounterDelay : 0.f;
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, radius - kTelegraphEdgeWidth, radius, 0.f, 2.f * kPi,
			kTelegraphSegments, r, g, b, kTelegraphEdge * pulse);
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, 0.f, radius * (fill > 1.f ? 1.f : fill), 0.f, 2.f * kPi,
			kTelegraphSegments, r, g, b, kTelegraphFill);
		return;
	}

	if (m_State != State::Windup)
	{
		return;
	}

	float progress = m_StateTime / m_Windup;
	if (progress > 1.f)
	{
		progress = 1.f;
	}

	if (m_Blow == Blow::Cut)
	{
		float radius = Tuning::Noctis::kCutRange;
		float center = atan2f(m_DirRow, m_DirCol);
		float from = center - kPi * 0.5f;
		float to = center + kPi * 0.5f;
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, 0.f, radius, from, to, kTelegraphSegments, r, g, b, kTelegraphBase);
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, 0.f, radius * progress, from, to, kTelegraphSegments,
			r, g, b, kTelegraphFill);
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, radius - kTelegraphEdgeWidth, radius, from, to, kTelegraphSegments,
			r, g, b, kTelegraphEdge);
	}
	else if (m_Blow == Blow::Lunge)
	{
		float length = Tuning::Noctis::kLungeLength;
		float width = Tuning::Noctis::kLungeWidth;
		out.AddGroundStrip(WorldX(), WorldY(), x, baseY, m_DirCol, m_DirRow, 0.f, length, width, r, g, b, kTelegraphBase);
		out.AddGroundStrip(WorldX(), WorldY(), x, baseY, m_DirCol, m_DirRow, 0.f, length * progress, width,
			r, g, b, kTelegraphFill);
	}
}

void NoctisActor::DrawBody(DrawList& out, float depth, float x, float groundY) const
{
	const Palette::Color& armor = Palette::kNoctisArmor;
	const Palette::Color& mantle = Palette::kNoctisMantle;

	float f = Facing();
	float kneel = Kneel();
	float windup = 0.f;
	if (m_State == State::Windup && m_Windup > 0.f)
	{
		windup = m_StateTime / m_Windup;
		if (windup > 1.f)
		{
			windup = 1.f;
		}
	}
	float strike = (m_StrikeFlash > 0.f) ? m_StrikeFlash : 0.f;
	bool casting = (m_State == State::Windup)
		&& (m_Blow == Blow::Snares || m_Blow == Blow::Shades || m_Blow == Blow::Orbs);
	bool stance = (m_State == State::Counter);
	bool lunging = (m_State == State::Lunge) || (m_State == State::Windup && m_Blow == Blow::Lunge);

	float stride = m_Moving ? sinf(m_Gait) * 4.f : 0.f;
	float bob = m_Moving ? fabsf(sinf(m_Gait)) * 1.2f : sinf(Time() * 1.4f) * 0.5f;
	float crouch = (lunging ? 5.f : 0.f) + (stance ? 3.f : 0.f) + kneel * 12.f;
	float lean = f * ((lunging ? 7.f : 0.f) - windup * 2.f + strike * 4.f);

	out.AddShadow(depth - 0.01f, x, groundY, 44.f, 22.f);
	out.SetFigure(true);

	// A long ragged mantle behind him.
	out.AddTaperedBox(depth - 0.006f, x - f * 7.f, groundY + 1.f, 0.44f, 0.2f, 38.f - crouch, 0.5f, lean * 0.4f - f * 2.f, 0.f,
		mantle.r, mantle.g, mantle.b);

	// Thin legs, set wide and bent.
	const float kLegHeight = 16.f;
	out.AddTaperedBox(depth, x - 7.f + stride, groundY, 0.14f, 0.14f, kLegHeight - crouch * 0.5f, 0.85f, -f * 1.5f, 0.f,
		armor.r * 0.85f, armor.g * 0.85f, armor.b * 0.85f);
	out.AddTaperedBox(depth, x + 7.f - stride + f * kneel * 6.f, groundY, 0.14f, 0.14f, kLegHeight - crouch, 0.85f, f * 1.5f, 0.f,
		armor.r * 0.85f, armor.g * 0.85f, armor.b * 0.85f);

	// Black plate, narrow at the waist and broad and hard at the shoulder.
	float waist = groundY + kLegHeight - crouch + bob;
	const float kTorsoHeight = 22.f;
	out.AddTaperedBox(depth + 0.015f, x + lean * 0.3f, waist, 0.24f, 0.22f, kTorsoHeight, 1.45f, lean, 0.f,
		armor.r, armor.g, armor.b);
	float shoulderY = waist + kTorsoHeight;

	// Barbed pauldrons, a spike on each thrust up and out.
	for (int side = -1; side <= 1; side += 2)
	{
		float px = x + side * 15.f + lean;
		out.AddBox(depth + 0.02f, px, shoulderY - 4.f, 0.18f, 0.18f, 5.f, armor.r * 1.2f, armor.g * 1.2f, armor.b * 1.2f);
		out.AddTaperedBox(depth + 0.021f, px + side * 3.f, shoulderY + 1.f, 0.06f, 0.06f, 9.f, 0.1f, side * 5.f, 0.f,
			armor.r * 1.2f, armor.g * 1.2f, armor.b * 1.2f);
	}

	// A narrow helm, bowed when he kneels, crowned with black spikes, and
	// violet eyes that give off no light.
	float headX = x + f * (2.f + kneel * 4.f) + lean * 1.1f;
	float headBase = shoulderY - kneel * 3.f;
	out.AddTaperedBox(depth + 0.03f, headX, headBase, 0.17f, 0.17f, 12.f, 0.75f, 0.f, 0.f, kFace.r, kFace.g, kFace.b);
	out.AddTaperedBox(depth + 0.031f, headX, headBase + 5.f, 0.19f, 0.19f, 8.f, 0.85f, 0.f, 0.f,
		armor.r * 1.1f, armor.g * 1.1f, armor.b * 1.1f);
	for (int i = -2; i <= 2; i++)
	{
		float height = (i == 0) ? 11.f : ((i == -1 || i == 1) ? 8.f : 5.f);
		out.AddTaperedBox(depth + 0.035f, headX + i * 3.f, headBase + 12.f, 0.04f, 0.04f, height, 0.1f, i * 1.5f, 0.f,
			armor.r * 1.3f, armor.g * 1.3f, armor.b * 1.3f);
	}
	float er, eg, eb;
	AlignmentColor::Core(Alignment::Dark, er, eg, eb);
	out.AddRect(depth + 0.04f, headX + f * 3.5f, headBase + 7.f, 4.f, 1.4f, er, eg, eb, 1.f);

	// The off hand: raised and cupping a darkness while he works something.
	if (casting)
	{
		float handX = x - f * 12.f + lean;
		float handY = shoulderY + 6.f + windup * 8.f;
		float gr, gg, gb;
		AlignmentColor::Glow(Alignment::Dark, gr, gg, gb);
		out.AddGlow(depth + 0.05f, handX, handY, 24.f * windup, gr, gg, gb, windup);
		out.AddDiamond(depth + 0.055f, handX, handY, 5.f, 7.f, er, eg, eb, 1.f);
	}

	// The curved blade: low at his side at rest, drawn back to cut, held
	// level across him in the stance, swept through after a blow, laid flat
	// on the ground before him when he kneels.
	float handX = x + f * 13.f + lean;
	float handY = shoulderY - 12.f;
	float angle = -60.f;
	if (kneel > 0.f)
	{
		handY = groundY + 8.f;
		angle = -5.f;
	}
	else if (stance)
	{
		angle = 8.f;
		handX = x + f * 6.f + lean;
		handY = shoulderY - 6.f;
	}
	else if (m_State == State::Windup && (m_Blow == Blow::Cut || m_Blow == Blow::Lunge))
	{
		angle = -60.f + windup * 210.f;
	}
	else if (strike > 0.f)
	{
		angle = -60.f - strike * 20.f;
	}

	float midX, midY, tipX, tipY;
	Edge(out, depth + 0.05f, handX, handY, f, angle, 22.f, 4.f, midX, midY);
	Edge(out, depth + 0.05f, midX, midY, f, angle + 22.f, 18.f, 3.f, tipX, tipY);
}

void NoctisActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 160.f, 200.f))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	if (IsAwake() && !HasYielded())
	{
		DrawTelegraph(out, x, groundY);
	}

	DrawFury(out, depth - 0.005f, x, groundY, 70.f);

	if (HitFlash() > 0.f)
	{
		out.SetFlash(HitFlash() * 0.75f);
	}
	DrawBody(out, depth, x, groundY);
}
