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
	void SubmitSorted(Renderer* renderer);

	void AddQuad(float depth, float x, float y, float edgeAX, float edgeAY, float edgeBX, float edgeBY,
		float r, float g, float b, float a);
	void AddDiamond(float depth, float x, float y, float width, float height,
		float r, float g, float b, float a);

	// An isometric box: two shaded side faces then a lit top, which covers the
	// seam between them. baseY is where it meets the ground.
	void AddBox(float depth, float x, float baseY, float footWidth, float footHeight, float boxHeight,
		float r, float g, float b);
	// Same box, but each face colored outright - for something that carries a
	// different power on each side.
	void AddBoxFaces(float depth, float x, float baseY, float footWidth, float footHeight, float boxHeight,
		float topR, float topG, float topB,
		float leftR, float leftG, float leftB,
		float rightR, float rightG, float rightB);

	// A dark patch on the ground, so a standing thing does not look pasted on.
	void AddShadow(float depth, float x, float baseY, float width, float height);

	// Translucent rings stacked large to small. A bright color reads as a glow;
	// a near-black one darkens what is under it instead - a light that gives off
	// darkness. strength (0..1) scales both size and opacity.
	void AddGlow(float depth, float x, float y, float radius, float r, float g, float b, float strength);

private:
	struct Quad
	{
		float depth;
		float x, y;
		float edgeAX, edgeAY;
		float edgeBX, edgeBY;
		float r, g, b, a;
	};

	// Kept across frames so collecting does not reallocate every time.
	std::vector<Quad> m_Quads;
};
