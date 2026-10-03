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

		// Longer than the knights' start ranges, so a cut can land before one
		// of them begins winding up a blow of its own.
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

		// Tiles per second, before each kind's own scale.
		const float kChaseSpeed = 2.5f;
		const float kRoamSpeed = 1.f;
		const float kRoamRadius = 2.f;

		// A light knight's shield: light blows from in front of it, within
		// this arc, land at this share.
		const float kShieldArcDegrees = 120.f;
		const float kShieldFactor = 0.5f;

		// Mages back off once the player is this far inside the distance they
		// like to keep, and will cast from up to this far beyond it.
		const float kMageBackOff = 1.5f;
		const float kMageCastSlack = 1.5f;

		// How close a thrown spell has to pass to hit the player.
		const float kProjectileHitRadius = 0.35f;
	}

	// Eroded mage of the light: keeps its distance and chants standing still.
	namespace LightMage
	{
		const float kHealth = 25.f;
		const float kSpeedScale = 0.9f;
		const float kPreferredRange = 5.f;

		// A straight bolt, aimed where the player is when it lets go.
		const float kBoltWindup = 0.6f;
		const float kBoltSpeed = 6.f;
		const float kBoltLifetime = 2.f;
		const float kBoltDamage = 10.f;
		const float kBoltCooldown = 2.f;

		// Lightning marked on the ground where the player stood, landing later.
		const float kSmiteWindup = 0.4f;
		const float kSmiteDelay = 1.f;
		const float kSmiteRadius = 1.2f;
		const float kSmiteDamage = 18.f;
		const float kSmiteCooldown = 6.f;

		const float kRecover = 0.3f;
	}

	// Eroded mage of the dark: keeps its distance, drifting sideways as it casts.
	namespace DarkMage
	{
		const float kHealth = 25.f;
		const float kSpeedScale = 0.9f;
		const float kPreferredRange = 4.f;

		// Sidesteps around the player at this share of its chase speed,
		// turning back the other way every so often.
		const float kStrafeScale = 0.6f;
		const float kStrafeSwitchMin = 1.5f;
		const float kStrafeSwitchMax = 3.f;

		// A slow orb that bends after the player.
		const float kOrbWindup = 0.5f;
		const float kOrbSpeed = 4.5f;
		const float kOrbTurnRate = 1.8f; // radians per second
		const float kOrbLifetime = 4.f;
		const float kOrbDamage = 10.f;
		const float kOrbCooldown = 2.5f;

		// A cursed patch under the player: it takes shape first, then bites
		// every tick for as long as it lasts. Ticks are spaced past the
		// player's invulnerability after a hit, so each one can land.
		const float kCurseWindup = 0.5f;
		const float kCurseRadius = 1.3f;
		const float kCurseArmTime = 0.5f;
		const float kCurseDuration = 3.f;
		const float kCurseTickDamage = 4.f;
		const float kCurseTickInterval = 0.5f;
		const float kCurseCooldown = 7.f;

		const float kRecover = 0.3f;
	}

	// Eroded knight of the light: upright, slow, shielded, one honest cut.
	namespace LightKnight
	{
		const float kHealth = 50.f;
		const float kSpeedScale = 0.8f;

		// Starts winding up a cut once the player is this close.
		const float kStartRange = 1.f;
		const float kCleaveWindup = 0.5f;
		const float kCleaveRange = 1.2f;
		const float kCleaveArcDegrees = 100.f;
		const float kCleaveDamage = 14.f;
		const float kCleaveCooldown = 1.6f;
		const float kRecover = 0.6f;
	}

	// Eroded knight of the dark: low, quick, two blades, and a charge.
	namespace DarkKnight
	{
		const float kHealth = 35.f;
		const float kSpeedScale = 1.25f;

		const float kStartRange = 0.9f;
		const float kCutWindup = 0.3f;
		const float kCutRange = 1.1f;
		const float kCutArcDegrees = 120.f;
		const float kCutDamage = 7.f;
		// The second cut follows the first after this long - just past the
		// player's invulnerability after a hit, so both can land.
		const float kSecondCutGap = 0.45f;
		const float kCutCooldown = 1.2f;
		const float kRecover = 0.35f;

		// From this band of distance it may charge instead, the path marked
		// on the ground while it gathers itself.
		const float kChargeMinRange = 3.f;
		const float kChargeMaxRange = 5.f;
		const float kChargeWindup = 0.5f;
		const float kChargeDistance = 4.f;
		const float kChargeDuration = 0.3f;
		const float kChargeWidth = 0.8f;
		const float kChargeHitRadius = 0.7f;
		const float kChargeDamage = 12.f;
		const float kChargeCooldown = 5.f;
	}

	// How hard a blow is felt through the screen.
	namespace Feel
	{
		// Struck: the world catches for a moment and the screen jolts.
		const float kHurtStop = 0.06f;
		const float kHurtShake = 6.f;
		// A heavy blow of his landing - the third cut, a dash, a strike.
		const float kHeavyStop = 0.07f;
		const float kHeavyShake = 4.f;
		// Blows that land often, kept to a tremor.
		const float kLightShake = 1.5f;
		// Lightning hitting the ground, whether or not it catches anything.
		const float kStrikeShake = 7.f;
	}

	namespace Power
	{
		// Every attack of a power grows with the lifetime total of that power:
		// this much stronger per unit absorbed, so 100 absorbed doubles it.
		const float kScalePerAbsorbed = 0.01f;
	}

	namespace Balance
	{
		// How far the lifetime totals may drift apart before the gray takes
		// him, as a multiple of the vessel's capacity - so it widens as the
		// vessel grows.
		const float kAllowancePerCapacity = 2.5f;
		// Past this share of the allowance he is warned.
		const float kWarningShare = 0.5f;

		// Past the allowance the gray eats his health: this much per second
		// the moment he crosses, and this much more for every further
		// allowance-width he leans. It stops as soon as he is back inside.
		const float kErosionBaseRate = 4.f;
		const float kErosionPerExcess = 20.f;
	}
}
