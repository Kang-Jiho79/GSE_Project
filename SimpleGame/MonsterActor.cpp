#include "stdafx.h"
#include "MonsterActor.h"
#include "ParticleActor.h"
#include "SceneGraph.h"
#include "DrawList.h"
#include "PlayerStatus.h"
#include "Random.h"
#include "WorldGrid.h"
#include <cmath>

namespace
{
	const float kDetectRadius = 200.f;
	const float kGiveUpRadius = 300.f;
	const float kAttackRadius = 34.f;

	const float kBaseChaseSpeed = 90.f; // pixels per second
	const float kBaseRoamSpeed = 36.f;
	const float kRoamRadius = 70.f;

	const float kAttackCooldown = 1.2f;
	const float kAttackDamage = 8.f;
	// How long after landing a hit the body is still thrown forward.
	const float kLungeDuration = 0.18f;
	const float kLungeDistance = 9.f;

	const float kGaitRate = 0.09f; // radians of gait per pixel moved
	const float kClimbRate = 14.f;

	const int kBurstCount = 5;
	const float kBurstSpeed = 90.f;

	const float kBody = 0.235f;
	const float kEye = 0.07f;

	float SpeedScale(MonsterActor::Kind kind)
	{
		switch (kind)
		{
		case MonsterActor::Kind::Brute:
		{
			return 0.72f;
		}
		case MonsterActor::Kind::Stalker:
		{
			return 1.3f;
		}
		default:
		{
			return 1.f;
		}
		}
	}

	float MaxHealth(MonsterActor::Kind kind)
	{
		switch (kind)
		{
		case MonsterActor::Kind::Brute:
		{
			return 5.f;
		}
		case MonsterActor::Kind::Stalker:
		{
			return 2.f;
		}
		default:
		{
			return 3.f;
		}
		}
	}
}

MonsterActor::MonsterActor(Alignment alignment, Kind kind)
	: m_Alignment(alignment)
	, m_Kind(kind)
	, m_Health(MaxHealth(kind))
{
	m_RoamWaitTimer = Random::Unit() * 2.f;
	// Desynchronised, so a group does not sway in lockstep.
	m_Time = Random::Unit() * 10.f;
}

void MonsterActor::TakeHit(float damage)
{
	m_Health -= damage;
}

void MonsterActor::OnUpdate(const ActorContext& ctx)
{
	// The spawn position is only known once it has been placed, so the patch it
	// wanders is anchored on the first frame rather than in the constructor.
	if (!m_Anchored)
	{
		m_OriginX = WorldX();
		m_OriginY = WorldY();
		m_RoamTargetX = m_OriginX;
		m_RoamTargetY = m_OriginY;
		m_GroundHeight = WorldGrid::GroundHeightAt(WorldX(), WorldY());
		m_Anchored = true;
	}

	if (m_Health <= 0.f)
	{
		SpawnDeathBurst(ctx);
		Destroy();
		return;
	}

	m_Time += ctx.deltaTime;
	if (m_AttackCooldown > 0.f)
	{
		m_AttackCooldown -= ctx.deltaTime;
	}

	m_GroundHeight = WorldGrid::SmoothApproach(m_GroundHeight,
		WorldGrid::GroundHeightAt(WorldX(), WorldY()), kClimbRate, ctx.deltaTime);

	float toPlayerX = ctx.playerX - WorldX();
	float toPlayerY = ctx.playerY - WorldY();
	float distance = sqrtf(toPlayerX * toPlayerX + toPlayerY * toPlayerY);

	UpdateState(distance);

	if (m_State != State::Roam)
	{
		m_Facing = (toPlayerX < 0.f) ? -1.f : 1.f;
	}

	if (m_State == State::Attack)
	{
		if (m_AttackCooldown <= 0.f && ctx.status->TryDamage(kAttackDamage))
		{
			m_AttackCooldown = kAttackCooldown;
		}
		return;
	}

	if (m_State == State::Chase)
	{
		MoveToward(ctx.playerX, ctx.playerY, kBaseChaseSpeed * SpeedScale(m_Kind), ctx.deltaTime);
		return;
	}

	Roam(ctx);
}

void MonsterActor::UpdateState(float distanceToPlayer)
{
	// The give-up radius is deliberately wider than the detect radius, so one
	// sitting on the boundary does not flicker between wandering and chasing.
	if (m_State == State::Attack)
	{
		if (distanceToPlayer > kAttackRadius)
		{
			m_State = State::Chase;
		}
		return;
	}

	if (m_State == State::Chase)
	{
		if (distanceToPlayer > kGiveUpRadius)
		{
			m_State = State::Roam;
			m_RoamTargetX = m_OriginX;
			m_RoamTargetY = m_OriginY;
		}
		else if (distanceToPlayer <= kAttackRadius)
		{
			m_State = State::Attack;
		}
		return;
	}

	if (distanceToPlayer <= kDetectRadius)
	{
		m_State = State::Chase;
	}
}

void MonsterActor::MoveToward(float targetX, float targetY, float speed, float deltaTime)
{
	float dx = targetX - WorldX();
	float dy = targetY - WorldY();
	float distance = sqrtf(dx * dx + dy * dy);
	if (distance <= 0.001f)
	{
		return;
	}

	float step = speed * deltaTime;
	if (step > distance)
	{
		step = distance;
	}

	SetLocalPosition(LocalX() + (dx / distance) * step, LocalY() + (dy / distance) * step);
	m_GaitPhase += step * kGaitRate;

	if (fabsf(dx) > 0.5f)
	{
		m_Facing = (dx < 0.f) ? -1.f : 1.f;
	}
}

void MonsterActor::Roam(const ActorContext& ctx)
{
	float toTargetX = m_RoamTargetX - WorldX();
	float toTargetY = m_RoamTargetY - WorldY();
	float distance = sqrtf(toTargetX * toTargetX + toTargetY * toTargetY);

	if (distance >= 4.f)
	{
		MoveToward(m_RoamTargetX, m_RoamTargetY, kBaseRoamSpeed * SpeedScale(m_Kind), ctx.deltaTime);
		return;
	}

	if (m_RoamWaitTimer > 0.f)
	{
		m_RoamWaitTimer -= ctx.deltaTime;
		return;
	}

	float angle = Random::Angle();
	float radius = Random::Unit() * kRoamRadius;
	m_RoamTargetX = m_OriginX + cosf(angle) * radius;
	m_RoamTargetY = m_OriginY + sinf(angle) * radius;
	m_RoamWaitTimer = 1.5f + Random::Unit() * 2.f;
}

void MonsterActor::SpawnDeathBurst(const ActorContext& ctx) const
{
	for (int i = 0; i < kBurstCount; i++)
	{
		float angle = Random::Angle();
		float speed = kBurstSpeed * (0.5f + Random::Unit() * 0.5f);

		std::unique_ptr<ParticleActor> mote(
			new ParticleActor(m_Alignment, cosf(angle) * speed, sinf(angle) * speed));
		mote->SetLocalPosition(WorldX(), WorldY());

		ctx.graph->Spawn(std::move(mote));
	}
}

void MonsterActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);

	if (!ctx.IsOnScreen(x, y, WorldGrid::kTileWidth, WorldGrid::kTileHeight * 4.f))
	{
		return;
	}

	float groundY = y + m_GroundHeight;
	float depth = WorldGrid::TileDepth(WorldX(), WorldY());

	// Right after landing a hit the whole body is thrown toward the player and
	// eases back - the only tell the player gets that they were just struck.
	float lunge = 0.f;
	float sinceHit = kAttackCooldown - m_AttackCooldown;
	if (m_State == State::Attack && sinceHit < kLungeDuration)
	{
		lunge = (1.f - sinceHit / kLungeDuration) * kLungeDistance * m_Facing;
	}

	switch (m_Kind)
	{
	case Kind::Brute:
	{
		DrawBrute(out, depth, x, groundY, lunge);
		break;
	}
	case Kind::Stalker:
	{
		DrawStalker(out, depth, x, groundY, lunge);
		break;
	}
	default:
	{
		DrawCrawler(out, depth, x, groundY, lunge);
		break;
	}
	}
}

void MonsterActor::DrawBrute(DrawList& out, float depth, float x, float groundY, float lunge) const
{
	// Heavy: squat legs, a torso that swells enormously toward the shoulders,
	// and a head sunk down between them. Slow, deep bob.
	float bob = fabsf(sinf(m_GaitPhase * 0.7f)) * 2.2f;
	float bodyX = x + lunge;
	float stride = sinf(m_GaitPhase * 0.7f) * 2.5f;

	out.AddShadow(depth, x, groundY, 40.f, 20.f);

	// Stubby legs, thicker at the foot than the hip - it stands like a pillar.
	out.AddTaperedBox(depth, bodyX - 7.f + stride, groundY, 0.2f, 0.2f, 6.f,
		0.85f, 0.f, 0.f, kBody * 0.88f, kBody * 0.88f, kBody * 0.88f);
	out.AddTaperedBox(depth, bodyX + 7.f - stride, groundY, 0.2f, 0.2f, 6.f,
		0.85f, 0.f, 0.f, kBody * 0.88f, kBody * 0.88f, kBody * 0.88f);

	float torsoBase = groundY + 6.f + bob;
	out.AddTaperedBox(depth + 0.01f, bodyX, torsoBase, 0.34f, 0.34f, 16.f,
		1.55f, m_Facing * 1.f, 0.f, kBody, kBody, kBody);

	// Arms hang from the wide shoulders and swell into fists at the ground.
	float armSwing = stride * 0.8f;
	float armTop = torsoBase + 15.f;
	out.AddTaperedBox(depth + 0.02f, bodyX - 19.f - armSwing, armTop - 14.f, 0.13f, 0.13f, 14.f,
		1.5f, armSwing * 0.6f, 0.f, kBody * 0.94f, kBody * 0.94f, kBody * 0.94f);
	out.AddTaperedBox(depth + 0.02f, bodyX + 19.f + armSwing, armTop - 14.f, 0.13f, 0.13f, 14.f,
		1.5f, -armSwing * 0.6f, 0.f, kBody * 0.94f, kBody * 0.94f, kBody * 0.94f);

	// Almost no neck: the skull sits straight on the shoulder line.
	float headBase = torsoBase + 14.f;
	out.AddTaperedBox(depth + 0.03f, bodyX + m_Facing * 3.f, headBase, 0.26f, 0.26f, 8.f,
		0.72f, m_Facing * 1.5f, 0.f, kBody * 1.08f, kBody * 1.08f, kBody * 1.08f);

	DrawEyes(out, depth + 0.04f, bodyX + m_Facing * 7.f, headBase + 3.5f, 3.f);
}

void MonsterActor::DrawStalker(DrawList& out, float depth, float x, float groundY, float lunge) const
{
	// Tall and unsteady: spindly legs, a narrow body that tapers as it rises,
	// and long arms that trail behind the sway.
	float sway = sinf(m_Time * 2.6f) * 1.6f;
	float bob = fabsf(sinf(m_GaitPhase * 1.3f)) * 1.2f;
	float bodyX = x + lunge + sway;
	float stride = sinf(m_GaitPhase * 1.3f) * 3.f;

	out.AddShadow(depth, x, groundY, 20.f, 10.f);

	// Legs thin to almost nothing at the ground - it walks on points.
	out.AddTaperedBox(depth, x - 3.5f + stride, groundY, 0.09f, 0.09f, 14.f,
		1.5f, sway * 0.3f, 0.f, kBody * 0.85f, kBody * 0.85f, kBody * 0.85f);
	out.AddTaperedBox(depth, x + 3.5f - stride, groundY, 0.09f, 0.09f, 14.f,
		1.5f, sway * 0.3f, 0.f, kBody * 0.85f, kBody * 0.85f, kBody * 0.85f);

	float torsoBase = groundY + 14.f + bob;
	out.AddTaperedBox(depth + 0.01f, bodyX, torsoBase, 0.19f, 0.19f, 17.f,
		0.72f, sway * 0.5f, 0.f, kBody, kBody, kBody);

	// Arms longer than the body, dragging a beat behind the sway.
	float armDrag = sway * 0.9f;
	float armTop = torsoBase + 15.f;
	out.AddTaperedBox(depth + 0.02f, bodyX - 8.f - armDrag, armTop - 23.f, 0.055f, 0.055f, 23.f,
		1.8f, armDrag * 0.8f, 0.f, kBody * 0.9f, kBody * 0.9f, kBody * 0.9f);
	out.AddTaperedBox(depth + 0.02f, bodyX + 8.f - armDrag, armTop - 23.f, 0.055f, 0.055f, 23.f,
		1.8f, armDrag * 0.8f, 0.f, kBody * 0.9f, kBody * 0.9f, kBody * 0.9f);

	// A long neck carries a small head well above the shoulders.
	float neckBase = torsoBase + 17.f;
	out.AddTaperedBox(depth + 0.025f, bodyX + sway * 0.4f, neckBase, 0.07f, 0.07f, 5.f,
		1.f, m_Facing * 0.8f, 0.f, kBody * 0.92f, kBody * 0.92f, kBody * 0.92f);

	float headBase = neckBase + 5.f;
	out.AddTaperedBox(depth + 0.03f, bodyX + m_Facing * 2.f + sway * 0.4f, headBase, 0.15f, 0.15f, 7.f,
		0.6f, m_Facing * 1.2f, 0.f, kBody * 1.08f, kBody * 1.08f, kBody * 1.08f);

	DrawEyes(out, depth + 0.04f, bodyX + m_Facing * 4.5f + sway * 0.4f, headBase + 3.f, 2.f);
}

void MonsterActor::DrawCrawler(DrawList& out, float depth, float x, float groundY, float lunge) const
{
	// Low and wide, stretched along the col axis, on six legs that skitter on a
	// fast clock even when it is still.
	float bodyX = x + lunge;

	out.AddShadow(depth, x, groundY, 42.f, 21.f);

	// Legs splay outward as they go down, so it squats over its own footprint.
	const float kLegSpacing[6] = { -15.f, -9.f, -3.f, 3.f, 9.f, 15.f };
	for (int i = 0; i < 6; i++)
	{
		float lift = fabsf(sinf(m_Time * 14.f + i * 1.3f)) * 1.6f;
		float splay = (kLegSpacing[i] < 0.f) ? -1.2f : 1.2f;
		out.AddTaperedBox(depth, bodyX + kLegSpacing[i], groundY + lift, 0.06f, 0.06f, 5.f,
			1.6f, -splay, 0.f, kBody * 0.82f, kBody * 0.82f, kBody * 0.82f);
	}

	// A domed carapace: two slabs, the upper one drawn well in, so the back
	// curves rather than sitting flat.
	float bodyBase = groundY + 4.f + fabsf(sinf(m_GaitPhase * 2.f)) * 0.8f;
	out.AddTaperedBox(depth + 0.01f, bodyX, bodyBase, 0.66f, 0.38f, 5.f,
		0.88f, 0.f, 0.f, kBody, kBody, kBody);
	out.AddTaperedBox(depth + 0.02f, bodyX, bodyBase + 5.f, 0.58f, 0.33f, 4.f,
		0.55f, m_Facing * -1.5f, 0.f, kBody * 1.06f, kBody * 1.06f, kBody * 1.06f);

	// No head: a blunt snout juts from the front of the shell, eyes on it.
	float snoutX = bodyX + m_Facing * 15.f;
	out.AddTaperedBox(depth + 0.03f, snoutX, bodyBase + 1.f, 0.16f, 0.16f, 4.f,
		0.6f, m_Facing * 2.f, 0.f, kBody * 1.02f, kBody * 1.02f, kBody * 1.02f);

	DrawEyes(out, depth + 0.04f, snoutX + m_Facing * 2.f, bodyBase + 3.5f, 2.5f);
}

void MonsterActor::DrawEyes(DrawList& out, float depth, float x, float y, float spacing) const
{
	out.AddRect(depth, x - spacing, y, 1.8f, 1.8f, kEye, kEye * 0.9f, kEye * 1.1f, 1.f);
	out.AddRect(depth, x + spacing, y, 1.8f, 1.8f, kEye, kEye * 0.9f, kEye * 1.1f, 1.f);
}
