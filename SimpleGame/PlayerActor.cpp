#include "stdafx.h"
#include "PlayerActor.h"
#include "DrawList.h"
#include "PlayerStatus.h"
#include "WorldGrid.h"
#include "Tuning.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "AfterimageActor.h"
#include "BurstActor.h"
#include "Combat.h"
#include "Growth.h"
#include <cmath>

namespace
{
	const float kGlowRadius = 46.f;
	// How much of his glow is gone by the time the gray reaches the allowance.
	const float kAshGlowLoss = 0.7f;

	// The roll: he folds down to this fraction of his height at its middle and
	// pitches his upper body this many pixels toward where he is going.
	const float kRollSquash = 0.45f;
	const float kRollLean = 7.f;
	// Light thrown off at either end of a blink, or as a shadow dash sets off.
	const int kBlinkSparks = 10;
	const float kBlinkSparkSpeed = 160.f;
	const float kBlinkSparkLift = 20.f;
	// A puff of dust kicked up as the roll starts.
	const int kRollDust = 8;
	const float kRollDustSpeed = 60.f;
	const float kRollDustLift = 2.f;

	// Dashing: pitched forward this far, a little crouched.
	const float kLungeLean = 9.f;
	const float kLungeSquash = 0.88f;

	// The sword in the ground: crouched over it.
	const float kPlantSquash = 0.82f;

	// How long each move his body makes lasts.
	const float kCastTime = 0.18f;
	const float kBeamTime = 0.4f;
	const float kCallTime = 0.45f;
	const float kSlashTime = 0.18f;
	const float kSweepTime = 0.26f;
	const float kFinisherTime = 0.3f;
	const float kThrustTime = 0.22f;
	const float kSwapTime = 0.3f;

	// Arms as limbs from shoulder to hand, wide at the shoulder.
	const float kShoulderWidth = 5.f;
	const float kWristWidth = 3.5f;
	const float kSwingBladeLength = 26.f;
	const float kPi = 3.14159265f;

	// How fast he turns to face the cursor.
	const float kTurnRate = 14.f;       // radians per second
	const float kTwoPi = 6.2831853f;

	// Fallen: slumped this far down and hunched toward the side he faces.
	const float kFallSquash = 0.4f;
	const float kFallLean = 4.f;

	// Fast enough that a single terrace is cleared in a stride, slow enough to
	// read as climbing rather than snapping.
	const float kClimbRate = 14.f;

	// Radians of walk cycle per tile walked. One full stride every ~1.2 tiles.
	const float kStrideRate = 5.2f;
	const float kStrideSwing = 3.5f;  // how far a leg swings on screen, in pixels
	const float kStrideLift = 2.5f;   // how high the trailing foot comes up
	const float kWalkBob = 1.8f;
	const float kBreathBob = 0.7f;
	const float kBreathRate = 2.4f;

	// Look direction only flips once the cursor is clearly to one side, so it
	// does not flicker while aiming straight up or down.
	const float kFacingDeadZone = 16.f;

	// The cloak down his back: how far its hem trails behind him while he
	// walks, and how much it swings with each stride.
	const float kCloakTrail = 4.f;
	const float kCloakFlutter = 1.5f;
	// How wide it is seen flat on, and how deep the coloured band at its hem.
	const float kCloakBreadth = 22.f;
	const float kCloakHem = 3.5f;

	// The aura of the form he is in: a haze round him, and wisps rising off
	// him and fading as they go.
	const float kAuraRadius = 38.f;
	const float kAuraStrength = 0.45f;
	const int kAuraWisps = 6;
	const float kWispRate = 0.7f;   // rises per second
	const float kWispRise = 46.f;   // pixels
	const float kWispSpread = 11.f; // pixels either side

	// Light form: shards circling the hand. Dark form: a blade of shadow.
	const int kShards = 3;
	const float kShardOrbit = 7.f;
	const float kShardRate = 3.2f;
	const float kBladeLength = 22.f;
	const float kBladeWidth = 3.4f;

	const float kLegsHeight = 12.f;
	const float kTorsoHeight = 18.f;
	const float kArmHeight = 15.f;
	const float kHeadHeight = 11.f;

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

	float ActionLength(PlayerActor::Action action)
	{
		switch (action)
		{
		case PlayerActor::Action::Cast:
		{
			return kCastTime;
		}
		case PlayerActor::Action::Beam:
		{
			return kBeamTime;
		}
		case PlayerActor::Action::Call:
		{
			return kCallTime;
		}
		case PlayerActor::Action::Slash:
		{
			return kSlashTime;
		}
		case PlayerActor::Action::Sweep:
		{
			return kSweepTime;
		}
		case PlayerActor::Action::Finisher:
		{
			return kFinisherTime;
		}
		case PlayerActor::Action::Thrust:
		{
			return kThrustTime;
		}
		default:
		{
			return kSwapTime;
		}
		}
	}

	// A straight piece of him between two points on screen, from one width to
	// another - an arm.
	void Limb(DrawList& out, float depth, float x0, float y0, float x1, float y1, float width0, float width1,
		const Palette::Color& c)
	{
		float dx = x1 - x0;
		float dy = y1 - y0;
		float length = sqrtf(dx * dx + dy * dy);
		if (length < 0.001f)
		{
			return;
		}
		float sideX = -dy / length * 0.5f;
		float sideY = dx / length * 0.5f;
		float corners[8] =
		{
			x0 - sideX * width0, y0 - sideY * width0,
			x1 - sideX * width1, y1 - sideY * width1,
			x1 + sideX * width1, y1 + sideY * width1,
			x0 + sideX * width0, y0 + sideY * width0,
		};
		out.AddCorners(depth, corners, c.r, c.g, c.b, 1.f);
	}

	// A straight blade from the hand, held point-down and forward, narrowing
	// to the tip.
	void HeldBlade(DrawList& out, float depth, float handX, float handY, float facing, float length, float width,
		const Palette::Color& c, float& tipX, float& tipY)
	{
		tipX = handX + facing * length * 0.45f;
		tipY = handY - length * 0.9f;
		float dirX = tipX - handX;
		float dirY = tipY - handY;
		float span = sqrtf(dirX * dirX + dirY * dirY);
		float sideX = -dirY / span * width * 0.5f;
		float sideY = dirX / span * width * 0.5f;
		float corners[8] =
		{
			handX - sideX, handY - sideY,
			tipX - sideX * 0.2f, tipY - sideY * 0.2f,
			tipX + sideX * 0.2f, tipY + sideY * 0.2f,
			handX + sideX, handY + sideY,
		};
		out.AddCorners(depth, corners, c.r, c.g, c.b, 1.f);
	}

	// Drains a colour toward a flat gray of the same brightness - how the
	// gray shows on him as his balance goes.
	void Ashen(float amount, float& r, float& g, float& b)
	{
		float gray = (r + g + b) / 3.f;
		r += (gray - r) * amount;
		g += (gray - g) * amount;
		b += (gray - b) * amount;
	}

	// Screen directions converted onto the grid axes, so one key walks straight
	// along a screen axis and two adjacent keys blend into a screen diagonal.
	// Normalized, so holding two directions is a diagonal, not a sprint.
	bool MoveDirection(const InputState& input, float& outCol, float& outRow)
	{
		float col = input.moveRight - input.moveUp;
		float row = -input.moveRight - input.moveUp;

		float length = sqrtf(col * col + row * row);
		if (length < 0.001f)
		{
			return false;
		}

		outCol = col / length;
		outRow = row / length;
		return true;
	}
}

void PlayerActor::Respawn(float x, float y)
{
	SetLocalPosition(x, y);

	// Snap to the new ground instead of climbing to it from where he fell.
	m_GroundInitialized = false;

	m_DodgeTimer = 0.f;
	m_DodgeCooldown = 0.f;
	m_LungeTimer = 0.f;
	m_Action = Action::None;
	m_ActionTimer = 0.f;
	m_LightMoves = LightFormMoves();
	m_DarkMoves = DarkFormMoves();
	m_AshMoves = AshFormMoves();
}

void PlayerActor::SettleForm(PlayerStatus& status, Form form)
{
	if (status.form == form)
	{
		return;
	}
	MovesFor(status.form).Interrupt();
	status.form = form;
}

void PlayerActor::Lunge(float dirCol, float dirRow, float tiles, float seconds, bool trail)
{
	m_LungeDirCol = dirCol;
	m_LungeDirRow = dirRow;
	m_LungeSpeed = tiles / seconds;
	m_LungeTimer = seconds;
	m_LungeTrail = trail;
}

void PlayerActor::Act(Action action, float aimX, float aimY)
{
	float length = sqrtf(aimX * aimX + aimY * aimY);
	if (length > 0.001f)
	{
		m_ActionX = aimX / length;
		m_ActionY = aimY / length;
		m_Heading = atan2f(m_ActionY, m_ActionX);
		if (m_Heading < 0.f)
		{
			m_Heading += kTwoPi;
		}
	}
	m_Action = action;
	m_ActionLength = ActionLength(action);
	m_ActionTimer = m_ActionLength;
	// The quick cut always goes one way and the sweep the other.
	if (action == Action::Slash)
	{
		m_SlashSide = 1.f;
	}
	else if (action == Action::Sweep)
	{
		m_SlashSide = -1.f;
	}
}

bool PlayerActor::IsTravelling() const
{
	return m_DodgeTimer > 0.f || m_LungeTimer > 0.f;
}

void PlayerActor::OnUpdate(const ActorContext& ctx)
{
	ctx.status->Tick(ctx.deltaTime);
	m_Time += ctx.deltaTime;

	if (m_DodgeCooldown > 0.f)
	{
		m_DodgeCooldown -= ctx.deltaTime;
	}
	if (ctx.input.dodge && !IsTravelling() && m_DodgeCooldown <= 0.f)
	{
		StartDodge(ctx);
	}

	m_Moving = false;
	if (m_DodgeTimer > 0.f)
	{
		Roll(ctx);
	}
	else if (m_LungeTimer > 0.f)
	{
		LungeStep(ctx);
	}
	else if (!m_DarkMoves.IsPlanting())
	{
		Move(ctx);
	}

	float ground = WorldGrid::GroundHeightAt(WorldX(), WorldY());
	if (!m_GroundInitialized)
	{
		m_GroundHeight = ground;
		m_GroundInitialized = true;
	}
	else
	{
		m_GroundHeight = WorldGrid::SmoothApproach(m_GroundHeight, ground, kClimbRate, ctx.deltaTime);
	}

	if (ctx.input.aimX > kFacingDeadZone)
	{
		m_Facing = 1.f;
	}
	else if (ctx.input.aimX < -kFacingDeadZone)
	{
		m_Facing = -1.f;
	}

	// Mid-roll or mid-dash he faces where he is going, not where the cursor is.
	if (IsTravelling())
	{
		float dirCol = (m_DodgeTimer > 0.f) ? m_DodgeDirCol : m_LungeDirCol;
		float dirRow = (m_DodgeTimer > 0.f) ? m_DodgeDirRow : m_LungeDirRow;

		float screenX, screenY;
		WorldGrid::IsoPosition(dirCol, dirRow, screenX, screenY);
		if (fabsf(screenX) > 0.01f)
		{
			m_Facing = (screenX > 0.f) ? 1.f : -1.f;
		}
	}

	// He turns toward the cursor - or where he is going, mid-roll or
	// mid-dash - rather than snapping round; with his sword in the ground
	// he holds still.
	float wantX = ctx.input.aimX;
	float wantY = ctx.input.aimY;
	if (IsTravelling())
	{
		float dirCol = (m_DodgeTimer > 0.f) ? m_DodgeDirCol : m_LungeDirCol;
		float dirRow = (m_DodgeTimer > 0.f) ? m_DodgeDirRow : m_LungeDirRow;
		WorldGrid::IsoPosition(dirCol, dirRow, wantX, wantY);
		wantX *= 100.f;
		wantY *= 100.f;
	}
	if (!m_DarkMoves.IsPlanting() && wantX * wantX + wantY * wantY > kFacingDeadZone * kFacingDeadZone)
	{
		float turn = atan2f(wantY, wantX) - m_Heading;
		while (turn > kTwoPi * 0.5f)
		{
			turn -= kTwoPi;
		}
		while (turn < -kTwoPi * 0.5f)
		{
			turn += kTwoPi;
		}
		float most = kTurnRate * ctx.deltaTime;
		m_Heading += Clamp(turn, -most, most);
	}
	m_Heading = fmodf(m_Heading + kTwoPi, kTwoPi);

	if (m_ActionTimer > 0.f)
	{
		m_ActionTimer -= ctx.deltaTime;
		if (m_ActionTimer <= 0.f)
		{
			m_Action = Action::None;
		}
	}

	m_LightMoves.Update(ctx, *this);
	m_DarkMoves.Update(ctx, *this);
	m_AshMoves.Update(ctx, *this);

	if (ctx.input.switchForm)
	{
		Form leaving = ctx.status->form;
		if (ctx.status->TrySwitchForm())
		{
			MovesFor(leaving).Interrupt();
			// The burst as he turns over is learned, not born with.
			if (Growth::Has(*ctx.status, Tuning::Growth::kSwap))
			{
				MovesFor(ctx.status->form).Enter(ctx, *this);
			}
		}
	}

	// Nothing is thrown or swung mid-roll or mid-dash.
	if (!IsTravelling())
	{
		FormMoves& moves = MovesFor(ctx.status->form);
		if (ctx.input.attack)
		{
			moves.Attack(ctx, *this);
		}
		if (ctx.input.skill)
		{
			moves.Skill(ctx, *this);
		}
		if (ctx.input.special)
		{
			moves.Special(ctx, *this);
		}
	}
}

FormMoves& PlayerActor::MovesFor(Form form)
{
	switch (form)
	{
	case Form::Light:
	{
		return m_LightMoves;
	}
	case Form::Dark:
	{
		return m_DarkMoves;
	}
	default:
	{
		return m_AshMoves;
	}
	}
}

void PlayerActor::Move(const ActorContext& ctx)
{
	float dirCol, dirRow;
	if (!MoveDirection(ctx.input, dirCol, dirRow))
	{
		return;
	}

	// Stepped in tiles rather than pixels, so he covers the same ground per
	// second in every direction instead of appearing to sprint north.
	float speed = Tuning::Player::kMoveSpeed;
	if (ctx.input.run)
	{
		speed *= Tuning::Player::kRunMultiplier;
	}
	float step = speed * ctx.deltaTime;

	StepOnGrid(dirCol, dirRow, step);

	m_Moving = true;
	m_WalkPhase += step * kStrideRate;

	float strideX, strideY;
	WorldGrid::IsoPosition(dirCol, dirRow, strideX, strideY);
	float strideLength = sqrtf(strideX * strideX + strideY * strideY);
	if (strideLength > 0.001f)
	{
		m_StrideX = strideX / strideLength;
		m_StrideY = strideY / strideLength;
	}
}

void PlayerActor::StartDodge(const ActorContext& ctx)
{
	// Rolls the way he is walking; standing still, toward the cursor; with the
	// cursor on top of him, the way he is facing.
	if (!MoveDirection(ctx.input, m_DodgeDirCol, m_DodgeDirRow)
		&& !WorldGrid::ScreenToGroundDirection(ctx.input.aimX, ctx.input.aimY, m_DodgeDirCol, m_DodgeDirRow))
	{
		WorldGrid::ScreenToGroundDirection(m_Facing, 0.f, m_DodgeDirCol, m_DodgeDirRow);
	}

	// From the first upgrade, each form dodges its own way; the gray, and
	// anyone before then, rolls.
	m_DodgeCooldown = Tuning::Dodge::kCooldown;
	if (Growth::Has(*ctx.status, Tuning::Growth::kFormDodge) && ctx.status->form == Form::Light)
	{
		Blink(ctx);
		return;
	}
	if (Growth::Has(*ctx.status, Tuning::Growth::kFormDodge) && ctx.status->form == Form::Dark)
	{
		ShadowDash(ctx);
		return;
	}

	SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dust,
		kRollDust, kRollDustSpeed, kRollDustLift);

	m_DodgeTimer = Tuning::Dodge::kDuration;
	ctx.status->GrantInvuln(Tuning::Dodge::kInvulnDuration);
}

void PlayerActor::Blink(const ActorContext& ctx)
{
	float fromX = WorldX();
	float fromY = WorldY();

	// His outline left where he was, and light at both ends.
	std::unique_ptr<AfterimageActor> echo(new AfterimageActor(Alignment::Light, cosf(m_Heading)));
	echo->SetLocalPosition(fromX, fromY);
	ctx.graph->Spawn(std::move(echo));
	SparkActor::Burst(*ctx.graph, fromX, fromY, SparkActor::Style::Light, kBlinkSparks, kBlinkSparkSpeed, kBlinkSparkLift);

	// Checked in pieces, so a wall stops him at it rather than past it.
	float step = Tuning::Dodge::kBlinkDistance / Tuning::Dodge::kBlinkSteps;
	for (int i = 0; i < Tuning::Dodge::kBlinkSteps; i++)
	{
		StepOnGrid(m_DodgeDirCol, m_DodgeDirRow, step);
	}
	SparkActor::Burst(*ctx.graph, LocalX(), LocalY(), SparkActor::Style::Light, kBlinkSparks, kBlinkSparkSpeed, kBlinkSparkLift);
	ctx.status->GrantInvuln(Tuning::Dodge::kBlinkInvuln);

	// Grown stronger, where he was bursts.
	if (Growth::Has(*ctx.status, Tuning::Growth::kDodgeII))
	{
		Combat::HitRadius(*ctx.graph, fromX, fromY, Tuning::Dodge::kBlinkBurstRadius, Combat::BlinkBurstDamage(*ctx.status), 0.f);
		std::unique_ptr<BurstActor> burst(new BurstActor(Alignment::Light, Tuning::Dodge::kBlinkBurstRadius, false));
		burst->SetLocalPosition(fromX, fromY);
		ctx.graph->Spawn(std::move(burst));
	}
}

void PlayerActor::ShadowDash(const ActorContext& ctx)
{
	SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dark, kBlinkSparks, kBlinkSparkSpeed, kBlinkSparkLift);

	// Grown stronger, everything along the way is cut - and a share of it
	// drunk, as from any dark blow in reach.
	if (Growth::Has(*ctx.status, Tuning::Growth::kDodgeII))
	{
		float landed = Combat::HitLine(*ctx.graph, WorldX(), WorldY(), m_DodgeDirCol, m_DodgeDirRow,
			Tuning::Dodge::kShadowDashDistance, Tuning::Dodge::kShadowDashWidth, Combat::ShadowDashDamage(*ctx.status), false);
		ctx.status->Heal(Combat::Lifesteal(*ctx.status, landed));
	}

	Lunge(m_DodgeDirCol, m_DodgeDirRow, Tuning::Dodge::kShadowDashDistance, Tuning::Dodge::kShadowDashTime);
	ctx.status->GrantInvuln(Tuning::Dodge::kShadowDashTime + Tuning::Dodge::kBlinkInvuln);
}

void PlayerActor::Roll(const ActorContext& ctx)
{
	// The last frame only covers what is left, so every roll travels exactly
	// the same distance whatever the frame rate.
	float time = (ctx.deltaTime < m_DodgeTimer) ? ctx.deltaTime : m_DodgeTimer;
	float speed = Tuning::Dodge::kDistance / Tuning::Dodge::kDuration;

	StepOnGrid(m_DodgeDirCol, m_DodgeDirRow, speed * time);

	m_DodgeTimer -= ctx.deltaTime;
}

void PlayerActor::LungeStep(const ActorContext& ctx)
{
	// Like the roll, the last frame covers only what is left.
	float time = (ctx.deltaTime < m_LungeTimer) ? ctx.deltaTime : m_LungeTimer;

	StepOnGrid(m_LungeDirCol, m_LungeDirRow, m_LungeSpeed * time);

	// His shadow left behind along the way.
	m_AfterimageTimer -= ctx.deltaTime;
	if (m_LungeTrail && m_AfterimageTimer <= 0.f)
	{
		std::unique_ptr<AfterimageActor> shadow(new AfterimageActor(Alignment::Dark, cosf(m_Heading)));
		shadow->SetLocalPosition(WorldX(), WorldY());
		ctx.graph->Spawn(std::move(shadow));
		m_AfterimageTimer = Tuning::DarkForm::kAfterimageGap;
	}

	m_LungeTimer -= ctx.deltaTime;
}

void PlayerActor::StepOnGrid(float dirCol, float dirRow, float tiles)
{
	float col, row;
	WorldGrid::InverseIso(LocalX(), LocalY(), col, row);

	WorldGrid::Move(col, row, dirCol * tiles, dirRow * tiles);

	float x, y;
	WorldGrid::IsoPosition(col, row, x, y);
	SetLocalPosition(x, y);
}

void PlayerActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);

	float groundY = y + m_GroundHeight;
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	// The two halves of the vessel spill onto the ground as their own light, so
	// how full each side is can be read without looking at the HUD.
	float lightGlowR, lightGlowG, lightGlowB;
	AlignmentColor::Glow(Alignment::Light, lightGlowR, lightGlowG, lightGlowB);
	// As the gray creeps in, the powers he carries gutter out.
	float ash = ctx.status->WarningLevel();
	float glowStrength = 1.f - ash * kAshGlowLoss;

	out.AddGlow(depth - 0.02f, x, groundY, kGlowRadius, lightGlowR, lightGlowG, lightGlowB,
		ctx.status->light / ctx.status->capacity * glowStrength);

	float darkGlowR, darkGlowG, darkGlowB;
	AlignmentColor::Glow(Alignment::Dark, darkGlowR, darkGlowG, darkGlowB);
	out.AddGlow(depth - 0.02f, x, groundY, kGlowRadius, darkGlowR, darkGlowG, darkGlowB,
		ctx.status->dark / ctx.status->capacity * glowStrength);

	// The aura of his form: gold that shines, violet that darkens what is
	// under it, or the gray's dull haze - fading as the gray creeps in.
	Alignment formPower = FormAlignment(ctx.status->form);
	float auraR, auraG, auraB, hazeR, hazeG, hazeB;
	AlignmentColor::Core(formPower, auraR, auraG, auraB);
	AlignmentColor::Glow(formPower, hazeR, hazeG, hazeB);
	float auraPulse = (0.85f + 0.15f * sinf(m_Time * 3.f)) * glowStrength;
	out.SetGlow(formPower == Alignment::Light ? 0.5f : 0.f);
	out.AddGlow(depth - 0.015f, x, groundY + 18.f, kAuraRadius, hazeR, hazeG, hazeB, kAuraStrength * auraPulse);
	for (int i = 0; i < kAuraWisps; i++)
	{
		float rise = fmodf(m_Time * kWispRate + i / (float)kAuraWisps, 1.f);
		float wispX = x + sinf(i * 2.4f + m_Time * 1.7f) * kWispSpread;
		float wispY = groundY + 4.f + rise * kWispRise;
		float fade = (1.f - rise) * Clamp(rise * 4.f, 0.f, 1.f) * 0.7f * glowStrength;
		float front = (i % 2 == 0) ? 0.06f : -0.005f;
		out.AddDiamond(depth + front, wispX, wispY, 2.5f, 5.f, auraR, auraG, auraB, fade);
	}
	out.SetGlow(0.f);

	out.AddShadow(depth - 0.01f, x, groundY, 26.f, 13.f);

	// Everything from here to the effects is his body.
	out.SetFigure(true);

	// Walking: legs scissor, the trailing foot lifts, and the body bobs with
	// each stride. Standing: only a slow breath.
	float swing = 0.f;
	float leftLift = 0.f;
	float rightLift = 0.f;
	float bob = sinf(m_Time * kBreathRate) * kBreathBob;
	if (m_Moving)
	{
		float phase = sinf(m_WalkPhase);
		swing = phase * kStrideSwing;
		leftLift = (phase > 0.f) ? phase * kStrideLift : 0.f;
		rightLift = (phase < 0.f) ? -phase * kStrideLift : 0.f;
		bob = fabsf(phase) * kWalkBob;
	}

	// Rolling: folds down and pitches toward where he is going, deepest at the
	// middle of the roll. The legs stay planted; everything above them leans.
	float squash = 1.f;
	float lean = 0.f;
	if (m_DodgeTimer > 0.f)
	{
		float progress = 1.f - m_DodgeTimer / Tuning::Dodge::kDuration;
		float fold = sinf(progress * 3.14159265f);

		float dirX, dirY;
		WorldGrid::IsoPosition(m_DodgeDirCol, m_DodgeDirRow, dirX, dirY);
		float dirLength = sqrtf(dirX * dirX + dirY * dirY);

		squash = 1.f - kRollSquash * fold;
		lean = (dirX / dirLength) * kRollLean * fold;
		swing = 0.f;
		leftLift = 0.f;
		rightLift = 0.f;
		bob = 0.f;
	}

	if (m_LungeTimer > 0.f)
	{
		float dirX, dirY;
		WorldGrid::IsoPosition(m_LungeDirCol, m_LungeDirRow, dirX, dirY);
		float dirLength = sqrtf(dirX * dirX + dirY * dirY);

		squash = kLungeSquash;
		lean = (dirX / dirLength) * kLungeLean;
		swing = 0.f;
		leftLift = 0.f;
		rightLift = 0.f;
		bob = 0.f;
	}

	if (m_DarkMoves.IsPlanting())
	{
		squash = kPlantSquash;
		swing = 0.f;
		leftLift = 0.f;
		rightLift = 0.f;
		bob = 0.f;
	}

	if (ctx.status->health <= 0.f)
	{
		squash = 1.f - kFallSquash;
		lean = m_Facing * kFallLean;
		swing = 0.f;
		leftLift = 0.f;
		rightLift = 0.f;
		bob = 0.f;
	}

	// Which way he looks: side runs from -1 (screen left) to 1 (right),
	// toward from -1 (his back to us) to 1 (facing us).
	float side = cosf(m_Heading);
	float toward = -sinf(m_Heading);
	float upperX = x + lean;

	// The move he is making: how far through it (0..1), and a swell that
	// rises and falls over it. Each throws his weight, and his hands, its
	// own way - toward where he aimed it.
	float act = 0.f;
	float swell = 0.f;
	float hold = 0.f; // up fast, held, down fast
	float hop = 0.f;
	float rightReachX = 0.f;
	float rightReachY = 0.f;
	float leftReachX = 0.f;
	float leftReachY = 0.f;
	if (m_Action != Action::None && ctx.status->health > 0.f)
	{
		act = 1.f - m_ActionTimer / m_ActionLength;
		swell = sinf(act * kPi);
		hold = Clamp(act * 5.f, 0.f, 1.f) * Clamp((1.f - act) * 5.f, 0.f, 1.f);
		float aimX = m_ActionX;
		float aimY = m_ActionY;
		switch (m_Action)
		{
		case Action::Cast:
		{
			// Out and back: thrown forward, then the recoil.
			lean += aimX * 3.f * sinf(act * kPi * 2.f);
			rightReachX = aimX * 11.f * swell;
			rightReachY = aimY * 6.f * swell + 7.f * swell;
			break;
		}
		case Action::Beam:
		{
			lean -= aimX * 4.f * hold;
			rightReachX = aimX * 12.f * hold;
			rightReachY = aimY * 6.f * hold + 8.f * hold;
			leftReachX = rightReachX;
			leftReachY = rightReachY;
			break;
		}
		case Action::Call:
		{
			squash *= 1.f + 0.06f * hold;
			rightReachX = aimX * 3.f * hold;
			rightReachY = 30.f * hold;
			break;
		}
		case Action::Slash:
		{
			lean += aimX * 5.f * swell;
			rightReachX = aimX * 7.f * swell;
			rightReachY = aimY * 4.f * swell + 6.f * swell;
			break;
		}
		case Action::Sweep:
		{
			// The whole body turns into it, low.
			lean += aimX * 3.f * swell + m_SlashSide * 4.f * sinf(act * kPi * 2.f);
			squash *= 1.f - 0.07f * swell;
			rightReachX = aimX * 9.f * swell - m_SlashSide * 6.f * (1.f - act * 2.f) * swell;
			rightReachY = aimY * 4.f * swell + 4.f * swell;
			leftReachX = -aimX * 5.f * swell;
			leftReachY = 5.f * swell;
			break;
		}
		case Action::Finisher:
		{
			lean += aimX * 7.f * swell;
			hop = 8.f * swell;
			rightReachX = aimX * 8.f * swell;
			rightReachY = 14.f * (1.f - act) * swell;
			break;
		}
		case Action::Thrust:
		{
			lean += aimX * 6.f * swell;
			rightReachX = aimX * 12.f * swell;
			rightReachY = aimY * 6.f * swell + 7.f * swell;
			break;
		}
		default:
		{
			// Flung wide as the power turns over in him.
			squash *= 1.f - 0.08f * swell;
			rightReachX = -toward * 9.f * swell;
			rightReachY = 10.f * swell;
			leftReachX = toward * 9.f * swell;
			leftReachY = 10.f * swell;
			break;
		}
		}
		upperX = x + lean;
	}
	groundY += hop;

	float legsHeight = kLegsHeight * squash;
	float torsoHeight = kTorsoHeight * squash;
	float armHeight = kArmHeight * squash;
	float headHeight = kHeadHeight * (1.f - (1.f - squash) * 0.3f);

	const Palette::Color& cloak = Palette::kCloak;
	const Palette::Color& cloakShade = Palette::kCloakShade;
	const Palette::Color& trousers = Palette::kTrousers;
	const Palette::Color& boot = Palette::kBoot;
	const Palette::Color& skin = Palette::kSkin;

	// Boots and legs.
	float leftX = x - 4.f + swing;
	float rightX = x + 4.f - swing;
	out.AddTaperedBox(depth, leftX, groundY + leftLift, 0.14f, 0.14f, 3.f, 0.8f, 0.f, 0.f, boot.r, boot.g, boot.b);
	out.AddTaperedBox(depth, rightX, groundY + rightLift, 0.14f, 0.14f, 3.f, 0.8f, 0.f, 0.f, boot.r, boot.g, boot.b);
	out.AddTaperedBox(depth + 0.005f, leftX, groundY + leftLift + 3.f, 0.12f, 0.12f, legsHeight - 3.f,
		0.85f, -swing * 0.25f, 0.f, trousers.r, trousers.g, trousers.b);
	out.AddTaperedBox(depth + 0.005f, rightX, groundY + rightLift + 3.f, 0.12f, 0.12f, legsHeight - 3.f,
		0.85f, swing * 0.25f, 0.f, trousers.r, trousers.g, trousers.b);

	// A tunic over his body, flaring to the shoulders, and a belt.
	const Palette::Color& tunic = Palette::kTunic;
	float torsoBase = groundY + legsHeight + bob;
	float shoulderY = torsoBase + torsoHeight * 0.82f;
	out.AddTaperedBox(depth + 0.01f, upperX, torsoBase, 0.24f, 0.22f, torsoHeight, 1.25f, lean * 0.6f, 0.f,
		tunic.r, tunic.g, tunic.b);
	out.AddTaperedBox(depth + 0.012f, upperX + lean * 0.1f, torsoBase + 2.f, 0.25f, 0.23f, 2.5f, 1.f, 0.f, 0.f,
		boot.r, boot.g, boot.b);

	// The cloak hangs from his shoulders down his back: seen whole when his
	// back is to us, only at its edges behind him when he faces us, and as a
	// narrow fall down his back when he is side-on. Its hem trails behind him
	// as he walks and swings with each stride.
	float trail = m_Moving ? -m_StrideX * kCloakTrail : 0.f;
	float flutter = m_Moving ? sinf(m_WalkPhase * 2.f) * kCloakFlutter : sinf(m_Time * 1.3f) * 0.4f;
	float hemY = groundY + 3.f + bob * 0.5f - (m_Moving ? m_StrideY * 1.5f : 0.f);
	float breadth = kCloakBreadth * (0.4f + 0.6f * fabsf(toward));
	float topX = upperX - side * 3.f + lean * 0.6f;
	float hemX = upperX - side * 5.f + trail + flutter;
	float topHalf = breadth * 0.42f;
	float hemHalf = breadth * 0.5f;
	bool backToUs = (toward < 0.f);
	float cloakDepth = depth + (backToUs ? 0.028f : 0.002f);
	const Palette::Color& cloakFace = backToUs ? cloak : cloakShade;
	float cloakCorners[8] =
	{
		topX - topHalf, shoulderY,
		hemX - hemHalf, hemY,
		hemX + hemHalf, hemY,
		topX + topHalf, shoulderY,
	};
	out.AddCorners(cloakDepth, cloakCorners, cloakFace.r, cloakFace.g, cloakFace.b, 1.f);

	// The hem carries both powers, dark on the left and light on the right
	// as in the vessel. Leaning toward a side brightens it and drains the
	// other toward gray.
	float lightStrength = Clamp(0.5f + ctx.status->BalanceRatio() * 0.5f, 0.f, 1.f);
	float darkStrength = 1.f - lightStrength;
	float darkFaceR = 0.18f + 0.22f * darkStrength;
	float darkFaceG = 0.16f + 0.10f * darkStrength;
	float darkFaceB = 0.22f + 0.46f * darkStrength;
	float lightFaceR = 0.24f + 0.66f * lightStrength;
	float lightFaceG = 0.21f + 0.48f * lightStrength;
	float lightFaceB = 0.18f + 0.10f * lightStrength;
	Ashen(ash, darkFaceR, darkFaceG, darkFaceB);
	Ashen(ash, lightFaceR, lightFaceG, lightFaceB);
	float hemBand = kCloakHem;
	float bandHalf = hemHalf - (hemHalf - topHalf) * hemBand / (shoulderY - hemY);
	float darkHem[8] =
	{
		hemX - bandHalf, hemY + hemBand,
		hemX - hemHalf, hemY,
		hemX, hemY,
		hemX, hemY + hemBand,
	};
	float lightHem[8] =
	{
		hemX, hemY + hemBand,
		hemX, hemY,
		hemX + hemHalf, hemY,
		hemX + bandHalf, hemY + hemBand,
	};
	out.AddCorners(cloakDepth + 0.0005f, darkHem, darkFaceR, darkFaceG, darkFaceB, 1.f);
	out.AddCorners(cloakDepth + 0.0005f, lightHem, lightFaceR, lightFaceG, lightFaceB, 1.f);

	// Where it is gathered over the shoulders.
	out.AddTaperedBox(depth + 0.015f, upperX + lean * 0.3f, shoulderY - 5.f, 0.27f, 0.25f, 6.f, 0.75f, 0.f, 0.f,
		cloakShade.r, cloakShade.g, cloakShade.b);

	// Arms from the shoulders to the hands: hanging and swinging along the
	// way he faces at rest, thrown out, up or across as he fights. His right
	// hand holds his form's weapon; each arm is in front of the cloak or
	// behind it depending on which way he has turned.
	float armBase = shoulderY - armHeight;
	float shoulderTop = shoulderY - 1.f;
	float rightShoulderX = upperX - toward * 7.f;
	float leftShoulderX = upperX + toward * 7.f;
	float rightHandX = upperX - toward * 10.f + side * swing * 0.8f + rightReachX;
	float leftHandX = upperX + toward * 10.f - side * swing * 0.8f + leftReachX;
	float rightHandY = armBase + rightReachY;
	float leftHandY = armBase + leftReachY;
	float rightDepth = (side > -0.05f) ? 0.022f : 0.004f;
	float leftDepth = (side < 0.05f) ? 0.022f : 0.004f;
	const Palette::Color& rightSleeve = (rightDepth > 0.01f) ? cloak : cloakShade;
	const Palette::Color& leftSleeve = (leftDepth > 0.01f) ? cloak : cloakShade;
	Limb(out, depth + rightDepth, rightShoulderX, shoulderTop, rightHandX, rightHandY, kShoulderWidth, kWristWidth, rightSleeve);
	Limb(out, depth + leftDepth, leftShoulderX, shoulderTop, leftHandX, leftHandY, kShoulderWidth, kWristWidth, leftSleeve);
	out.AddRect(depth + rightDepth + 0.003f, rightHandX, rightHandY - 1.f, 3.2f, 3.2f, skin.r, skin.g, skin.b, 1.f);
	out.AddRect(depth + leftDepth + 0.003f, leftHandX, leftHandY - 1.f, 3.f, 3.f, skin.r * 0.9f, skin.g * 0.9f, skin.b * 0.9f, 1.f);
	float frontHandX = rightHandX;
	float weaponDepth = rightDepth + 0.02f;

	// The hood: drawn down low, peaked at the back. Facing us, the face inside
	// it is all shadow but two eyes - one gold, one violet; turned side-on the
	// eyes crowd to the front of it; turned away, there is only the hood.
	float headBase = shoulderY - 1.f;
	float headX = upperX + lean * 0.6f + side;
	out.AddTaperedBox(depth + 0.03f, headX, headBase, 0.24f, 0.24f, headHeight + 3.f, 0.6f, -side * 1.5f, 0.f,
		cloak.r * 1.05f, cloak.g * 1.05f, cloak.b * 1.05f);
	out.AddTaperedBox(depth + 0.032f, headX - side * 3.f, headBase + headHeight + 1.f + Clamp(-toward, 0.f, 1.f), 0.08f, 0.08f,
		4.f, 0.3f, -side * 3.f, 0.f, cloak.r * 1.05f, cloak.g * 1.05f, cloak.b * 1.05f);

	float goldR, goldG, goldB, violetR, violetG, violetB;
	AlignmentColor::Core(Alignment::Light, goldR, goldG, goldB);
	AlignmentColor::Core(Alignment::Dark, violetR, violetG, violetB);

	float faceShown = Clamp((toward + 0.3f) / 0.5f, 0.f, 1.f);
	if (faceShown > 0.f)
	{
		float faceX = headX + side * 3.f;
		float faceY = headBase + headHeight * 0.45f;
		float faceWidth = 4.f + 3.f * Clamp(toward, 0.f, 1.f);
		out.AddRect(depth + 0.04f, faceX, faceY, faceWidth, 7.f,
			Palette::kHoodShadow.r, Palette::kHoodShadow.g, Palette::kHoodShadow.b, faceShown);

		// His right eye is the gold one: on our left when he faces us.
		float eyeGap = 1.8f * Clamp(toward, 0.25f, 1.f);
		float rightEyeX = faceX - eyeGap * (toward >= 0.f ? 1.f : -1.f) + side * 0.8f;
		float leftEyeX = faceX + eyeGap * (toward >= 0.f ? 1.f : -1.f) + side * 0.8f;
		out.SetGlow(0.8f * glowStrength * faceShown);
		out.AddRect(depth + 0.05f, rightEyeX, faceY + 0.5f, 1.6f, 1.6f, goldR, goldG, goldB, faceShown);
		out.SetGlow(0.f);
		out.AddRect(depth + 0.05f, leftEyeX, faceY + 0.5f, 1.6f, 1.6f, violetR, violetG, violetB, faceShown);
	}

	// What his form puts in his hand: shards of light circling it, a blade
	// of shadow, or a knot of the gray.
	float handY = rightHandY - 1.5f;
	switch (ctx.status->form)
	{
	case Form::Light:
	{
		out.SetFigure(false);
		out.SetGlow(1.f);
		if (m_Action == Action::Cast || m_Action == Action::Beam || m_Action == Action::Call)
		{
			float flare = (m_Action == Action::Cast) ? swell : hold;
			out.AddGlow(depth + weaponDepth + 0.04f, frontHandX, handY, 18.f * flare, goldR, goldG, goldB, flare);
			if (m_Action == Action::Beam)
			{
				out.AddGlow(depth + leftDepth + 0.04f, leftHandX, leftHandY - 1.5f, 14.f * flare, goldR, goldG, goldB, flare);
			}
		}
		for (int i = 0; i < kShards; i++)
		{
			float angle = m_Time * kShardRate + i * 2.f * 3.14159265f / kShards;
			float shardX = frontHandX + cosf(angle) * kShardOrbit;
			float shardY = handY + sinf(angle) * kShardOrbit * 0.45f + 2.f;
			float front = (sinf(angle) < 0.f) ? weaponDepth + 0.02f : weaponDepth - 0.03f;
			out.AddDiamond(depth + front, shardX, shardY, 3.f, 6.f, goldR, goldG, goldB, 0.95f * glowStrength);
		}
		out.SetGlow(0.f);
		out.SetFigure(true);
		break;
	}
	case Form::Dark:
	{
		float tipX, tipY;
		if (m_DarkMoves.IsPlanting())
		{
			// Driven point-down into the ground in front of him, both hands on it.
			float plantX = upperX + side * 6.f;
			float plant[8] =
			{
				plantX - 1.7f, handY,
				plantX, groundY - 3.f,
				plantX, groundY - 3.f,
				plantX + 1.7f, handY,
			};
			out.AddCorners(depth + 0.03f, plant, Palette::kShade.r, Palette::kShade.g, Palette::kShade.b, 1.f);
			float plantEdge[8] =
			{
				plantX + 0.3f, handY,
				plantX, groundY - 3.f,
				plantX, groundY - 3.f,
				plantX + 1.2f, handY,
			};
			out.AddCorners(depth + 0.031f, plantEdge, violetR, violetG, violetB, 0.9f);
			break;
		}
		bool swinging = (m_Action == Action::Slash || m_Action == Action::Sweep || m_Action == Action::Finisher
			|| m_Action == Action::Thrust);
		if (swinging)
		{
			// From one side of the aim across to the other, eased hard into
			// the end of the stroke; the last cut comes down from overhead;
			// a thrust goes straight out. Two fading copies trail the blade.
			float aim = atan2f(m_ActionY, m_ActionX);
			float ease = 1.f - (1.f - act) * (1.f - act) * (1.f - act);
			float angle = aim;
			if (m_Action == Action::Slash)
			{
				angle = aim + m_SlashSide * (1.1f - 2.2f * ease);
			}
			else if (m_Action == Action::Sweep)
			{
				angle = aim + m_SlashSide * (1.9f - 3.8f * ease);
			}
			else if (m_Action == Action::Finisher)
			{
				angle = (kPi * 0.5f + 0.3f) + (aim - kPi * 0.5f - 0.3f) * ease;
			}
			for (int ghost = 2; ghost >= 0; ghost--)
			{
				bool cutting = (m_Action == Action::Slash || m_Action == Action::Sweep);
				float back = ghost * ((m_Action == Action::Sweep) ? 0.32f : 0.22f) * (cutting ? m_SlashSide : -1.f);
				float a = angle + ((m_Action == Action::Thrust) ? 0.f : back);
				float bladeTipX = frontHandX + cosf(a) * kSwingBladeLength;
				float bladeTipY = handY + sinf(a) * kSwingBladeLength * 0.75f;
				const Palette::Color& bladeColor = (ghost == 0) ? Palette::kShade : Palette::kCloakShade;
				float alpha = (ghost == 0) ? 1.f : 0.35f / ghost;
				float sideX = -sinf(a) * kBladeWidth * 0.5f;
				float sideY = cosf(a) * kBladeWidth * 0.5f;
				float swingCorners[8] =
				{
					frontHandX - sideX, handY - sideY,
					bladeTipX, bladeTipY,
					bladeTipX, bladeTipY,
					frontHandX + sideX, handY + sideY,
				};
				out.AddCorners(depth + weaponDepth - ghost * 0.001f, swingCorners, bladeColor.r, bladeColor.g, bladeColor.b, alpha);
				if (ghost == 0)
				{
					float edgeCorners[8] =
					{
						frontHandX + sideX * 0.2f, handY + sideY * 0.2f,
						bladeTipX, bladeTipY,
						bladeTipX, bladeTipY,
						frontHandX + sideX, handY + sideY,
					};
					out.AddCorners(depth + weaponDepth + 0.001f, edgeCorners, violetR, violetG, violetB, 0.95f);
				}
			}
			break;
		}
		HeldBlade(out, depth + weaponDepth, frontHandX, handY, side, kBladeLength, kBladeWidth, Palette::kShade, tipX, tipY);
		// The edge, a thin line of violet down its length.
		float edge = (side >= 0.f) ? 1.f : -1.f;
		float corners[8] =
		{
			frontHandX + edge * 0.6f, handY,
			tipX + edge * 0.3f, tipY,
			tipX + edge * 0.9f, tipY + 0.5f,
			frontHandX + edge * 1.4f, handY,
		};
		out.AddCorners(depth + weaponDepth + 0.001f, corners, violetR, violetG, violetB, 0.9f);
		break;
	}
	default:
	{
		out.SetFigure(false);
		float ashR, ashG, ashB;
		AlignmentColor::Core(Alignment::Ashen, ashR, ashG, ashB);
		out.AddGlow(depth + weaponDepth, frontHandX, handY + 1.f, 9.f, ashR, ashG, ashB, 0.6f);
		out.AddDiamond(depth + weaponDepth + 0.01f, frontHandX, handY + 1.f, 4.f, 6.f, ashR, ashG, ashB, 1.f);
		out.SetFigure(true);
		break;
	}
	}

	out.SetFigure(false);
	m_LightMoves.Draw(out, *this, x, groundY);
	m_DarkMoves.Draw(out, *this, x, groundY);
	m_AshMoves.Draw(out, *this, x, groundY);
}
