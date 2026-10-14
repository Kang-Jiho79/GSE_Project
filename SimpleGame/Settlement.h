#pragma once

#include <vector>

// Where a village or capital's buildings stand. Laid out from the region's
// seed, so the same place looks the same every time he comes back.
//
// Everything is gathered round a paved square with the waystone in its
// middle: the temple behind it, its altar at the square's edge, a notice
// board, and houses scattered round the outside.
//
// A capital is laid out from the south, where its road comes in: a city gate
// between two towers, a street lined with houses up to the square, the great
// temple to the west of it, and to the north the castle, with a carpet from
// the square to its door. Inside the castle - a map of its own - the king
// stands at the end of the throne room's carpet.
namespace Settlement
{
	enum class BuildingType
	{
		House,
		Temple,
		NoticeBoard,
		Palace,
		CityGate, // one of the two towers either side of a capital's gate
	};

	struct Building
	{
		BuildingType type;
		float col;       // centre, in tiles
		float row;
		float footprint; // tiles along each side
		int variant;
	};

	struct Layout
	{
		std::vector<Building> buildings;

		float squareCol = 0.f;
		float squareRow = 0.f;
		float squareRadius = 0.f;

		bool hasTemple = false;
		float altarCol = 0.f;
		float altarRow = 0.f;

		// Where the king stands, at the end of the throne room's carpet.
		bool hasThrone = false;
		float throneCol = 0.f;
		float throneRow = 0.f;

		// A carpet laid along +row from here, this many tiles.
		bool hasCarpet = false;
		float carpetCol = 0.f;
		float carpetRow = 0.f;
		float carpetLength = 0.f;

		// A capital's castle door: where he stands to go in.
		bool hasCastle = false;
		float doorCol = 0.f;
		float doorRow = 0.f;

		// How far out from the square the settlement reaches; nothing wild
		// grows inside it.
		float radius = 0.f;
	};

	Layout Generate(int region, float centerCol, float centerRow);

	// Whether this building stops anyone walking through it.
	bool IsSolid(const Building& building);
}
