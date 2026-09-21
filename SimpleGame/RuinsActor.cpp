#include "stdafx.h"
#include "RuinsActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include <algorithm>
#include <cmath>

namespace
{
	const int kRuinChancePercent = 7;

	int ViewRadius(const ActorContext& ctx)
	{
		float uMax = (ctx.windowSizeX / 2.f) / (WorldGrid::kTileWidth / 2.f) + 2.f;
		float vMax = (ctx.windowSizeY / 2.f) / (WorldGrid::kTileHeight / 2.f) + 4.f;
		return (int)std::ceil((uMax + vMax) / 2.f);
	}
}

void RuinsActor::OnDraw(DrawList& out, const ActorContext& ctx) const
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
			unsigned int hash = WorldGrid::Hash(col + 7717, row + 3313);
			if (hash % 100 >= kRuinChancePercent)
			{
				continue;
			}

			float worldX, worldY;
			WorldGrid::IsoPosition((float)col, (float)row, worldX, worldY);

			float x, y;
			ctx.ToScreen(worldX, worldY, x, y);

			if (!ctx.IsOnScreen(x, y, WorldGrid::kTileWidth, WorldGrid::kTileHeight * 5.f))
			{
				continue;
			}

			float baseY = y + WorldGrid::Elevation(col, row);
			float depth = (float)(col + row);

			int variant = (hash / 100) % 3;
			float footWidth = (variant == 2) ? 40.f : 24.f;
			float footHeight = footWidth / 2.f;
			float boxHeight = (variant == 2) ? 9.f : (30.f + variant * 16.f);

			float shade = 0.145f + (hash % 7) * 0.004f;

			// Same depth as the tile it stands on. The sort is stable and the
			// terrain is collected first, so the shadow lands on the ground and
			// the stone lands on the shadow without any nudging.
			out.AddShadow(depth, x, baseY, footWidth + 8.f, (footWidth + 8.f) / 2.f);
			out.AddBox(depth, x, baseY, footWidth, footHeight, boxHeight, shade, shade, shade);
		}
	}
}
