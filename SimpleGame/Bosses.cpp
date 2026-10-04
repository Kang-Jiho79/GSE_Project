#include "stdafx.h"
#include "Bosses.h"
#include "Regions.h"
#include "Progress.h"
#include "SceneGraph.h"
#include "WorldGrid.h"
#include "MutantActor.h"
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
		float size = (float)Regions::Get(region).size;
		return PlaceAt(graph, boss, size * kArenaCol, size * kArenaRow);
	}
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

	// The gray's mutants: the one that drove him from home, the one of both
	// powers on the border, and the one at the front where the two still clash.
	switch (region)
	{
	case Regions::LiminaWoods:
	{
		return Place(graph, new MutantActor(region, MutantActor::Origin::Dark, 300.f), region);
	}
	case Regions::Crepus:
	{
		return Place(graph, new MutantActor(region, MutantActor::Origin::Mixed, 500.f), region);
	}
	case Regions::Divisio:
	{
		return Place(graph, new MutantActor(region, MutantActor::Origin::Light, 900.f), region);
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
