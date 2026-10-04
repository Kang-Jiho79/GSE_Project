#pragma once

#include <vector>

// Where a village or capital's buildings stand. Laid out from the region's
// seed, so the same place looks the same every time he comes back.
//
// Everything is gathered round a paved square with the waystone in its
// middle: the temple behind it, its altar at the square's edge, a notice
// board, and houses scattered round the outside. A capital also has its
// king's palace off to one side, with a carpet laid out from its door to
// where the king stands.
namespace Settlement
{
	enum class BuildingType
	{
		House,
		Temple,
		NoticeBoard,
		Palace,
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

		// Where the king stands, at the end of the carpet from the palace.
		bool hasThrone = false;
		float throneCol = 0.f;
		float throneRow = 0.f;
		float carpetCol = 0.f;   // the carpet runs along +row from here
		float carpetRow = 0.f;

		// How far out from the square the settlement reaches; nothing wild
		// grows inside it.
		float radius = 0.f;
	};

	Layout Generate(int region, float centerCol, float centerRow);

	// Whether this building stops anyone walking through it.
	bool IsSolid(const Building& building);
}
