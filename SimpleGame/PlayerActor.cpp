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

	const float kLegsHeight = 13.f;
	const float kTorsoHeight = 20.f;
	const float kHeadHeight = 12.f;

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

	Move(ctx);

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

	float col, row;
	WorldGrid::InverseIso(LocalX(), LocalY(), col, row);

	col += (moveCol / length) * kMoveSpeed * ctx.deltaTime;
	row += (moveRow / length) * kMoveSpeed * ctx.deltaTime;
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
		(ctx.input.aimX / aimLength) * kProjectileSpeed,
		(ctx.input.aimY / aimLength) * kProjectileSpeed));
	bolt->SetLocalPosition(WorldX(), WorldY());

	ctx.graph->Spawn(std::move(bolt));
}

void PlayerActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);

	float baseY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	// The two halves of the vessel spill onto the ground as their own light, so
	// how full each side is can be read without looking at the HUD.
	float lightGlowR, lightGlowG, lightGlowB;
	AlignmentColor::Glow(Alignment::Light, lightGlowR, lightGlowG, lightGlowB);
	out.AddGlow(depth - 0.02f, x, baseY, kGlowRadius, lightGlowR, lightGlowG, lightGlowB,
		ctx.status->light / ctx.status->capacity);

	float darkGlowR, darkGlowG, darkGlowB;
	AlignmentColor::Glow(Alignment::Dark, darkGlowR, darkGlowG, darkGlowB);
	out.AddGlow(depth - 0.02f, x, baseY, kGlowRadius, darkGlowR, darkGlowG, darkGlowB,
		ctx.status->dark / ctx.status->capacity);

	out.AddShadow(depth - 0.01f, x, baseY, 26.f, 13.f);

	const float kLegsShade = 0.21f;
	out.AddBox(depth, x, baseY, 20.f, 10.f, kLegsHeight, kLegsShade, kLegsShade, kLegsShade);

	// The torso carries both powers, one on each face. Tilting toward a side
	// brightens that face and drains the other toward gray.
	float lightStrength = Clamp(0.5f + ctx.status->tilt * 0.5f, 0.f, 1.f);
	float darkStrength = 1.f - lightStrength;

	out.AddBoxFaces(depth + 0.01f, x, baseY + kLegsHeight, 24.f, 12.f, kTorsoHeight,
		0.60f, 0.58f, 0.60f,
		0.26f + 0.20f * darkStrength, 0.24f + 0.10f * darkStrength, 0.28f + 0.42f * darkStrength,
		0.30f + 0.62f * lightStrength, 0.26f + 0.44f * lightStrength, 0.24f + 0.10f * lightStrength);

	const float kHeadShade = 0.78f;
	out.AddBox(depth + 0.02f, x, baseY + kLegsHeight + kTorsoHeight, 15.f, 8.f, kHeadHeight,
		kHeadShade, kHeadShade * 0.97f, kHeadShade * 0.88f);
}
