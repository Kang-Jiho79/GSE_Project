#include "stdafx.h"
#include "PlayerStatus.h"

namespace
{
	const float kTiltPerOverflow = 0.05f;
	const float kCapacityPerUpgrade = 4.f;
	const float kHealthPerUpgrade = 10.f;
	const float kInvulnDuration = 0.4f;

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

void PlayerStatus::Tick(float deltaTime)
{
	if (m_InvulnTimer > 0.f)
	{
		m_InvulnTimer -= deltaTime;
	}
}

void PlayerStatus::AddLight(float amount)
{
	light += amount;
	if (light > capacity)
	{
		tilt = Clamp(tilt + (light - capacity) * kTiltPerOverflow, -1.f, 1.f);
		light = capacity;
	}
	SpendIfBothFull();
}

void PlayerStatus::AddDark(float amount)
{
	dark += amount;
	if (dark > capacity)
	{
		tilt = Clamp(tilt - (dark - capacity) * kTiltPerOverflow, -1.f, 1.f);
		dark = capacity;
	}
	SpendIfBothFull();
}

void PlayerStatus::SpendIfBothFull()
{
	if (light < capacity || dark < capacity)
	{
		return;
	}

	light = 0.f;
	dark = 0.f;
	capacity += kCapacityPerUpgrade;
	maxHealth += kHealthPerUpgrade;
	health = maxHealth;
	upgrades++;
}

bool PlayerStatus::TryDamage(float amount)
{
	if (m_InvulnTimer > 0.f)
	{
		return false;
	}

	health = Clamp(health - amount, 0.f, maxHealth);
	m_InvulnTimer = kInvulnDuration;
	return true;
}
