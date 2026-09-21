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

	// The nearest cell, since each tile's diamond owns the +/-0.5 span around
	// its own integer (col, row). Keeps heights and depths locked to one whole
	// tile at a time instead of drifting continuously between two.
	void WorldToTile(float worldX, float worldY, int& outCol, int& outRow);

	// Sort key shared by terrain and everything standing on it.
	float TileDepth(float worldX, float worldY);

	float Elevation(int col, int row);
	float GroundHeightAt(float worldX, float worldY);

	void ClampToMap(float& col, float& row);
}
