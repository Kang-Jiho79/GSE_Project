#include "stdafx.h"
#include "ArenaActor.h"
#include "Bosses.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Palette.h"
#include <cmath>
#include <cstdlib>

namespace
{
	// Its walls: dark stone, crenellated, a little taller than a man.
	const Palette::Color kWall = { 0.24f, 0.24f, 0.26f };
	const float kWallHeight = 26.f;
	const float kCrenel = 6.f;
	// The threshold's pillars, and the statues.
	const Palette::Color kPale = { 0.62f, 0.62f, 0.64f };
	const Palette::Color kDark = { 0.14f, 0.13f, 0.16f };
	const float kPillarHeight = 70.f;
	const int kOpeningHalf = 1;          // squares either side of the middle

	// The approach: statues in pairs this many squares out, and this far to
	// either side.
	const int kStatueFirst = 9;
	const int kStatueGap = 3;
	const int kStatuePairs = 3;
	const float kStatueSide = 2.5f;
	// The great piece behind it, seen from afar.
	const float kLandmarkReach = 8.f;
	const float kLandmarkScale = 3.4f;

	const float kSealAlpha = 0.55f;
}

ArenaActor::ArenaActor(int region)
{
	Bosses::BoardCenter(region, m_CenterCol, m_CenterRow);
	Bosses::Opening(region, m_DirCol, m_DirRow);

	// Each side a long box; the opening's side in two, either side of it.
	int reach = Bosses::WallReach();
	for (int side = 0; side < 4; side++)
	{
		int normalCol = (side == 0) ? 1 : ((side == 1) ? -1 : 0);
		int normalRow = (side == 2) ? 1 : ((side == 3) ? -1 : 0);
		float alongMin = -reach - 0.5f;
		float alongMax = reach + 0.5f;
		bool opening = (normalCol == m_DirCol && normalRow == m_DirRow);
		for (int part = 0; part < (opening ? 2 : 1); part++)
		{
			float from = alongMin;
			float to = alongMax;
			if (opening)
			{
				from = (part == 0) ? alongMin : kOpeningHalf + 0.5f;
				to = (part == 0) ? -kOpeningHalf - 0.5f : alongMax;
			}
			if (normalCol != 0)
			{
				float col = m_CenterCol + normalCol * (float)reach;
				WorldGrid::AddBlocker(col - 0.5f, m_CenterRow + from, col + 0.5f, m_CenterRow + to);
			}
			else
			{
				float row = m_CenterRow + normalRow * (float)reach;
				WorldGrid::AddBlocker(m_CenterCol + from, row - 0.5f, m_CenterCol + to, row + 0.5f);
			}
		}
	}

	float x, y;
	WorldGrid::IsoPosition((float)m_CenterCol, (float)m_CenterRow, x, y);
	SetLocalPosition(x, y);
}

void ArenaActor::OpeningBox(float& minCol, float& minRow, float& maxCol, float& maxRow) const
{
	int reach = Bosses::WallReach();
	float col = m_CenterCol + m_DirCol * (float)reach;
	float row = m_CenterRow + m_DirRow * (float)reach;
	float half = kOpeningHalf + 0.5f;
	minCol = (m_DirCol != 0) ? col - 0.5f : col - half;
	maxCol = (m_DirCol != 0) ? col + 0.5f : col + half;
	minRow = (m_DirRow != 0) ? row - 0.5f : row - half;
	maxRow = (m_DirRow != 0) ? row + 0.5f : row + half;
}

void ArenaActor::SetSealed(bool sealed)
{
	if (sealed == m_Sealed)
	{
		return;
	}
	float minCol, minRow, maxCol, maxRow;
	OpeningBox(minCol, minRow, maxCol, maxRow);
	if (sealed)
	{
		WorldGrid::AddBlocker(minCol, minRow, maxCol, maxRow);
	}
	else
	{
		WorldGrid::RemoveBlocker(minCol, minRow, maxCol, maxRow);
	}
	m_Sealed = sealed;
}

bool ArenaActor::IsInside(float col, float row) const
{
	float reach = Bosses::WallReach() - 0.5f;
	return fabsf(col - m_CenterCol) < reach && fabsf(row - m_CenterRow) < reach;
}

void ArenaActor::OnUpdate(const ActorContext& ctx)
{
	m_Time += ctx.deltaTime;
}

void ArenaActor::DrawStatue(DrawList& out, const ActorContext& ctx, float col, float row, float scale, bool white,
	int kind) const
{
	float worldX, worldY, x, y;
	WorldGrid::IsoPosition(col, row, worldX, worldY);
	ctx.ToScreen(worldX, worldY, x, y);
	if (!ctx.IsOnScreen(x, y, 60.f * scale, 120.f * scale))
	{
		return;
	}
	float groundY = y + WorldGrid::GroundHeightAt(worldX, worldY);
	float depth = WorldGrid::TileDepth(worldX, worldY) + 0.5f;
	const Palette::Color& c = white ? kPale : kDark;

	out.AddShadow(depth - 0.01f, x, groundY, 40.f * scale, 20.f * scale);
	out.AddTaperedBox(depth, x, groundY, 0.9f * scale, 0.9f * scale, 6.f * scale, 0.9f, 0.f, 0.f, c.r * 0.9f, c.g * 0.9f, c.b * 0.9f);
	out.AddTaperedBox(depth + 0.005f, x, groundY + 6.f * scale, 0.5f * scale, 0.5f * scale, 22.f * scale, 0.75f, 0.f, 0.f,
		c.r, c.g, c.b);
	float top = groundY + 28.f * scale;
	switch (kind % 3)
	{
	case 0:
	{
		// A pawn's round head.
		out.AddTaperedBox(depth + 0.01f, x, top, 0.45f * scale, 0.45f * scale, 9.f * scale, 1.2f, 0.f, 0.f, c.r, c.g, c.b);
		out.AddTaperedBox(depth + 0.015f, x, top + 9.f * scale, 0.55f * scale, 0.55f * scale, 6.f * scale, 0.35f, 0.f, 0.f,
			c.r, c.g, c.b);
		break;
	}
	case 1:
	{
		// A rook's crown.
		out.AddTaperedBox(depth + 0.01f, x, top, 0.6f * scale, 0.6f * scale, 10.f * scale, 1.1f, 0.f, 0.f, c.r, c.g, c.b);
		break;
	}
	default:
	{
		// A king's: tall, with a cross on top.
		out.AddTaperedBox(depth + 0.01f, x, top, 0.55f * scale, 0.55f * scale, 16.f * scale, 0.8f, 0.f, 0.f, c.r, c.g, c.b);
		out.AddTaperedBox(depth + 0.015f, x, top + 16.f * scale, 0.12f * scale, 0.12f * scale, 10.f * scale, 1.f, 0.f, 0.f,
			c.r, c.g, c.b);
		out.AddRect(depth + 0.02f, x, top + 22.f * scale, 10.f * scale, 3.f * scale, c.r, c.g, c.b, 1.f);
		break;
	}
	}
}

void ArenaActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	// Big enough that the great piece behind is seen well before the walls.
	if (!ctx.IsOnScreen(x, y, 1200.f, 900.f))
	{
		return;
	}

	int reach = Bosses::WallReach();
	int perpCol = m_DirRow;
	int perpRow = m_DirCol;

	// The walls, square by square round the board, but for the opening.
	for (int a = -reach; a <= reach; a++)
	{
		for (int side = 0; side < 4; side++)
		{
			int col = m_CenterCol;
			int row = m_CenterRow;
			int normalCol = (side == 0) ? 1 : ((side == 1) ? -1 : 0);
			int normalRow = (side == 2) ? 1 : ((side == 3) ? -1 : 0);
			// Corners are walked twice; draw them from the col sides only.
			if (normalRow != 0 && abs(a) == reach)
			{
				continue;
			}
			col += normalCol * reach + (normalCol == 0 ? a : 0);
			row += normalRow * reach + (normalRow == 0 ? a : 0);
			bool opening = (normalCol == m_DirCol && normalRow == m_DirRow && abs(a) <= kOpeningHalf);
			if (opening)
			{
				continue;
			}
			float worldX, worldY, wx, wy;
			WorldGrid::IsoPosition((float)col, (float)row, worldX, worldY);
			ctx.ToScreen(worldX, worldY, wx, wy);
			if (!ctx.IsOnScreen(wx, wy, 60.f, 80.f))
			{
				continue;
			}
			float groundY = wy + WorldGrid::GroundHeightAt(worldX, worldY);
			float depth = (float)(col + row) + 0.5f;

			// Either side of the opening: the threshold's tall pillars.
			bool threshold = (normalCol == m_DirCol && normalRow == m_DirRow && abs(a) == kOpeningHalf + 1);
			if (threshold)
			{
				out.AddBox(depth, wx, groundY, 0.9f, 0.9f, 8.f, kPale.r * 0.8f, kPale.g * 0.8f, kPale.b * 0.8f);
				out.AddTaperedBox(depth + 0.005f, wx, groundY + 8.f, 0.65f, 0.65f, kPillarHeight - 16.f, 0.85f, 0.f, 0.f,
					kPale.r, kPale.g, kPale.b);
				out.AddBox(depth + 0.01f, wx, groundY + kPillarHeight - 8.f, 0.85f, 0.85f, 8.f, kPale.r, kPale.g, kPale.b);
				continue;
			}
			out.AddBox(depth, wx, groundY, 0.95f, 0.95f, kWallHeight, kWall.r, kWall.g, kWall.b);
			if ((col + row) % 2 == 0)
			{
				out.AddBox(depth + 0.01f, wx, groundY + kWallHeight, 0.4f, 0.4f, kCrenel, kWall.r, kWall.g, kWall.b);
			}
		}
	}

	// The approach: pairs of small pieces, white and black, either side.
	for (int i = 0; i < kStatuePairs; i++)
	{
		float out1 = (float)(kStatueFirst + i * kStatueGap);
		for (int s = -1; s <= 1; s += 2)
		{
			float col = m_CenterCol + m_DirCol * out1 + perpCol * kStatueSide * s;
			float row = m_CenterRow + m_DirRow * out1 + perpRow * kStatueSide * s;
			if (WorldGrid::ZoneAt((int)floorf(col + 0.5f), (int)floorf(row + 0.5f)) < 0)
			{
				continue;
			}
			DrawStatue(out, ctx, col, row, 1.f, s < 0, i);
		}
	}

	// Behind it all, a great king of stone.
	DrawStatue(out, ctx, m_CenterCol - m_DirCol * kLandmarkReach, m_CenterRow - m_DirRow * kLandmarkReach,
		kLandmarkScale, true, 2);

	// Sealed: bars of gray fog across the opening.
	if (m_Sealed)
	{
		float r = Palette::kAshCore.r;
		float g = Palette::kAshCore.g;
		float b = Palette::kAshCore.b;
		out.SetGlow(0.4f);
		for (int a = -kOpeningHalf; a <= kOpeningHalf; a++)
		{
			float col = m_CenterCol + m_DirCol * (float)reach + perpCol * (float)a;
			float row = m_CenterRow + m_DirRow * (float)reach + perpRow * (float)a;
			float worldX, worldY, wx, wy;
			WorldGrid::IsoPosition(col, row, worldX, worldY);
			ctx.ToScreen(worldX, worldY, wx, wy);
			float groundY = wy + WorldGrid::GroundHeightAt(worldX, worldY);
			float pulse = 0.8f + 0.2f * sinf(m_Time * 3.f + a);
			out.AddRect((float)((int)col + (int)row) + 0.55f, wx, groundY + 24.f, 26.f, 48.f, r, g, b, kSealAlpha * pulse);
		}
		out.SetGlow(0.f);
	}
}
