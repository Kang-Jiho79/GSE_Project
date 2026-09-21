#include "stdafx.h"
#include "TerrainActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include <algorithm>
#include <cmath>

namespace
{
	// How far out to consider tiles, derived from the window so the view is
	// always filled to the corners no matter the window size.
	int ViewRadius(const ActorContext& ctx)
	{
		float uMax = (ctx.windowSizeX / 2.f) / (WorldGrid::kTileWidth / 2.f) + 2.f;
		float vMax = (ctx.windowSizeY / 2.f) / (WorldGrid::kTileHeight / 2.f) + 4.f;
		return (int)std::ceil((uMax + vMax) / 2.f);
	}
}

void TerrainActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	int centerCol, centerRow;
	WorldGrid::WorldToTile(ctx.camX, ctx.camY, centerCol, centerRow);

	int radius = ViewRadius(ctx);
	int colMin = std::max(0, centerCol - radius);
	int colMax = std::min(WorldGrid::kMapCols - 1, centerCol + radius);
	int rowMin = std::max(0, centerRow - radius);
	int rowMax = std::min(WorldGrid::kMapRows - 1, centerRow + radius);

	for (int row = rowMin; row <= rowMax; row++)
	{
		for (int col = colMin; col <= colMax; col++)
		{
			float worldX, worldY;
			WorldGrid::IsoPosition((float)col, (float)row, worldX, worldY);

			float x, y;
			ctx.ToScreen(worldX, worldY, x, y);

			if (!ctx.IsOnScreen(x, y, WorldGrid::kTileWidth, WorldGrid::kTileHeight * 4.f))
			{
				continue;
			}

			float elevation = WorldGrid::Elevation(col, row);
			float topY = y + elevation;
			float depth = (float)(col + row);

			// Everything is gray; only the grain and the lighting vary.
			float grain = (WorldGrid::Hash(col, row) % 100) / 100.f * 0.04f;
			float shade = 0.17f + grain + elevation * 0.0016f;

			// A wall is only drawn where the neighbour that way sits lower, so
			// flat ground costs nothing and only the cliffs show thickness.
			float rightDrop = elevation - WorldGrid::Elevation(col + 1, row);
			if (rightDrop > 0.f)
			{
				out.AddQuad(depth,
					x + WorldGrid::kTileWidth / 4.f, topY - WorldGrid::kTileHeight / 4.f - rightDrop / 2.f,
					WorldGrid::kTileWidth / 2.f, WorldGrid::kTileHeight / 2.f,
					0.f, rightDrop,
					shade * SurfaceLight::kRight, shade * SurfaceLight::kRight * 0.98f,
					shade * SurfaceLight::kRight * 1.08f, 1.f);
			}

			float leftDrop = elevation - WorldGrid::Elevation(col, row + 1);
			if (leftDrop > 0.f)
			{
				out.AddQuad(depth,
					x - WorldGrid::kTileWidth / 4.f, topY - WorldGrid::kTileHeight / 4.f - leftDrop / 2.f,
					WorldGrid::kTileWidth / 2.f, -WorldGrid::kTileHeight / 2.f,
					0.f, leftDrop,
					shade * SurfaceLight::kLeft, shade * SurfaceLight::kLeft * 0.98f,
					shade * SurfaceLight::kLeft * 1.08f, 1.f);
			}

			out.AddDiamond(depth, x, topY, WorldGrid::kTileWidth - 1.f, WorldGrid::kTileHeight - 1.f,
				shade * SurfaceLight::kTop, shade * SurfaceLight::kTop * 0.98f,
				shade * SurfaceLight::kTop * 1.06f, 1.f);
		}
	}
}
