#include "stdafx.h"
#include "LuciusActor.h"
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

	const float kFlashTime = 0.35f;
	const float kStrikeShowTime = 0.25f;
	const float kGaitPerTile = 3.2f;
	const float kStrafeShare = 0.6f; // of his speed, while circling

	// The marked ground, as every boss marks it.
	const float kTelegraphLift = 2.f;
	const float kTelegraphBase = 0.14f;
	const float kTelegraphFill = 0.26f;
	const float kTelegraphEdge = 0.5f;
	const float kTelegraphEdgeWidth = 0.08f;
	const int kTelegraphSegments = 24;
	const float kVolleyMarkReach = 5.f;

	// His bare face, and the steel of his spear and pauldrons.
	const Palette::Color kFace = { 0.70f, 0.66f, 0.60f };
	const Palette::Color kSteel = { 0.86f, 0.85f, 0.82f };

	const int kStepSparks = 8;
	const float kStepSparkSpeed = 140.f;
	const int kBreakSparks = 18;
	const float kBreakSparkSpeed = 220.f;
	const float kSparkLift = 30.f;

	const KingActor::Lines kLines =
	{
		"빛만으로 내게 닿아 보아라.",
		"쓰러진 자를 치는 것은 빛의 길이 아니다.",
		"등을 보이는 것 또한 하나의 답이겠지.",
		"……인정하마. 그대의 빛은 진짜다.",
		"루키우스의 축복  ·  빛의 공격이 강해진다",
		Dialogue::KingLucius,
	};

	// A long shaft held in the hand at an angle on screen, measured from the
	// way he faces - 0 straight ahead, 90 straight up - narrowing to a point.
	void Shaft(DrawList& out, float depth, float handX, float handY, float facing, float angleDegrees,
		float back, float length, float width, const Palette::Color& c, float& tipX, float& tipY)
	{
		float angle = angleDegrees * kDegToRad;
		float dirX = cosf(angle) * facing;
		float dirY = sinf(angle);

		float buttX = handX - dirX * back;
		float buttY = handY - dirY * back;
		tipX = handX + dirX * length;
		tipY = handY + dirY * length;

		float sideX = -dirY * width * 0.5f;
		float sideY = dirX * width * 0.5f;
		float corners[8] =
		{
			buttX - sideX, buttY - sideY,
			tipX - sideX * 0.3f, tipY - sideY * 0.3f,
			tipX + sideX * 0.3f, tipY + sideY * 0.3f,
			buttX + sideX, buttY + sideY,
		};
		out.AddCorners(depth, corners, c.r, c.g, c.b, 1.f);
	}
}

LuciusActor::LuciusActor(int region, bool recognized)
	: KingActor("빛의 왕 루키우스", region, Tuning::Lucius::kMaxHealth, Tuning::Lucius::kFuryTime, Form::Light,
		kLines, recognized)
{
}

float LuciusActor::BodyRadius() const
{
	return Tuning::Lucius::kBodyRadius;
}

void LuciusActor::Pierce()
{
	if (m_State != State::Sanctuary)
	{
		return;
	}

	// Straight through the dome: it shatters, and the same beam lands on him.
	m_State = State::Stagger;
	m_StateTime = 0.f;
	m_SanctuaryCooldown = Tuning::Lucius::kSanctuaryCooldown;
	m_BreakPending = true;
}

float LuciusActor::Resist(float damage)
{
	if (m_State == State::Sanctuary)
	{
		return 0.f;
	}
	if (m_State == State::Stagger)
	{
		return damage * Tuning::Lucius::kStaggerDamageScale;
	}
	return damage;
}

int LuciusActor::JudgmentLines() const
{
	return (HealthFraction() < Tuning::Lucius::kPhaseShare) ? 4 : 2;
}

void LuciusActor::JudgmentLine(int index, float& dirCol, float& dirRow) const
{
	const float kDiagonal = 0.70710678f;
	switch (index)
	{
	case 0:
	{
		dirCol = 1.f;
		dirRow = 0.f;
		break;
	}
	case 1:
	{
		dirCol = 0.f;
		dirRow = 1.f;
		break;
	}
	case 2:
	{
		dirCol = kDiagonal;
		dirRow = kDiagonal;
		break;
	}
	default:
	{
		dirCol = kDiagonal;
		dirRow = -kDiagonal;
		break;
	}
	}
}

void LuciusActor::Fight(const ActorContext& ctx)
{
	m_StateTime += ctx.deltaTime;
	if (m_Cooldown > 0.f)
	{
		m_Cooldown -= ctx.deltaTime;
	}
	if (m_SanctuaryCooldown > 0.f)
	{
		m_SanctuaryCooldown -= ctx.deltaTime;
	}
	if (m_JudgmentFlash > 0.f)
	{
		m_JudgmentFlash -= ctx.deltaTime / kFlashTime;
	}
	if (m_StrikeFlash > 0.f)
	{
		m_StrikeFlash -= ctx.deltaTime / kStrikeShowTime;
	}

	if (m_BreakPending)
	{
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Light, kBreakSparks, kBreakSparkSpeed, kSparkLift);
		ctx.impact->Shake(Tuning::Lucius::kBreakShake);
		m_BreakPending = false;
	}

	float dirCol, dirRow;
	float distance = ToPlayer(ctx, dirCol, dirRow);
	m_Moving = false;

	switch (m_State)
	{
	case State::Walk:
	{
		FaceAlong(dirCol, dirRow);
		if (m_Cooldown <= 0.f)
		{
			Choose(ctx, distance);
			break;
		}
		Walk(ctx, distance, dirCol, dirRow);
		break;
	}
	case State::Windup:
	{
		if (m_StateTime >= m_Windup)
		{
			Release(ctx, dirCol, dirRow);
		}
		break;
	}
	case State::Volley:
	{
		FaceAlong(dirCol, dirRow);
		if (m_StateTime >= m_NextWave)
		{
			ThrowWave(ctx, dirCol, dirRow);
		}
		break;
	}
	case State::Sanctuary:
	{
		Mend(Tuning::Lucius::kSanctuaryHeal * ctx.deltaTime);
		if (m_StateTime >= Tuning::Lucius::kSanctuaryTime)
		{
			m_SanctuaryCooldown = Tuning::Lucius::kSanctuaryCooldown;
			Finish(Tuning::Lucius::kCooldown);
		}
		break;
	}
	case State::Stagger:
	{
		if (m_StateTime >= Tuning::Lucius::kStaggerTime)
		{
			Finish(Tuning::Lucius::kCooldown * 0.5f);
		}
		break;
	}
	case State::Recover:
	{
		if (m_StateTime >= Tuning::Lucius::kRecover / FurySpeed())
		{
			m_State = State::Walk;
			m_StateTime = 0.f;
		}
		break;
	}
	}
}

void LuciusActor::Walk(const ActorContext& ctx, float distance, float dirCol, float dirRow)
{
	m_StrafeTimer -= ctx.deltaTime;
	if (m_StrafeTimer <= 0.f)
	{
		m_StrafeSign = -m_StrafeSign;
		m_StrafeTimer = Tuning::Lucius::kStrafeTime;
	}

	// Too close and he gives ground; too far and he comes on; between, he
	// circles, never standing still to be aimed at.
	float step = Tuning::Lucius::kSpeed * FurySpeed() * ctx.deltaTime;
	if (distance < Tuning::Lucius::kKeepNear)
	{
		StepAlong(-dirCol, -dirRow, step);
	}
	else if (distance > Tuning::Lucius::kKeepFar)
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

void LuciusActor::Choose(const ActorContext& ctx, float distance)
{
	if (HealthFraction() < Tuning::Lucius::kSanctuaryShare && m_SanctuaryCooldown <= 0.f)
	{
		m_State = State::Sanctuary;
		m_StateTime = 0.f;
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Light, kStepSparks, kStepSparkSpeed, kSparkLift);
		return;
	}

	if (distance < Tuning::Lucius::kRadianceRadius)
	{
		Start(Blow::Radiance, Tuning::Lucius::kRadianceWindup);
		return;
	}

	if (m_BlowCount % 3 == 2)
	{
		// Called down where he stands as it begins - stay, and it lands.
		WorldGrid::InverseIso(ctx.playerX, ctx.playerY, m_TargetCol, m_TargetRow);
		Start(Blow::Judgment, Tuning::Lucius::kJudgmentWindup);
		return;
	}

	Start(Blow::Volley, Tuning::Lucius::kVolleyWindup);
}

void LuciusActor::Start(Blow blow, float windup)
{
	m_Blow = blow;
	m_Windup = windup / FurySpeed();
	m_State = State::Windup;
	m_StateTime = 0.f;
}

void LuciusActor::Release(const ActorContext& ctx, float dirCol, float dirRow)
{
	switch (m_Blow)
	{
	case Blow::Volley:
	{
		m_State = State::Volley;
		m_StateTime = 0.f;
		m_WavesLeft = Tuning::Lucius::kVolleyWaves;
		m_NextWave = 0.f;
		return;
	}
	case Blow::Judgment:
	{
		float centerX, centerY;
		WorldGrid::IsoPosition(m_TargetCol, m_TargetRow, centerX, centerY);
		float reach = Tuning::Lucius::kJudgmentReach;
		for (int i = 0; i < JudgmentLines(); i++)
		{
			float lineCol, lineRow, originX, originY;
			JudgmentLine(i, lineCol, lineRow);
			WorldGrid::IsoPosition(m_TargetCol - lineCol * reach, m_TargetRow - lineRow * reach, originX, originY);
			Combat::StrikePlayerLine(ctx, originX, originY, lineCol, lineRow, reach * 2.f,
				Tuning::Lucius::kJudgmentWidth, Tuning::Lucius::kJudgmentDamage * FuryDamage());
		}
		SparkActor::Burst(*ctx.graph, centerX, centerY, SparkActor::Style::Light, kStepSparks, kStepSparkSpeed, kSparkLift);
		ctx.impact->Shake(Tuning::Lucius::kJudgmentShake);
		m_JudgmentFlash = 1.f;
		break;
	}
	case Blow::Radiance:
	{
		Combat::StrikePlayerRadius(ctx, WorldX(), WorldY(), Tuning::Lucius::kRadianceRadius,
			Tuning::Lucius::kRadianceDamage * FuryDamage());

		// Then a step back out of his reach, leaving light where he stood.
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Light, kStepSparks, kStepSparkSpeed, kSparkLift);
		StepAlong(-dirCol, -dirRow, Tuning::Lucius::kRadianceStep);
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Light, kStepSparks, kStepSparkSpeed, kSparkLift);
		break;
	}
	}

	m_StrikeFlash = 1.f;
	Finish(Tuning::Lucius::kCooldown);
}

void LuciusActor::ThrowWave(const ActorContext& ctx, float dirCol, float dirRow)
{
	// Aimed afresh every wave, and each fan turned half a step from the last,
	// so the gap that saved him once does not save him twice.
	int count = Tuning::Lucius::kVolleySpears;
	float spacing = Tuning::Lucius::kVolleySpread * kDegToRad / (count - 1);
	float offset = (m_WaveIndex % 2 == 0) ? 0.f : 0.5f;
	float center = atan2f(dirRow, dirCol);

	for (int i = 0; i < count; i++)
	{
		float angle = center + (i - (count - 1) * 0.5f + offset) * spacing;
		std::unique_ptr<EnemyProjectileActor> spear(new EnemyProjectileActor(Alignment::Light, cosf(angle), sinf(angle),
			Tuning::Lucius::kSpearSpeed * FurySpeed(), 0.f, Tuning::Lucius::kSpearLifetime,
			Tuning::Lucius::kSpearDamage * FuryDamage()));
		spear->SetLocalPosition(WorldX(), WorldY());
		ctx.graph->Spawn(std::move(spear));
	}

	m_StrikeFlash = 1.f;
	m_WaveIndex++;
	m_WavesLeft--;
	m_NextWave += Tuning::Lucius::kVolleyGap / FurySpeed();
	if (m_WavesLeft <= 0)
	{
		Finish(Tuning::Lucius::kCooldown);
	}
}

void LuciusActor::Finish(float cooldown)
{
	m_Cooldown = cooldown / FurySpeed();
	m_BlowCount++;
	m_State = State::Recover;
	m_StateTime = 0.f;
}

void LuciusActor::ResetMoves()
{
	m_State = State::Walk;
	m_StateTime = 0.f;
	m_Cooldown = 1.f;
	m_SanctuaryCooldown = 0.f;
	m_BlowCount = 0;
	m_WavesLeft = 0;
	m_JudgmentFlash = 0.f;
	m_StrikeFlash = 0.f;
	m_BreakPending = false;
}

void LuciusActor::Reward(const ActorContext& ctx)
{
	ctx.status->lightBlessing = true;
	ctx.progress->recognizedByLucius = true;
}

void LuciusActor::DrawJudgment(DrawList& out, const ActorContext& ctx, float fill, float alpha, bool burning) const
{
	float r, g, b;
	if (burning)
	{
		r = Palette::kLightCore.r;
		g = Palette::kLightCore.g;
		b = Palette::kLightCore.b;
		out.SetGlow(1.f);
	}
	else
	{
		AlignmentColor::Warning(Alignment::Light, Palette::kWarningWash, r, g, b);
	}

	float reach = Tuning::Lucius::kJudgmentReach;
	float width = Tuning::Lucius::kJudgmentWidth;
	for (int i = 0; i < JudgmentLines(); i++)
	{
		float lineCol, lineRow;
		JudgmentLine(i, lineCol, lineRow);

		// Laid from the far end, so the strip runs through where he stood.
		float startCol = m_TargetCol - lineCol * reach;
		float startRow = m_TargetRow - lineRow * reach;
		float worldX, worldY, x, y;
		WorldGrid::IsoPosition(startCol, startRow, worldX, worldY);
		ctx.ToScreen(worldX, worldY, x, y);
		float groundY = y + WorldGrid::GroundHeightAt(worldX, worldY) + kTelegraphLift;

		if (burning)
		{
			out.AddGroundStrip(worldX, worldY, x, groundY, lineCol, lineRow, 0.f, reach * 2.f, width, r, g, b, alpha);
			continue;
		}

		// A faint band of the whole line, and a stronger one spreading out
		// from where he stood as it gathers.
		out.AddGroundStrip(worldX, worldY, x, groundY, lineCol, lineRow, 0.f, reach * 2.f, width, r, g, b, alpha);
		out.AddGroundStrip(worldX, worldY, x, groundY, lineCol, lineRow, reach * (1.f - fill), reach * (1.f + fill), width,
			r, g, b, kTelegraphFill);
	}

	out.SetGlow(0.f);
}

void LuciusActor::DrawTelegraph(DrawList& out, const ActorContext& ctx, float x, float groundY) const
{
	float progress = m_StateTime / m_Windup;
	if (progress > 1.f)
	{
		progress = 1.f;
	}

	float r, g, b;
	AlignmentColor::Warning(Alignment::Light, Palette::kWarningWash, r, g, b);
	float baseY = groundY + kTelegraphLift;

	switch (m_Blow)
	{
	case Blow::Judgment:
	{
		DrawJudgment(out, ctx, progress, kTelegraphBase, false);
		break;
	}
	case Blow::Radiance:
	{
		float radius = Tuning::Lucius::kRadianceRadius;
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, 0.f, radius, 0.f, 2.f * kPi, kTelegraphSegments,
			r, g, b, kTelegraphBase);
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, 0.f, radius * progress, 0.f, 2.f * kPi, kTelegraphSegments,
			r, g, b, kTelegraphFill);
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, radius - kTelegraphEdgeWidth, radius, 0.f, 2.f * kPi,
			kTelegraphSegments, r, g, b, kTelegraphEdge);
		break;
	}
	default:
	{
		// The spears are seen in flight; the fan toward him is only hinted at.
		float col, row, playerCol, playerRow;
		WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
		WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
		float center = atan2f(playerRow - row, playerCol - col);
		float half = Tuning::Lucius::kVolleySpread * 0.5f * kDegToRad;
		float inner = Tuning::Lucius::kBodyRadius;
		out.AddGroundArc(WorldX(), WorldY(), x, baseY, inner, inner + (kVolleyMarkReach - inner) * progress,
			center - half, center + half, kTelegraphSegments, r, g, b, kTelegraphBase);
		break;
	}
	}
}

void LuciusActor::DrawSanctuary(DrawList& out, float depth, float x, float groundY) const
{
	float pulse = 0.85f + 0.15f * sinf(Time() * 5.f);
	float r = Palette::kLightGlow.r;
	float g = Palette::kLightGlow.g;
	float b = Palette::kLightGlow.b;

	// Rings stacked up round him and a glow filling them: a dome of light.
	out.SetGlow(0.9f);
	out.AddGroundArc(WorldX(), WorldY(), x, groundY + 2.f, 1.25f, 1.4f, 0.f, 2.f * kPi, kTelegraphSegments, r, g, b, 0.7f * pulse);
	out.AddGroundArc(WorldX(), WorldY(), x, groundY + 34.f, 1.05f, 1.15f, 0.f, 2.f * kPi, kTelegraphSegments, r, g, b, 0.45f * pulse);
	out.AddGroundArc(WorldX(), WorldY(), x, groundY + 62.f, 0.6f, 0.7f, 0.f, 2.f * kPi, kTelegraphSegments, r, g, b, 0.35f * pulse);
	out.AddGlow(depth + 0.2f, x, groundY + 36.f, 90.f, r, g, b, 0.45f * pulse);
	out.SetGlow(0.f);
}

void LuciusActor::DrawBody(DrawList& out, float depth, float x, float groundY, float worldX, float worldY) const
{
	const Palette::Color& armor = Palette::kLuciusArmor;
	const Palette::Color& mantle = Palette::kLuciusMantle;
	const Palette::Color& gold = Palette::kLightCore;

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
	bool reeling = (m_State == State::Stagger);

	float stride = m_Moving ? sinf(m_Gait) * 4.f : 0.f;
	float bob = m_Moving ? fabsf(sinf(m_Gait)) * 1.5f : sinf(Time() * 1.8f) * 0.6f;
	float lean = f * (-windup * 2.f + strike * 3.f) - (reeling ? f * (5.f + sinf(Time() * 18.f) * 1.5f) : 0.f);

	// Down on one knee once he yields.
	float drop = kneel * 12.f;

	out.AddShadow(depth - 0.01f, x, groundY, 48.f, 24.f);
	out.SetFigure(true);

	// The mantle, falling behind him to the ground.
	out.AddTaperedBox(depth - 0.006f, x - f * 6.f, groundY + 2.f, 0.42f, 0.2f, 40.f - drop, 0.55f, lean * 0.3f, 0.f,
		mantle.r * 0.9f, mantle.g * 0.9f, mantle.b * 0.9f);

	// Long legs in greaves; kneeling, the back one folds under him.
	const float kLegHeight = 18.f;
	out.AddTaperedBox(depth, x - 6.f + stride, groundY, 0.17f, 0.17f, kLegHeight - drop * 0.4f,
		0.9f, 0.f, 0.f, armor.r * 0.85f, armor.g * 0.85f, armor.b * 0.85f);
	out.AddTaperedBox(depth, x + 6.f - stride + f * kneel * 6.f, groundY, 0.17f, 0.17f, kLegHeight - drop,
		0.9f, 0.f, 0.f, armor.r * 0.85f, armor.g * 0.85f, armor.b * 0.85f);

	// A long skirt of plates, then a tall breastplate flaring to the shoulders.
	float waist = groundY + kLegHeight - drop + bob;
	out.AddTaperedBox(depth + 0.01f, x + lean * 0.3f, waist - 6.f, 0.36f, 0.3f, 9.f, 0.8f, lean * 0.3f, 0.f,
		armor.r * 0.95f, armor.g * 0.95f, armor.b * 0.95f);
	const float kTorsoHeight = 24.f;
	out.AddTaperedBox(depth + 0.015f, x + lean * 0.3f, waist + 3.f, 0.34f, 0.28f, kTorsoHeight, 1.2f, lean, 0.f,
		armor.r, armor.g, armor.b);

	// A sun on his breast, still burning.
	out.SetGlow(0.6f);
	out.AddDiamond(depth + 0.02f, x + f * 2.f + lean * 0.6f, waist + 3.f + kTorsoHeight * 0.55f, 8.f, 10.f,
		gold.r, gold.g, gold.b, 1.f);
	out.SetGlow(0.f);

	// Great rounded pauldrons, edged in gold.
	float shoulderY = waist + kTorsoHeight;
	for (int side = -1; side <= 1; side += 2)
	{
		float px = x + side * 15.f + lean;
		out.AddTaperedBox(depth + 0.02f, px, shoulderY - 2.f, 0.2f, 0.2f, 6.f, 0.7f, 0.f, 0.f,
			kSteel.r, kSteel.g, kSteel.b);
		out.AddRect(depth + 0.021f, px, shoulderY - 2.f, 12.f, 1.5f, gold.r, gold.g, gold.b, 1.f);
	}

	// The head bowed when he kneels, bare but for a crown of gold.
	float headX = x + f * (1.f + kneel * 4.f) + lean * 1.1f;
	float headBase = shoulderY + 1.f - kneel * 3.f;
	out.AddTaperedBox(depth + 0.03f, headX, headBase, 0.2f, 0.2f, 12.f, 0.9f, 0.f, 0.f,
		kFace.r, kFace.g, kFace.b);
	out.AddRect(depth + 0.04f, headX + f * 3.f, headBase + 7.f, 3.f, 1.5f, 0.15f, 0.13f, 0.12f, 1.f);
	for (int i = -1; i <= 1; i++)
	{
		out.AddTaperedBox(depth + 0.035f, headX + i * 4.f, headBase + 11.f, 0.05f, 0.05f, (i == 0) ? 7.f : 5.f, 0.2f, 0.f, 0.f,
			gold.r, gold.g, gold.b);
	}

	// The halo over the crown: whole and bright, as no eroded's is.
	out.SetFigure(false);
	out.SetGlow(0.9f);
	out.AddGroundArc(worldX, worldY, headX, headBase + 22.f, 0.34f, 0.44f, 0.f, 2.f * kPi, 18,
		gold.r, gold.g, gold.b, 0.85f);
	out.SetGlow(0.f);
	out.SetFigure(true);

	// The spear: upright at his side at rest, drawn back to throw as a volley
	// gathers, raised to the sky as he calls judgment, planted when he kneels.
	float handX = x + f * 14.f + lean;
	float handY = shoulderY - 10.f;
	float angle = 88.f;
	float back = 26.f;
	if (kneel > 0.f)
	{
		angle = 90.f;
		handY = groundY + 22.f;
		back = 22.f;
	}
	else if (m_State == State::Windup && m_Blow == Blow::Volley)
	{
		angle = 88.f - windup * 70.f;
		handX -= f * windup * 8.f;
		handY += windup * 8.f;
	}
	else if (m_State == State::Windup && m_Blow == Blow::Judgment)
	{
		handY += windup * 18.f;
	}
	else if (m_State == State::Volley)
	{
		angle = 20.f;
	}

	float tipX, tipY;
	Shaft(out, depth + 0.05f, handX, handY, f, angle, back, 40.f, 3.f, kSteel, tipX, tipY);
	out.SetGlow(0.8f);
	out.AddDiamond(depth + 0.055f, tipX, tipY, 6.f, 11.f, gold.r, gold.g, gold.b, 1.f);
	out.SetGlow(0.f);
}

void LuciusActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	// Judgment reaches far from him, so its mark is drawn even when he is not
	// on screen.
	bool fighting = IsAwake() && !HasYielded();
	if (fighting && m_State == State::Windup)
	{
		DrawTelegraph(out, ctx, x, groundY);
	}
	if (fighting && m_JudgmentFlash > 0.f)
	{
		DrawJudgment(out, ctx, 1.f, m_JudgmentFlash, true);
	}

	if (!ctx.IsOnScreen(x, y, 160.f, 220.f))
	{
		return;
	}

	DrawFury(out, depth - 0.005f, x, groundY, 70.f);

	if (fighting && m_State == State::Windup && m_Blow == Blow::Judgment)
	{
		// Light gathering over his raised spear.
		float windup = m_StateTime / m_Windup;
		out.SetGlow(1.f);
		out.AddGlow(depth + 0.2f, x, groundY + 100.f, 50.f * windup, Palette::kLightGlow.r, Palette::kLightGlow.g,
			Palette::kLightGlow.b, windup);
		out.SetGlow(0.f);
	}

	if (HitFlash() > 0.f)
	{
		out.SetFlash(HitFlash() * 0.75f);
	}
	DrawBody(out, depth, x, groundY, WorldX(), WorldY());

	if (fighting && m_State == State::Sanctuary)
	{
		out.SetFigure(false);
		DrawSanctuary(out, depth, x, groundY);
	}
}
