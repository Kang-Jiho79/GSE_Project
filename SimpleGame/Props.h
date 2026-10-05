#pragma once

// What stands on each tile of the current region: dead trees in groves,
// boulders, rubble, and the broken stonework of whatever stood here before.
// Nothing is stored - it is worked out from the tile and the region's seed
// the same way every time, so what is drawn and what blocks the way always
// agree.
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

	// The region's field: how thickly things stand, 1 being an ordinary
	// field, and a clearing round a point - a waystone, a village - where
	// nothing does.
	void SetField(float density, float clearCol, float clearRow, float clearRadius);

	// What stands on a tile. variant and grain vary its look.
	Kind At(int col, int row, int& variant, float& grain);
	Kind At(int col, int row);

	// Whether a point on the ground is inside anything big enough to stand in
	// the way - towers, walls, fallen houses and the larger boulders - with
	// margin tiles to spare round it, a body's width say. Trees, pillars,
	// small stones and rubble are passed by.
	bool IsSolid(float col, float row, float margin);
}
