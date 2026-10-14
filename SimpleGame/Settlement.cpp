#include "stdafx.h"
#include "Settlement.h"
#include "Regions.h"
#include <cmath>

namespace
{
	const float kVillageSquare = 4.f;
	const float kCapitalSquare = 5.f;
	const float kVillageReach = 15.f;
	const float kCapitalReach = 27.f;

	const float kVillageTemple = 3.f;
	const float kCapitalTemple = 6.f;
	// How far in front of the temple's near corner its altar stands.
	const float kAltarStandOff = 0.9f;

	// A capital, from the south: the gate's towers, the street up to the
	// square with houses either side, the great temple west of the square,
	// and the castle north of it, its door this far out from its face.
	const float kCastleSize = 10.f;
	const float kCastleBack = 17.f;     // from the square's centre to the castle's
	const float kDoorStandOff = 0.8f;
	const float kTempleWest = 13.f;
	const float kTempleSouth = 2.f;
	const float kGateReach = 26.f;      // from the square's centre to the gate
	const float kGateHalfWidth = 3.5f;
	const float kGateTower = 2.f;
	const float kStreetHalfWidth = 3.f; // kept clear down the middle
	const float kStreetHouseOffset = 6.f;
	const float kStreetFirst = 9.f;
	const float kStreetGap = 4.5f;
	const int kStreetHouses = 4;        // each side

	const int kVillageHouses = 9;
	const int kBorderbornHouses = 7;
	const int kHamletHouses = 3;
	const int kCapitalHouses = 16;
	const int kPlacementTries = 300;

	// Space kept clear between buildings, and around the altar and the board.
	const float kGap = 1.6f;
	const float kOpenGround = 3.f;
	const float kMapMargin = 3.f;

	// A small generator of its own, seeded per region, so a layout does not
	// depend on whatever else happened to draw random numbers first.
	struct SeededRandom
	{
		unsigned int state;

		float Next()
		{
			state = state * 1664525u + 1013904223u;
			return (state >> 8) / 16777216.f;
		}
	};

	float HouseSize(Regions::Style style)
	{
		switch (style)
		{
		case Regions::Style::Light:
		{
			return 2.2f;
		}
		case Regions::Style::Dark:
		{
			return 2.4f;
		}
		case Regions::Style::Borderborn:
		{
			return 1.7f;
		}
		default:
		{
			return 2.f;
		}
		}
	}

	float Distance(float colA, float rowA, float colB, float rowB)
	{
		float dc = colA - colB;
		float dr = rowA - rowB;
		return sqrtf(dc * dc + dr * dr);
	}
}

bool Settlement::IsSolid(const Building& building)
{
	return building.type != BuildingType::NoticeBoard;
}

Settlement::Layout Settlement::Generate(int region, float centerCol, float centerRow)
{
	const Regions::Info& info = Regions::Get(region);
	bool capital = (info.kind == Regions::Kind::Capital);

	Layout layout;
	layout.squareCol = centerCol;
	layout.squareRow = centerRow;
	layout.squareRadius = capital ? kCapitalSquare : kVillageSquare;
	layout.radius = capital ? kCapitalReach : kVillageReach;

	SeededRandom random = { Regions::Seed(region) * 2246822519u + 1u };

	// The temple stands behind the square as seen on screen, its altar facing
	// out onto it; a capital's great temple stands to the west of it.
	if (info.temple)
	{
		float size = capital ? kCapitalTemple : kVillageTemple;
		float back = layout.squareRadius * 0.75f + size * 0.5f + 1.f;
		float templeCol = capital ? centerCol - kTempleWest : centerCol - back;
		float templeRow = capital ? centerRow - kTempleSouth : centerRow - back;

		Building temple = { BuildingType::Temple, templeCol, templeRow, size, (int)(random.Next() * 8.f) };
		layout.buildings.push_back(temple);

		layout.hasTemple = true;
		layout.altarCol = temple.col + size * 0.5f + kAltarStandOff;
		layout.altarRow = temple.row + size * 0.5f + kAltarStandOff;
	}

	if (capital)
	{
		// The castle to the north, a carpet from the square to its door.
		Building castle = { BuildingType::Palace, centerCol, centerRow - kCastleBack, kCastleSize, 0 };
		layout.buildings.push_back(castle);
		layout.hasCastle = true;
		layout.doorCol = castle.col;
		layout.doorRow = castle.row + kCastleSize * 0.5f + kDoorStandOff;
		layout.hasCarpet = true;
		layout.carpetCol = castle.col;
		layout.carpetRow = castle.row + kCastleSize * 0.5f;
		layout.carpetLength = (centerRow - layout.squareRadius) - layout.carpetRow;

		// The city gate to the south, a tower either side of the road.
		for (int side = -1; side <= 1; side += 2)
		{
			Building tower = { BuildingType::CityGate, centerCol + side * kGateHalfWidth, centerRow + kGateReach, kGateTower, side };
			layout.buildings.push_back(tower);
		}

		// The street up from the gate, lined with houses.
		float streetSize = HouseSize(info.style) * 1.2f;
		for (int i = 0; i < kStreetHouses; i++)
		{
			for (int side = -1; side <= 1; side += 2)
			{
				Building house = { BuildingType::House, centerCol + side * kStreetHouseOffset,
					centerRow + kStreetFirst + i * kStreetGap, streetSize, (int)(random.Next() * 8.f) };
				layout.buildings.push_back(house);
			}
		}
	}

	Building board = { BuildingType::NoticeBoard, centerCol + 2.5f, centerRow - 2.5f, 1.f, 0 };
	layout.buildings.push_back(board);

	int houses = capital ? kCapitalHouses
		: (info.style == Regions::Style::Borderborn ? kBorderbornHouses : kVillageHouses);
	// A hamlet is a few houses; a ruined altar stands alone.
	if (info.feature == Regions::Feature::Hamlet)
	{
		houses = kHamletHouses;
	}
	else if (info.feature == Regions::Feature::RuinedAltar)
	{
		houses = 0;
	}
	float baseSize = HouseSize(info.style) * (capital ? 1.2f : 1.f);

	int placed = 0;
	for (int attempt = 0; attempt < kPlacementTries && placed < houses; attempt++)
	{
		float angle = random.Next() * 6.2831853f;
		float inner = layout.squareRadius + 3.f;
		float distance = inner + random.Next() * (layout.radius - inner - 2.f);
		float size = baseSize * (0.9f + random.Next() * 0.3f);

		Building house = { BuildingType::House,
			centerCol + cosf(angle) * distance, centerRow + sinf(angle) * distance, size, (int)(random.Next() * 8.f) };

		// Inside its own place, clear of the edge.
		float half = size * 0.5f + kMapMargin;
		float minCol, minRow, maxCol, maxRow;
		Regions::Bounds(region, minCol, minRow, maxCol, maxRow);
		if (house.col < minCol + half || house.row < minRow + half || house.col > maxCol - half || house.row > maxRow - half)
		{
			continue;
		}

		bool clear = true;
		for (size_t i = 0; i < layout.buildings.size() && clear; i++)
		{
			const Building& other = layout.buildings[i];
			clear = Distance(house.col, house.row, other.col, other.row) >= (house.footprint + other.footprint) * 0.5f + kGap;
		}
		if (clear && layout.hasTemple)
		{
			clear = Distance(house.col, house.row, layout.altarCol, layout.altarRow) >= size * 0.5f + kOpenGround;
		}
		// A capital's street and the way up to its castle door stay open.
		if (clear && capital)
		{
			bool street = fabsf(house.col - centerCol) < kStreetHalfWidth + size * 0.5f && house.row > centerRow;
			bool approach = fabsf(house.col - centerCol) < kStreetHalfWidth + 1.f + size * 0.5f
				&& house.row < centerRow && house.row > layout.doorRow - 1.f;
			clear = !street && !approach;
		}
		if (!clear)
		{
			continue;
		}

		layout.buildings.push_back(house);
		placed++;
	}

	return layout;
}
