#pragma once

// Every gameplay number in one place, so balancing means editing this file and
// nothing else. Distances and speeds are in tiles unless the name says px.
//
// Visual-only values (animation rates, model proportions, colours) stay next
// to the code that draws them.
namespace Tuning
{
	namespace Player
	{
		const float kMoveSpeed = 5.f; // tiles per second
		const float kRunMultiplier = 1.6f;

		const float kStartHealth = 100.f;
		const float kHitInvulnDuration = 0.4f;
	}

	namespace Dodge
	{
		const float kDuration = 0.3f;
		const float kDistance = 2.5f;
		// Counted from the start of the roll, so this includes the roll itself.
		const float kCooldown = 0.8f;
		const float kInvulnDuration = 0.3f;
	}

	namespace Death
	{
		// The world fades to dark, holds, and fades back with him at the start.
		const float kFadeOut = 0.8f;
		const float kHold = 0.4f;
		const float kFadeIn = 0.6f;
		// Outlasts the fade-in, so he is not struck the moment he can be seen.
		const float kReviveInvuln = 1.5f;
	}

	namespace FormSwitch
	{
		// Space is ignored until this runs out; a successful switch restarts it.
		const float kSwitchCooldown = 1.f;
	}

	namespace LightForm
	{
		const float kBoltCooldown = 0.3f;
		const float kBoltSpeedPx = 420.f;
		const float kBoltDamage = 1.f;
		const float kBoltLifetime = 1.1f;
		const float kBoltHitRadiusPx = 20.f;
	}

	namespace DarkForm
	{
		// Three cuts; the last one hits hardest.
		const int kComboLength = 3;
		const float kSlashDamage[kComboLength] = { 1.f, 1.f, 2.f };

		// Kept clear of Monster::kAttackRange, so a cut can land before the
		// monster is close enough to strike back.
		const float kSlashRange = 1.5f;
		const float kSlashArcDegrees = 120.f;
		// Anything this close counts as in front, whichever way he is facing.
		const float kPointBlank = 0.35f;

		const float kSlashInterval = 0.28f;   // between cuts while the combo runs
		const float kFinisherRecovery = 0.55f; // after the last cut
		const float kComboWindow = 0.7f;       // a longer pause starts the combo over
	}

	namespace Monster
	{
		// Measured on the ground in tiles, the same as the player's attacks, so
		// reach compares fairly in every direction on screen.
		const float kDetectRange = 5.5f;
		// Wider than detect, so one on the boundary does not flicker between
		// wandering and chasing.
		const float kGiveUpRange = 8.5f;
		const float kAttackRange = 0.9f;

		const float kAttackCooldown = 1.2f;
		const float kAttackDamage = 8.f;

		// Still in screen pixels until monster movement moves onto the grid.
		const float kChaseSpeedPx = 90.f;
		const float kRoamSpeedPx = 36.f;
		const float kRoamRadiusPx = 70.f;
	}

	namespace Vessel
	{
		const float kStartCapacity = 12.f;
		const float kCapacityPerUpgrade = 4.f;
		const float kHealthPerUpgrade = 10.f;

		// How far surplus poured into a full half tilts the player, per unit.
		const float kTiltPerOverflow = 0.05f;
	}
}
