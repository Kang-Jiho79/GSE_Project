#pragma once

#include <vector>

class Renderer;

// One light direction for the whole world. Shared so terrain walls and the
// side faces of every box agree about which way the light comes from.
namespace SurfaceLight
{
	const float kTop = 1.f;
	const float kRight = 0.70f;
	const float kLeft = 0.44f;
}

// Everything drawn in the world goes through here. Actors push shapes with a
// depth; the whole list is sorted once and handed to the renderer, so terrain
// and the things standing on it share a single ordering.
//
// The composite shapes live here too, so no actor has to build a lit box or a
// soft glow out of raw quads by itself.
class DrawList
{
public:
	void Clear();
	void Sort();
	void Submit(Renderer* renderer);
	size_t Count() const;

	// The world darkens and fogs out with distance from the player, which is
	// what the near/far radii are measured from. Applied as quads are pushed,
	// so every actor gets it without knowing it exists. HUD and minimap go
	// straight to the renderer and are deliberately left untouched.
	void SetAtmosphere(float lightRadius, float fogStart, float fogEnd,
		float fogR, float fogG, float fogB);

	void AddQuad(float depth, float x, float y, float edgeAX, float edgeAY, float edgeBX, float edgeBY,
		float r, float g, float b, float a);
	// Four corners in screen pixels, wound bottom-left, top-left, top-right,
	// bottom-right. A face whose top edge is shorter than its bottom is what
	// makes a trunk taper or a boulder round off.
	void AddCorners(float depth, const float corners[8], float r, float g, float b, float a);
	void AddDiamond(float depth, float x, float y, float width, float height,
		float r, float g, float b, float a);
	// A screen-aligned rectangle - for small details like eyes that should not
	// tilt with the ground.
	void AddRect(float depth, float x, float y, float width, float height,
		float r, float g, float b, float a);

	// An isometric box: two shaded side faces then a lit top, which covers the
	// seam between them. baseY is where it meets the ground.
	//
	// The footprint is given in tiles along the two ground axes, so a cube on
	// one tile is (1, 1) and a wall running along the col axis is e.g. (2, 0.3).
	void AddBox(float depth, float x, float baseY, float footCols, float footRows, float boxHeight,
		float r, float g, float b);
	// A box whose top face is a fraction of its base - the side faces come out
	// as trapezoids. taper of 1 is an ordinary box, 0.5 halves the top, and
	// above 1 flares outward (shoulders wider than a waist). lean shifts the
	// top sideways so the whole thing tilts.
	void AddTaperedBox(float depth, float x, float baseY, float footCols, float footRows, float boxHeight,
		float taper, float leanX, float leanY, float r, float g, float b);
	void AddTaperedBoxFaces(float depth, float x, float baseY, float footCols, float footRows, float boxHeight,
		float taper, float leanX, float leanY,
		float topR, float topG, float topB,
		float leftR, float leftG, float leftB,
		float rightR, float rightG, float rightB);
	// Same box, but each face colored outright - for something that carries a
	// different power on each side.
	void AddBoxFaces(float depth, float x, float baseY, float footCols, float footRows, float boxHeight,
		float topR, float topG, float topB,
		float leftR, float leftG, float leftB,
		float rightR, float rightG, float rightB);

	// A band of a circle lying flat on the ground around a world point - a
	// ring, a wedge, a slash trail. Cut into pieces that each sort with the
	// ground under them, so the near side is not buried under the tiles in
	// front of its centre. Radii are in tiles; angles are radians on the ground
	// (0 along +col). screenX/screenY is the centre on screen.
	void AddGroundArc(float worldX, float worldY, float screenX, float screenY,
		float innerTiles, float outerTiles, float fromAngle, float toAngle, int segments,
		float r, float g, float b, float a);

	// A straight band lying flat on the ground from a world point along a unit
	// direction, between two distances along it - a charge's path, say.
	// Sorted in pieces like AddGroundArc. Lengths and width in tiles.
	void AddGroundStrip(float worldX, float worldY, float screenX, float screenY,
		float dirCol, float dirRow, float fromTiles, float toTiles, float widthTiles,
		float r, float g, float b, float a);

	// A dark patch on the ground, so a standing thing does not look pasted on.
	void AddShadow(float depth, float x, float baseY, float width, float height);

	// Translucent rings stacked large to small. A bright color reads as a glow;
	// a near-black one darkens what is under it instead - a light that gives off
	// darkness. strength (0..1) scales both size and opacity.
	void AddGlow(float depth, float x, float y, float radius, float r, float g, float b, float strength);

private:
	// Returns the colour after light falloff and fog at that screen point.
	void Weather(float x, float y, float& r, float& g, float& b) const;

	float m_LightRadius = 0.f;
	float m_FogStart = 0.f;
	float m_FogEnd = 0.f;
	float m_FogR = 0.f;
	float m_FogG = 0.f;
	float m_FogB = 0.f;

	struct Quad
	{
		float depth;
		float x, y;
		float edgeAX, edgeAY;
		float edgeBX, edgeBY;
		float corners[8];
		bool useCorners;
		float r, g, b, a;
	};

	// Kept across frames so collecting does not reallocate every time.
	std::vector<Quad> m_Quads;
};
