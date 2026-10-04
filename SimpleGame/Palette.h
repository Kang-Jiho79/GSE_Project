#pragma once

// Every colour the look is built on, in one place. Three families carry the
// story - light (gold and white), dark (violet and ink), and the gray (gray
// and silver) that has eaten everything else - and the HUD is drawn from
// them too. Colours that belong to one model alone stay next to it.
namespace Palette
{
	struct Color
	{
		float r;
		float g;
		float b;
	};

	inline Color Mix(Color from, Color to, float amount)
	{
		Color mixed =
		{
			from.r + (to.r - from.r) * amount,
			from.g + (to.g - from.g) * amount,
			from.b + (to.b - from.b) * amount,
		};
		return mixed;
	}

	// ---- Light ----
	const Color kLightCore = { 0.95f, 0.78f, 0.30f };
	const Color kLightGlow = { 0.95f, 0.80f, 0.42f };

	// ---- Dark ----
	const Color kDarkCore = { 0.42f, 0.30f, 0.86f };
	// A light that gives off darkness: near-black, so its glow sinks what it
	// falls on instead of brightening it.
	const Color kDarkGlow = { 0.03f, 0.02f, 0.05f };

	// ---- The gray ----
	// The near-black behind screens laid over the world. Each region has its
	// own fog for the world itself (Regions::AtmosphereOf); this is close to
	// the battlefield's.
	const Color kVoid = { 0.045f, 0.043f, 0.055f };
	// Under anything standing on the ground.
	const Color kShadow = { 0.085f, 0.08f, 0.10f };
	// The gray creeping into the HUD as his balance goes.
	const Color kAsh = { 0.62f, 0.62f, 0.64f };
	// The gray laid over the world as his balance goes.
	const Color kAshVeil = { 0.42f, 0.42f, 0.44f };
	// What a threat is marked in: the side's own colour, washed most of the
	// way to this - ground about to be struck, a spell thrown at him.
	const Color kAshWarning = { 0.75f, 0.73f, 0.72f };
	const float kWarningWash = 0.6f;
	const float kHostileSpellWash = 0.35f;
	// The dark rim round a hostile light bolt, so it never reads as his own.
	const Color kHostileRim = { 0.18f, 0.17f, 0.19f };

	// The eroded's armour: plain grays, the light side's a shade paler.
	const float kErodedLight = 0.31f;
	const float kErodedDark = 0.23f;
	const float kErodedRobe = 0.34f;
	const float kErodedSteel = 0.52f;
	// How much of their old colour is left in eyes and emblems.
	const float kRemnantWash = 0.45f;
	const float kRemnantDim = 0.75f;

	// A boss past its time, burning with fury.
	const Color kFury = { 0.85f, 0.22f, 0.12f };

	// The kings, untouched by the gray: the light's in white steel and a
	// pale mantle, the dark's in black plate and a mantle the colour of night.
	const Color kLuciusArmor = { 0.78f, 0.76f, 0.70f };
	const Color kLuciusMantle = { 0.88f, 0.86f, 0.80f };
	const Color kNoctisArmor = { 0.13f, 0.12f, 0.16f };
	const Color kNoctisMantle = { 0.20f, 0.13f, 0.30f };
	// His shades: shadow with nothing of him in it but the eyes.
	const Color kShade = { 0.05f, 0.045f, 0.07f };

	// The people of each land, in plain clothes: pale linen in the light's
	// villages, dyed near-black in the dark's, patched brown among the
	// borderborn. Priests wear their side's colour full, with its mark.
	const Color kClothLight = { 0.62f, 0.58f, 0.50f };
	const Color kClothDark = { 0.17f, 0.14f, 0.22f };
	const Color kClothBorderborn = { 0.33f, 0.26f, 0.19f };
	const Color kClothAshen = { 0.36f, 0.36f, 0.37f };
	const Color kRobeLight = { 0.86f, 0.84f, 0.78f };
	const Color kRobeDark = { 0.10f, 0.08f, 0.13f };
	const Color kSkin = { 0.62f, 0.52f, 0.44f };

	// ---- Figures ----
	// The ink line round anyone standing in the world, and the cold light
	// catching the edge of them that faces the sky.
	const Color kOutline = { 0.02f, 0.02f, 0.03f };
	const Color kRim = { 0.70f, 0.72f, 0.80f };

	// ---- HUD ----
	const Color kUiFrame = { 0.32f, 0.31f, 0.34f };
	const Color kUiWell = { 0.10f, 0.10f, 0.12f };
	const Color kUiNotch = { 0.55f, 0.54f, 0.56f };
	const Color kUiInk = { 0.06f, 0.06f, 0.07f };
	const Color kUiBone = { 0.92f, 0.90f, 0.86f };
	const Color kHealth = { 0.62f, 0.16f, 0.18f };

	// The vessel's halves, brighter once full.
	const Color kVesselDark = { 0.42f, 0.28f, 0.68f };
	const Color kVesselDarkFull = { 0.58f, 0.40f, 0.88f };
	const Color kVesselLight = { 0.82f, 0.62f, 0.24f };
	const Color kVesselLightFull = { 0.98f, 0.80f, 0.38f };

	const Color kPipEarned = { 0.72f, 0.70f, 0.64f };
	const Color kPipEmpty = { 0.20f, 0.20f, 0.22f };

	const Color kMapWell = { 0.08f, 0.075f, 0.095f };
	const Color kMapEnemy = { 0.74f, 0.22f, 0.20f };
	const Color kMapPlayerRim = { 0.30f, 0.30f, 0.34f };
	const Color kMapPlayer = { 0.94f, 0.92f, 0.88f };
}
