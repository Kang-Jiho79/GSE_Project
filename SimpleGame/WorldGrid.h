#pragma once

// The isometric grid the whole world is laid out on: how a (col, row) cell maps
// onto the screen, which cell a loose world position belongs to, and how high
// the terraced ground stands there.
//
// World positions are in iso-pixel space - the plane the camera pans across -
// so a screen direction and a world direction are the same vector, and the
// camera only ever translates.
namespace WorldGrid
{
	const float kTileWidth = 64.f;
	const float kTileHeight = 32.f;

	// The map currently being played: its size, the seed every layout is
	// hashed from, and how many terrace levels its ground rises through (1 is
	// flat). Set when a region is entered; everything below answers for it.
	void SetMap(int cols, int rows, unsigned int seed, int terraceLevels);
	int MapCols();
	int MapRows();

	unsigned int Hash(int col, int row);

	void IsoPosition(float col, float row, float& outX, float& outY);
	void InverseIso(float worldX, float worldY, float& outCol, float& outRow);

	// A screen offset - an aim, a key direction - turned into a unit direction
	// on the ground. The iso mapping is linear, so the conversion that turns
	// positions into tiles turns directions too. False for a zero offset.
	bool ScreenToGroundDirection(float screenX, float screenY, float& outCol, float& outRow);

	// The nearest cell, since each tile's diamond owns the +/-0.5 span around
	// its own integer (col, row). Keeps heights and depths locked to one whole
	// tile at a time instead of drifting continuously between two.
	void WorldToTile(float worldX, float worldY, int& outCol, int& outRow);

	// Sort key shared by terrain and everything standing on it.
	float TileDepth(float worldX, float worldY);

	// Smooth 0..1 value noise over the grid. cellSize is how many tiles one
	// "feature" spans - small for fine grain, large for broad regions.
	float Noise(float col, float row, float cellSize);

	float Elevation(int col, int row);
	float GroundHeightAt(float worldX, float worldY);

	void ClampToMap(float& col, float& row);

	// Ground that cannot be walked: buildings. A box in tiles, cleared when a
	// region is entered and filled by whatever stands in it.
	void ClearBlockers();
	void AddBlocker(float minCol, float minRow, float maxCol, float maxRow);
	// Whether something of ordinary size standing here would be inside one.
	bool IsBlocked(float col, float row);
	// Moves a position by an offset, staying on the map and out of anything
	// blocked. Walking into a wall slides along it rather than stopping dead.
	void Move(float& col, float& row, float offsetCol, float offsetRow);

	// Eases a value toward a target at a rate independent of frame length.
	// The terraces are genuine steps, so anything walking them has to be
	// carried up smoothly or it reads as teleporting between levels.
	float SmoothApproach(float current, float target, float rate, float deltaTime);
}
