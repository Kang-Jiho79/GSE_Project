#include "stdafx.h"
#include "Settlement.h"
#include "Regions.h"
#include <cmath>

namespace
{
	const float kVillageSquare = 4.f;
	const float kCapitalSquare = 5.f;
	const float kVillageReach = 15.f;
	const float kCapitalReach = 22.f;

	const float kVillageTemple = 3.f;
	const float kCapitalTemple = 4.f;
	// How far in front of the temple's near corner its altar stands.
	const float kAltarStandOff = 0.9f;

	// The palace stands to the right of the square on screen; the king, this
	// far out from its near-left face. Nothing is built within kCourt of him,
	// so a duel has room.
	const float kPalaceSize = 5.f;
	const float kThroneStandOff = 2.f;
	const float kCourt = 5.f;

	const int kVillageHouses = 9;
	const int kBorderbornHouses = 7;
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
	// out onto it.
	if (info.temple)
	{
		float size = capital ? kCapitalTemple : kVillageTemple;
		float back = layout.squareRadius * 0.75f + size * 0.5f + 1.f;

		Building temple = { BuildingType::Temple, centerCol - back, centerRow - back, size, (int)(random.Next() * 8.f) };
		layout.buildings.push_back(temple);

		layout.hasTemple = true;
		layout.altarCol = temple.col + size * 0.5f + kAltarStandOff;
		layout.altarRow = temple.row + size * 0.5f + kAltarStandOff;
	}

	if (capital)
	{
		float back = layout.squareRadius * 0.75f + kPalaceSize * 0.5f + 1.f;
		Building palace = { BuildingType::Palace, centerCol + back, centerRow - back, kPalaceSize, 0 };
		layout.buildings.push_back(palace);

		layout.hasThrone = true;
		layout.carpetCol = palace.col;
		layout.carpetRow = palace.row + kPalaceSize * 0.5f;
		layout.throneCol = palace.col;
		layout.throneRow = layout.carpetRow + kThroneStandOff;
	}

	Building board = { BuildingType::NoticeBoard, centerCol + 2.5f, centerRow - 2.5f, 1.f, 0 };
	layout.buildings.push_back(board);

	int houses = capital ? kCapitalHouses
		: (info.style == Regions::Style::Borderborn ? kBorderbornHouses : kVillageHouses);
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

		float half = size * 0.5f + kMapMargin;
		if (house.col < half || house.row < half || house.col > info.size - half || house.row > info.size - half)
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
		if (clear && layout.hasThrone)
		{
			clear = Distance(house.col, house.row, layout.throneCol, layout.throneRow) >= size * 0.5f + kCourt;
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
