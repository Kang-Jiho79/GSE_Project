#pragma once

#include "Regions.h"

class Renderer;
class TextRenderer;

// The map of the world: the squares of the board, five across and seven
// down, each in the colour of whose land it is, with the cliffs round the
// gray's heartland drawn in, and where he stands right now. Places he has
// not been to and cannot see from here are left blank.
//
// Opened anywhere just to look (M), or at a waystone, where a place he has
// been, or one next to where he stands, can be picked to travel there. The
// world holds still while it is open.
class WorldMapScreen
{
public:
	// travel: opened at a waystone, so a place can be picked to go there.
	// playerCol/playerRow: where he stands, in the world's tiles; a place off
	// the map has no square, and then he is not marked.
	void Open(int currentRegion, const bool* visited, float playerCol, float playerRow, bool travel);
	void Close();
	bool IsOpen() const;

	// The region under the cursor that he can travel to, or -1. Positions
	// are pixels from the window centre, +y up.
	int RegionAt(float x, float y) const;

	void Render(Renderer* renderer, TextRenderer* text, int windowSizeX, int windowSizeY,
		float mouseX, float mouseY) const;

private:
	bool IsKnown(int region) const;
	bool CanTravelTo(int region) const;
	// The centre of a place's square on screen.
	void NodePosition(int region, float& x, float& y) const;
	void DrawSquare(Renderer* renderer, TextRenderer* text, int region, bool hovered) const;
	// The cliffs between two neighbouring squares, if any.
	void DrawWalls(Renderer* renderer) const;
	void DrawPlayer(Renderer* renderer) const;

	bool m_Open = false;
	bool m_Travel = false;
	int m_Current = 0;
	float m_PlayerCol = 0.f;
	float m_PlayerRow = 0.f;
	bool m_Visited[Regions::Count] = {};
};
