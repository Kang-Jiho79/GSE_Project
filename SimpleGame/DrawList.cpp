#include "stdafx.h"
#include "DrawList.h"
#include "Renderer.h"
#include <algorithm>

void DrawList::Clear()
{
	m_Quads.clear();
}

void DrawList::SubmitSorted(Renderer* renderer)
{
	// Stable, so shapes pushed at the same depth keep the order the actors
	// pushed them in - that is what puts a shadow over its tile and a box over
	// its own shadow without needing a separate epsilon for each.
	std::stable_sort(m_Quads.begin(), m_Quads.end(), [](const Quad& a, const Quad& b)
	{
		return a.depth < b.depth;
	});

	for (size_t i = 0; i < m_Quads.size(); i++)
	{
		const Quad& quad = m_Quads[i];
		renderer->DrawSolidQuad(quad.x, quad.y, quad.edgeAX, quad.edgeAY, quad.edgeBX, quad.edgeBY,
			quad.r, quad.g, quad.b, quad.a);
	}
}

void DrawList::AddQuad(float depth, float x, float y, float edgeAX, float edgeAY, float edgeBX, float edgeBY,
	float r, float g, float b, float a)
{
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
	m_Quads.push_back(quad);
}

void DrawList::AddDiamond(float depth, float x, float y, float width, float height,
	float r, float g, float b, float a)
{
	AddQuad(depth, x, y, width / 2.f, height / 2.f, -width / 2.f, height / 2.f, r, g, b, a);
}

void DrawList::AddBox(float depth, float x, float baseY, float footWidth, float footHeight, float boxHeight,
	float r, float g, float b)
{
	// A faint cool lift on the blue channel keeps the grays from reading flat.
	const float kCool = 1.08f;

	AddBoxFaces(depth, x, baseY, footWidth, footHeight, boxHeight,
		r * SurfaceLight::kTop, g * SurfaceLight::kTop, b * SurfaceLight::kTop * kCool,
		r * SurfaceLight::kLeft, g * SurfaceLight::kLeft, b * SurfaceLight::kLeft * kCool,
		r * SurfaceLight::kRight, g * SurfaceLight::kRight, b * SurfaceLight::kRight * kCool);
}

void DrawList::AddBoxFaces(float depth, float x, float baseY, float footWidth, float footHeight, float boxHeight,
	float topR, float topG, float topB,
	float leftR, float leftG, float leftB,
	float rightR, float rightG, float rightB)
{
	AddQuad(depth,
		x - footWidth / 4.f, baseY - footHeight / 4.f + boxHeight / 2.f,
		footWidth / 2.f, -footHeight / 2.f,
		0.f, boxHeight,
		leftR, leftG, leftB, 1.f);

	AddQuad(depth,
		x + footWidth / 4.f, baseY - footHeight / 4.f + boxHeight / 2.f,
		footWidth / 2.f, footHeight / 2.f,
		0.f, boxHeight,
		rightR, rightG, rightB, 1.f);

	AddDiamond(depth, x, baseY + boxHeight, footWidth, footHeight, topR, topG, topB, 1.f);
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
