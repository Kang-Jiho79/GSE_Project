#include "stdafx.h"
#include "PlayerActor.h"
#include "DrawList.h"
#include "PlayerStatus.h"
#include "WorldGrid.h"
#include "Tuning.h"
#include "SparkActor.h"
#include "SceneGraph.h"
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
	// A puff of dust kicked up as the roll starts.
	const int kRollDust = 8;
	const float kRollDustSpeed = 60.f;
	const float kRollDustLift = 2.f;

	// Dashing: pitched forward this far, a little crouched.
	const float kLungeLean = 9.f;
	const float kLungeSquash = 0.88f;

	// Spinning: how often he turns from one side to the other.
	const float kSpinFlipTime = 0.08f;

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
	m_LightMoves = LightFormMoves();
	m_DarkMoves = DarkFormMoves();
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

void PlayerActor::Lunge(float dirCol, float dirRow, float tiles, float seconds)
{
	m_LungeDirCol = dirCol;
	m_LungeDirRow = dirRow;
	m_LungeSpeed = tiles / seconds;
	m_LungeTimer = seconds;
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
	else
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

	// Spinning, he whips from side to side.
	if (m_DarkMoves.IsSpinning())
	{
		m_Facing = (fmodf(m_Time, kSpinFlipTime * 2.f) < kSpinFlipTime) ? 1.f : -1.f;
	}

	m_LightMoves.Update(ctx, *this);
	m_DarkMoves.Update(ctx, *this);

	if (ctx.input.switchForm)
	{
		Form leaving = ctx.status->form;
		if (ctx.status->TrySwitchForm())
		{
			MovesFor(leaving).Interrupt();
			MovesFor(ctx.status->form).Enter(ctx, *this);
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
	if (form == Form::Light)
	{
		return m_LightMoves;
	}
	return m_DarkMoves;
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

	SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dust,
		kRollDust, kRollDustSpeed, kRollDustLift);

	m_DodgeTimer = Tuning::Dodge::kDuration;
	m_DodgeCooldown = Tuning::Dodge::kCooldown;
	ctx.status->GrantInvuln(Tuning::Dodge::kInvulnDuration);
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

	if (ctx.status->health <= 0.f)
	{
		squash = 1.f - kFallSquash;
		lean = m_Facing * kFallLean;
		swing = 0.f;
		leftLift = 0.f;
		rightLift = 0.f;
		bob = 0.f;
	}

	float legsHeight = kLegsHeight * squash;
	float torsoHeight = kTorsoHeight * squash;
	float armHeight = kArmHeight * squash;
	float headHeight = kHeadHeight * (1.f - (1.f - squash) * 0.3f);
	float upperX = x + lean;

	const float kSkin = 0.78f;
	const float kCloth = 0.21f;
	const float kBoot = 0.15f;
	const float kHair = 0.13f;

	// Boots first, then legs narrowing from hip to ankle so they read as limbs
	// rather than posts.
	float leftX = x - 4.5f + swing;
	float rightX = x + 4.5f - swing;

	out.AddTaperedBox(depth, leftX, groundY + leftLift, 0.15f, 0.15f, 3.f,
		0.8f, 0.f, 0.f, kBoot, kBoot, kBoot);
	out.AddTaperedBox(depth, rightX, groundY + rightLift, 0.15f, 0.15f, 3.f,
		0.8f, 0.f, 0.f, kBoot, kBoot, kBoot);

	out.AddTaperedBox(depth + 0.005f, leftX, groundY + leftLift + 3.f, 0.14f, 0.14f, legsHeight - 3.f,
		0.8f, -swing * 0.25f, 0.f, kCloth, kCloth, kCloth);
	out.AddTaperedBox(depth + 0.005f, rightX, groundY + rightLift + 3.f, 0.14f, 0.14f, legsHeight - 3.f,
		0.8f, swing * 0.25f, 0.f, kCloth, kCloth, kCloth);

	float torsoBase = groundY + legsHeight + bob;

	// The torso carries both powers, one on each face. Leaning toward a side
	// brightens that face and drains the other toward gray. Flared upward, so
	// the shoulders are wider than the waist.
	float lightStrength = Clamp(0.5f + ctx.status->BalanceRatio() * 0.5f, 0.f, 1.f);
	float darkStrength = 1.f - lightStrength;

	float darkFaceR = 0.26f + 0.20f * darkStrength;
	float darkFaceG = 0.24f + 0.10f * darkStrength;
	float darkFaceB = 0.28f + 0.42f * darkStrength;
	float lightFaceR = 0.30f + 0.62f * lightStrength;
	float lightFaceG = 0.26f + 0.44f * lightStrength;
	float lightFaceB = 0.24f + 0.10f * lightStrength;
	Ashen(ash, darkFaceR, darkFaceG, darkFaceB);
	Ashen(ash, lightFaceR, lightFaceG, lightFaceB);

	out.AddTaperedBoxFaces(depth + 0.01f, upperX, torsoBase, 0.26f, 0.24f, torsoHeight,
		1.32f, lean * 0.6f, 0.f,
		0.60f, 0.58f, 0.60f,
		darkFaceR, darkFaceG, darkFaceB,
		lightFaceR, lightFaceG, lightFaceB);

	// Arms swing opposite to the legs, hanging from the shoulder and thinning
	// toward the hand.
	float shoulderY = torsoBase + torsoHeight * 0.82f;
	float armBase = shoulderY - armHeight;
	out.AddTaperedBox(depth + 0.02f, upperX - 11.f - swing * 0.7f, armBase, 0.1f, 0.1f, armHeight,
		1.25f, swing * 0.5f, 0.f, kCloth * 1.3f, kCloth * 1.3f, kCloth * 1.35f);
	out.AddTaperedBox(depth + 0.02f, upperX + 11.f + swing * 0.7f, armBase, 0.1f, 0.1f, armHeight,
		1.25f, -swing * 0.5f, 0.f, kCloth * 1.3f, kCloth * 1.3f, kCloth * 1.35f);

	// Hands, left bare at the end of each sleeve.
	out.AddTaperedBox(depth + 0.025f, upperX - 11.f - swing * 0.7f + swing * 0.25f, armBase - 2.5f, 0.085f, 0.085f, 2.5f,
		1.f, 0.f, 0.f, kSkin, kSkin * 0.97f, kSkin * 0.88f);
	out.AddTaperedBox(depth + 0.025f, upperX + 11.f + swing * 0.7f - swing * 0.25f, armBase - 2.5f, 0.085f, 0.085f, 2.5f,
		1.f, 0.f, 0.f, kSkin, kSkin * 0.97f, kSkin * 0.88f);

	float neckBase = torsoBase + torsoHeight;
	out.AddTaperedBox(depth + 0.03f, upperX + lean * 0.6f, neckBase, 0.1f, 0.1f, 2.5f, 1.f, 0.f, 0.f,
		kSkin * 0.8f, kSkin * 0.78f, kSkin * 0.72f);

	// Head: widening to the cheekbones, then a cap of hair narrowing above, so
	// the silhouette rounds off instead of ending in a cube.
	float headBase = neckBase + 2.5f;
	float headX = upperX + lean * 0.6f + m_Facing;
	out.AddTaperedBox(depth + 0.035f, headX, headBase, 0.2f, 0.2f, headHeight * 0.62f,
		1.15f, 0.f, 0.f, kSkin, kSkin * 0.97f, kSkin * 0.88f);
	out.AddTaperedBox(depth + 0.04f, headX, headBase + headHeight * 0.62f, 0.23f, 0.23f, headHeight * 0.38f,
		0.55f, m_Facing * -0.8f, 0.f, kHair, kHair, kHair * 1.2f);

	// Two dark marks on the side of the head he is looking toward.
	float eyeY = headBase + headHeight * 0.42f;
	float eyeX = headX + m_Facing * 4.f;
	out.AddRect(depth + 0.05f, eyeX - 2.f, eyeY, 1.6f, 1.8f, 0.10f, 0.09f, 0.11f, 1.f);
	out.AddRect(depth + 0.05f, eyeX + 2.f, eyeY, 1.6f, 1.8f, 0.10f, 0.09f, 0.11f, 1.f);

	out.SetFigure(false);
	m_LightMoves.Draw(out, *this, x, groundY);
	m_DarkMoves.Draw(out, *this, x, groundY);
}
