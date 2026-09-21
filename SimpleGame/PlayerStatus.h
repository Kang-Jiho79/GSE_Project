#pragma once

// The vessel (그릇) and its consequences, shared between the world and the HUD.
// Power banks into two halves; only when both fill do they spend together and
// upgrade the player. Surplus poured into an already-full half leaks out and
// tilts the player away from the midpoint.
//
// The brief window of invulnerability after a hit lives here too, so several
// monsters landing on the same frame cannot all bill the player for it.
struct PlayerStatus
{
	float light = 0.f;
	float dark = 0.f;
	float capacity = 12.f;

	float health = 100.f;
	float maxHealth = 100.f;

	// -1 is fully dark, 0 is still the midpoint, +1 is fully light
	float tilt = 0.f;

	int upgrades = 0;

	void Tick(float deltaTime);

	void AddLight(float amount);
	void AddDark(float amount);

	// Returns whether the hit actually landed.
	bool TryDamage(float amount);

private:
	void SpendIfBothFull();

	float m_InvulnTimer = 0.f;
};
