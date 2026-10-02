#include "stdafx.h"
#include "PlayerActor.h"
#include "ProjectileActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "PlayerStatus.h"
#include "WorldGrid.h"
#include "Tuning.h"
#include "Combat.h"
#include <cmath>

namespace
{
	const float kGlowRadius = 46.f;

	const float kDegToRad = 3.14159265f / 180.f;

	// The slash trail: a fan of quads on the ground around him, swept out over
	// the first part of its life and faded over the rest.
	const float kSlashShowTime = 0.16f;
	const float kSlashSweepShare = 0.45f;
	const int kSlashSegments = 7;
	const float kSlashInner = 0.4f;   // tiles
	const float kSlashBlade = 0.6f;   // where the bright outer band starts, 0..1 across the trail
	const float kSlashLift = 12.f;    // pixels above the ground, about waist height

	// The roll: he folds down to this fraction of his height at its middle and
	// pitches his upper body this many pixels toward where he is going.
	const float kRollSquash = 0.45f;
	const float kRollLean = 7.f;

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

	// The aim is a screen offset; the iso mapping is linear, so the same
	// conversion that turns positions into tiles turns it into a tile direction.
	bool AimDirection(const InputState& input, float& outCol, float& outRow)
	{
		float col, row;
		WorldGrid::InverseIso(input.aimX, input.aimY, col, row);

		float length = sqrtf(col * col + row * row);
		if (length < 0.001f)
		{
			return false;
		}

		outCol = col / length;
		outRow = row / length;
		return true;
	}

	// A point on the ground at this angle and distance from him, in tiles,
	// returned as a screen offset.
	void SlashPoint(float angle, float radius, float& outX, float& outY)
	{
		WorldGrid::IsoPosition(cosf(angle) * radius, sinf(angle) * radius, outX, outY);
	}
}

void PlayerActor::Respawn(float x, float y)
{
	SetLocalPosition(x, y);

	// Snap to the new ground instead of climbing to it from where he fell.
	m_GroundInitialized = false;

	m_AttackCooldown = 0.f;
	m_DodgeTimer = 0.f;
	m_DodgeCooldown = 0.f;
	m_ComboStep = 0;
	m_ComboTimer = 0.f;
	m_SlashTimer = 0.f;
}

void PlayerActor::OnUpdate(const ActorContext& ctx)
{
	ctx.status->Tick(ctx.deltaTime);
	m_Time += ctx.deltaTime;

	if (m_DodgeCooldown > 0.f)
	{
		m_DodgeCooldown -= ctx.deltaTime;
	}
	if (ctx.input.dodge && m_DodgeTimer <= 0.f && m_DodgeCooldown <= 0.f)
	{
		StartDodge(ctx);
	}

	m_Moving = false;
	if (m_DodgeTimer > 0.f)
	{
		Roll(ctx);
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

	// Mid-roll he faces where he is rolling, not where the cursor is.
	if (m_DodgeTimer > 0.f)
	{
		float screenX, screenY;
		WorldGrid::IsoPosition(m_DodgeDirCol, m_DodgeDirRow, screenX, screenY);
		if (fabsf(screenX) > 0.01f)
		{
			m_Facing = (screenX > 0.f) ? 1.f : -1.f;
		}
	}

	if (ctx.input.switchForm && ctx.status->TrySwitchForm())
	{
		m_ComboStep = 0;
	}

	if (m_AttackCooldown > 0.f)
	{
		m_AttackCooldown -= ctx.deltaTime;
	}
	if (m_ComboTimer > 0.f)
	{
		m_ComboTimer -= ctx.deltaTime;
	}
	if (m_SlashTimer > 0.f)
	{
		m_SlashTimer -= ctx.deltaTime;
	}

	if (ctx.input.attack && m_AttackCooldown <= 0.f && m_DodgeTimer <= 0.f)
	{
		Attack(ctx);
	}
}

void PlayerActor::Attack(const ActorContext& ctx)
{
	if (ctx.status->form == Form::Dark)
	{
		Slash(ctx);
		return;
	}

	Fire(ctx, Alignment::Light);
	m_AttackCooldown = Tuning::LightForm::kBoltCooldown;
}

void PlayerActor::Slash(const ActorContext& ctx)
{
	float dirCol, dirRow;
	if (!AimDirection(ctx.input, dirCol, dirRow))
	{
		return;
	}

	if (m_ComboTimer <= 0.f)
	{
		m_ComboStep = 0;
	}

	Combat::HitArc(*ctx.graph, WorldX(), WorldY(), dirCol, dirRow,
		Tuning::DarkForm::kSlashRange, Tuning::DarkForm::kSlashArcDegrees,
		Combat::SlashDamage(m_ComboStep));

	m_SlashStep = m_ComboStep;
	m_SlashTimer = kSlashShowTime;
	m_SlashDirCol = dirCol;
	m_SlashDirRow = dirRow;

	bool finisher = (m_ComboStep == Tuning::DarkForm::kComboLength - 1);
	m_AttackCooldown = finisher ? Tuning::DarkForm::kFinisherRecovery : Tuning::DarkForm::kSlashInterval;
	m_ComboStep = finisher ? 0 : m_ComboStep + 1;
	m_ComboTimer = Tuning::DarkForm::kComboWindow;
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
		&& !AimDirection(ctx.input, m_DodgeDirCol, m_DodgeDirRow))
	{
		float col, row;
		WorldGrid::InverseIso(m_Facing, 0.f, col, row);
		float length = sqrtf(col * col + row * row);
		m_DodgeDirCol = col / length;
		m_DodgeDirRow = row / length;
	}

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

void PlayerActor::StepOnGrid(float dirCol, float dirRow, float tiles)
{
	float col, row;
	WorldGrid::InverseIso(LocalX(), LocalY(), col, row);

	col += dirCol * tiles;
	row += dirRow * tiles;
	WorldGrid::ClampToMap(col, row);

	float x, y;
	WorldGrid::IsoPosition(col, row, x, y);
	SetLocalPosition(x, y);
}

void PlayerActor::Fire(const ActorContext& ctx, Alignment alignment) const
{
	float aimLength = sqrtf(ctx.input.aimX * ctx.input.aimX + ctx.input.aimY * ctx.input.aimY);
	if (aimLength < 0.001f)
	{
		return;
	}

	std::unique_ptr<ProjectileActor> bolt(new ProjectileActor(alignment,
		(ctx.input.aimX / aimLength) * Tuning::LightForm::kBoltSpeedPx,
		(ctx.input.aimY / aimLength) * Tuning::LightForm::kBoltSpeedPx));
	bolt->SetLocalPosition(WorldX(), WorldY());

	ctx.graph->Spawn(std::move(bolt));
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
	out.AddGlow(depth - 0.02f, x, groundY, kGlowRadius, lightGlowR, lightGlowG, lightGlowB,
		ctx.status->light / ctx.status->capacity);

	float darkGlowR, darkGlowG, darkGlowB;
	AlignmentColor::Glow(Alignment::Dark, darkGlowR, darkGlowG, darkGlowB);
	out.AddGlow(depth - 0.02f, x, groundY, kGlowRadius, darkGlowR, darkGlowG, darkGlowB,
		ctx.status->dark / ctx.status->capacity);

	out.AddShadow(depth - 0.01f, x, groundY, 26.f, 13.f);

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

	// The torso carries both powers, one on each face. Tilting toward a side
	// brightens that face and drains the other toward gray. Flared upward, so
	// the shoulders are wider than the waist.
	float lightStrength = Clamp(0.5f + ctx.status->tilt * 0.5f, 0.f, 1.f);
	float darkStrength = 1.f - lightStrength;

	out.AddTaperedBoxFaces(depth + 0.01f, upperX, torsoBase, 0.26f, 0.24f, torsoHeight,
		1.32f, lean * 0.6f, 0.f,
		0.60f, 0.58f, 0.60f,
		0.26f + 0.20f * darkStrength, 0.24f + 0.10f * darkStrength, 0.28f + 0.42f * darkStrength,
		0.30f + 0.62f * lightStrength, 0.26f + 0.44f * lightStrength, 0.24f + 0.10f * lightStrength);

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

	DrawSlash(out, x, y);
}

void PlayerActor::DrawSlash(DrawList& out, float screenX, float screenY) const
{
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

	float inner = kSlashInner;
	float outer = Tuning::DarkForm::kSlashRange;
	float blade = inner + (outer - inner) * kSlashBlade;
	float baseY = screenY + m_GroundHeight + kSlashLift;

	for (int i = 0; i < visible; i++)
	{
		// The middle cut sweeps back the other way, so the combo reads as
		// swings to and fro rather than the same stroke three times.
		int segment = (m_SlashStep == 1) ? kSlashSegments - 1 - i : i;
		float a0 = center - half + segment * segmentAngle;
		float a1 = a0 + segmentAngle;

		// Sorted by the ground under the middle of the piece, so the part of
		// the arc in front of him is not buried under the tiles it passes over.
		float midX, midY;
		SlashPoint((a0 + a1) * 0.5f, (inner + outer) * 0.5f, midX, midY);
		float depth = WorldGrid::TileDepth(WorldX() + midX, WorldY() + midY) + 0.7f;

		// Dim trail on the inside, the bright edge of the blade on the outside.
		const float radii[3] = { inner, blade, outer };
		const float bandAlpha[2] = { alpha * 0.45f, alpha };
		for (int band = 0; band < 2; band++)
		{
			float corners[8];
			SlashPoint(a0, radii[band], corners[0], corners[1]);
			SlashPoint(a0, radii[band + 1], corners[2], corners[3]);
			SlashPoint(a1, radii[band + 1], corners[4], corners[5]);
			SlashPoint(a1, radii[band], corners[6], corners[7]);
			for (int c = 0; c < 4; c++)
			{
				corners[c * 2] += screenX;
				corners[c * 2 + 1] += baseY;
			}

			out.AddCorners(depth, corners, r, g, b, bandAlpha[band]);
		}
	}
}
