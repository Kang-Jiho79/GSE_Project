#include "stdafx.h"
#include "PlayerStatus.h"
#include <cmath>

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

	lightSkill.Tick(deltaTime);
	lightSpecial.Tick(deltaTime);
	darkSkill.Tick(deltaTime);
	darkSpecial.Tick(deltaTime);

	if (!IsEroding())
	{
		erosionTime = 0.f;
		return;
	}

	erosionTime += deltaTime;

	// Not a blow, so invulnerability does not hold it off.
	float excess = fabsf(BalanceRatio()) - 1.f;
	float rate = Tuning::Balance::kErosionBaseRate + Tuning::Balance::kErosionPerExcess * excess;
	health = Clamp(health - rate * deltaTime, 0.f, maxHealth);
}

const Cooldown& PlayerStatus::SkillCooldown(Form which) const
{
	return (which == Form::Light) ? lightSkill : darkSkill;
}

const Cooldown& PlayerStatus::SpecialCooldown(Form which) const
{
	return (which == Form::Light) ? lightSpecial : darkSpecial;
}

void PlayerStatus::AddLight(float amount)
{
	lightTotal += amount;
	light = Clamp(light + amount, 0.f, capacity);
	SpendIfBothFull();
}

void PlayerStatus::AddDark(float amount)
{
	darkTotal += amount;
	dark = Clamp(dark + amount, 0.f, capacity);
	SpendIfBothFull();
}

float PlayerStatus::Balance() const
{
	return lightTotal - darkTotal;
}

float PlayerStatus::BalanceAllowance() const
{
	return capacity * Tuning::Balance::kAllowancePerCapacity;
}

float PlayerStatus::BalanceRatio() const
{
	return Balance() / BalanceAllowance();
}

float PlayerStatus::WarningLevel() const
{
	float lean = fabsf(BalanceRatio());
	float share = Tuning::Balance::kWarningShare;
	return Clamp((lean - share) / (1.f - share), 0.f, 1.f);
}

bool PlayerStatus::IsEroding() const
{
	return health > 0.f && fabsf(BalanceRatio()) > 1.f;
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

void PlayerStatus::Heal(float amount)
{
	if (health <= 0.f)
	{
		return;
	}

	health = Clamp(health + amount, 0.f, maxHealth);
}

void PlayerStatus::Revive()
{
	if (fabsf(BalanceRatio()) > 1.f)
	{
		float kept = BalanceAllowance() * Tuning::Balance::kWarningShare;
		float balance = Balance();
		if (balance > 0.f)
		{
			lightTotal -= balance - kept;
		}
		else
		{
			darkTotal -= -balance - kept;
		}
	}

	erosionTime = 0.f;
	health = maxHealth;
	formCooldown = 0.f;
	formLocked = false;
	lightSkill = Cooldown();
	lightSpecial = Cooldown();
	darkSkill = Cooldown();
	darkSpecial = Cooldown();
	GrantInvuln(Tuning::Death::kReviveInvuln);
}

bool PlayerStatus::TrySwitchForm()
{
	if (formCooldown > 0.f || formLocked)
	{
		return false;
	}

	form = (form == Form::Light) ? Form::Dark : Form::Light;
	formCooldown = Tuning::FormSwitch::kSwitchCooldown;
	return true;
}
