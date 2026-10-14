#include "stdafx.h"
#include "Regions.h"
#include <cmath>

namespace
{
	using Regions::Kind;
	using Regions::Style;
	using Regions::Feature;

	// Field and gate names are working names - the design does not name them yet.
	const Regions::Info kRegions[Regions::Count] =
	{
		// The squares, five across and seven down - north at the top:
		//
		//   Lumen      AshRift    Kinera    FallenKing    Tenebrae
		//   LightGate  Spes       Divisio   Luctus        DarkGate
		//   Aura       Fides      Refugium  Requiem       Crypta
		//   Sola       Candida    AshMarch  Obscura       Nox
		//   Alba       Treuga     Bellum    Wreckage      Umbra
		//   LightHmlt  Vespera    Crepus    WayfarerCamp  DarkHamlet
		//   RuinedAlt  Limina     LiminaW.  Hideout       PilgrimRest
		//
		// Settlements stand on flat ground. The borderborn's hidden village has
		// no temple - hunted by both nations, they kept no faith with either.
		//
		// name             kind            style              temple size levels props  count light  mage  col row  feature
		{ "리미나",          Kind::Village, Style::Borderborn, false,  60,  1,  0.4f,   0, 0.5f, 0.5f,  1,  6, Feature::None },
		{ "리미나 숲",       Kind::Field,   Style::Borderborn, false,  60,  3,  1.6f,  12, 0.5f, 0.3f,  2,  6, Feature::None },
		{ "크레푸스",        Kind::Field,   Style::Mixed,      false,  60,  4,  1.0f,  12, 0.5f, 0.5f,  2,  5, Feature::None },
		{ "벨룸 평원",       Kind::Field,   Style::Mixed,      false,  60,  2,  0.6f,  14, 0.5f, 0.5f,  2,  4, Feature::None },

		{ "알바",            Kind::Village, Style::Light,      true,   60,  1,  0.3f,   0, 0.8f, 0.5f,  0,  4, Feature::None },
		{ "솔라",            Kind::Field,   Style::Light,      false,  60,  4,  0.8f,  12, 0.8f, 0.5f,  0,  3, Feature::None },
		{ "아우라",          Kind::Village, Style::Light,      true,   60,  1,  0.3f,   0, 0.8f, 0.5f,  0,  2, Feature::None },
		{ "빛의 관문",       Kind::Gate,    Style::Light,      false,  60,  3,  0.5f,   6, 0.8f, 0.5f,  0,  1, Feature::None },
		{ "루멘",            Kind::Capital, Style::Light,      true,   60,  1,  0.0f,   0, 0.8f, 0.5f,  0,  0, Feature::None },

		{ "움브라",          Kind::Village, Style::Dark,       true,   60,  1,  0.3f,   0, 0.2f, 0.5f,  4,  4, Feature::None },
		{ "녹스",            Kind::Field,   Style::Dark,       false,  60,  4,  0.8f,  12, 0.2f, 0.5f,  4,  3, Feature::None },
		{ "크립타",          Kind::Village, Style::Dark,       true,   60,  1,  0.3f,   0, 0.2f, 0.5f,  4,  2, Feature::None },
		{ "어둠의 관문",     Kind::Gate,    Style::Dark,       false,  60,  3,  0.5f,   6, 0.2f, 0.5f,  4,  1, Feature::None },
		{ "테네브라에",      Kind::Capital, Style::Dark,       true,   60,  1,  0.0f,   0, 0.2f, 0.5f,  4,  0, Feature::None },

		{ "잿빛 변경",       Kind::Field,   Style::Ashen,      false,  60,  4,  1.0f,  12, 0.5f, 0.5f,  2,  3, Feature::None },
		{ "피데스",          Kind::Village, Style::Ashen,      true,   60,  1,  0.5f,   4, 0.7f, 0.5f,  1,  2, Feature::None },
		{ "스페스",          Kind::Village, Style::Ashen,      true,   60,  1,  0.5f,   4, 0.7f, 0.5f,  1,  1, Feature::None },
		{ "레퀴엠",          Kind::Village, Style::Ashen,      true,   60,  1,  0.5f,   4, 0.3f, 0.5f,  3,  2, Feature::None },
		{ "루크투스",        Kind::Village, Style::Ashen,      true,   60,  1,  0.5f,   4, 0.3f, 0.5f,  3,  1, Feature::None },
		{ "디비시오",        Kind::Field,   Style::Ashen,      false,  60,  3,  1.0f,  14, 0.5f, 0.5f,  2,  1, Feature::None },
		{ "키네라",          Kind::Field,   Style::Ashen,      false,  60,  4,  0.4f,  12, 0.5f, 0.5f,  2,  0, Feature::None },
		{ "신의 영역",       Kind::Field,   Style::Ashen,      false,  44,  1,  0.0f,   0, 0.5f, 0.5f,  0,  0, Feature::None },

		{ "베스페라",        Kind::Village, Style::Mixed,      true,   60,  1,  0.4f,   0, 0.5f, 0.5f,  1,  5, Feature::None },
		{ "트레우가",        Kind::Village, Style::Mixed,      true,   60,  1,  0.3f,   0, 0.5f, 0.5f,  1,  4, Feature::None },
		{ "칸디다",          Kind::Village, Style::Light,      true,   60,  1,  0.4f,   0, 0.8f, 0.5f,  1,  3, Feature::None },
		{ "오브스쿠라",      Kind::Village, Style::Dark,       true,   60,  1,  0.4f,   0, 0.2f, 0.5f,  3,  3, Feature::None },
		{ "레푸기움",        Kind::Village, Style::Ashen,      true,   60,  1,  0.5f,   0, 0.5f, 0.5f,  2,  2, Feature::None },
		{ "잿빛 균열",       Kind::Field,   Style::Ashen,      false,  60,  3,  0.6f,  12, 0.5f, 0.6f,  1,  0, Feature::Rift },
		{ "쓰러진 왕의 말",  Kind::Field,   Style::Ashen,      false,  60,  2,  0.5f,   6, 0.5f, 0.5f,  3,  0, Feature::FallenKing },
		{ "전쟁터 잔해",     Kind::Field,   Style::Mixed,      false,  60,  2,  0.7f,  12, 0.5f, 0.4f,  3,  4, Feature::Wreckage },
		{ "이름 없는 마을",  Kind::Village, Style::Light,      true,   60,  1,  0.6f,   0, 0.8f, 0.5f,  0,  5, Feature::Hamlet },
		{ "나그네 야영지",   Kind::Field,   Style::Mixed,      false,  60,  2,  0.8f,   6, 0.5f, 0.5f,  3,  5, Feature::Camp },
		{ "이름 없는 마을",  Kind::Village, Style::Dark,       true,   60,  1,  0.6f,   0, 0.2f, 0.5f,  4,  5, Feature::Hamlet },
		{ "버려진 제단",     Kind::Village, Style::Borderborn, true,   60,  2,  1.0f,   8, 0.5f, 0.4f,  0,  6, Feature::RuinedAltar },
		{ "경계인의 은신처", Kind::Field,   Style::Borderborn, false,  60,  3,  1.2f,   6, 0.5f, 0.3f,  3,  6, Feature::Hideout },
		{ "순례자 쉼터",     Kind::Field,   Style::Mixed,      false,  60,  1,  0.5f,   4, 0.5f, 0.5f,  4,  6, Feature::PilgrimRest },
	};

	// The world's grid: squares this many tiles across, five across and
	// seven down, laid out in a square of seven so the grid is the same both
	// ways.
	const int kCellTiles = 60;
	const int kCells = 7;
	// The coarse grid the walkable ground is worked out on, and how much of
	// it either side of a walled edge falls away to cliff and fog.
	const int kMaskTiles = 4;
	const int kWallCells = 1;
	const unsigned int kWorldSeed = 20261013u;

	// The gray's heartland: walled round, but for its march.
	bool IsGray(int region)
	{
		return region == Regions::Kinera || region == Regions::AshRift || region == Regions::FallenKing
			|| region == Regions::Spes || region == Regions::Divisio || region == Regions::Luctus
			|| region == Regions::Fides || region == Regions::Refugium || region == Regions::Requiem
			|| region == Regions::AshMarch;
	}

	// Which place stands in a square of the grid, or -1.
	int RegionInCell(int col, int row)
	{
		for (int region = 0; region < Regions::Count; region++)
		{
			if (!Regions::IsOffMap(region) && kRegions[region].cellCol == col && kRegions[region].cellRow == row)
			{
				return region;
			}
		}
		return -1;
	}

	// Two neighbouring places are walled off where one is of the gray's
	// heartland and the other is not - unless the gray one is its march.
	bool IsWalled(int a, int b)
	{
		bool grayA = IsGray(a);
		bool grayB = IsGray(b);
		if (grayA == grayB)
		{
			return false;
		}
		int gray = grayA ? a : b;
		return gray != Regions::AshMarch;
	}

	struct Link
	{
		int a;
		int b;
	};

	// Which place each coarse cell of the world belongs to, and which places
	// he can walk between; worked out once.
	std::vector<int> s_Mask;
	std::vector<Link> s_Links;

	void BuildLinks()
	{
		s_Links.clear();
		for (int a = 0; a < Regions::Count; a++)
		{
			if (Regions::IsOffMap(a))
			{
				continue;
			}
			int b1 = RegionInCell(kRegions[a].cellCol + 1, kRegions[a].cellRow);
			int b2 = RegionInCell(kRegions[a].cellCol, kRegions[a].cellRow + 1);
			if (b1 >= 0 && !IsWalled(a, b1))
			{
				Link link = { a, b1 };
				s_Links.push_back(link);
			}
			if (b2 >= 0 && !IsWalled(a, b2))
			{
				Link link = { a, b2 };
				s_Links.push_back(link);
			}
		}
	}

	void BuildMask()
	{
		int side = kCells * kCellTiles / kMaskTiles;
		int perCell = kCellTiles / kMaskTiles;
		s_Mask.assign(side * side, -1);

		for (int j = 0; j < side; j++)
		{
			for (int i = 0; i < side; i++)
			{
				int col = i / perCell;
				int row = j / perCell;
				int region = RegionInCell(col, row);
				if (region < 0)
				{
					continue;
				}
				// Within reach of a walled edge, the ground falls away.
				int inCol = i % perCell;
				int inRow = j % perCell;
				bool cliff = false;
				int west = RegionInCell(col - 1, row);
				int east = RegionInCell(col + 1, row);
				int north = RegionInCell(col, row - 1);
				int south = RegionInCell(col, row + 1);
				cliff = cliff || (inCol < kWallCells && west >= 0 && IsWalled(region, west));
				cliff = cliff || (inCol >= perCell - kWallCells && east >= 0 && IsWalled(region, east));
				cliff = cliff || (inRow < kWallCells && north >= 0 && IsWalled(region, north));
				cliff = cliff || (inRow >= perCell - kWallCells && south >= 0 && IsWalled(region, south));
				if (!cliff)
				{
					s_Mask[j * side + i] = region;
				}
			}
		}
	}
}

const Regions::Info& Regions::Get(int region)
{
	return kRegions[region];
}

bool Regions::IsOffMap(int region)
{
	return region == Realm;
}

const Regions::Atmosphere& Regions::AtmosphereOf(int region)
{
	//                                 fog                       light  start  end   sat   contrast  tint
	static const Atmosphere kBorder = { { 0.050f, 0.046f, 0.042f }, 160.f, 220.f, 620.f, 0.85f, 1.00f, { 1.03f, 1.00f, 0.95f } };
	static const Atmosphere kWar = { { 0.045f, 0.043f, 0.050f }, 150.f, 210.f, 620.f, 0.70f, 1.00f, { 1.00f, 0.99f, 0.98f } };
	static const Atmosphere kLightLand = { { 0.060f, 0.052f, 0.040f }, 170.f, 240.f, 660.f, 0.90f, 1.04f, { 1.05f, 1.01f, 0.92f } };
	static const Atmosphere kLightCapital = { { 0.085f, 0.075f, 0.055f }, 260.f, 330.f, 780.f, 1.15f, 1.10f, { 1.07f, 1.03f, 0.94f } };
	static const Atmosphere kDarkLand = { { 0.040f, 0.032f, 0.065f }, 140.f, 200.f, 580.f, 0.90f, 1.06f, { 0.95f, 0.93f, 1.06f } };
	static const Atmosphere kDarkCapital = { { 0.050f, 0.035f, 0.085f }, 220.f, 290.f, 700.f, 1.10f, 1.12f, { 0.97f, 0.92f, 1.10f } };
	static const Atmosphere kAshen = { { 0.050f, 0.050f, 0.052f }, 140.f, 200.f, 560.f, 0.50f, 0.95f, { 0.97f, 0.97f, 0.98f } };
	// The heart of the gray: almost no colour left at all, and the dark close.
	static const Atmosphere kHeart = { { 0.055f, 0.055f, 0.056f }, 120.f, 170.f, 480.f, 0.30f, 0.90f, { 0.96f, 0.96f, 0.97f } };

	// The god's place: colour turned up hard, as if everything were lit for
	// someone watching - and lit far, so the whole board and the one leaning
	// over it can be seen.
	static const Atmosphere kRealm = { { 0.018f, 0.018f, 0.024f }, 420.f, 700.f, 1500.f, 0.75f, 1.18f, { 1.00f, 1.00f, 1.02f } };

	// The rift: the gray lying thick on the ground, the dark close in.
	static const Atmosphere kRift = { { 0.040f, 0.040f, 0.043f }, 90.f, 130.f, 380.f, 0.35f, 0.92f, { 0.96f, 0.96f, 0.97f } };

	if (region == Kinera)
	{
		return kHeart;
	}
	if (region == AshRift)
	{
		return kRift;
	}
	if (region == Realm)
	{
		return kRealm;
	}

	const Info& info = Get(region);
	bool capital = (info.kind == Kind::Capital);
	switch (info.style)
	{
	case Style::Light:
	{
		return capital ? kLightCapital : kLightLand;
	}
	case Style::Dark:
	{
		return capital ? kDarkCapital : kDarkLand;
	}
	case Style::Ashen:
	{
		return kAshen;
	}
	case Style::Borderborn:
	{
		return kBorder;
	}
	default:
	{
		return kWar;
	}
	}
}

unsigned int Regions::Seed(int region)
{
	return (unsigned int)region * 7919u + 13u;
}

bool Regions::AreLinked(int a, int b)
{
	if (s_Links.empty())
	{
		BuildLinks();
	}
	for (size_t i = 0; i < s_Links.size(); i++)
	{
		if ((s_Links[i].a == a && s_Links[i].b == b) || (s_Links[i].a == b && s_Links[i].b == a))
		{
			return true;
		}
	}
	return false;
}

int Regions::LinkCount()
{
	if (s_Links.empty())
	{
		BuildLinks();
	}
	return (int)s_Links.size();
}

void Regions::GetLink(int index, int& a, int& b)
{
	if (s_Links.empty())
	{
		BuildLinks();
	}
	a = s_Links[index].a;
	b = s_Links[index].b;
}

int Regions::WorldSize()
{
	return kCells * kCellTiles;
}

unsigned int Regions::WorldSeed()
{
	return kWorldSeed;
}

void Regions::Center(int region, float& col, float& row)
{
	const Info& info = Get(region);
	if (IsOffMap(region))
	{
		col = info.size * 0.5f;
		row = info.size * 0.5f;
		return;
	}
	col = (float)(info.cellCol * kCellTiles + kCellTiles / 2);
	row = (float)(info.cellRow * kCellTiles + kCellTiles / 2);
}

void Regions::Bounds(int region, float& minCol, float& minRow, float& maxCol, float& maxRow)
{
	float col, row;
	Center(region, col, row);
	float half = Get(region).size * 0.5f;
	minCol = col - half;
	minRow = row - half;
	maxCol = col + half;
	maxRow = row + half;
}

float Regions::DistanceOutside(int region, float col, float row)
{
	float minCol, minRow, maxCol, maxRow;
	Bounds(region, minCol, minRow, maxCol, maxRow);
	float dc = (col < minCol) ? minCol - col : ((col > maxCol) ? col - maxCol : 0.f);
	float dr = (row < minRow) ? minRow - row : ((row > maxRow) ? row - maxRow : 0.f);
	return sqrtf(dc * dc + dr * dr);
}

const std::vector<int>& Regions::RegionCells(int& cellsPerSide, int& cellSize)
{
	if (s_Mask.empty())
	{
		BuildMask();
	}
	cellsPerSide = kCells * kCellTiles / kMaskTiles;
	cellSize = kMaskTiles;
	return s_Mask;
}

int Regions::RegionAt(int col, int row)
{
	int side, cell;
	const std::vector<int>& cells = RegionCells(side, cell);
	int i = col / cell;
	int j = row / cell;
	if (col < 0 || row < 0 || i >= side || j >= side)
	{
		return -1;
	}
	return cells[j * side + i];
}

void Regions::MapPosition(int region, float& x, float& y)
{
	// The grid as it is laid out: west to the left, north at the top.
	const Info& info = Get(region);
	x = (info.cellCol - 2) / 2.4f;
	y = -(info.cellRow - 3) / 3.3f;
}
