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

		// From the first upgrade, each form dodges its own way. Light: a flash
		// straight to a spot a little way off, untouchable for a moment.
		const float kBlinkDistance = 3.2f;
		const float kBlinkInvuln = 0.25f;
		const int kBlinkSteps = 8;         // pieces it is checked in, so walls stop it
		// Dark: a dash through as a shadow.
		const float kShadowDashDistance = 3.5f;
		const float kShadowDashTime = 0.16f;

		// Once the dodge grows stronger: the light's leaves a burst where he
		// was, the dark's cuts everything it passes through.
		const float kBlinkBurstRadius = 1.6f;
		const float kBlinkBurstDamage = 18.f;
		const float kShadowDashWidth = 1.f;
		const float kShadowDashDamage = 16.f;
	}

	// What each upgrade of the vessel brings: the upgrade it arrives at.
	namespace Growth
	{
		const int kFormDodge = 1;   // each form's own dodge
		const int kSkills = 2;      // E and Q, both forms
		const int kAttackI = 3;     // light: two bolts; dark: blades round the last cut
		const int kSwap = 4;        // the swap skills
		const int kSkillsII = 5;    // E and Q stronger
		const int kDodgeII = 6;     // the dodges strike
		const int kAttackII = 7;    // light: three bolts; dark: a fourth cut
		const int kSwapII = 8;      // the swap skills wider

		// What the stronger steps do.
		const float kBoltSpread = 8.f;       // degrees between bolts
		const float kBeamWidthScale = 1.6f;
		const float kSecondStrikeDelay = 0.3f;
		const float kDashScale = 1.4f;
		const float kEruptRadiusII = 5.5f;
		const int kEruptWavesII = 2;
		const float kSwapScale = 1.4f;
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

		// The world: a place's monsters and boss are set out once he comes
		// within this many tiles of it, and cleared away once he is further
		// than the second - to be set out afresh when he comes back.
		const float kWakeReach = 16.f;
		const float kForgetReach = 40.f;
		// Within this many tiles of the next place, its air begins to mix in.
		const float kBlendReach = 12.f;
		// No monsters this near a walled board: quiet before the threshold.
		const float kArenaQuiet = 16.f;
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

	// The god (「   」): a white giant leaning over the far corner of its
	// board, its two hands down on it. Three wards in turn - its light, its
	// dark, then the two made white - each over a third of its life. While
	// a ward stands it cannot be touched; its cores, on the board, come
	// undone only to the gray's unravelling (E). With every core gone it
	// bows its face to the board for a while, and can be cut.
	namespace God
	{
		const float kMaxHealth = 4500.f;
		const float kBodyRadius = 2.f;    // from where it stands to the reach of its bowed face
		const float kWakeRange = 30.f;
		const float kLeashRange = 99.f;   // nowhere to run to in its place
		const float kFuryTime = 360.f;

		// The board: its squares run from kBoardFrom to kBoardTo tiles from
		// the centre of the place on both axes, walled round. The god stands
		// beyond its far corner, and bows its face down just inside it.
		const int kBoardFrom = -6;
		const int kBoardTo = 8;
		const float kStandOffset = -7.2f;
		const float kFaceOffset = -5.6f;
		// Drawn back a little in its place, so the giant fits on screen.
		const float kRealmZoom = 0.8f;

		// The wards' cores, and where they stand (tiles from the centre):
		// two for the first ward, three for the second, four for the last.
		const float kCoreRadius = 0.6f;
		const int kCoresPerWard[3] = { 2, 3, 4 };
		const float kCoreSpots[9][2] =
		{
			{ -3.f, 3.f }, { 3.f, -3.f },
			{ -4.f, 1.f }, { 1.f, -4.f }, { 4.f, 4.f },
			{ -3.f, 5.f }, { 5.f, -3.f }, { 6.f, 2.f }, { 2.f, 6.f },
		};
		// How long it stays bowed with its ward broken, before raising it again.
		const float kGapTime = 8.f;
		const float kGapShake = 8.f;
		const float kBowRate = 2.f;       // of the way down or up a second

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

		// A fan of its ward's power thrown from its face at him.
		const float kVolleyWindup = 0.9f;
		const int kVolleyCount = 5;
		const float kVolleySpread = 50.f; // degrees, edge to edge
		const float kVolleySpeed = 5.5f;
		const float kVolleyLifetime = 4.f;
		const float kVolleyDamage = 12.f;

		// From the second ward: every other square round him strikes.
		const float kBoardWindup = 1.3f;
		const int kBoardRadius = 5;
		const float kBoardDamage = 20.f;

		// From the second ward: curses laid round where he stands.
		const float kMireWindup = 0.6f;
		const int kMireCount = 3;
		const float kMireSpread = 2.5f;   // tiles from him

		// Under the last: a ring of shot, light and dark and white, all round.
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

	// The requests on the villages' notice boards.
	namespace Quest
	{
		const float kBoardReach = 1.4f; // tiles from a notice board to read it

		// How many of the gray each hunt asks for, and what each request
		// pays in light and dark when it is reported.
		const int kWoodsKnights = 5;
		const float kWoodsReward = 10.f;
		const int kSoldiers = 6;
		const float kSoldiersReward = 25.f;
		const float kMerchantReward = 15.f;
		const int kGateMages = 4;
		const float kGateMagesReward = 30.f;
		const int kGraveKnights = 4;
		const float kGraveKnightsReward = 30.f;
		const int kMarchHorde = 8;
		const float kMarchHordeReward = 25.f;

		// The gray round the stranded merchant: how many, how far out from
		// him, and how near one must still be for him to be besieged.
		const int kBesiegers = 5;
		const float kBesiegeRing = 4.f;
		const float kBesiegeReach = 8.f;
	}

	// The special (R): a gauge of its own, filled by blows landed and taken,
	// apart from the vessel and the balance. Full, R plays a cut-in - his eyes
	// across the screen, the world stopped - and the form's special goes off.
	namespace Limit
	{
		const float kMax = 100.f;
		const float kPerDealt = 0.12f;  // per point of damage he lands
		const float kPerTaken = 1.f;    // per point of damage he takes
		// One special takes no more than this share of a boss's whole life.
		const float kBossCap = 0.2f;

		// Light: pillars of light falling round the cursor one after another,
		// then one great one in the middle. Scaled like his other light blows.
		const float kPillarReach = 9.f;     // tiles to the cursor, at most
		const int kPillarCount = 8;
		const float kPillarSpread = 5.f;    // tiles round the centre
		const float kPillarRadius = 1.4f;
		const float kPillarDamage = 35.f;
		const float kPillarFirst = 0.25f;   // seconds until the first lands
		const float kPillarGap = 0.1f;
		const float kFinalRadius = 2.5f;
		const float kFinalDamage = 110.f;
		const float kFinalDelay = 1.2f;

		// Dark: onto the nearest enemy ahead, held, cut again and again, and
		// blown apart. Scaled like his other dark blows; drinks twice as deep.
		const float kGrabReach = 6.f;
		const float kGrabDegrees = 120.f;
		const float kGrabDashTime = 0.12f;
		const float kGrabStop = 0.9f;       // tiles short of it he stops
		const int kGrabHits = 6;
		const float kGrabGap = 0.15f;
		const float kGrabRadius = 1.3f;
		const float kGrabDamage = 18.f;
		const float kGrabBlastRadius = 2.5f;
		const float kGrabBlastDamage = 100.f;
		const float kGrabLifesteal = 2.f;

		// Ash: in a stopped, colourless world, through up to six enemies near
		// him one after another; as the scythe is drawn back, every cut bursts
		// at once. Fixed, like all the gray's.
		const float kReapReach = 7.f;
		const int kReapMost = 6;
		const float kReapRadius = 1.2f;
		const float kReapDamage = 100.f;
		const float kReapDelay = 0.05f;     // world seconds after the world runs again
	}

	namespace FormSwitch
	{
		// Space is ignored until this runs out; a successful switch restarts it.
		const float kSwitchCooldown = 1.f;
	}

	// The gray, taken up at the very end - and then the only power he has.
	// A scythe: cut, dash, cut again. Every number is fixed, owing nothing to
	// the light and dark he once absorbed.
	namespace AshForm
	{
		// The combo: a rising cut, a wide cut back, a full turn, and a long
		// cut brought down whose trail bursts again a moment later.
		const int kComboLength = 4;
		const float kCutDamage[kComboLength] = { 22.f, 22.f, 26.f, 40.f };
		const float kCutRange[kComboLength] = { 2.4f, 2.6f, 2.2f, 3.6f };
		const float kCutArcDegrees[kComboLength] = { 140.f, 200.f, 360.f, 120.f };
		// Each cut carries him into it, the last furthest.
		const float kCutStep[kComboLength] = { 0.35f, 0.2f, 0.15f, 0.6f };
		const float kCutStepTime = 0.08f;
		const float kCutInterval = 0.26f;    // between cuts while the combo runs
		const float kLastCutRecovery = 0.5f; // after the last
		const float kComboWindow = 0.7f;     // a longer pause starts it over
		// The last cut's trail, bursting again.
		const float kEchoDelay = 0.35f;
		const float kEchoDamage = 30.f;

		// Right click, the dash cut: to the far side of the nearest enemy
		// ahead, cutting it on the way, while an image of him left where he
		// stood bursts. With no one ahead, a plain dash.
		const float kDashSearch = 5.f;          // tiles ahead it looks
		const float kDashSearchDegrees = 90.f;  // how wide
		const float kDashBehind = 0.8f;         // tiles past the enemy's body
		const float kDashDistance = 4.f;        // with no one to cut
		const float kDashCutRadius = 1.3f;
		const float kDashDamage = 35.f;
		const float kDashBlastDelay = 0.3f;
		const float kDashBlastRadius = 1.6f;
		const float kDashBlastDamage = 30.f;
		const float kDashInvuln = 0.3f;
		const float kDashCooldown = 1.2f;

		// Space, the parry: a moment's stance. A blow that lands in it does
		// not, and he is at once behind the nearest enemy, cutting. With no
		// one in reach, he lashes out all round instead.
		const float kParryWindow = 0.5f;
		const float kParryCooldown = 6.f;
		const float kParryReach = 6.f;        // tiles to the one he answers
		const float kCounterRadius = 1.6f;
		const float kCounterDamage = 80.f;
		const float kCounterRingRadius = 2.5f;
		const float kCounterRingDamage = 40.f;
		const float kCounterInvuln = 0.6f;

		// E, unravel: a line through everything, shields and wards included.
		const float kUnravelLength = 9.f;
		const float kUnravelWidth = 1.8f;
		const float kUnravelDamage = 70.f;
		const float kUnravelCooldown = 4.f;

		// Q, erase: every shot round him wiped away, a heavy blow to all
		// within reach, and a moment untouched.
		const float kEraseRadius = 4.5f;
		const float kEraseDamage = 90.f;
		const float kEraseInvuln = 0.8f;
		const float kEraseCooldown = 10.f;
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
		// Three cuts at first, four once the basic attack has grown: a quick
		// slash, a sweep, a second quick slash, and the last brought down.
		// Before the fourth is learned the third quick slash is skipped.
		const int kComboLength = 4;
		const float kSlashDamage[kComboLength] = { 10.f, 10.f, 12.f, 20.f };

		// Share of melee damage dealt that comes back as health. Only blows that
		// land in reach count - never anything thrown or lingering.
		const float kLifesteal = 0.09f;

		// Each cut its own: a quick narrow slash, a wide sweep back the other
		// way, and the last brought down on a spot in front of him. The first
		// reaches past the knights' start ranges, so it can land before one of
		// them winds up a blow of its own.
		const float kSlashRange[kComboLength] = { 1.5f, 1.9f, 1.6f, 0.f };
		const float kSlashArcDegrees[kComboLength] = { 120.f, 210.f, 150.f, 0.f };
		const float kSmashReach = 1.3f;   // tiles ahead the last cut comes down
		const float kSmashRadiusPlain = 1.1f; // before the basic attack grows
		const float kSmashRadius = 1.4f;
		const int kSmashBlades = 4;       // blades of darkness thrown up round it
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

		// Q: the sword driven into the ground. A circle is marked out round
		// him, and a moment later a blade of darkness bursts up under every
		// enemy inside it (up to kEruptMax), with a few more where there is
		// no one. He cannot move while the sword is down.
		const float kPlantTime = 0.45f;
		const float kEruptRadius = 4.5f;
		const float kEruptDelay = 0.35f;
		const float kEruptWaveGap = 0.4f;   // between waves, once there are two
		const float kEruptDamage = 45.f;
		const int kEruptMax = 10;
		const int kEruptExtras = 7;
		const float kEruptCooldown = 10.f;

		// The dash leaves a shadow of him behind this often.
		const float kAfterimageGap = 0.035f;

		// Each cut carries him into it: a half step, less while he turns into
		// the sweep, and further with the last.
		const float kSlashStep[kComboLength] = { 0.3f, 0.15f, 0.3f, 0.55f };
		const float kSlashStepTime = 0.08f;
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

	// The tainted pieces (잠식된 신의 말): pieces from the god's board the
	// gray has eaten into, waiting at the far end of the field regions. Each
	// keeps its own piece's way of moving as the grammar of its fight.
	namespace TaintedPiece
	{
		const float kWakeRange = 7.f;
		const float kLeashRange = 16.f;
		const float kBodyRadius = 0.9f;
		// Its board: this many squares each way from the arena's centre.
		const int kBoardHalf = 4;
		// Beaten, it topples onto the board and rolls before it is gone.
		const float kToppleTime = 1.1f;
		// What it leaves: this many motes of what it was - split evenly if it
		// was of both.
		const int kRewardMotes = 20;
	}

	// The prologue's: a black pawn, with two of its soldiers beside it. They
	// step forward a square at a time toward the far rank and take only on
	// the forward diagonals; stand square in front of it and it cannot step.
	// At the far rank - or once half its life is gone - it is promoted.
	namespace TaintedPawn
	{
		const float kMaxHealth = 320.f;
		const float kFuryTime = 240.f;

		const float kTurnTime = 2.f;       // between its moves
		const float kStepTime = 0.25f;
		const float kStrikeReach = 3.f;    // he is this near: it strikes rather than steps
		const float kStrikeWindup = 0.8f;
		const float kStrikeRadius = 1.f;   // round each forward-diagonal square
		const float kStrikeDamage = 20.f;
		const float kBlockReach = 0.8f;    // he stands this near the square ahead: it cannot step

		const float kPromoteShare = 0.5f;
		const float kPromoteHeal = 0.2f;   // of its whole life, if it reached the far rank
		const float kPromotedScale = 1.3f;
		const float kPromotedWindupScale = 0.8f;
		// Promoted: it slides along the eight lines like a queen.
		const float kSlideWindup = 0.7f;
		const float kSlideMax = 7.f;
		const float kSlideWidth = 1.2f;
		const float kSlideTime = 0.3f;
		const float kSlideDamage = 24.f;
		const float kCooldown = 1.f;

		// Its soldiers: weak, stepping with it; one that reaches the far rank
		// gives itself to the pawn, mending it.
		const int kSoldiers = 2;
		const float kSoldierSpread = 2.f;  // squares to either side
		const float kSoldierHealth = 60.f;
		const float kSoldierRadius = 0.5f;
		const float kSoldierTurn = 2.4f;
		const float kSoldierStrikeReach = 2.2f;
		const float kSoldierWindup = 0.8f;
		const float kSoldierDamage = 12.f;
		const float kSoldierHeal = 0.1f;   // of the pawn's whole life
	}

	// The first chapter's: a white bishop. It moves only on diagonals and
	// strikes along them - light beams from a pale square, slow dark orbs from
	// a dark one - and now and then breaks the rules with a straight step to
	// change its colour. Past half its life, all but every third diagonal of
	// one direction strikes at once: the safe ground is a lattice.
	namespace TaintedBishop
	{
		const float kMaxHealth = 520.f;
		const float kFuryTime = 240.f;
		const float kCooldown = 1.1f;
		const float kRecover = 0.6f;

		const float kBlinkWindup = 0.5f;
		const int kBlinkReach = 4;         // squares along a diagonal
		const float kBeamWindup = 0.9f;
		const float kBeamLength = 9.f;
		const float kBeamWidth = 0.8f;
		const float kBeamDamage = 20.f;
		const float kOrbWindup = 0.7f;
		const float kOrbSpeed = 3.f;
		const float kOrbLifetime = 4.f;
		const float kOrbDamage = 16.f;
		const int kShiftEvery = 4;         // every fourth move, a straight step onto the other colour
		const float kShiftWindup = 0.4f;

		const float kLatticeShare = 0.5f;
		const int kLatticeEvery = 3;
		const int kLatticeSpacing = 3;     // one diagonal in this many is safe
		const float kLatticeWindup = 1.3f;
		const float kLatticeDamage = 24.f;
	}

	// The fourth chapter's: a rook of both colours at the front. It charges
	// straight along the board's lines and raises walls to pen him in. Four
	// pillars stand on its board; charging into one breaks it and leaves the
	// rook stunned, open to heavier blows. Its own walls only crumble.
	namespace TaintedRook
	{
		const float kMaxHealth = 950.f;
		const float kFuryTime = 300.f;
		const float kCooldown = 1.2f;
		const float kRecover = 0.7f;

		const float kChargeWindup = 0.9f;
		const float kChargeMax = 10.f;
		const float kChargeSpeed = 9.f;    // tiles per second
		const float kChargeWidth = 1.2f;
		const float kChargeDamage = 26.f;
		const float kChargeShake = 7.f;

		const float kStunTime = 3.f;
		const float kStunScale = 1.5f;     // what it takes while stunned

		const float kPillarOffset = 2.f;   // from the centre along both axes, four of them
		const float kPillarRadius = 0.45f; // tiles, what a charge strikes and what blocks him

		const int kWallEvery = 3;
		const float kWallWindup = 0.8f;
		const int kWallLength = 5;
		const float kWallBehind = 1.5f;    // tiles past him, away from the rook
		const float kWallTime = 6.f;
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

	// Cutscenes: the camera they move, the bars, and how one is skipped.
	namespace Cutscene
	{
		// How far the camera may close in or draw back.
		const float kMinZoom = 0.5f;
		const float kMaxZoom = 2.5f;
		// Each black bar's share of the window's height, and how long the
		// bars take to slide in or out.
		const float kLetterboxShare = 0.1f;
		const float kLetterboxEase = 0.5f;
		// How fast the world runs under a slowed shot.
		const float kSlowScale = 0.25f;
		// Seconds a caption takes to fade in.
		const float kCaptionFade = 0.35f;
		// Seconds F, Space or Enter must be held to skip.
		const float kSkipHold = 1.f;
		// Kept from harm while one plays, a little at a time.
		const float kShield = 0.1f;
		// The pale mark over a king asked about the collusion: pixels above
		// its feet, and its height.
		const float kMarkLift = 120.f;
		const float kMarkSize = 26.f;
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

	// The vessel: how much of each power it holds at first, and how much more
	// - and how much more health - each upgrade brings.
	namespace Vessel
	{
		const float kStartCapacity = 12.f;
		const float kCapacityPerUpgrade = 4.f;
		const float kHealthPerUpgrade = 10.f;
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
