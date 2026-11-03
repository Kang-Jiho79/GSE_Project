#include "stdafx.h"
#include "Npcs.h"
#include "NpcActor.h"
#include "Dialogue.h"
#include "Regions.h"
#include "SceneGraph.h"
#include "WorldGrid.h"
#include "Progress.h"
#include "Quests.h"
#include "Bosses.h"
#include "LandmarkActor.h"
#include "MonsterActor.h"
#include "Random.h"
#include "Tuning.h"
#include <cmath>
#include <cstdlib>

namespace
{
	// Beside the temple's right-hand face, well apart from the altar, so
	// standing at one never means speaking to the other.
	const float kPriestCol = 1.2f;
	const float kPriestRow = -2.6f;

	// The queen, a little to the side of where her king stands, by the throne.
	const float kQueenCol = 3.f;
	const float kQueenRow = -1.5f;

	// Two places at the edge of the square, clear of the waystone, where he
	// arrives, and the notice board.
	const float kSpotCol[2] = { -3.f, 0.8f };
	const float kSpotRow[2] = { 1.f, -3.2f };

	// The battlefield village's two neighbours, standing well apart.
	const float kNeighbourCol[2] = { -5.5f, 1.5f };
	const float kNeighbourRow[2] = { -0.5f, 4.5f };

	// Round what stands in a square with no name: where its people stand, or
	// what is written lies.
	const float kCampCol = 2.f;
	const float kCampRow = 1.5f;
	const float kRestApart = 3.f;
	const float kRestRow = 1.5f;
	const float kPlaqueRow = 2.5f;
	const float kHideoutNote = 2.5f;
	const float kAltarNoteCol = -2.5f;
	const float kAltarNoteRow = 1.5f;
	// The signpost, on the far side of the waystone from the board.
	const float kSignpostReach = 9.f;
	// The peddler, by the wreckage; the lantern, beyond the village's edge.
	const float kMerchantOffset = 3.f;
	const float kLanternCol = -6.f;
	const float kLanternRow = 9.f;

	// How far out to look for open ground when the chosen spot is taken.
	const int kSearchReach = 4;

	NpcActor* Place(SceneGraph& graph, NpcActor::Kind kind, Regions::Style style, int conversation, float col, float row)
	{
		if (WorldGrid::IsBlocked(col, row))
		{
			return nullptr;
		}

		std::unique_ptr<NpcActor> npc(new NpcActor(kind, style, conversation));
		NpcActor* placed = npc.get();
		float x, y;
		WorldGrid::IsoPosition(col, row, x, y);
		npc->SetLocalPosition(x, y);
		graph.Spawn(std::move(npc));
		return placed;
	}

	// The nearest open ground to a spot, out to a few tiles: in rings, so the
	// spot itself if it is free.
	bool FindOpen(float& col, float& row)
	{
		for (int reach = 0; reach <= kSearchReach; reach++)
		{
			for (int dr = -reach; dr <= reach; dr++)
			{
				for (int dc = -reach; dc <= reach; dc++)
				{
					if (abs(dc) != reach && abs(dr) != reach)
					{
						continue;
					}
					if (!WorldGrid::IsBlocked(col + dc, row + dr))
					{
						col += dc;
						row += dr;
						return true;
					}
				}
			}
		}
		return false;
	}

	NpcActor* PlaceNear(SceneGraph& graph, NpcActor::Kind kind, Regions::Style style, int conversation, float col, float row)
	{
		if (!FindOpen(col, row))
		{
			return nullptr;
		}
		return Place(graph, kind, style, conversation, col, row);
	}

	void PlaceAtLandmark(SceneGraph& graph, int region, NpcActor::Kind kind, Regions::Style style, int conversation,
		float offsetCol, float offsetRow)
	{
		float col, row;
		LandmarkActor::Spot(region, col, row);
		PlaceNear(graph, kind, style, conversation, col + offsetCol, row + offsetRow);
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

	// In the throne room, the queen at her king's side - Solena of the
	// light, Nocta of the dark - unless the god has taken her back as its
	// queen of the board; once that is broken, sunk to the floor there.
	if (Regions::IsCastle(region) && settlement.hasThrone)
	{
		bool light = (region == Regions::LumenCastle);
		bool risen = progress.Has(light ? Story::LightPairRisen : Story::DarkPairRisen);
		bool broken = progress.Has(light ? Story::LightPairBroken : Story::DarkPairBroken);
		if (risen && !broken)
		{
			return;
		}
		NpcActor* queen = Place(graph, NpcActor::Kind::Queen, info.style, light ? Dialogue::QueenSolena : Dialogue::QueenNocta,
			settlement.throneCol + kQueenCol, settlement.throneRow + kQueenRow);
		if (queen != nullptr)
		{
			queen->SetFallen(broken);
		}
		return;
	}
	// The squares with no name: whoever has stopped there, or what was left.
	switch (region)
	{
	case Regions::WayfarerCamp:
	{
		PlaceAtLandmark(graph, region, NpcActor::Kind::Adult, Regions::Style::Borderborn, Dialogue::Wayfarer, kCampCol, kCampRow);
		break;
	}
	case Regions::PilgrimRest:
	{
		// One of each side, on benches well apart, the same stone between them.
		PlaceAtLandmark(graph, region, NpcActor::Kind::Elder, Regions::Style::Light, Dialogue::PilgrimRest, -kRestApart, kRestRow);
		PlaceAtLandmark(graph, region, NpcActor::Kind::Adult, Regions::Style::Dark, Dialogue::PilgrimRest, kRestApart, kRestRow);
		break;
	}
	case Regions::Hideout:
	{
		PlaceAtLandmark(graph, region, NpcActor::Kind::Note, info.style, Dialogue::HideoutNote, kHideoutNote, kHideoutNote);
		break;
	}
	case Regions::FallenKing:
	{
		PlaceAtLandmark(graph, region, NpcActor::Kind::Note, info.style, Dialogue::FallenKingPlaque, 0.f, kPlaqueRow);
		break;
	}
	case Regions::Crepus:
	{
		// On the hills, on the far side of the waystone from the board.
		float col, row;
		Regions::Center(region, col, row);
		int boardCol, boardRow;
		Bosses::BoardCenter(region, boardCol, boardRow);
		float dc = col - boardCol;
		float dr = row - boardRow;
		float length = sqrtf(dc * dc + dr * dr);
		if (length > 0.f)
		{
			col += dc / length * kSignpostReach;
			row += dr / length * kSignpostReach;
		}
		PlaceNear(graph, NpcActor::Kind::Sign, info.style, Dialogue::Signpost, col, row);
		break;
	}
	default:
	{
		break;
	}
	}

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
		// In Alba, the pilgrim's mother, waiting for word of him.
		if (region == Regions::Alba)
		{
			PlaceAtSpot(graph, settlement, 1, NpcActor::Kind::Elder, info.style, Dialogue::AlbaMother);
		}
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
	// The villages between.
	case Regions::Vespera:
	{
		PlaceAtSpot(graph, settlement, 0, NpcActor::Kind::Elder, info.style, Dialogue::EveningKeeper);
		PlaceAtSpot(graph, settlement, 1, NpcActor::Kind::Child, info.style, Dialogue::VesperaChild);
		break;
	}
	case Regions::Treuga:
	{
		PlaceAtSpot(graph, settlement, 0, NpcActor::Kind::Adult, info.style, Dialogue::TreugaGuide);
		for (int i = 0; i < 2; i++)
		{
			Regions::Style side = (i == 0) ? Regions::Style::Light : Regions::Style::Dark;
			PlaceNear(graph, NpcActor::Kind::Adult, side, Dialogue::TreugaNeighbours,
				settlement.squareCol + kNeighbourCol[i], settlement.squareRow + kNeighbourRow[i]);
		}
		break;
	}
	case Regions::Candida:
	{
		PlaceAtSpot(graph, settlement, 0, NpcActor::Kind::Adult, info.style, Dialogue::PilgrimCandida);
		break;
	}
	case Regions::Obscura:
	{
		PlaceAtSpot(graph, settlement, 0, NpcActor::Kind::Adult, info.style, Dialogue::PilgrimObscura);
		break;
	}
	case Regions::Refugium:
	{
		PlaceAtSpot(graph, settlement, 0, NpcActor::Kind::Adult, info.style, Dialogue::RefugeeWidow);
		PlaceAtSpot(graph, settlement, 1, NpcActor::Kind::Elder, info.style, Dialogue::RefugeeElder);
		break;
	}
	case Regions::LightHamlet:
	{
		PlaceAtSpot(graph, settlement, 0, NpcActor::Kind::Adult, info.style, Dialogue::HamletMother);
		break;
	}
	case Regions::DarkHamlet:
	{
		PlaceAtSpot(graph, settlement, 0, NpcActor::Kind::Adult, info.style, Dialogue::HamletSmith);
		break;
	}
	case Regions::RuinedAltar:
	{
		if (settlement.hasTemple)
		{
			PlaceNear(graph, NpcActor::Kind::Note, info.style, Dialogue::RuinedAltarWords,
				settlement.altarCol + kAltarNoteCol, settlement.altarRow + kAltarNoteRow);
		}
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

void Npcs::SpawnErrands(int region, const Progress& progress, const Settlement::Layout& settlement, SceneGraph& graph)
{
	const Regions::Info& info = Regions::Get(region);

	// A lantern by Crypta's tombs, just past the village's edge.
	if (region == Regions::Crypta && Quests::StageOf(progress, Quests::LostLantern) == Quests::Active)
	{
		NpcActor* lantern = PlaceNear(graph, NpcActor::Kind::Lantern, info.style, Dialogue::LostLantern,
			settlement.squareCol + kLanternCol, settlement.squareRow + kLanternRow);
		if (lantern != nullptr)
		{
			lantern->SetZone(region);
		}
	}

	// A peddler among the wreckage, and the gray in a ring round him.
	if (region == Regions::Wreckage && Quests::StageOf(progress, Quests::StrandedMerchant) == Quests::Active)
	{
		float col, row;
		LandmarkActor::Spot(region, col, row);
		col += kMerchantOffset;
		row += kMerchantOffset;
		if (!FindOpen(col, row))
		{
			return;
		}
		NpcActor* merchant = Place(graph, NpcActor::Kind::Merchant, Regions::Style::Mixed, Dialogue::MerchantBesieged, col, row);
		if (merchant == nullptr)
		{
			return;
		}
		merchant->SetZone(region);

		const float kTau = 6.2831853f;
		for (int i = 0; i < Tuning::Quest::kBesiegers; i++)
		{
			float angle = kTau * i / Tuning::Quest::kBesiegers;
			float ringCol = col + cosf(angle) * Tuning::Quest::kBesiegeRing;
			float ringRow = row + sinf(angle) * Tuning::Quest::kBesiegeRing;
			if (!FindOpen(ringCol, ringRow))
			{
				continue;
			}
			Alignment alignment = (i % 2 == 0) ? Alignment::Light : Alignment::Dark;
			MonsterActor::Kind kind = (Random::Unit() < info.mageShare) ? MonsterActor::Kind::Mage : MonsterActor::Kind::Knight;
			std::unique_ptr<MonsterActor> monster(new MonsterActor(kind, alignment));
			float x, y;
			WorldGrid::IsoPosition(ringCol, ringRow, x, y);
			monster->SetLocalPosition(x, y);
			monster->SetZone(region);
			graph.Spawn(std::move(monster));
		}
	}
}
