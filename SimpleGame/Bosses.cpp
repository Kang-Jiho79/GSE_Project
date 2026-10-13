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

namespace
{
	// Arenas lie well away from the waystone, toward the far edge of the map
	// on screen, so a boss is something he walks to rather than into.
	const float kArenaCol = 0.82f;
	const float kArenaRow = 0.22f;

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
	float size = (float)Regions::Get(region).size;
	col = size * kArenaCol;
	row = size * kArenaRow;
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
	// The kings, waiting at the ends of their carpets whether he has beaten
	// them or not.
	if (settlement.hasThrone && region == Regions::Lumen)
	{
		return PlaceAt(graph, new LuciusActor(region, progress.recognizedByLucius), settlement.throneCol, settlement.throneRow);
	}
	if (settlement.hasThrone && region == Regions::Tenebrae)
	{
		return PlaceAt(graph, new NoctisActor(region, progress.recognizedByNoctis), settlement.throneCol, settlement.throneRow);
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
