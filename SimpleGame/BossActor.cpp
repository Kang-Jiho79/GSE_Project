#include "stdafx.h"
#include "BossActor.h"
#include "ParticleActor.h"
#include "SparkActor.h"
#include "SceneGraph.h"
#include "PlayerStatus.h"
#include "Progress.h"
#include "Impact.h"
#include "WorldGrid.h"
#include "Random.h"
#include "Tuning.h"
#include "DrawList.h"
#include "Palette.h"
#include <cmath>

namespace
{
	const float kHitFlashTime = 0.12f;

	const int kHitSparks = 10;
	const float kHitSparkSpeed = 180.f;
	const float kHitSparkLift = 30.f;

	const float kMoteSpeed = 160.f;

	// A blow turned aside throws gray grit instead of sparks.
	const int kDeflectDust = 6;
	const float kDeflectSpeed = 120.f;
}

BossActor::BossActor(const char* name, float maxHealth, int region, float wakeRange, float leashRange, float furyTime)
	: m_Name(name)
	, m_MaxHealth(maxHealth)
	, m_Health(maxHealth)
	, m_Region(region)
	, m_WakeRange(wakeRange)
	, m_LeashRange(leashRange)
	, m_FuryTime(furyTime)
{
	m_Time = Random::Unit() * 10.f;
}

const char* BossActor::Name() const
{
	return m_Name;
}

float BossActor::HealthFraction() const
{
	return (m_Health > 0.f) ? m_Health / m_MaxHealth : 0.f;
}

bool BossActor::IsAwake() const
{
	return m_Awake;
}

const char* BossActor::Hint() const
{
	return nullptr;
}

int BossActor::FuryLevel() const
{
	return m_FuryLevel;
}

float BossActor::FurySpeed() const
{
	return 1.f + m_FuryLevel * Tuning::Boss::kFurySpeedPerLevel;
}

float BossActor::FuryDamage() const
{
	return 1.f + m_FuryLevel * Tuning::Boss::kFuryDamagePerLevel;
}

Form BossActor::PlayerForm() const
{
	return m_PlayerForm;
}

void BossActor::DrawFury(DrawList& out, float depth, float x, float groundY, float radius) const
{
	if (m_FuryLevel <= 0)
	{
		return;
	}

	float strength = 0.35f + 0.08f * m_FuryLevel;
	if (strength > 1.f)
	{
		strength = 1.f;
	}
	float flicker = 0.85f + 0.15f * sinf(m_Time * 11.f);

	out.SetGlow(0.6f);
	out.AddGlow(depth, x, groundY, radius, Palette::kFury.r, Palette::kFury.g, Palette::kFury.b, strength * flicker);
	out.SetGlow(0.f);
}

bool BossActor::IsDormant() const
{
	return !m_Awake;
}

float BossActor::Facing() const
{
	return m_Facing;
}

float BossActor::Time() const
{
	return m_Time;
}

float BossActor::HitFlash() const
{
	return (m_HitFlash > 0.f) ? m_HitFlash : 0.f;
}

void BossActor::OnReset(const ActorContext& ctx)
{
}

float BossActor::Resist(float damage)
{
	return damage;
}

void BossActor::RequestWake()
{
	m_WakeRequested = true;
}

void BossActor::Mend(float amount)
{
	if (m_Health <= 0.f)
	{
		return;
	}
	m_Health += amount;
	if (m_Health > m_MaxHealth)
	{
		m_Health = m_MaxHealth;
	}
}

void BossActor::ReturnHome(const ActorContext& ctx)
{
	m_Awake = false;
	m_WakeRequested = false;
	m_Health = m_MaxHealth;
	m_AwakeTime = 0.f;
	m_FuryLevel = 0;
	SetLocalPosition(m_HomeX, m_HomeY);
	OnReset(ctx);
}

void BossActor::Defeated(const ActorContext& ctx)
{
	Reward(ctx);
	ctx.progress->bossDefeated[m_Region] = true;
	ctx.impact->HitStop(Tuning::Boss::kDefeatStop);
	ctx.impact->Shake(Tuning::Boss::kDefeatShake);
	Destroy();
}

float BossActor::TakeHit(float damage)
{
	if (m_Health <= 0.f)
	{
		return 0.f;
	}

	// Struck in its sleep, it wakes.
	m_WakeRequested = true;

	damage = Resist(damage);
	if (damage <= 0.f)
	{
		m_DeflectPending = true;
		return 0.f;
	}

	float landed = (damage < m_Health) ? damage : m_Health;
	m_Health -= damage;
	m_HitFlash = 1.f;
	m_SparksPending = true;
	return landed;
}

void BossActor::OnUpdate(const ActorContext& ctx)
{
	if (!m_Anchored)
	{
		m_HomeX = WorldX();
		m_HomeY = WorldY();
		m_Anchored = true;
	}

	m_Time += ctx.deltaTime;
	if (m_HitFlash > 0.f)
	{
		m_HitFlash -= ctx.deltaTime / kHitFlashTime;
	}

	m_PlayerForm = ctx.status->form;

	if (m_DeflectPending)
	{
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), SparkActor::Style::Dust, kDeflectDust, kDeflectSpeed, kHitSparkLift);
		m_DeflectPending = false;
	}

	if (m_SparksPending)
	{
		SparkActor::Style style = SparkActor::StyleFor(ctx.status->form);
		SparkActor::Burst(*ctx.graph, WorldX(), WorldY(), style, kHitSparks, kHitSparkSpeed, kHitSparkLift);
		m_SparksPending = false;
	}

	if (m_Health <= 0.f)
	{
		Defeated(ctx);
		return;
	}

	float dirCol, dirRow;
	float distance = ToPlayer(ctx, dirCol, dirRow);

	if (!m_Awake)
	{
		if (distance <= m_WakeRange || m_WakeRequested)
		{
			m_Awake = true;
			m_WakeRequested = false;
			ctx.impact->Shake(Tuning::Boss::kWakeShake);
		}
		return;
	}

	// Run far enough from its place and it gives up: back home, whole again.
	float homeCol, homeRow, playerCol, playerRow;
	WorldGrid::InverseIso(m_HomeX, m_HomeY, homeCol, homeRow);
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
	float fromHomeCol = playerCol - homeCol;
	float fromHomeRow = playerRow - homeRow;
	if (fromHomeCol * fromHomeCol + fromHomeRow * fromHomeRow > m_LeashRange * m_LeashRange)
	{
		ReturnHome(ctx);
		return;
	}

	// Past its time, it rages harder with every interval.
	m_AwakeTime += ctx.deltaTime;
	if (m_AwakeTime > m_FuryTime)
	{
		int level = 1 + (int)((m_AwakeTime - m_FuryTime) / Tuning::Boss::kFuryInterval);
		if (level > Tuning::Boss::kFuryMaxLevel)
		{
			level = Tuning::Boss::kFuryMaxLevel;
		}
		if (level > m_FuryLevel)
		{
			m_FuryLevel = level;
			ctx.impact->Shake(Tuning::Boss::kFuryShake);
		}
	}

	Fight(ctx);
}

void BossActor::ScatterMotes(const ActorContext& ctx, Alignment alignment, int count) const
{
	for (int i = 0; i < count; i++)
	{
		float angle = Random::Angle();
		float speed = kMoteSpeed * (0.4f + Random::Unit() * 0.6f);

		std::unique_ptr<ParticleActor> mote(
			new ParticleActor(alignment, cosf(angle) * speed, sinf(angle) * speed));
		mote->SetLocalPosition(WorldX(), WorldY());
		ctx.graph->Spawn(std::move(mote));
	}
}

float BossActor::ToPlayer(const ActorContext& ctx, float& dirCol, float& dirRow) const
{
	float col, row, playerCol, playerRow;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);

	float toCol = playerCol - col;
	float toRow = playerRow - row;
	float distance = sqrtf(toCol * toCol + toRow * toRow);
	dirCol = (distance > 0.001f) ? toCol / distance : 1.f;
	dirRow = (distance > 0.001f) ? toRow / distance : 0.f;
	return distance;
}

void BossActor::StepAlong(float dirCol, float dirRow, float tiles)
{
	float col, row;
	WorldGrid::InverseIso(LocalX(), LocalY(), col, row);
	WorldGrid::Move(col, row, dirCol * tiles, dirRow * tiles);

	float x, y;
	WorldGrid::IsoPosition(col, row, x, y);
	SetLocalPosition(x, y);
}

void BossActor::FaceAlong(float dirCol, float dirRow)
{
	float screenX, screenY;
	WorldGrid::IsoPosition(dirCol, dirRow, screenX, screenY);
	if (fabsf(screenX) > 0.01f)
	{
		m_Facing = (screenX < 0.f) ? -1.f : 1.f;
	}
}
