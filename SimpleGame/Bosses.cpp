#include "stdafx.h"
#include "Bosses.h"
#include "Regions.h"
#include "Progress.h"
#include "SceneGraph.h"
#include "WorldGrid.h"
#include <cmath>
#include "TaintedPawnActor.h"
#include "TaintedBishopActor.h"
#include "TaintedRookActor.h"
#include "Tuning.h"
#include "GodPieceActor.h"
#include "LuciusActor.h"
#include "NoctisActor.h"
#include "KingPieceActor.h"
#include "QueenPieceActor.h"
#include "ThroneRoomActor.h"

namespace
{
	// Arenas lie well away from the waystone, toward the far edge of the map
	// on screen, so a boss is something he walks to rather than into.
	const float kArenaCol = 0.82f;
	const float kArenaRow = 0.22f;

	// The arena's walls, squares out from the board's centre, and its
	// threshold, past the opening.
	const int kWallReach = 6;
	const float kThresholdReach = 8.5f;

	// The queen of the board, a little to her king's side.
	const float kQueenBeside = 3.f;

	BossActor* PlaceAt(SceneGraph& graph, BossActor* boss, float col, float row)
	{
		float x, y;
		WorldGrid::IsoPosition(col, row, x, y);
		boss->SetLocalPosition(x, y);
		graph.Spawn(std::unique_ptr<Actor>(boss));
		return boss;
	}

	BossActor* Place(SceneGraph& graph, BossActor* boss, int region)
	{
		float col, row;
		Bosses::Arena(region, col, row);
		return PlaceAt(graph, boss, col, row);
	}
}

void Bosses::Arena(int region, float& col, float& row)
{
	// In a throne room, its floor is the board.
	if (Regions::IsCastle(region))
	{
		ThroneRoomActor::Center(col, row);
		return;
	}
	float minCol, minRow, maxCol, maxRow;
	Regions::Bounds(region, minCol, minRow, maxCol, maxRow);
	float size = (float)Regions::Get(region).size;
	col = minCol + size * kArenaCol;
	row = minRow + size * kArenaRow;
}

int Bosses::WallReach()
{
	return kWallReach;
}

void Bosses::Opening(int region, int& dirCol, int& dirRow)
{
	int col, row;
	BoardCenter(region, col, row);
	float waystoneCol, waystoneRow;
	Regions::Center(region, waystoneCol, waystoneRow);
	float dc = waystoneCol - col;
	float dr = waystoneRow - row;
	dirCol = 0;
	dirRow = 0;
	if (fabsf(dc) >= fabsf(dr))
	{
		dirCol = (dc >= 0.f) ? 1 : -1;
	}
	else
	{
		dirRow = (dr >= 0.f) ? 1 : -1;
	}
}

void Bosses::Threshold(int region, float& col, float& row)
{
	int centerCol, centerRow, dirCol, dirRow;
	BoardCenter(region, centerCol, centerRow);
	Opening(region, dirCol, dirRow);
	col = centerCol + dirCol * kThresholdReach;
	row = centerRow + dirRow * kThresholdReach;
}

bool Bosses::HasBoard(int region)
{
	return region == Regions::LiminaWoods || region == Regions::Crepus || region == Regions::Divisio
		|| region == Regions::LightGate || region == Regions::DarkGate;
}

void Bosses::BoardCenter(int region, int& col, int& row)
{
	float arenaCol, arenaRow;
	Arena(region, arenaCol, arenaRow);
	col = (int)floorf(arenaCol + 0.5f);
	row = (int)floorf(arenaRow + 0.5f);
}

BossActor* Bosses::Spawn(int region, const Progress& progress, const Settlement::Layout& settlement, SceneGraph& graph)
{
	// The kings, in their throne rooms at the ends of their carpets, whether
	// he has beaten them or not - unless the god has taken king and queen
	// back as its pieces, and they wait for him there as a king and queen of
	// the board. Once those are broken, the king is left fallen where it
	// stood.
	if (settlement.hasThrone && Regions::IsCastle(region))
	{
		bool light = (region == Regions::LumenCastle);
		bool risen = progress.Has(light ? Story::LightPairRisen : Story::DarkPairRisen);
		bool broken = progress.Has(light ? Story::LightPairBroken : Story::DarkPairBroken);
		if (risen && !broken)
		{
			QueenPieceActor* queen = new QueenPieceActor(region, light);
			PlaceAt(graph, queen, settlement.throneCol + kQueenBeside, settlement.throneRow);
			return PlaceAt(graph, new KingPieceActor(region, light), settlement.throneCol, settlement.throneRow);
		}
		KingActor* king = light ? (KingActor*)new LuciusActor(region, progress.recognizedByLucius)
			: (KingActor*)new NoctisActor(region, progress.recognizedByNoctis);
		if (broken)
		{
			king->SetFallen();
		}
		return PlaceAt(graph, king, settlement.throneCol, settlement.throneRow);
	}

	if (progress.bossDefeated[region])
	{
		return nullptr;
	}

	// The tainted pieces: the black pawn that drove him from home, starting on
	// its board's back rank; the white bishop on the border; and the rook of
	// both colours at the front where the two still clash.
	switch (region)
	{
	case Regions::LiminaWoods:
	{
		int col, row;
		BoardCenter(region, col, row);
		return PlaceAt(graph, new TaintedPawnActor(region), (float)col, (float)(row - Tuning::TaintedPiece::kBoardHalf));
	}
	case Regions::Crepus:
	{
		return Place(graph, new TaintedBishopActor(region), region);
	}
	case Regions::Divisio:
	{
		return Place(graph, new TaintedRookActor(region), region);
	}

	// The god's pieces, one on the road to each king.
	case Regions::LightGate:
	{
		return Place(graph, new GodPieceActor(region, GodPieceActor::Kind::Rook, 700.f), region);
	}
	case Regions::DarkGate:
	{
		return Place(graph, new GodPieceActor(region, GodPieceActor::Kind::Knight, 700.f), region);
	}
	default:
	{
		return nullptr;
	}
	}
}
