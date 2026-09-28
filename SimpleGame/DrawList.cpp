#include "stdafx.h"
#include "DrawList.h"
#include "Renderer.h"
#include "WorldGrid.h"
#include <algorithm>

void DrawList::Clear()
{
	m_Quads.clear();
}

void DrawList::Sort()
{
	// Stable, so shapes pushed at the same depth keep the order the actors
	// pushed them in - that is what puts a shadow over its tile and a box over
	// its own shadow without needing a separate epsilon for each.
	std::stable_sort(m_Quads.begin(), m_Quads.end(), [](const Quad& a, const Quad& b)
	{
		return a.depth < b.depth;
	});
}

void DrawList::Submit(Renderer* renderer)
{
	for (size_t i = 0; i < m_Quads.size(); i++)
	{
		const Quad& quad = m_Quads[i];
		if (quad.useCorners)
		{
			renderer->DrawSolidCorners(quad.corners, quad.r, quad.g, quad.b, quad.a);
		}
		else
		{
			renderer->DrawSolidQuad(quad.x, quad.y, quad.edgeAX, quad.edgeAY, quad.edgeBX, quad.edgeBY,
				quad.r, quad.g, quad.b, quad.a);
		}
	}
}

size_t DrawList::Count() const
{
	return m_Quads.size();
}

void DrawList::SetAtmosphere(float lightRadius, float fogStart, float fogEnd,
	float fogR, float fogG, float fogB)
{
	m_LightRadius = lightRadius;
	m_FogStart = fogStart;
	m_FogEnd = fogEnd;
	m_FogR = fogR;
	m_FogG = fogG;
	m_FogB = fogB;
}

void DrawList::Weather(float x, float y, float& r, float& g, float& b) const
{
	if (m_FogEnd <= m_FogStart)
	{
		return;
	}

	// The camera is centered on the player, so distance from the screen origin
	// is distance from him. Measured with the vertical doubled, because the iso
	// projection squashes that axis - otherwise the lit area reads as an oval.
	float dy = y * 2.f;
	float distance = sqrtf(x * x + dy * dy);

	// Near the player everything is at full value; past that it falls away
	// toward the fog rather than staying evenly lit to the horizon.
	if (distance > m_LightRadius)
	{
		float shade = 1.f - (distance - m_LightRadius) / (m_FogEnd - m_LightRadius) * 0.45f;
		if (shade < 0.55f)
		{
			shade = 0.55f;
		}
		r *= shade;
		g *= shade;
		b *= shade;
	}

	if (distance <= m_FogStart)
	{
		return;
	}

	float fog = (distance - m_FogStart) / (m_FogEnd - m_FogStart);
	if (fog > 1.f)
	{
		fog = 1.f;
	}

	r += (m_FogR - r) * fog;
	g += (m_FogG - g) * fog;
	b += (m_FogB - b) * fog;
}

void DrawList::AddQuad(float depth, float x, float y, float edgeAX, float edgeAY, float edgeBX, float edgeBY,
	float r, float g, float b, float a)
{
	Weather(x, y, r, g, b);

	Quad quad;
	quad.depth = depth;
	quad.x = x;
	quad.y = y;
	quad.edgeAX = edgeAX;
	quad.edgeAY = edgeAY;
	quad.edgeBX = edgeBX;
	quad.edgeBY = edgeBY;
	quad.r = r;
	quad.g = g;
	quad.b = b;
	quad.a = a;
	quad.corners[0] = 0.f;
	quad.useCorners = false;
	m_Quads.push_back(quad);
}

void DrawList::AddCorners(float depth, const float corners[8], float r, float g, float b, float a)
{
	// Fogged by the centre of the face, which is close enough at these sizes.
	float centerX = (corners[0] + corners[2] + corners[4] + corners[6]) / 4.f;
	float centerY = (corners[1] + corners[3] + corners[5] + corners[7]) / 4.f;
	Weather(centerX, centerY, r, g, b);

	Quad quad;
	quad.depth = depth;
	quad.r = r;
	quad.g = g;
	quad.b = b;
	quad.a = a;
	for (int i = 0; i < 8; i++)
	{
		quad.corners[i] = corners[i];
	}
	quad.useCorners = true;
	m_Quads.push_back(quad);
}

void DrawList::AddDiamond(float depth, float x, float y, float width, float height,
	float r, float g, float b, float a)
{
	AddQuad(depth, x, y, width / 2.f, height / 2.f, -width / 2.f, height / 2.f, r, g, b, a);
}

void DrawList::AddRect(float depth, float x, float y, float width, float height,
	float r, float g, float b, float a)
{
	AddQuad(depth, x, y, width, 0.f, 0.f, height, r, g, b, a);
}

void DrawList::AddBox(float depth, float x, float baseY, float footCols, float footRows, float boxHeight,
	float r, float g, float b)
{
	// A faint cool lift on the blue channel keeps the grays from reading flat.
	const float kCool = 1.08f;

	AddBoxFaces(depth, x, baseY, footCols, footRows, boxHeight,
		r * SurfaceLight::kTop, g * SurfaceLight::kTop, b * SurfaceLight::kTop * kCool,
		r * SurfaceLight::kLeft, g * SurfaceLight::kLeft, b * SurfaceLight::kLeft * kCool,
		r * SurfaceLight::kRight, g * SurfaceLight::kRight, b * SurfaceLight::kRight * kCool);
}

namespace
{
	// The four corners of a footprint's diamond, in the same parameterisation
	// AddBoxFaces uses: near, left, far, right as seen on screen.
	struct Footprint
	{
		float nearX, nearY;
		float leftX, leftY;
		float farX, farY;
		float rightX, rightY;
	};

	Footprint MakeFootprint(float x, float y, float footCols, float footRows)
	{
		float colX = footCols * WorldGrid::kTileWidth / 2.f;
		float colY = footCols * WorldGrid::kTileHeight / 2.f;
		float rowX = footRows * WorldGrid::kTileWidth / 2.f;
		float rowY = footRows * WorldGrid::kTileHeight / 2.f;

		Footprint print;
		print.nearX = x - colX / 2.f + rowX / 2.f;
		print.nearY = y - colY / 2.f - rowY / 2.f;
		print.leftX = x - colX / 2.f - rowX / 2.f;
		print.leftY = y - colY / 2.f + rowY / 2.f;
		print.farX = x + colX / 2.f - rowX / 2.f;
		print.farY = y + colY / 2.f + rowY / 2.f;
		print.rightX = x + colX / 2.f + rowX / 2.f;
		print.rightY = y + colY / 2.f - rowY / 2.f;
		return print;
	}
}

void DrawList::AddTaperedBox(float depth, float x, float baseY, float footCols, float footRows, float boxHeight,
	float taper, float leanX, float leanY, float r, float g, float b)
{
	const float kCool = 1.08f;

	AddTaperedBoxFaces(depth, x, baseY, footCols, footRows, boxHeight, taper, leanX, leanY,
		r * SurfaceLight::kTop, g * SurfaceLight::kTop, b * SurfaceLight::kTop * kCool,
		r * SurfaceLight::kLeft, g * SurfaceLight::kLeft, b * SurfaceLight::kLeft * kCool,
		r * SurfaceLight::kRight, g * SurfaceLight::kRight, b * SurfaceLight::kRight * kCool);
}

void DrawList::AddTaperedBoxFaces(float depth, float x, float baseY, float footCols, float footRows, float boxHeight,
	float taper, float leanX, float leanY,
	float topR, float topG, float topB,
	float leftR, float leftG, float leftB,
	float rightR, float rightG, float rightB)
{
	Footprint bottom = MakeFootprint(x, baseY, footCols, footRows);
	Footprint top = MakeFootprint(x + leanX, baseY + boxHeight + leanY, footCols * taper, footRows * taper);

	// Only the two faces turned toward the viewer are drawn, each spanning one
	// of the edges that meet at the nearest corner. Because the top footprint
	// is smaller, these come out as trapezoids rather than rectangles.
	float left[8] =
	{
		bottom.leftX, bottom.leftY,
		top.leftX, top.leftY,
		top.nearX, top.nearY,
		bottom.nearX, bottom.nearY,
	};
	AddCorners(depth, left, leftR, leftG, leftB, 1.f);

	float right[8] =
	{
		bottom.nearX, bottom.nearY,
		top.nearX, top.nearY,
		top.rightX, top.rightY,
		bottom.rightX, bottom.rightY,
	};
	AddCorners(depth, right, rightR, rightG, rightB, 1.f);

	float cap[8] =
	{
		top.nearX, top.nearY,
		top.leftX, top.leftY,
		top.farX, top.farY,
		top.rightX, top.rightY,
	};
	AddCorners(depth, cap, topR, topG, topB, 1.f);
}

void DrawList::AddBoxFaces(float depth, float x, float baseY, float footCols, float footRows, float boxHeight,
	float topR, float topG, float topB,
	float leftR, float leftG, float leftB,
	float rightR, float rightG, float rightB)
{
	// One tile along either ground axis spans half a tile's width and half its
	// height on screen; these are the top face's two edge vectors.
	float colX = footCols * WorldGrid::kTileWidth / 2.f;
	float colY = footCols * WorldGrid::kTileHeight / 2.f;
	float rowX = footRows * WorldGrid::kTileWidth / 2.f;
	float rowY = footRows * WorldGrid::kTileHeight / 2.f;

	// The left face hangs off the lower-left edge, which runs along the row axis.
	AddQuad(depth,
		x - colX / 2.f, baseY - colY / 2.f + boxHeight / 2.f,
		rowX, -rowY,
		0.f, boxHeight,
		leftR, leftG, leftB, 1.f);

	// The right face hangs off the lower-right edge, which runs along the col axis.
	AddQuad(depth,
		x + rowX / 2.f, baseY - rowY / 2.f + boxHeight / 2.f,
		colX, colY,
		0.f, boxHeight,
		rightR, rightG, rightB, 1.f);

	AddQuad(depth, x, baseY + boxHeight, colX, colY, -rowX, rowY, topR, topG, topB, 1.f);
}

void DrawList::AddShadow(float depth, float x, float baseY, float width, float height)
{
	AddDiamond(depth, x, baseY, width, height, 0.085f, 0.08f, 0.10f, 1.f);
}

void DrawList::AddGlow(float depth, float x, float y, float radius, float r, float g, float b, float strength)
{
	if (strength <= 0.01f)
	{
		return;
	}

	// Largest first, smallest last, so the rings stack into a soft falloff
	// rather than each one fighting the last for what is on top.
	const int kRingCount = 3;
	for (int ring = kRingCount - 1; ring >= 0; ring--)
	{
		float t = (float)(ring + 1) / (float)kRingCount;
		float ringRadius = radius * t * strength;
		float ringAlpha = ((1.f - t) * 0.28f + 0.04f) * strength;

		AddDiamond(depth, x, y, ringRadius * 2.f, ringRadius, r, g, b, ringAlpha);
	}
}
