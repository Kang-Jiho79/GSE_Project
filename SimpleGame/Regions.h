#pragma once

#include "Palette.h"

// Every place the player can be, and how they join up. Each region is its own
// map, generated from its seed when he enters it; nothing about a region is
// kept once he leaves, so its monsters are back the next time he comes.
namespace Regions
{
	enum Id
	{
		Limina,       // the hidden village of the borderborn, where he grew up
		LiminaWoods,
		Crepus,       // the border between the two nations
		Bellum,       // the old battlefield between them, where every road meets

		Alba,
		Sola,
		Aura,
		LightGate,
		Lumen,        // the light's capital

		Umbra,
		Nox,
		Crypta,
		DarkGate,
		Tenebrae,     // the dark's capital

		AshMarch,     // the edge of the gray's heartland
		Fides,
		Spes,
		Requiem,
		Luctus,
		Divisio,      // the front where light and dark still clash
		Kinera,       // the heart of the gray
		Realm,        // the god's own place, reached only at the very end

		Count,
	};

	enum class Kind
	{
		Village,
		Field,
		Gate,
		Capital,
	};

	// Whose land it is - how it looks, and which side its eroded mostly were.
	enum class Style
	{
		Borderborn,
		Light,
		Dark,
		Ashen,
		Mixed,
	};

	struct Info
	{
		const char* name;
		Kind kind;
		Style style;

		bool temple;        // a village or capital with a temple to rest at

		int size;           // tiles along each side
		int terraceLevels;  // 1 is flat ground
		float propDensity;  // trees and ruins, 1 as dense as an ordinary field

		int monsterCount;
		float lightShare;   // of its monsters, how many were of the light
		float mageShare;    // of its monsters, how many are mages

		// Where it sits on the world map, -1..1 across and up.
		float mapX;
		float mapY;
	};

	// How a region feels to stand in: the dark the world fades into at its
	// edges, how far the light around him reaches, and the grade laid over
	// the whole frame. The gray's land is drained and dull; the capitals,
	// untouched by it, are vivid.
	struct Atmosphere
	{
		Palette::Color fog;
		float lightRadius; // pixels of full light around him
		float fogStart;    // pixels out, where the fog begins
		float fogEnd;      // and where it has swallowed everything

		float saturation;
		float contrast;
		Palette::Color tint;
	};

	const Info& Get(int region);
	// Not on the world map and with no waystone: reached and left only by
	// the story.
	bool IsOffMap(int region);
	const Atmosphere& AtmosphereOf(int region);
	unsigned int Seed(int region);

	// Roads run both ways.
	bool AreLinked(int a, int b);
	int LinkCount();
	void GetLink(int index, int& a, int& b);
}
