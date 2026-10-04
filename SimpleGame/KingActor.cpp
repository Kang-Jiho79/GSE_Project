#include "stdafx.h"
#include "KingActor.h"
#include "PlayerStatus.h"
#include "Progress.h"
#include "Impact.h"
#include "Tuning.h"

namespace
{
	const float kRemarkFade = 0.8f;
	const float kKneelTime = 0.6f;
}

KingActor::KingActor(const char* name, int region, float maxHealth, float furyTime, Form power, const Lines& lines,
	bool recognized)
	: BossActor(name, maxHealth, region, -1.f, Tuning::Duel::kLeashRange, furyTime)
	, m_Power(power)
	, m_Lines(lines)
	, m_Recognized(recognized)
{
}

float KingActor::TakeHit(float damage)
{
	if (!m_Dueling)
	{
		return 0.f;
	}
	return BossActor::TakeHit(damage);
}

Form KingActor::Power() const
{
	return m_Power;
}

bool KingActor::CanChallenge() const
{
	return !m_Recognized && !m_Yielded && !m_Dueling && IsDormant();
}

bool KingActor::IsDueling() const
{
	return m_Dueling;
}

bool KingActor::HasYielded() const
{
	return m_Yielded;
}

float KingActor::Kneel() const
{
	if (!m_Yielded)
	{
		return 0.f;
	}
	return (m_YieldTime < kKneelTime) ? m_YieldTime / kKneelTime : 1.f;
}

void KingActor::BeginDuel(PlayerStatus& status)
{
	status.health = status.maxHealth;
	status.formLocked = true;
	status.formCooldown = 0.f;

	m_Dueling = true;
	RequestWake();
	Say(m_Lines.challenge, nullptr);
}

void KingActor::EndDuel(PlayerStatus& status)
{
	m_Dueling = false;
	status.formLocked = false;
}

bool KingActor::TakeSendOff()
{
	if (!m_SendOffPending)
	{
		return false;
	}
	m_SendOffPending = false;
	m_SentOff = true;
	return true;
}

void KingActor::Say(const char* line, const char* aside)
{
	m_Remark = line;
	m_Aside = aside;
	m_RemarkTimer = Tuning::Duel::kRemarkTime;
}

const char* KingActor::Remark() const
{
	return (m_RemarkTimer > 0.f) ? m_Remark : nullptr;
}

const char* KingActor::Aside() const
{
	return (m_RemarkTimer > 0.f) ? m_Aside : nullptr;
}

float KingActor::RemarkAlpha() const
{
	if (m_RemarkTimer <= 0.f)
	{
		return 0.f;
	}
	return (m_RemarkTimer < kRemarkFade) ? m_RemarkTimer / kRemarkFade : 1.f;
}

void KingActor::OnUpdate(const ActorContext& ctx)
{
	if (m_RemarkTimer > 0.f)
	{
		m_RemarkTimer -= ctx.deltaTime;
	}

	// He has fallen in the duel: it will not take his life. He is left on
	// his feet with a little health, and it goes back to its place.
	if (m_Dueling && ctx.status->health <= 0.f)
	{
		EndDuel(*ctx.status);
		ctx.status->health = ctx.status->maxHealth * Tuning::Duel::kSpareHealth;
		ctx.status->GrantInvuln(Tuning::Duel::kSpareInvuln);
		ReturnHome(ctx);
		Say(m_Lines.spare, nullptr);
		return;
	}

	BossActor::OnUpdate(ctx);
}

void KingActor::OnReset(const ActorContext& ctx)
{
	// Reset while the duel was still on means he walked away from it.
	if (m_Dueling)
	{
		EndDuel(*ctx.status);
		Say(m_Lines.flee, nullptr);
	}
	ResetMoves();
}

void KingActor::Defeated(const ActorContext& ctx)
{
	if (!m_Yielded)
	{
		m_Yielded = true;
		m_YieldTime = 0.f;
		EndDuel(*ctx.status);
		// Both fell on the same blow: the win is his, and so is his life.
		if (ctx.status->health <= 0.f)
		{
			ctx.status->health = ctx.status->maxHealth * Tuning::Duel::kSpareHealth;
		}
		Reward(ctx);
		ctx.impact->HitStop(Tuning::Boss::kDefeatStop);
		ctx.impact->Shake(Tuning::Boss::kDefeatShake);
		Say(m_Lines.yield, m_Lines.blessing);
		return;
	}

	m_YieldTime += ctx.deltaTime;
	if (!m_SentOff && m_YieldTime >= Tuning::Duel::kSendOffDelay)
	{
		m_SendOffPending = true;
	}
}
