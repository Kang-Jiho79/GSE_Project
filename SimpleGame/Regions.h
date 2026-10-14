#pragma once

#include <vector>
#include "Palette.h"

// Every place the player can be, and how they join up. All but the god's own
// place lie together in one world he walks across: squares of ground the
// same size, set edge to edge on a grid five across and seven deep - the
// light's land down the west side, the dark's down the east, the gray's
// heartland at the top between them, the battlefield below it, and the
// border and the hidden village at the bottom - with small places, some
// without names, in the squares between. He walks from any square into the
// next, but for the gray's heartland, walled round with cliffs and fog
// except where it meets its own march. The god's place has a map of its own.
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

		// The squares between: the villages that grew up along the roads,
		// and places with no name of their own.
		Vespera,      // the border's resting place
		Treuga,       // the outpost on the battlefield, where every road meets
		Candida,      // pilgrims' village on the light's side
		Obscura,      // pilgrims' village on the dark's side
		Refugium,     // the last refuge before the gray
		AshRift,
		FallenKing,
		Wreckage,
		LightHamlet,
		WayfarerCamp,
		DarkHamlet,
		RuinedAltar,
		Hideout,
		PilgrimRest,

		// Inside the two castles: the throne rooms, each a map of its own,
		// entered by the castle's door in its capital.
		LumenCastle,
		TenebraeCastle,

		Count,
	};

	// What stands in a square with no name of its own, past its ground.
	enum class Feature
	{
		None,
		Hamlet,      // a few houses and a small altar
		Camp,        // a fire and tents
		FallenKing,  // a great king of stone lying on its side
		Wreckage,    // the broken weapons and banners of both sides
		Rift,        // pools of the gray lying on the ground
		RuinedAltar, // a temple fallen in, its altar still working
		Hideout,     // where the borderborn used to hide
		PilgrimRest, // a resting place where both sides sit apart
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

		// Which square of the world it is: across from the west, and down
		// from the north.
		int cellCol;
		int cellRow;

		Feature feature;
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
	// Not on the world map and with no waystone: a map of its own, reached
	// and left only by the story or a door.
	bool IsOffMap(int region);
	// Inside a castle: which one, and the capital it stands in.
	bool IsCastle(int region);
	// Where a place is on the world map: itself, or for a castle its capital.
	int MapRegion(int region);
	const Atmosphere& AtmosphereOf(int region);
	unsigned int Seed(int region);

	// Whether he can walk from one place into another: they share an edge,
	// and it is not walled off.
	bool AreLinked(int a, int b);
	int LinkCount();
	void GetLink(int index, int& a, int& b);

	// The world, in tiles along each side, and the seed its ground grows from.
	int WorldSize();
	unsigned int WorldSeed();
	// Where a place lies: its centre and its square, in tiles. A place off
	// the map lies at the middle of a map of its own.
	void Center(int region, float& col, float& row);
	void Bounds(int region, float& minCol, float& minRow, float& maxCol, float& maxRow);
	// How far a point lies outside a place's square, in tiles; 0 inside it.
	float DistanceOutside(int region, float col, float row);
	// Which place a tile of the world belongs to, or -1 for the cliffs and
	// fog round the gray's heartland and past the world's edge, where nobody
	// walks.
	int RegionAt(int col, int row);
	// The coarse grid that is worked out from: cellSize tiles to a cell.
	const std::vector<int>& RegionCells(int& cellsPerSide, int& cellSize);
	// Where it is drawn on the world map, -1..1 across and up.
	void MapPosition(int region, float& x, float& y);
}
