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

	namespace SwapSkill
	{
		// Light -> dark: a burst at his feet that drags everything nearby in,
		// so the switch to melee arrives already in reach.
		const float kDarkBurstRadius = 2.f;
		const float kDarkBurstDamage = 15.f;
		const float kDarkBurstPull = 1.2f;

		// Dark -> light: a burst that throws everything back and covers the
		// way out of melee.
		const float kLightBurstRadius = 2.f;
		const float kLightBurstPush = 2.f;
		const float kLightBurstInvuln = 0.5f;
	}

	namespace Shove
	{
		// A push or pull plays out over this long, and the monster does nothing
		// else meanwhile.
		const float kDuration = 0.18f;
		// A pull stops this far from its centre instead of stacking everything
		// on top of the player.
		const float kPullStop = 0.6f;
	}

	namespace LightForm
	{
		const float kBoltCooldown = 0.3f;
		const float kBoltSpeedPx = 420.f;
		const float kBoltDamage = 10.f;
		const float kBoltLifetime = 1.1f;
		const float kBoltHitRadiusPx = 20.f;

		// E: a beam straight through everything in a line.
		const float kBeamLength = 7.f;
		const float kBeamWidth = 0.6f;
		const float kBeamDamage = 25.f;
		const float kBeamCooldown = 4.f;

		// Q: lightning called down on the cursor - slow to land, wide when it
		// does.
		const float kStrikeMaxRange = 7.f;
		const float kStrikeDelay = 0.8f;
		const float kStrikeRadius = 2.f;
		const float kStrikeDamage = 50.f;
		const float kStrikeCooldown = 10.f;
	}

	namespace DarkForm
	{
		// Three cuts; the last one hits hardest.
		const int kComboLength = 3;
		const float kSlashDamage[kComboLength] = { 10.f, 10.f, 20.f };

		// Share of melee damage dealt that comes back as health. Only blows that
		// land in reach count - never anything thrown or lingering.
		const float kLifesteal = 0.09f;

		// Kept clear of Monster::kAttackRange, so a cut can land before the
		// monster is close enough to strike back.
		const float kSlashRange = 1.5f;
		const float kSlashArcDegrees = 120.f;
		// Anything this close counts as in front, whichever way he is facing.
		const float kPointBlank = 0.35f;

		const float kSlashInterval = 0.28f;   // between cuts while the combo runs
		const float kFinisherRecovery = 0.55f; // after the last cut
		const float kComboWindow = 0.7f;       // a longer pause starts the combo over

		// E: a short dash through whatever is in the way, cutting all of it.
		// Invulnerable for the length of the dash.
		const float kDashDistance = 3.f;
		const float kDashDuration = 0.2f;
		const float kDashWidth = 1.f;
		const float kDashDamage = 20.f;
		const float kDashCooldown = 5.f;

		// Q: a spin that keeps cutting everything around him for a while.
		const float kSpinDuration = 1.5f;
		const float kSpinInterval = 0.25f;
		const float kSpinRadius = 1.6f;
		const float kSpinDamage = 8.f;
		const float kSpinLifestealScale = 2.f;
		const float kSpinCooldown = 10.f;
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

		const float kBruteHealth = 50.f;
		const float kStalkerHealth = 20.f;
		const float kCrawlerHealth = 30.f;

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
