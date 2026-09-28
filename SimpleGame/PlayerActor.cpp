#include "stdafx.h"
#include "PlayerActor.h"
#include "ProjectileActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "PlayerStatus.h"
#include "WorldGrid.h"
#include <cmath>

namespace
{
	const float kMoveSpeed = 5.f; // tiles per second
	const float kAttackCooldown = 0.3f;
	const float kProjectileSpeed = 420.f;

	const float kGlowRadius = 46.f;

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
}

void PlayerActor::OnUpdate(const ActorContext& ctx)
{
	ctx.status->Tick(ctx.deltaTime);
	m_Time += ctx.deltaTime;

	m_Moving = false;
	Move(ctx);

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

	if (m_LightCooldown > 0.f)
	{
		m_LightCooldown -= ctx.deltaTime;
	}
	if (m_DarkCooldown > 0.f)
	{
		m_DarkCooldown -= ctx.deltaTime;
	}

	if (ctx.input.fireLight && m_LightCooldown <= 0.f)
	{
		Fire(ctx, Alignment::Light);
		m_LightCooldown = kAttackCooldown;
	}
	if (ctx.input.fireDark && m_DarkCooldown <= 0.f)
	{
		Fire(ctx, Alignment::Dark);
		m_DarkCooldown = kAttackCooldown;
	}
}

void PlayerActor::Move(const ActorContext& ctx)
{
	// Screen directions converted onto the grid axes, so one key walks straight
	// along a screen axis and two adjacent keys blend into a screen diagonal.
	float moveCol = ctx.input.moveRight - ctx.input.moveUp;
	float moveRow = -ctx.input.moveRight - ctx.input.moveUp;

	float lengthSquared = moveCol * moveCol + moveRow * moveRow;
	if (lengthSquared <= 0.0001f)
	{
		return;
	}

	// Stepped in tiles rather than pixels, so he covers the same ground per
	// second in every direction instead of appearing to sprint north.
	// Normalized, so holding two directions is a diagonal, not a sprint.
	float length = sqrtf(lengthSquared);
	float step = kMoveSpeed * ctx.deltaTime;

	float col, row;
	WorldGrid::InverseIso(LocalX(), LocalY(), col, row);

	col += (moveCol / length) * step;
	row += (moveRow / length) * step;
	WorldGrid::ClampToMap(col, row);

	float x, y;
	WorldGrid::IsoPosition(col, row, x, y);
	SetLocalPosition(x, y);

	m_Moving = true;
	m_WalkPhase += step * kStrideRate;
}

void PlayerActor::Fire(const ActorContext& ctx, Alignment alignment) const
{
	float aimLength = sqrtf(ctx.input.aimX * ctx.input.aimX + ctx.input.aimY * ctx.input.aimY);
	if (aimLength < 0.001f)
	{
		return;
	}

	std::unique_ptr<ProjectileActor> bolt(new ProjectileActor(alignment,
		(ctx.input.aimX / aimLength) * kProjectileSpeed,
		(ctx.input.aimY / aimLength) * kProjectileSpeed));
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

	out.AddTaperedBox(depth + 0.005f, leftX, groundY + leftLift + 3.f, 0.14f, 0.14f, kLegsHeight - 3.f,
		0.8f, -swing * 0.25f, 0.f, kCloth, kCloth, kCloth);
	out.AddTaperedBox(depth + 0.005f, rightX, groundY + rightLift + 3.f, 0.14f, 0.14f, kLegsHeight - 3.f,
		0.8f, swing * 0.25f, 0.f, kCloth, kCloth, kCloth);

	float torsoBase = groundY + kLegsHeight + bob;

	// The torso carries both powers, one on each face. Tilting toward a side
	// brightens that face and drains the other toward gray. Flared upward, so
	// the shoulders are wider than the waist.
	float lightStrength = Clamp(0.5f + ctx.status->tilt * 0.5f, 0.f, 1.f);
	float darkStrength = 1.f - lightStrength;

	out.AddTaperedBoxFaces(depth + 0.01f, x, torsoBase, 0.26f, 0.24f, kTorsoHeight,
		1.32f, 0.f, 0.f,
		0.60f, 0.58f, 0.60f,
		0.26f + 0.20f * darkStrength, 0.24f + 0.10f * darkStrength, 0.28f + 0.42f * darkStrength,
		0.30f + 0.62f * lightStrength, 0.26f + 0.44f * lightStrength, 0.24f + 0.10f * lightStrength);

	// Arms swing opposite to the legs, hanging from the shoulder and thinning
	// toward the hand.
	float shoulderY = torsoBase + kTorsoHeight * 0.82f;
	float armBase = shoulderY - kArmHeight;
	out.AddTaperedBox(depth + 0.02f, x - 11.f - swing * 0.7f, armBase, 0.1f, 0.1f, kArmHeight,
		1.25f, swing * 0.5f, 0.f, kCloth * 1.3f, kCloth * 1.3f, kCloth * 1.35f);
	out.AddTaperedBox(depth + 0.02f, x + 11.f + swing * 0.7f, armBase, 0.1f, 0.1f, kArmHeight,
		1.25f, -swing * 0.5f, 0.f, kCloth * 1.3f, kCloth * 1.3f, kCloth * 1.35f);

	// Hands, left bare at the end of each sleeve.
	out.AddTaperedBox(depth + 0.025f, x - 11.f - swing * 0.7f + swing * 0.25f, armBase - 2.5f, 0.085f, 0.085f, 2.5f,
		1.f, 0.f, 0.f, kSkin, kSkin * 0.97f, kSkin * 0.88f);
	out.AddTaperedBox(depth + 0.025f, x + 11.f + swing * 0.7f - swing * 0.25f, armBase - 2.5f, 0.085f, 0.085f, 2.5f,
		1.f, 0.f, 0.f, kSkin, kSkin * 0.97f, kSkin * 0.88f);

	float neckBase = torsoBase + kTorsoHeight;
	out.AddTaperedBox(depth + 0.03f, x, neckBase, 0.1f, 0.1f, 2.5f, 1.f, 0.f, 0.f,
		kSkin * 0.8f, kSkin * 0.78f, kSkin * 0.72f);

	// Head: widening to the cheekbones, then a cap of hair narrowing above, so
	// the silhouette rounds off instead of ending in a cube.
	float headBase = neckBase + 2.5f;
	float headX = x + m_Facing;
	out.AddTaperedBox(depth + 0.035f, headX, headBase, 0.2f, 0.2f, kHeadHeight * 0.62f,
		1.15f, 0.f, 0.f, kSkin, kSkin * 0.97f, kSkin * 0.88f);
	out.AddTaperedBox(depth + 0.04f, headX, headBase + kHeadHeight * 0.62f, 0.23f, 0.23f, kHeadHeight * 0.38f,
		0.55f, m_Facing * -0.8f, 0.f, kHair, kHair, kHair * 1.2f);

	// Two dark marks on the side of the head he is looking toward.
	float eyeY = headBase + kHeadHeight * 0.42f;
	float eyeX = headX + m_Facing * 4.f;
	out.AddRect(depth + 0.05f, eyeX - 2.f, eyeY, 1.6f, 1.8f, 0.10f, 0.09f, 0.11f, 1.f);
	out.AddRect(depth + 0.05f, eyeX + 2.f, eyeY, 1.6f, 1.8f, 0.10f, 0.09f, 0.11f, 1.f);
}
