#include "stdafx.h"
#include "Props.h"
#include "WorldGrid.h"
#include <cmath>
#include <vector>

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

	struct Clearing
	{
		float col;
		float row;
		float radius;
	};

	// Each place's density, and its clearings - kept with the place their
	// centre lies in, so a tile only checks its own place's.
	std::vector<float> s_Density;
	std::vector<std::vector<Clearing>> s_Clearings;

	float DensityAt(int zone)
	{
		return (zone >= 0 && zone < (int)s_Density.size()) ? s_Density[zone] : 0.f;
	}

	bool InClearing(int zone, int col, int row)
	{
		if (zone < 0 || zone >= (int)s_Clearings.size())
		{
			return false;
		}
		const std::vector<Clearing>& clearings = s_Clearings[zone];
		for (size_t i = 0; i < clearings.size(); i++)
		{
			float fromCol = col - clearings[i].col;
			float fromRow = row - clearings[i].row;
			if (fromCol * fromCol + fromRow * fromRow < clearings[i].radius * clearings[i].radius)
			{
				return true;
			}
		}
		return false;
	}
}

void Props::Clear()
{
	s_Density.clear();
	s_Clearings.clear();
}

void Props::SetDensity(int zone, float density)
{
	if (zone < 0)
	{
		return;
	}
	if (zone >= (int)s_Density.size())
	{
		s_Density.resize(zone + 1, 0.f);
	}
	s_Density[zone] = density;
}

void Props::AddClearing(float col, float row, float radius)
{
	int zone = WorldGrid::ZoneAt((int)floorf(col + 0.5f), (int)floorf(row + 0.5f));
	if (zone < 0)
	{
		return;
	}
	if (zone >= (int)s_Clearings.size())
	{
		s_Clearings.resize(zone + 1);
	}
	Clearing clearing = { col, row, radius };
	s_Clearings[zone].push_back(clearing);
}

Props::Kind Props::At(int col, int row, int& variant, float& grain)
{
	variant = 0;
	grain = 0.f;
	int zone = WorldGrid::ZoneAt(col, row);
	float density = DensityAt(zone);
	if (density <= 0.f || InClearing(zone, col, row))
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
	if (roll < grove * grove * kGroveDensity * density)
	{
		return Kind::Tree;
	}

	if (grove < kClearingLimit && roll2 < kStructureChance * density)
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
	if (roll2 < kBoulderChance * density)
	{
		return Kind::Boulder;
	}
	if (roll2 < kRubbleChance * density)
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
