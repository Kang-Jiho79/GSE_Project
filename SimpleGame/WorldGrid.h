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
	const int kMapCols = 120;
	const int kMapRows = 120;

	const float kTileWidth = 64.f;
	const float kTileHeight = 32.f;

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

	// Eases a value toward a target at a rate independent of frame length.
	// The terraces are genuine steps, so anything walking them has to be
	// carried up smoothly or it reads as teleporting between levels.
	float SmoothApproach(float current, float target, float rate, float deltaTime);
}
