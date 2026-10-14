#pragma once

// What stands on each tile of the map: dead trees in groves, boulders,
// rubble, and the broken stonework of whatever stood here before. Nothing is
// stored - it is worked out from the tile and the map's seed the same way
// every time, so what is drawn and what blocks the way always agree. How
// thickly things stand is each place's own (WorldGrid::ZoneAt).
namespace Props
{
	enum class Kind
	{
		None,
		Tree,
		Boulder,
		Rubble, // low enough to walk over
		Wall,
		Pillar,
		Tower,
		House,
	};

	// Nothing standing anywhere, and no clearings.
	void Clear();
	// How thickly things stand in a place, 1 being an ordinary field.
	void SetDensity(int zone, float density);
	// A clearing round a point where nothing stands - a waystone, a village,
	// where a boss waits.
	void AddClearing(float col, float row, float radius);
	// Whether a tile lies in one of those clearings.
	bool IsClear(int col, int row);

	// What stands on a tile. variant and grain vary its look.
	Kind At(int col, int row, int& variant, float& grain);
	Kind At(int col, int row);

	// Whether a point on the ground is inside anything big enough to stand in
	// the way - towers, walls, fallen houses and the larger boulders - with
	// margin tiles to spare round it, a body's width say. Trees, pillars,
	// small stones and rubble are passed by.
	bool IsSolid(float col, float row, float margin);
}
