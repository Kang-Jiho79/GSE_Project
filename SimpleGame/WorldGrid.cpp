#include "stdafx.h"
#include "WorldGrid.h"
#include <cmath>
#include <vector>

namespace
{
	const float kTerraceStep = 11.f; // pixels of rise per terrace level
	const float kNoiseCell = 9.f;

	int s_MapCols = 120;
	int s_MapRows = 120;
	unsigned int s_Seed = 0;
	int s_TerraceLevels = 4;

	struct Blocker
	{
		float minCol;
		float minRow;
		float maxCol;
		float maxRow;
	};
	std::vector<Blocker> s_Blockers;

	// How far a walker's body reaches from its centre, in tiles, so it stops
	// against a wall instead of half inside it.
	const float kBodyRadius = 0.3f;

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
	void SetMap(int cols, int rows, unsigned int seed, int terraceLevels)
	{
		s_MapCols = cols;
		s_MapRows = rows;
		s_Seed = seed;
		s_TerraceLevels = (terraceLevels < 1) ? 1 : terraceLevels;
	}

	int MapCols()
	{
		return s_MapCols;
	}

	int MapRows()
	{
		return s_MapRows;
	}

	unsigned int Hash(int col, int row)
	{
		// The seed is mixed in, so the same grid position grows different
		// ground in every region.
		unsigned int h = (unsigned int)(col * 374761393 + row * 668265263 + 2166136261u) + s_Seed * 2654435761u;
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

	bool ScreenToGroundDirection(float screenX, float screenY, float& outCol, float& outRow)
	{
		float col, row;
		InverseIso(screenX, screenY, col, row);

		float length = sqrtf(col * col + row * row);
		if (length < 0.001f)
		{
			return false;
		}

		outCol = col / length;
		outRow = row / length;
		return true;
	}

	void WorldToTile(float worldX, float worldY, int& outCol, int& outRow)
	{
		float col, row;
		InverseIso(worldX, worldY, col, row);

		outCol = (int)(col + 0.5f);
		outRow = (int)(row + 0.5f);
		outCol = (int)Clamp((float)outCol, 0.f, (float)(s_MapCols - 1));
		outRow = (int)Clamp((float)outRow, 0.f, (float)(s_MapRows - 1));
	}

	float TileDepth(float worldX, float worldY)
	{
		int col, row;
		WorldToTile(worldX, worldY, col, row);
		return (float)(col + row);
	}

	float Noise(float col, float row, float cellSize)
	{
		float fx = col / cellSize;
		float fy = row / cellSize;
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
		return top + (bottom - top) * ty;
	}

	// The same noise quantized into terraces, so the ground has plateaus and
	// cliffs instead of per-tile static.
	float Elevation(int col, int row)
	{
		float noise = Noise((float)col, (float)row, kNoiseCell);

		int level = (int)(noise * s_TerraceLevels);
		if (level < 0)
		{
			level = 0;
		}
		if (level > s_TerraceLevels - 1)
		{
			level = s_TerraceLevels - 1;
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
		col = Clamp(col, 0.f, (float)(s_MapCols - 1));
		row = Clamp(row, 0.f, (float)(s_MapRows - 1));
	}

	float SmoothApproach(float current, float target, float rate, float deltaTime)
	{
		// Exponential decay of the remaining gap, so the result does not change
		// with the frame rate the way a plain lerp by (rate * dt) would.
		float blend = 1.f - expf(-rate * deltaTime);
		return current + (target - current) * blend;
	}

	void ClearBlockers()
	{
		s_Blockers.clear();
	}

	void AddBlocker(float minCol, float minRow, float maxCol, float maxRow)
	{
		Blocker blocker = { minCol, minRow, maxCol, maxRow };
		s_Blockers.push_back(blocker);
	}

	bool IsBlocked(float col, float row)
	{
		for (size_t i = 0; i < s_Blockers.size(); i++)
		{
			const Blocker& b = s_Blockers[i];
			if (col > b.minCol - kBodyRadius && col < b.maxCol + kBodyRadius
				&& row > b.minRow - kBodyRadius && row < b.maxRow + kBodyRadius)
			{
				return true;
			}
		}
		return false;
	}

	void Move(float& col, float& row, float offsetCol, float offsetRow)
	{
		float nextCol = col + offsetCol;
		float nextRow = row + offsetRow;
		ClampToMap(nextCol, nextRow);
		if (!IsBlocked(nextCol, nextRow))
		{
			col = nextCol;
			row = nextRow;
			return;
		}

		// Buildings are squared to the grid, so sliding means keeping whichever
		// half of the step runs along the wall.
		float slideCol = col + offsetCol;
		float slideRow = row;
		ClampToMap(slideCol, slideRow);
		if (!IsBlocked(slideCol, slideRow))
		{
			col = slideCol;
			return;
		}

		slideCol = col;
		slideRow = row + offsetRow;
		ClampToMap(slideCol, slideRow);
		if (!IsBlocked(slideCol, slideRow))
		{
			row = slideRow;
		}
	}
}
