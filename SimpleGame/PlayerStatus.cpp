#include "stdafx.h"
#include "PlayerStatus.h"

namespace
{
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

	if (formCooldown > 0.f)
	{
		formCooldown -= deltaTime;
		if (formCooldown < 0.f)
		{
			formCooldown = 0.f;
		}
	}
}

void PlayerStatus::AddLight(float amount)
{
	light += amount;
	if (light > capacity)
	{
		tilt = Clamp(tilt + (light - capacity) * Tuning::Vessel::kTiltPerOverflow, -1.f, 1.f);
		light = capacity;
	}
	SpendIfBothFull();
}

void PlayerStatus::AddDark(float amount)
{
	dark += amount;
	if (dark > capacity)
	{
		tilt = Clamp(tilt - (dark - capacity) * Tuning::Vessel::kTiltPerOverflow, -1.f, 1.f);
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
	capacity += Tuning::Vessel::kCapacityPerUpgrade;
	maxHealth += Tuning::Vessel::kHealthPerUpgrade;
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
	m_InvulnTimer = Tuning::Player::kHitInvulnDuration;
	return true;
}

void PlayerStatus::GrantInvuln(float seconds)
{
	if (seconds > m_InvulnTimer)
	{
		m_InvulnTimer = seconds;
	}
}

void PlayerStatus::Revive()
{
	health = maxHealth;
	formCooldown = 0.f;
	GrantInvuln(Tuning::Death::kReviveInvuln);
}

bool PlayerStatus::TrySwitchForm()
{
	if (formCooldown > 0.f)
	{
		return false;
	}

	form = (form == Form::Light) ? Form::Dark : Form::Light;
	formCooldown = Tuning::FormSwitch::kSwitchCooldown;
	return true;
}
