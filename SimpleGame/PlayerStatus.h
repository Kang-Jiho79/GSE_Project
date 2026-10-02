#pragma once

#include "Form.h"
#include "Tuning.h"

// The vessel (그릇) and its consequences, shared between the world and the HUD.
// Power banks into two halves; only when both fill do they spend together and
// upgrade the player. Surplus poured into an already-full half leaks out and
// tilts the player away from the midpoint.
//
// The brief window of invulnerability after a hit lives here too, so several
// monsters landing on the same frame cannot all bill the player for it.
//
// The current form lives here rather than on the actor, because the HUD has to
// show it and how long until it can change again.
struct PlayerStatus
{
	float light = 0.f;
	float dark = 0.f;
	float capacity = Tuning::Vessel::kStartCapacity;

	float health = Tuning::Player::kStartHealth;
	float maxHealth = Tuning::Player::kStartHealth;

	// -1 is fully dark, 0 is still the midpoint, +1 is fully light
	float tilt = 0.f;

	int upgrades = 0;

	Form form = Form::Light;
	float formCooldown = 0.f; // seconds until Space works again

	void Tick(float deltaTime);

	void AddLight(float amount);
	void AddDark(float amount);

	// Returns whether the hit actually landed.
	bool TryDamage(float amount);

	// Hits are ignored for at least this long. Never shortens a window already
	// running.
	void GrantInvuln(float seconds);

	// Returns whether the form actually changed - false while cooling down.
	bool TrySwitchForm();

	// Back to full health after a death. Absorbed power is kept.
	void Revive();

private:
	void SpendIfBothFull();

	float m_InvulnTimer = 0.f;
};
