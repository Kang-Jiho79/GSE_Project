#pragma once

#include "Form.h"
#include "Tuning.h"
#include "Cooldown.h"

// The vessel (그릇) and its consequences, shared between the world and the HUD.
// Power banks into two halves; only when both fill do they spend together and
// upgrade the player. Surplus poured into an already-full half is lost to the
// vessel.
//
// Everything absorbed is also counted for good in the lifetime totals, which
// the vessel emptying never touches. They are what his power and his balance
// are measured by.
//
// The brief window of invulnerability after a hit lives here too, so several
// monsters landing on the same frame cannot all bill the player for it.
//
// The current form lives here rather than on the actor, because the HUD has to
// show it and how long until it can change again.
struct PlayerStatus
{
	// The vessel: progress toward the next upgrade, emptied by it.
	float light = 0.f;
	float dark = 0.f;
	float capacity = Tuning::Vessel::kStartCapacity;

	// Everything ever absorbed.
	float lightTotal = 0.f;
	float darkTotal = 0.f;

	float health = Tuning::Player::kStartHealth;
	float maxHealth = Tuning::Player::kStartHealth;

	int upgrades = 0;

	Form form = Form::Light;
	float formCooldown = 0.f; // seconds until Space works again
	// Space does nothing while this is set: a king's duel holds him to the
	// king's own power.
	bool formLocked = false;

	// The kings' blessings, kept for good once given. Lucius's makes his
	// light blows hit harder; Noctis's gives back more of what the dark's take.
	bool lightBlessing = false;
	bool darkBlessing = false;

	// E and Q for each form. Kept here rather than on the forms so the HUD can
	// show them; they run down whichever form he is in.
	Cooldown lightSkill;
	Cooldown lightSpecial;
	Cooldown darkSkill;
	Cooldown darkSpecial;

	const Cooldown& SkillCooldown(Form which) const;
	const Cooldown& SpecialCooldown(Form which) const;

	void Tick(float deltaTime);

	void AddLight(float amount);
	void AddDark(float amount);

	// How far the totals lean: positive toward light, negative toward dark.
	float Balance() const;
	// How far they may lean before the gray takes him.
	float BalanceAllowance() const;
	// Balance as a share of the allowance: -1 and +1 are its edges.
	float BalanceRatio() const;
	// 0 until the warning starts, rising to 1 at the edge of the allowance.
	float WarningLevel() const;
	// Leaning past the allowance, and being eaten by the gray for it.
	bool IsEroding() const;

	// Seconds spent eroding without a break, for anything that pulses with it.
	float erosionTime = 0.f;

	// Returns whether the hit actually landed.
	bool TryDamage(float amount);

	// Hits are ignored for at least this long. Never shortens a window already
	// running.
	void GrantInvuln(float seconds);

	// Never past max health, and nothing for someone already fallen.
	void Heal(float amount);

	// Returns whether the form actually changed - false while cooling down or
	// held to one form.
	bool TrySwitchForm();

	// Back to full health after a death, with every skill ready. Absorbed
	// power is kept - unless he fell leaning past the allowance, in which case
	// the heavier side is drained back to where the warning starts, so he
	// does not wake straight into the gray again.
	void Revive();

private:
	void SpendIfBothFull();

	float m_InvulnTimer = 0.f;
};
