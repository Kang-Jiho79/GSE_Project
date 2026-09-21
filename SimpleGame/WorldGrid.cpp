#include "stdafx.h"
#include "WorldGrid.h"
#include <cmath>

namespace
{
	const float kTerraceStep = 11.f; // pixels of rise per terrace level
	const int kTerraceLevels = 4;
	const float kNoiseCell = 9.f;

	float HashUnit(int col, int row)
	{
		return (WorldGrid::Hash(col, row) % 1000) / 1000.f;
	}

	float Clamp(float value, float low, float high)
	{
		if (value < low)
		{
			return low;
		}
		if (value > high)
		{
			return high;
		}
		return value;
	}
}

namespace WorldGrid
{
	unsigned int Hash(int col, int row)
	{
		unsigned int h = (unsigned int)(col * 374761393 + row * 668265263 + 2166136261u);
		h = (h ^ (h >> 13)) * 1274126177u;
		return h ^ (h >> 16);
	}

	void IsoPosition(float col, float row, float& outX, float& outY)
	{
		outX = (col - row) * (kTileWidth / 2.f);
		outY = -(col + row) * (kTileHeight / 2.f);
	}

	void InverseIso(float worldX, float worldY, float& outCol, float& outRow)
	{
		float u = worldX / (kTileWidth / 2.f);
		float v = -worldY / (kTileHeight / 2.f);
		outCol = (u + v) / 2.f;
		outRow = (v - u) / 2.f;
	}

	void WorldToTile(float worldX, float worldY, int& outCol, int& outRow)
	{
		float col, row;
		InverseIso(worldX, worldY, col, row);

		outCol = (int)(col + 0.5f);
		outRow = (int)(row + 0.5f);
		outCol = (int)Clamp((float)outCol, 0.f, (float)(kMapCols - 1));
		outRow = (int)Clamp((float)outRow, 0.f, (float)(kMapRows - 1));
	}

	float TileDepth(float worldX, float worldY)
	{
		int col, row;
		WorldToTile(worldX, worldY, col, row);
		return (float)(col + row);
	}

	// Smoothed value noise quantized into terraces, so the ground has plateaus
	// and cliffs instead of per-tile static.
	float Elevation(int col, int row)
	{
		float fx = col / kNoiseCell;
		float fy = row / kNoiseCell;
		int x0 = (int)floorf(fx);
		int y0 = (int)floorf(fy);

		float tx = fx - x0;
		float ty = fy - y0;
		tx = tx * tx * (3.f - 2.f * tx);
		ty = ty * ty * (3.f - 2.f * ty);

		float v00 = HashUnit(x0, y0);
		float v10 = HashUnit(x0 + 1, y0);
		float v01 = HashUnit(x0, y0 + 1);
		float v11 = HashUnit(x0 + 1, y0 + 1);

		float top = v00 + (v10 - v00) * tx;
		float bottom = v01 + (v11 - v01) * tx;
		float noise = top + (bottom - top) * ty;

		int level = (int)(noise * kTerraceLevels);
		if (level < 0)
		{
			level = 0;
		}
		if (level > kTerraceLevels - 1)
		{
			level = kTerraceLevels - 1;
		}
		return level * kTerraceStep;
	}

	float GroundHeightAt(float worldX, float worldY)
	{
		int col, row;
		WorldToTile(worldX, worldY, col, row);
		return Elevation(col, row);
	}

	void ClampToMap(float& col, float& row)
	{
		col = Clamp(col, 0.f, (float)(kMapCols - 1));
		row = Clamp(row, 0.f, (float)(kMapRows - 1));
	}
}
