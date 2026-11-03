#include "stdafx.h"
#include "ThroneRoomActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Palette.h"
#include "Alignment.h"
#include <cmath>
#include <cstdlib>

namespace
{
	// The hall: its floor runs this many squares each way from the centre,
	// the walls stand just past it, and the door is in the near wall.
	const int kCenter = 13;
	const int kFloorReach = 10;
	const float kWallHeightBack = 70.f;   // the two far walls, tall
	const float kWallHeightFront = 10.f;  // the two near ones, low, so the hall can be seen
	// The dais and throne at the far end, the king a little before them on
	// the carpet, which runs from just inside the door.
	const float kThroneRow = (float)(kCenter - kFloorReach + 1);
	const float kKingRow = kThroneRow + 3.f;
	const float kCarpetFrom = kThroneRow + 1.f;
	const float kCarpetTo = (float)(kCenter + kFloorReach);
	const float kArrivalRow = (float)(kCenter + kFloorReach - 2);
	const float kExitRow = (float)(kCenter + kFloorReach - 0.5f);
	// Two rows of pillars down the hall.
	const float kPillarSide = 5.f;
	const int kPillarRows = 4;
	const float kPillarFirst = kThroneRow + 3.f;
	const float kPillarGap = 4.f;
	const float kPillarRadius = 0.4f;

	struct Look
	{
		Palette::Color wall;
		Palette::Color stone;
		Palette::Color trim;
		Palette::Color cloth;
	};

	Look LookFor(Regions::Style style)
	{
		Look look;
		if (style == Regions::Style::Dark)
		{
			look.wall = { 0.14f, 0.13f, 0.17f };
			look.stone = { 0.20f, 0.18f, 0.24f };
			look.trim = { 0.36f, 0.28f, 0.55f };
			look.cloth = { 0.22f, 0.12f, 0.32f };
		}
		else
		{
			look.wall = { 0.46f, 0.45f, 0.44f };
			look.stone = { 0.56f, 0.55f, 0.53f };
			look.trim = { 0.66f, 0.55f, 0.30f };
			look.cloth = { 0.70f, 0.60f, 0.32f };
		}
		return look;
	}
}

Settlement::Layout ThroneRoomActor::Layout()
{
	Settlement::Layout layout;
	layout.squareCol = (float)kCenter;
	layout.squareRow = (float)kCenter;
	layout.hasThrone = true;
	layout.throneCol = (float)kCenter;
	layout.throneRow = kKingRow;
	layout.hasCarpet = true;
	layout.carpetCol = (float)kCenter;
	layout.carpetRow = kCarpetFrom;
	layout.carpetLength = kCarpetTo - kCarpetFrom;
	return layout;
}

int ThroneRoomActor::FloorReach()
{
	return kFloorReach;
}

void ThroneRoomActor::Center(float& col, float& row)
{
	col = (float)kCenter;
	row = (float)kCenter;
}

void ThroneRoomActor::Arrival(float& col, float& row)
{
	col = (float)kCenter;
	row = kArrivalRow;
}

void ThroneRoomActor::ExitDoor(float& col, float& row)
{
	col = (float)kCenter;
	row = kExitRow;
}

ThroneRoomActor::ThroneRoomActor(Regions::Style style)
	: m_Style(style)
{
	// The walls, just past the floor on every side.
	float from = kCenter - kFloorReach - 0.5f;
	float to = kCenter + kFloorReach + 0.5f;
	WorldGrid::AddBlocker(from - 3.f, from - 3.f, to + 3.f, from);
	WorldGrid::AddBlocker(from - 3.f, to, to + 3.f, to + 3.f);
	WorldGrid::AddBlocker(from - 3.f, from, from, to);
	WorldGrid::AddBlocker(to, from, to + 3.f, to);
	// The dais the throne stands on.
	WorldGrid::AddBlocker(kCenter - 1.5f, kThroneRow - 1.f, kCenter + 1.5f, kThroneRow + 0.5f);
	// The pillars.
	for (int i = 0; i < kPillarRows; i++)
	{
		float row = kPillarFirst + i * kPillarGap;
		for (int side = -1; side <= 1; side += 2)
		{
			float col = kCenter + side * kPillarSide;
			WorldGrid::AddBlocker(col - kPillarRadius, row - kPillarRadius, col + kPillarRadius, row + kPillarRadius);
		}
	}

	float x, y;
	WorldGrid::IsoPosition((float)kCenter, (float)kCenter, x, y);
	SetLocalPosition(x, y);
}

void ThroneRoomActor::OnUpdate(const ActorContext& ctx)
{
	m_Time += ctx.deltaTime;
}

bool ThroneRoomActor::Place(const ActorContext& ctx, float col, float row, float& x, float& y, float& depth) const
{
	float worldX, worldY;
	WorldGrid::IsoPosition(col, row, worldX, worldY);
	ctx.ToScreen(worldX, worldY, x, y);
	if (!ctx.IsOnScreen(x, y, 80.f, 160.f))
	{
		return false;
	}
	depth = WorldGrid::TileDepth(worldX, worldY) + 0.5f;
	return true;
}

void ThroneRoomActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	Look look = LookFor(m_Style);
	bool light = (m_Style != Regions::Style::Dark);
	float glowR, glowG, glowB;
	AlignmentColor::Core(light ? Alignment::Light : Alignment::Dark, glowR, glowG, glowB);
	float x, y, depth;

	// The carpet, laid on the floor from the door up to the dais.
	if (Place(ctx, (float)kCenter, kCarpetFrom, x, y, depth))
	{
		float worldX, worldY;
		WorldGrid::IsoPosition((float)kCenter, kCarpetFrom, worldX, worldY);
		out.SetGround(true);
		out.AddGroundStrip(worldX, worldY, x, y + 1.f, 0.f, 1.f, 0.f, kCarpetTo - kCarpetFrom, 2.2f,
			look.cloth.r * 0.7f, look.cloth.g * 0.7f, look.cloth.b * 0.7f, 1.f);
		out.AddGroundStrip(worldX, worldY, x, y + 1.5f, 0.f, 1.f, 0.f, kCarpetTo - kCarpetFrom, 1.6f,
			look.cloth.r * 0.35f, look.cloth.g * 0.35f, look.cloth.b * 0.35f, 1.f);
		out.SetGround(false);
	}

	// The walls, square by square: tall at the far sides, low at the near,
	// with the door in the near wall facing the throne.
	int from = kCenter - kFloorReach - 1;
	int to = kCenter + kFloorReach + 1;
	for (int i = from; i <= to; i++)
	{
		for (int side = 0; side < 4; side++)
		{
			int col = (side == 0) ? from : ((side == 1) ? to : i);
			int row = (side == 2) ? from : ((side == 3) ? to : i);
			if (side >= 2 && (i == from || i == to))
			{
				continue;
			}
			bool far = (col == from || row == from);
			bool door = (row == to && abs(col - kCenter) <= 1);
			if (!Place(ctx, (float)col, (float)row, x, y, depth))
			{
				continue;
			}
			float height = far ? kWallHeightBack : kWallHeightFront;
			if (door)
			{
				// The door: a dark opening between two posts.
				if (col == kCenter)
				{
					out.AddRect(depth, x, y + 4.f, 30.f, 8.f, 0.03f, 0.03f, 0.04f, 1.f);
				}
				else
				{
					out.AddBox(depth, x, y, 0.4f, 0.4f, 34.f, look.trim.r, look.trim.g, look.trim.b);
				}
				continue;
			}
			out.AddBox(depth, x, y, 1.f, 1.f, height, look.wall.r, look.wall.g, look.wall.b);
			// On the far walls, a banner every few squares, and between them
			// the land's light: a lit window, or a brazier.
			if (far && i % 4 == 0)
			{
				out.AddRect(depth + 0.01f, x, y + height * 0.55f, 14.f, 34.f, look.cloth.r, look.cloth.g, look.cloth.b, 1.f);
				out.AddRect(depth + 0.012f, x, y + height * 0.55f + 4.f, 6.f, 6.f, look.trim.r, look.trim.g, look.trim.b, 1.f);
			}
			else if (far && i % 4 == 2)
			{
				float flicker = light ? 1.f : 0.85f + 0.15f * sinf(m_Time * 7.f + i);
				out.SetGlow(light ? 0.8f : 0.4f);
				out.AddGlow(depth + 0.005f, x, y + height * 0.6f, 40.f * flicker, glowR, glowG, glowB, 0.6f);
				out.AddRect(depth + 0.01f, x, y + height * 0.6f, 10.f, light ? 24.f : 10.f, glowR, glowG, glowB, 1.f);
				out.SetGlow(0.f);
			}
		}
	}

	// The pillars down the hall.
	for (int i = 0; i < kPillarRows; i++)
	{
		float row = kPillarFirst + i * kPillarGap;
		for (int side = -1; side <= 1; side += 2)
		{
			if (!Place(ctx, kCenter + side * kPillarSide, row, x, y, depth))
			{
				continue;
			}
			out.AddShadow(depth - 0.01f, x, y, 34.f, 17.f);
			out.AddBox(depth, x, y, 0.9f, 0.9f, 6.f, look.stone.r * 0.9f, look.stone.g * 0.9f, look.stone.b * 0.9f);
			out.AddTaperedBox(depth + 0.005f, x, y + 6.f, 0.7f, 0.7f, 70.f, 0.9f, 0.f, 0.f, look.stone.r, look.stone.g, look.stone.b);
			out.AddBox(depth + 0.01f, x, y + 76.f, 0.9f, 0.9f, 6.f, look.trim.r * 0.8f, look.trim.g * 0.8f, look.trim.b * 0.8f);
		}
	}

	// The dais and the throne on it, its back high, the land's mark above.
	if (Place(ctx, (float)kCenter, kThroneRow, x, y, depth))
	{
		out.AddBox(depth - 0.2f, x, y, 3.f, 1.6f, 8.f, look.stone.r, look.stone.g, look.stone.b);
		out.AddBox(depth, x, y + 8.f, 1.f, 0.9f, 14.f, look.cloth.r * 0.6f, look.cloth.g * 0.6f, look.cloth.b * 0.6f);
		out.AddTaperedBox(depth - 0.05f, x, y + 8.f, 1.1f, 0.3f, 52.f, 0.8f, 0.f, 0.f, look.trim.r, look.trim.g, look.trim.b);
		out.SetGlow(light ? 0.7f : 0.f);
		out.AddDiamond(depth + 0.02f, x, y + 72.f, 12.f, 16.f, glowR, glowG, glowB, 1.f);
		out.SetGlow(0.f);
	}
}
