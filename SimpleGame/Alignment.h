#pragma once

#include "Palette.h"

// Which of the two primordial powers something belongs to. A gray monster keeps
// this hidden while it lives; only its death shows it. The gray itself is a
// third, held only by him at the very end - nothing is ever born to it.
enum class Alignment
{
	Light,
	Dark,
	Ashen,
};

namespace AlignmentColor
{
	// The solid core of a bolt or a mote.
	inline void Core(Alignment alignment, float& r, float& g, float& b)
	{
		const Palette::Color& c = (alignment == Alignment::Light) ? Palette::kLightCore
			: ((alignment == Alignment::Dark) ? Palette::kDarkCore : Palette::kAshCore);
		r = c.r;
		g = c.g;
		b = c.b;
	}

	// The halo around it. Light brightens the ground under it; dark, being a
	// light that gives off darkness, is near-black and darkens instead.
	inline void Glow(Alignment alignment, float& r, float& g, float& b)
	{
		const Palette::Color& c = (alignment == Alignment::Light) ? Palette::kLightGlow
			: ((alignment == Alignment::Dark) ? Palette::kDarkGlow : Palette::kAshGlow);
		r = c.r;
		g = c.g;
		b = c.b;
	}

	// A side's colour washed toward the warning ash - how a threat from it is
	// marked. amount 0 keeps the colour, 1 is pure ash.
	inline void Warning(Alignment alignment, float amount, float& r, float& g, float& b)
	{
		Core(alignment, r, g, b);
		r += (Palette::kAshWarning.r - r) * amount;
		g += (Palette::kAshWarning.g - g) * amount;
		b += (Palette::kAshWarning.b - b) * amount;
	}
}
