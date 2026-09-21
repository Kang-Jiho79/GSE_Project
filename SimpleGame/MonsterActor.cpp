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
	const float kMaxHealth = 3.f;

	const float kDetectRadius = 200.f;
	const float kGiveUpRadius = 300.f;
	const float kAttackRadius = 34.f;

	const float kChaseSpeed = 90.f; // pixels per second
	const float kRoamSpeed = 36.f;
	const float kRoamRadius = 70.f;

	const float kAttackCooldown = 1.2f;
	const float kAttackDamage = 8.f;

	const float kBodyShade = 0.235f;
	const float kFootWidth = 22.f;
	const float kFootHeight = 11.f;
	const float kBodyHeight = 24.f;

	const int kBurstCount = 5;
	const float kBurstSpeed = 90.f;
}

MonsterActor::MonsterActor(Alignment alignment)
	: m_Alignment(alignment)
	, m_Health(kMaxHealth)
{
	m_RoamWaitTimer = Random::Unit() * 2.f;
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
		m_Anchored = true;
	}

	if (m_Health <= 0.f)
	{
		SpawnDeathBurst(ctx);
		Destroy();
		return;
	}

	if (m_AttackCooldown > 0.f)
	{
		m_AttackCooldown -= ctx.deltaTime;
	}

	float toPlayerX = ctx.playerX - WorldX();
	float toPlayerY = ctx.playerY - WorldY();
	float distance = sqrtf(toPlayerX * toPlayerX + toPlayerY * toPlayerY);

	UpdateState(distance);

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
		if (distance > 0.001f)
		{
			SetLocalPosition(
				LocalX() + (toPlayerX / distance) * kChaseSpeed * ctx.deltaTime,
				LocalY() + (toPlayerY / distance) * kChaseSpeed * ctx.deltaTime);
		}
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

void MonsterActor::Roam(const ActorContext& ctx)
{
	float toTargetX = m_RoamTargetX - WorldX();
	float toTargetY = m_RoamTargetY - WorldY();
	float distance = sqrtf(toTargetX * toTargetX + toTargetY * toTargetY);

	if (distance >= 4.f)
	{
		SetLocalPosition(
			LocalX() + (toTargetX / distance) * kRoamSpeed * ctx.deltaTime,
			LocalY() + (toTargetY / distance) * kRoamSpeed * ctx.deltaTime);
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

	float baseY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY());

	// Gray the whole time it is alive: alive, it gives away nothing about what
	// it once was.
	out.AddShadow(depth, x, baseY, 30.f, 15.f);
	out.AddBox(depth, x, baseY, kFootWidth, kFootHeight, kBodyHeight, kBodyShade, kBodyShade, kBodyShade);
}
