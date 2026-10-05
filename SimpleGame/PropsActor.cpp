#include "stdafx.h"
#include "PropsActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Props.h"
#include <algorithm>
#include <cmath>

namespace
{
	const float kStone = 0.15f;
	const float kWood = 0.125f;

	int ViewRadius(const ActorContext& ctx)
	{
		float uMax = (ctx.windowSizeX / 2.f) / (WorldGrid::kTileWidth / 2.f) + 2.f;
		float vMax = (ctx.windowSizeY / 2.f) / (WorldGrid::kTileHeight / 2.f) + 4.f;
		return (int)std::ceil((uMax + vMax) / 2.f);
	}

	// One tile along the col axis, as a screen offset from a ground point.
	void ColStep(float tiles, float& outX, float& outY)
	{
		outX = tiles * WorldGrid::kTileWidth / 2.f;
		outY = -tiles * WorldGrid::kTileHeight / 2.f;
	}

	void RowStep(float tiles, float& outX, float& outY)
	{
		outX = -tiles * WorldGrid::kTileWidth / 2.f;
		outY = -tiles * WorldGrid::kTileHeight / 2.f;
	}
}

void PropsActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	int centerCol, centerRow;
	WorldGrid::WorldToTile(ctx.camX, ctx.camY, centerCol, centerRow);

	int radius = ViewRadius(ctx);
	int colMin = std::max(0, centerCol - radius);
	int colMax = std::min(WorldGrid::MapCols() - 1, centerCol + radius);
	int rowMin = std::max(0, centerRow - radius);
	int rowMax = std::min(WorldGrid::MapRows() - 1, centerRow + radius);

	for (int row = rowMin; row <= rowMax; row++)
	{
		for (int col = colMin; col <= colMax; col++)
		{
			int variant;
			float shadeGrain;
			Props::Kind kind = Props::At(col, row, variant, shadeGrain);
			if (kind == Props::Kind::None)
			{
				continue;
			}

			float worldX, worldY;
			WorldGrid::IsoPosition((float)col, (float)row, worldX, worldY);

			float x, y;
			ctx.ToScreen(worldX, worldY, x, y);

			if (!ctx.IsOnScreen(x, y, WorldGrid::kTileWidth * 1.5f, WorldGrid::kTileHeight * 6.f))
			{
				continue;
			}

			float groundY = y + WorldGrid::Elevation(col, row);
			float depth = (float)(col + row);
			float shade = kStone + shadeGrain;

			switch (kind)
			{
			case Props::Kind::Tree:
			{
				DrawTree(out, depth, x, groundY, variant, kWood + shadeGrain);
				break;
			}
			case Props::Kind::Wall:
			{
				DrawWall(out, depth, x, groundY, variant, shade);
				break;
			}
			case Props::Kind::Pillar:
			{
				DrawPillar(out, depth, x, groundY, variant, shade);
				break;
			}
			case Props::Kind::Tower:
			{
				DrawTower(out, depth, x, groundY, variant, shade);
				break;
			}
			case Props::Kind::House:
			{
				DrawHouse(out, depth, x, groundY, variant, shade);
				break;
			}
			case Props::Kind::Boulder:
			{
				DrawBoulder(out, depth, x, groundY, variant, shade);
				break;
			}
			default:
			{
				DrawRubble(out, depth, x, groundY, variant, shade);
				break;
			}
			}
		}
	}
}

void PropsActor::DrawTree(DrawList& out, float depth, float x, float groundY, int variant, float shade) const
{
	// Bare: a trunk that narrows as it rises, leaning off true, with a few
	// stiff branches tapering off it and nothing on them.
	float trunkHeight = 26.f + (variant % 3) * 7.f;
	float lean = ((variant % 2) == 0) ? 1.f : -1.f;
	float sway = 1.5f + (variant % 3);

	out.AddShadow(depth, x, groundY, 14.f, 7.f);

	// Two stacked segments so the trunk bends part-way up rather than being
	// one straight post.
	float midHeight = trunkHeight * 0.55f;
	out.AddTaperedBox(depth, x, groundY, 0.11f, 0.11f, midHeight,
		0.78f, sway * lean * 0.4f, 0.f, shade, shade, shade);
	out.AddTaperedBox(depth + 0.01f, x + sway * lean * 0.4f, groundY + midHeight, 0.086f, 0.086f,
		trunkHeight - midHeight, 0.6f, sway * lean * 0.6f, 0.f, shade, shade, shade);

	float crownX = x + sway * lean;
	float crown = groundY + trunkHeight;

	// Branches: thin, tapering to nothing, thrown out at different heights.
	out.AddTaperedBox(depth + 0.02f, crownX + 4.f * lean, crown - 7.f, 0.055f, 0.055f, 11.f,
		0.25f, 5.f * lean, 1.f, shade * 0.96f, shade * 0.96f, shade * 0.96f);
	out.AddTaperedBox(depth + 0.02f, crownX - 3.f * lean, crown - 11.f, 0.05f, 0.05f, 9.f,
		0.25f, -4.5f * lean, 0.f, shade * 0.96f, shade * 0.96f, shade * 0.96f);
	out.AddTaperedBox(depth + 0.03f, crownX, crown - 1.f, 0.045f, 0.045f, 8.f,
		0.2f, 1.5f * lean, 0.f, shade * 0.92f, shade * 0.92f, shade * 0.92f);

	if (variant >= 4)
	{
		out.AddTaperedBox(depth + 0.02f, crownX - 6.f * lean, crown - 16.f, 0.045f, 0.045f, 7.f,
			0.25f, -4.f * lean, 0.f, shade * 0.96f, shade * 0.96f, shade * 0.96f);
	}
}

void PropsActor::DrawBoulder(DrawList& out, float depth, float x, float groundY, int variant, float shade) const
{
	// Rounded by stacking three slabs that each pull in from the one below, so
	// the silhouette curves instead of stepping.
	out.AddShadow(depth, x, groundY, 30.f, 15.f);

	out.AddTaperedBox(depth, x, groundY, 0.46f, 0.36f, 6.f, 0.86f, 1.f, 0.f, shade, shade, shade);
	out.AddTaperedBox(depth + 0.01f, x + 1.f, groundY + 6.f, 0.40f, 0.31f, 6.f,
		0.74f, 1.5f, 0.f, shade * 1.04f, shade * 1.04f, shade * 1.04f);
	out.AddTaperedBox(depth + 0.02f, x + 2.5f, groundY + 12.f, 0.30f, 0.23f, 5.f,
		0.5f, 1.f, 0.f, shade * 1.08f, shade * 1.08f, shade * 1.08f);

	if ((variant % 2) == 0)
	{
		out.AddTaperedBox(depth + 0.01f, x - 11.f, groundY - 1.f, 0.2f, 0.17f, 5.f,
			0.6f, -1.f, 0.f, shade * 0.95f, shade * 0.95f, shade * 0.95f);
	}
}

void PropsActor::DrawRubble(DrawList& out, float depth, float x, float groundY, int variant, float shade) const
{
	// Broken stone: each piece leans a different way, none of them square.
	out.AddShadow(depth, x, groundY, 22.f, 11.f);

	out.AddTaperedBox(depth, x - 4.f, groundY, 0.22f, 0.18f, 5.f,
		0.72f, -1.5f, 0.f, shade, shade, shade);
	out.AddTaperedBox(depth + 0.01f, x + 6.f, groundY + 1.f, 0.13f, 0.12f, 8.f + (variant % 3) * 2.f,
		0.55f, 2.f, 0.f, shade * 1.04f, shade * 1.04f, shade * 1.04f);
}

void PropsActor::DrawPillar(DrawList& out, float depth, float x, float groundY, int variant, float shade) const
{
	bool broken = (variant % 2) == 1;
	float height = broken ? 22.f : 38.f;

	out.AddShadow(depth, x, groundY, 20.f, 10.f);

	// Slightly narrower at the top and leaning off plumb - nothing here has
	// stood straight in a long time.
	float lean = ((variant % 4) < 2) ? 1.5f : -1.5f;
	out.AddTaperedBox(depth, x, groundY, 0.2f, 0.2f, height, 0.85f, lean, 0.f, shade, shade, shade);

	if (!broken)
	{
		out.AddTaperedBox(depth + 0.01f, x + lean, groundY + height, 0.26f, 0.26f, 4.f,
			0.88f, 0.f, 0.f, shade * 1.05f, shade * 1.05f, shade * 1.05f);
		return;
	}

	// The top lies where it fell, along the col axis in front of the stump.
	float dx, dy;
	ColStep(0.55f, dx, dy);
	out.AddBox(depth + 0.3f, x + dx, groundY + dy, 0.6f, 0.16f, 8.f, shade * 0.97f, shade * 0.97f, shade * 0.97f);
}

void PropsActor::DrawWall(DrawList& out, float depth, float x, float groundY, int variant, float shade) const
{
	bool alongCol = (variant % 2) == 0;
	float footCols = alongCol ? 1.4f : 0.22f;
	float footRows = alongCol ? 0.22f : 1.4f;

	out.AddShadow(depth, x, groundY, alongCol ? 52.f : 30.f, alongCol ? 26.f : 15.f);
	out.AddBox(depth, x, groundY, footCols, footRows, 20.f, shade, shade, shade);

	// Two teeth left on top of what used to be a battlement.
	float dx, dy;
	if (alongCol)
	{
		ColStep(0.4f, dx, dy);
	}
	else
	{
		RowStep(0.4f, dx, dy);
	}
	out.AddBox(depth + 0.01f, x + dx, groundY + dy + 20.f, 0.16f, 0.16f, 6.f, shade * 1.04f, shade * 1.04f, shade * 1.04f);
	out.AddBox(depth + 0.01f, x - dx, groundY - dy + 20.f, 0.16f, 0.16f, 6.f, shade * 1.04f, shade * 1.04f, shade * 1.04f);
}

void PropsActor::DrawTower(DrawList& out, float depth, float x, float groundY, int variant, float shade) const
{
	out.AddShadow(depth, x, groundY, 60.f, 30.f);

	// Tapering slightly toward the top is what reads as a tower rather than a
	// crate: it gives the silhouette a batter, the way real stonework is laid.
	out.AddTaperedBox(depth, x, groundY, 0.9f, 0.9f, 40.f, 0.82f, 0.f, 0.f, shade, shade, shade);

	// The top storey has fallen partly away, so what is left sits off-center.
	float topShift = ((variant % 2) == 0) ? 4.f : -4.f;
	out.AddTaperedBox(depth + 0.01f, x + topShift, groundY + 40.f, 0.58f, 0.58f, 10.f,
		0.8f, topShift * 0.3f, 0.f, shade * 1.03f, shade * 1.03f, shade * 1.03f);

	// A doorway, dark, on the face that catches the light.
	out.AddRect(depth + 0.02f, x + 10.f, groundY + 6.f, 6.f, 12.f, 0.06f, 0.055f, 0.07f, 1.f);
}

void PropsActor::DrawHouse(DrawList& out, float depth, float x, float groundY, int variant, float shade) const
{
	// Two walls still meeting at their corner, everything else gone.
	float colX, colY;
	ColStep(0.55f, colX, colY);
	float rowX, rowY;
	RowStep(0.55f, rowX, rowY);

	out.AddShadow(depth, x, groundY, 56.f, 28.f);
	out.AddBox(depth + 0.3f, x + colX, groundY + colY, 1.1f, 0.22f, 18.f, shade, shade, shade);
	out.AddBox(depth + 0.3f, x + rowX, groundY + rowY, 0.22f, 1.1f, 18.f, shade, shade, shade);

	// Whatever furniture was inside, reduced to a lump on the floor.
	if ((variant % 2) == 0)
	{
		out.AddBox(depth + 0.1f, x - 2.f, groundY - 4.f, 0.2f, 0.2f, 6.f, shade * 0.95f, shade * 0.95f, shade * 0.95f);
	}
}
