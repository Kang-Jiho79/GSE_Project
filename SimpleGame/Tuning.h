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

	namespace Travel
	{
		// Leaving a region: a quicker version of the death fade.
		const float kFadeOut = 0.35f;
		const float kHold = 0.15f;
		const float kFadeIn = 0.45f;

		// How close he must stand to a waystone to use it, in tiles.
		const float kWaystoneReach = 1.6f;
		// Monsters are not placed within this many tiles of it, so he never
		// arrives in the middle of a fight.
		const float kSpawnClearance = 8.f;
	}

	// The prologue: a little while of the hidden village at peace, then the
	// gray comes for it.
	namespace Prologue
	{
		const float kCalmTime = 15.f;      // seconds of peace before the raid
		const int kRaiders = 6;
		const float kRaidRing = 6.5f;      // tiles from the square they come in at
		const float kTakeTime = 5.f;       // into the raid, the villagers are lost
		const float kRaidShake = 8.f;
		const float kAftermathDelay = 1.5f; // after the last of them falls
	}

	// The story's events between the prologue and the end.
	namespace Story
	{
		// Kinis speaks to him once he comes this near, in tiles.
		const float kKinisReach = 4.f;

		// A rescue: this many of the gray round the one being saved, this far
		// from them; it is over when none are left within kRescueArea of them,
		// and they speak once he is within kRescueTalk.
		const int kRescueRaiders = 4;
		const float kRescueRing = 3.f;
		const float kRescueArea = 9.f;
		const float kRescueTalk = 7.f;

		// The god's hold only lets go of one this strong: both lifetime totals
		// together, on top of having seen both kings fail to hear him.
		const float kChosenPower = 400.f;

		// Kinis, at the gray's heart, speaks once he comes this near; the two
		// he saved meet again at the front once he is this near them.
		const float kKinisTalk = 6.f;
		const float kReunionReach = 6.f;
	}

	// Kinis, the first of the gray. Both powers he once held still answer
	// him: a lance of light from afar, a cleave of darkness up close, and a
	// ring of the gray they became between.
	namespace Kinis
	{
		const float kMaxHealth = 1500.f;
		const float kBodyRadius = 0.6f;
		const float kWakeRange = 8.f;
		const float kLeashRange = 18.f;
		const float kFuryTime = 180.f;

		const float kSpeed = 2.6f;
		const float kKeepRange = 3.5f;
		const float kCooldown = 1.1f;
		const float kRecover = 0.5f;

		// Below this share of health: shorter windups, three lances at once,
		// and a fuller ring.
		const float kPhaseShare = 0.5f;
		const float kPhaseWindupScale = 0.8f;

		const float kLanceWindup = 0.7f;
		const float kLanceLength = 10.f;
		const float kLanceWidth = 1.2f;
		const float kLanceDamage = 20.f;
		const float kLanceSpread = 25.f; // degrees between lances, once there are three
		const float kLanceShake = 4.f;

		const float kCleaveWindup = 0.5f;
		const float kCleaveReach = 2.4f;  // he cleaves instead of lancing this near
		const float kCleaveRange = 2.6f;
		const float kCleaveArc = 200.f;
		const float kCleaveDamage = 22.f;

		const float kRingWindup = 0.9f;
		const int kRingCount = 12;
		const int kPhaseRingCount = 16;
		const float kRingSpeed = 4.5f;
		const float kRingLifetime = 3.f;
		const float kRingDamage = 12.f;
	}

	// The god (「   」): an empty place in the sky over its own board, and
	// two hands. Three wards in turn, light, dark, then the gray, each over a
	// third of its life.
	namespace God
	{
		const float kMaxHealth = 4500.f;
		const float kBodyRadius = 1.4f;
		const float kWakeRange = 30.f;
		const float kLeashRange = 99.f;   // nowhere to run to in its place
		const float kFuryTime = 300.f;

		const float kCooldown = 1.2f;
		const float kRecover = 0.6f;
		const float kShiftTime = 2.f;     // its ward turning over to the next power
		const float kShiftShake = 9.f;
		// Under the last ward, everything comes this much sooner.
		const float kLastWardWindupScale = 0.8f;

		// A hand coming down where he stood.
		const float kPlaceWindup = 1.f;
		const float kPlaceRadius = 1.8f;
		const float kPlaceDamage = 26.f;
		const float kPlaceShake = 6.f;

		// A hand sweeping a line across the board, across the way to it.
		const float kSweepWindup = 1.1f;
		const float kSweepLength = 18.f;
		const float kSweepWidth = 1.6f;
		const float kSweepDamage = 22.f;

		// From the second ward: every other square round him strikes.
		const float kBoardWindup = 1.3f;
		const int kBoardRadius = 5;
		const float kBoardDamage = 20.f;

		// Under the last: shot of all three powers, all round.
		const float kRingWindup = 1.f;
		const int kRingCount = 18;
		const float kRingSpeed = 4.2f;
		const float kRingLifetime = 4.f;
		const float kRingDamage = 12.f;
	}

	// Talking to people.
	namespace Dialogue
	{
		const float kTalkReach = 1.3f;     // tiles from someone to speak to them
		const float kTypeRate = 40.f;      // characters a second as a line is written out
		const float kGlitchRate = 12.f;    // times a second broken words churn
		const int kLogEntries = 200;       // lines the log keeps
	}

	namespace FormSwitch
	{
		// Space is ignored until this runs out; a successful switch restarts it.
		const float kSwitchCooldown = 1.f;
	}

	// The gray, taken up at the very end: neither near nor far, and best at
	// undoing - shields, wards, and shots in the air.
	namespace AshForm
	{
		// The wave: a wedge reaching past a blade and short of a bolt.
		const float kWaveInterval = 0.45f;
		const float kWaveRange = 3.2f;
		const float kWaveArcDegrees = 70.f;
		const float kWaveDamage = 14.f;

		// E, unravel: a line through everything, shields and wards included.
		const float kUnravelLength = 7.f;
		const float kUnravelWidth = 1.4f;
		const float kUnravelDamage = 30.f;
		const float kUnravelCooldown = 5.f;

		// Q, erase: every shot round him wiped away, a heavy blow to all
		// within reach, and a moment untouched.
		const float kEraseRadius = 3.5f;
		const float kEraseDamage = 35.f;
		const float kEraseInvuln = 0.5f;
		const float kEraseCooldown = 12.f;

		// Switching in: the shots nearest him wiped away.
		const float kVeilRadius = 2.5f;
		const float kVeilInvuln = 0.3f;
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

	namespace Boss
	{
		// Waking and falling are felt through the screen.
		const float kWakeShake = 6.f;
		const float kDefeatStop = 0.15f;
		const float kDefeatShake = 10.f;

		// Fury: once a boss has been awake past its own time limit, it grows
		// stronger every interval - faster and harder-hitting a step at a
		// time - until it can hardly be survived. Stalling on lifesteal is no
		// way to win. Running far enough away resets it with the rest.
		const float kFuryInterval = 30.f;
		const float kFurySpeedPerLevel = 0.15f;
		const float kFuryDamagePerLevel = 0.25f;
		const int kFuryMaxLevel = 8;
		const float kFuryShake = 4.f;
	}

	// A body the gray swallowed whole: huge, many-armed, slow, and every blow
	// shown on the ground before it falls.
	namespace Mutant
	{
		const float kWakeRange = 7.f;
		const float kLeashRange = 16.f;
		const float kBodyRadius = 1.1f;
		const float kFuryTime = 180.f;

		const float kSpeed = 1.6f;          // tiles per second
		const float kKeepRange = 1.8f;      // stops closing in this near
		const float kCooldown = 1.4f;       // between blows
		const float kRecover = 0.6f;

		// Below this share of its health it rages: faster, quicker blows, and
		// it calls eroded soldiers to it once.
		const float kRageShare = 0.5f;
		const float kRageSpeedScale = 1.3f;
		const float kRageWindupScale = 0.75f;
		const float kRageCooldownScale = 0.7f;
		const int kGuards = 2;

		// Slam: a circle marked under him, then struck.
		const float kSlamWindup = 1.f;
		const float kSlamRadius = 1.6f;
		const float kSlamDamage = 22.f;
		const float kSlamShake = 5.f;

		// Sweep: everything in a half circle in front of it, when he is close.
		const float kSweepRange = 3.2f;
		const float kSweepWindup = 0.7f;
		const float kSweepDamage = 16.f;

		// Burst: a ring of gray spat out in every direction, every third blow.
		const float kBurstWindup = 0.8f;
		const int kBurstCount = 8;
		const int kRageBurstCount = 12;
		const float kBurstSpeed = 4.5f;
		const float kBurstLifetime = 2.6f;
		const float kBurstDamage = 10.f;

		// What it leaves: this many motes of what it was - split evenly if it
		// was of both.
		const int kRewardMotes = 20;
	}

	// A piece the god set on its board to guard the way to each king. It moves
	// only as a piece may - the light's along straight lines, the dark's in
	// leaps - and tests his balance: while its ward burns gold only the dark
	// hurts it, while violet only the light.
	namespace GodPiece
	{
		const float kWakeRange = 7.f;
		const float kLeashRange = 16.f;
		const float kBodyRadius = 0.9f;
		const float kFuryTime = 150.f;

		const float kWardTime = 6.f;     // between its ward turning over
		const float kWardWarning = 1.f;  // it flickers this long before it does

		const float kCooldown = 1.2f;
		const float kRecover = 0.7f;

		// The light's piece: slides along a line through him - a straight one
		// or a diagonal, nothing in between.
		const float kSlideWindup = 0.7f;
		const float kSlideMax = 7.f;
		const float kSlideWidth = 1.f;
		const float kSlideTime = 0.3f;
		const float kSlideDamage = 18.f;

		// The dark's piece: leaps two-and-one like a knight, landing hard, two
		// leaps in a row.
		const float kLeapWindup = 0.6f;
		const float kChainWindup = 0.4f;
		const float kLeapScale = 1.6f;   // tiles per square of its move
		const float kLeapTime = 0.45f;
		const float kLandRadius = 1.6f;
		const float kLeapDamage = 20.f;
		const int kLeapChain = 2;

		// Both, every third move: every other square around it struck at once,
		// like the dark squares of a board. Stand on a light one.
		const float kBoardWindup = 1.2f;
		const int kBoardRadius = 4;
		const float kBoardDamage = 20.f;

		// What it leaves: the same of each power, so his balance is untouched.
		const int kRewardMotes = 15;
	}

	// The kings' duels. Neither fights unless he asks; each fights with one
	// power only and holds him to the same, so he must beat its master at it.
	namespace Duel
	{
		const float kReach = 2.2f;         // how near the king he must stand to ask
		const float kLeashRange = 18.f;    // walk this far off and the duel is over
		const float kSpareHealth = 0.3f;   // beaten, he is left this share of his health
		const float kSpareInvuln = 2.f;
		const float kSendOffDelay = 3.f;   // after a king yields, until he is sent to Bellum
		const float kRemarkTime = 3.5f;    // how long a king's words stay on screen

		// The blessings: worth the fight, never enough to make it necessary.
		const float kLightBlessing = 0.15f;     // Lucius: light attacks this much harder
		const float kLifestealBlessing = 0.03f; // Noctis: added to the lifesteal share
	}

	// The light's king. He keeps his distance and fights as the light does -
	// spears thrown from afar, judgment along the ground - and between the
	// blows he shields and heals himself, which is the twist in his goodness.
	namespace Lucius
	{
		const float kMaxHealth = 1100.f;
		const float kBodyRadius = 0.6f;
		const float kFuryTime = 150.f;

		const float kSpeed = 2.2f;
		const float kKeepNear = 3.5f;    // closer than this he steps back
		const float kKeepFar = 7.f;      // further than this he comes on
		const float kStrafeTime = 2.5f;  // between changing which way he circles
		const float kCooldown = 1.3f;
		const float kRecover = 0.5f;
		const float kPhaseShare = 0.5f;  // below this share his judgment falls on both diagonals too

		// Spears: fans of light thrown at him, wave after wave.
		const float kVolleyWindup = 0.6f;
		const int kVolleyWaves = 3;
		const float kVolleyGap = 0.3f;
		const int kVolleySpears = 5;
		const float kVolleySpread = 56.f; // degrees across the fan
		const float kSpearSpeed = 7.f;
		const float kSpearLifetime = 1.8f;
		const float kSpearDamage = 9.f;

		// Judgment: pillars of light along the ground through where he stood.
		const float kJudgmentWindup = 1.1f;
		const float kJudgmentReach = 9.f;  // tiles out to either side
		const float kJudgmentWidth = 1.2f;
		const float kJudgmentDamage = 22.f;
		const float kJudgmentShake = 5.f;

		// Radiance: when he presses close, a ring of light, then a step away.
		const float kRadianceWindup = 0.55f;
		const float kRadianceRadius = 2.6f;
		const float kRadianceDamage = 14.f;
		const float kRadianceStep = 3.f;

		// Sanctuary: a dome that turns every blow aside and closes his wounds,
		// until a beam goes straight through it and leaves him reeling.
		const float kSanctuaryShare = 0.75f; // never before he is down to this
		const float kSanctuaryCooldown = 16.f;
		const float kSanctuaryTime = 6.f;
		const float kSanctuaryHeal = 18.f;   // health per second
		const float kStaggerTime = 2.2f;
		const float kStaggerDamageScale = 1.5f;
		const float kBreakShake = 8.f;

	}

	// The dark's king. A schemer: he does not come to him but makes him come,
	// and punishes him for it - the closer he stays to feed on him, the readier
	// the counter, and the ground round him is sown with snares. Keep away
	// instead and his shades and shot come for him.
	namespace Noctis
	{
		const float kMaxHealth = 1000.f;
		const float kBodyRadius = 0.55f;
		const float kFuryTime = 120.f;

		const float kSpeed = 2.4f;
		const float kHoldRange = 3.2f;   // the distance he likes to keep
		const float kStrafeTime = 2.f;
		const float kCooldown = 0.9f;
		const float kRecover = 0.45f;
		const float kCloseRange = 2.5f;  // this near is close
		const float kFarRange = 5.5f;    // this far is keeping away

		// How long he has been kept close, slowly forgotten once he backs off.
		// Past the mark, the next blow at close range is a counter.
		const float kPressureDecay = 0.5f;   // per second spent away
		const float kPressureForCounter = 1.6f;

		// Below this share of health he thinks faster: shorter windups and
		// snares every third move instead of every fourth.
		const float kPhaseShare = 0.5f;
		const float kPhaseWindupScale = 0.8f;

		// Cut: a quick half circle when he is close.
		const float kCutWindup = 0.4f;
		const float kCutRange = 2.f;
		const float kCutDamage = 14.f;

		// Lunge: across the ground at him along a marked line.
		const float kLungeWindup = 0.55f;
		const float kLungeLength = 5.f;
		const float kLungeWidth = 1.f;
		const float kLungeTime = 0.22f;
		const float kLungeDamage = 18.f;

		// Counter: a stance. Strike him in it and the blow is turned aside -
		// nothing landed, nothing drunk - and answered far harder.
		const float kCounterTime = 1.3f;
		const float kCounterDelay = 0.15f;
		const float kCounterRange = 2.8f;
		const float kCounterDamage = 30.f;
		const float kCounterShake = 7.f;

		// Snares: hidden in a ring round him, to punish coming near.
		const float kSnareWindup = 0.6f;
		const int kSnares = 4;
		const int kMaxSnares = 8;
		const float kSnareRing = 2.4f;     // tiles from him
		const float kSnareArming = 0.8f;   // harmless, and plain to see, this long
		const float kSnareLifetime = 16.f;
		const float kSnareTrigger = 0.55f; // stepping this near springs it
		const float kSnareDelay = 0.3f;
		const float kSnareRadius = 1.1f;
		const float kSnareDamage = 16.f;

		// Shades: his shadows, sent at him when he keeps away. Struck, they
		// come apart and give nothing back.
		const float kShadeWindup = 0.5f;
		const int kShades = 2;
		const float kShadeSpread = 2.8f;   // tiles to either side of him they rise
		const float kShadeSpeed = 3.2f;
		const float kShadeReach = 1.4f;
		const float kShadeStrikeWindup = 0.45f;
		const float kShadeDamage = 12.f;
		const float kShadeLifetime = 5.f;
		const float kShadeCooldown = 9.f;

		// Orbs: dark shot that bends after him, when he keeps away.
		const float kOrbWindup = 0.6f;
		const int kOrbs = 3;
		const float kOrbSpread = 50.f;
		const float kOrbSpeed = 4.5f;
		const float kOrbTurn = 1.6f;
		const float kOrbLifetime = 3.f;
		const float kOrbDamage = 10.f;
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
