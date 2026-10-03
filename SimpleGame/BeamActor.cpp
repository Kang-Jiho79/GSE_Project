#include "stdafx.h"
#include "BeamActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include <cmath>

namespace
{
	const float kLifetime = 0.22f;
	const float kStart = 0.3f;        // tiles out from him, so it leaves his hand rather than his feet
	const float kPieceLength = 0.5f;  // tiles per sorted piece
	const float kLift = 14.f;         // pixels above the ground, the height bolts fly at
	const float kOuterWidth = 9.f;    // pixels across on screen
	const float kCoreWidth = 3.f;

	// One straight piece of the beam between two distances along it, as a
	// quad of the given screen width.
	void BeamPiece(DrawList& out, float depth, float x0, float y0, float x1, float y1,
		float perpX, float perpY, float width, float r, float g, float b, float a)
	{
		float halfX = perpX * width * 0.5f;
		float halfY = perpY * width * 0.5f;

		float corners[8] =
		{
			x0 - halfX, y0 - halfY,
			x0 + halfX, y0 + halfY,
			x1 + halfX, y1 + halfY,
			x1 - halfX, y1 - halfY,
		};
		out.AddCorners(depth, corners, r, g, b, a);
	}
}

BeamActor::BeamActor(Alignment alignment, float dirCol, float dirRow, float lengthTiles)
	: m_Alignment(alignment)
	, m_DirCol(dirCol)
	, m_DirRow(dirRow)
	, m_Length(lengthTiles)
{
}

void BeamActor::OnUpdate(const ActorContext& ctx)
{
	m_Age += ctx.deltaTime;
	if (m_Age >= kLifetime)
	{
		Destroy();
	}
}

void BeamActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);

	float t = m_Age / kLifetime;
	float fade = 1.f - t;
	// Thins as it fades, so it reads as a flash dying rather than a bar going
	// transparent.
	float thin = 1.f - 0.5f * t;

	float baseY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY()) + kLift;

	// Perpendicular to the beam on screen, for its width.
	float dirX, dirY;
	WorldGrid::IsoPosition(m_DirCol, m_DirRow, dirX, dirY);
	float dirLength = sqrtf(dirX * dirX + dirY * dirY);
	float perpX = -dirY / dirLength;
	float perpY = dirX / dirLength;

	// A bright core inside a band of the power's own colour.
	float r, g, b;
	AlignmentColor::Core(m_Alignment, r, g, b);
	float coreR = r + (1.f - r) * 0.7f;
	float coreG = g + (1.f - g) * 0.7f;
	float coreB = b + (1.f - b) * 0.7f;

	// The light's beam blazes; the dark's dash trail does not.
	out.SetGlow(m_Alignment == Alignment::Light ? 1.f : 0.f);

	int pieces = (int)ceilf((m_Length - kStart) / kPieceLength);
	for (int i = 0; i < pieces; i++)
	{
		float from = kStart + i * kPieceLength;
		float to = from + kPieceLength;
		if (to > m_Length)
		{
			to = m_Length;
		}

		float x0, y0, x1, y1, midX, midY;
		WorldGrid::IsoPosition(m_DirCol * from, m_DirRow * from, x0, y0);
		WorldGrid::IsoPosition(m_DirCol * to, m_DirRow * to, x1, y1);
		WorldGrid::IsoPosition(m_DirCol * (from + to) * 0.5f, m_DirRow * (from + to) * 0.5f, midX, midY);

		// Each piece sorts with the ground under it, so the beam passes in
		// front of what it passes in front of.
		float depth = WorldGrid::TileDepth(WorldX() + midX, WorldY() + midY) + 0.7f;

		BeamPiece(out, depth, x + x0, baseY + y0, x + x1, baseY + y1, perpX, perpY,
			kOuterWidth * thin, r, g, b, 0.55f * fade);
		BeamPiece(out, depth, x + x0, baseY + y0, x + x1, baseY + y1, perpX, perpY,
			kCoreWidth * thin, coreR, coreG, coreB, 0.95f * fade);
	}
}
