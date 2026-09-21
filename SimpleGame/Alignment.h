#pragma once

// Which of the two primordial powers something belongs to. A gray monster keeps
// this hidden while it lives; only its death shows it.
enum class Alignment
{
	Light,
	Dark,
};

namespace AlignmentColor
{
	// The solid core of a bolt or a mote.
	inline void Core(Alignment alignment, float& r, float& g, float& b)
	{
		if (alignment == Alignment::Light)
		{
			r = 0.95f;
			g = 0.78f;
			b = 0.30f;
		}
		else
		{
			r = 0.42f;
			g = 0.30f;
			b = 0.86f;
		}
	}

	// The halo around it. Light brightens the ground under it; dark, being a
	// light that gives off darkness, is near-black and darkens instead.
	inline void Glow(Alignment alignment, float& r, float& g, float& b)
	{
		if (alignment == Alignment::Light)
		{
			r = 0.95f;
			g = 0.80f;
			b = 0.42f;
		}
		else
		{
			r = 0.03f;
			g = 0.02f;
			b = 0.05f;
		}
	}
}
