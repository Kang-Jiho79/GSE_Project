#include "stdafx.h"
#include "Npcs.h"
#include "NpcActor.h"
#include "Dialogue.h"
#include "Regions.h"
#include "SceneGraph.h"
#include "WorldGrid.h"
#include "Progress.h"

namespace
{
	// Beside the temple's right-hand face, well apart from the altar, so
	// standing at one never means speaking to the other.
	const float kPriestCol = 1.2f;
	const float kPriestRow = -2.6f;

	// Two places at the edge of the square, clear of the waystone, where he
	// arrives, and the notice board.
	const float kSpotCol[2] = { -3.f, 0.8f };
	const float kSpotRow[2] = { 1.f, -3.2f };

	void Place(SceneGraph& graph, NpcActor::Kind kind, Regions::Style style, int conversation, float col, float row)
	{
		if (WorldGrid::IsBlocked(col, row))
		{
			return;
		}

		std::unique_ptr<NpcActor> npc(new NpcActor(kind, style, conversation));
		float x, y;
		WorldGrid::IsoPosition(col, row, x, y);
		npc->SetLocalPosition(x, y);
		graph.Spawn(std::move(npc));
	}

	void PlaceAtSpot(SceneGraph& graph, const Settlement::Layout& settlement, int spot, NpcActor::Kind kind,
		Regions::Style style, int conversation)
	{
		Place(graph, kind, style, conversation, settlement.squareCol + kSpotCol[spot], settlement.squareRow + kSpotRow[spot]);
	}
}

void Npcs::Spawn(int region, const Progress& progress, const Settlement::Layout& settlement, SceneGraph& graph)
{
	const Regions::Info& info = Regions::Get(region);
	if (info.kind != Regions::Kind::Village && info.kind != Regions::Kind::Capital)
	{
		return;
	}

	if (settlement.hasTemple && (info.style == Regions::Style::Light || info.style == Regions::Style::Dark))
	{
		int conversation = (info.style == Regions::Style::Light) ? Dialogue::PriestLight : Dialogue::PriestDark;
		Place(graph, NpcActor::Kind::Priest, info.style, conversation,
			settlement.altarCol + kPriestCol, settlement.altarRow + kPriestRow);
	}

	switch (region)
	{
	case Regions::Limina:
	{
		if (progress.Has(Story::VillageRaided))
		{
			break;
		}
		PlaceAtSpot(graph, settlement, 0, NpcActor::Kind::Elder, info.style, Dialogue::LiminaElder);
		PlaceAtSpot(graph, settlement, 1, NpcActor::Kind::Child, info.style, Dialogue::LiminaChild);
		break;
	}
	// On the roads to the capitals, talk of their kings.
	case Regions::Alba:
	case Regions::Aura:
	{
		PlaceAtSpot(graph, settlement, 0, NpcActor::Kind::Adult, info.style, Dialogue::RumorLucius);
		break;
	}
	case Regions::Umbra:
	case Regions::Crypta:
	{
		PlaceAtSpot(graph, settlement, 0, NpcActor::Kind::Adult, info.style, Dialogue::RumorNoctis);
		break;
	}
	case Regions::Lumen:
	{
		PlaceAtSpot(graph, settlement, 0, NpcActor::Kind::Adult, info.style, Dialogue::CitizenLumen);
		break;
	}
	case Regions::Tenebrae:
	{
		PlaceAtSpot(graph, settlement, 0, NpcActor::Kind::Adult, info.style, Dialogue::CitizenTenebrae);
		break;
	}
	default:
	{
		break;
	}
	}
}
