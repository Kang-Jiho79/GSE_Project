#include "stdafx.h"
#include "Props.h"
#include "WorldGrid.h"
#include <cmath>

namespace
{
	const float kGroveCell = 13.f;      // tiles per grove-sized noise feature
	const float kGroveDensity = 0.30f;  // tree chance at the heart of a grove
	const float kClearingLimit = 0.5f;  // stonework only where grove noise is below this

	const float kStructureChance = 0.025f;
	const float kBoulderChance = 0.045f;
	const float kRubbleChance = 0.065f;

	// How much of a tile the big things fill, from its centre. A wall is long
	// and thin, along one axis or the other.
	const float kBoulderRadius = 0.38f;
	const float kTowerRadius = 0.5f;
	const float kHouseRadius = 0.6f;
	const float kWallHalfLength = 0.7f;
	const float kWallHalfWidth = 0.12f;

	// Whether a point is inside what stands on a tile centred at (col, row).
	bool Inside(Props::Kind kind, int variant, float dc, float dr, float margin)
	{
		float radius = 0.f;
		switch (kind)
		{
		case Props::Kind::Wall:
		{
			bool alongCol = (variant % 2) == 0;
			float halfCols = (alongCol ? kWallHalfLength : kWallHalfWidth) + margin;
			float halfRows = (alongCol ? kWallHalfWidth : kWallHalfLength) + margin;
			return fabsf(dc) < halfCols && fabsf(dr) < halfRows;
		}
		case Props::Kind::Tower:
		{
			radius = kTowerRadius;
			break;
		}
		case Props::Kind::House:
		{
			radius = kHouseRadius;
			break;
		}
		case Props::Kind::Boulder:
		{
			// Only the big ones, with a second stone beside them.
			if ((variant % 2) != 0)
			{
				return false;
			}
			radius = kBoulderRadius;
			break;
		}
		default:
		{
			return false;
		}
		}
		radius += margin;
		return dc * dc + dr * dr < radius * radius;
	}

	// Plain values, so they are set before anything can ask.
	float s_Density = 0.f;
	float s_ClearCol = 0.f;
	float s_ClearRow = 0.f;
	float s_ClearRadius = 0.f;
}

void Props::SetField(float density, float clearCol, float clearRow, float clearRadius)
{
	s_Density = density;
	s_ClearCol = clearCol;
	s_ClearRow = clearRow;
	s_ClearRadius = clearRadius;
}

Props::Kind Props::At(int col, int row, int& variant, float& grain)
{
	variant = 0;
	grain = 0.f;
	if (s_Density <= 0.f)
	{
		return Kind::None;
	}

	float fromClearCol = col - s_ClearCol;
	float fromClearRow = row - s_ClearRow;
	if (fromClearCol * fromClearCol + fromClearRow * fromClearRow < s_ClearRadius * s_ClearRadius)
	{
		return Kind::None;
	}

	unsigned int hash = WorldGrid::Hash(col + 7717, row + 3313);
	float roll = (hash % 1000) / 1000.f;
	variant = (hash / 1000) % 8;
	float roll2 = ((hash / 8000) % 1000) / 1000.f;
	grain = ((hash / 8000000) % 7) * 0.004f;

	// Squared so groves have a dense heart and a sparse edge instead of a
	// hard line.
	float grove = WorldGrid::Noise((float)col, (float)row, kGroveCell);
	if (roll < grove * grove * kGroveDensity * s_Density)
	{
		return Kind::Tree;
	}

	if (grove < kClearingLimit && roll2 < kStructureChance * s_Density)
	{
		switch (variant % 4)
		{
		case 0:
		{
			return Kind::Wall;
		}
		case 1:
		{
			return Kind::Pillar;
		}
		case 2:
		{
			return Kind::Tower;
		}
		default:
		{
			return Kind::House;
		}
		}
	}
	if (roll2 < kBoulderChance * s_Density)
	{
		return Kind::Boulder;
	}
	if (roll2 < kRubbleChance * s_Density)
	{
		return Kind::Rubble;
	}
	return Kind::None;
}

Props::Kind Props::At(int col, int row)
{
	int variant;
	float grain;
	return At(col, row, variant, grain);
}

bool Props::IsSolid(float col, float row, float margin)
{
	if (s_Density <= 0.f)
	{
		return false;
	}

	// Anything standing is centred on its tile and no wider than one, so
	// only the tiles right round the point can reach it.
	int baseCol = (int)floorf(col + 0.5f);
	int baseRow = (int)floorf(row + 0.5f);
	for (int r = baseRow - 1; r <= baseRow + 1; r++)
	{
		for (int c = baseCol - 1; c <= baseCol + 1; c++)
		{
			if (c < 0 || r < 0 || c >= WorldGrid::MapCols() || r >= WorldGrid::MapRows())
			{
				continue;
			}
			int variant;
			float grain;
			Kind kind = At(c, r, variant, grain);
			if (kind != Kind::None && Inside(kind, variant, col - c, row - r, margin))
			{
				return true;
			}
		}
	}
	return false;
}
