#pragma once

#include "Regions.h"

class Renderer;
class TextRenderer;

// The map of the world, opened at a waystone. It shows every road; a place
// he has been, or one a road from here leads to, can be picked to travel
// there. The world holds still while it is open.
class WorldMapScreen
{
public:
	void Open(int currentRegion, const bool* visited);
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
	void NodePosition(int region, float& x, float& y) const;
	void DrawLink(Renderer* renderer, int a, int b) const;
	void DrawNode(Renderer* renderer, TextRenderer* text, int region, bool hovered) const;

	bool m_Open = false;
	int m_Current = 0;
	bool m_Visited[Regions::Count] = {};
};
