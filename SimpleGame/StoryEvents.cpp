#include "stdafx.h"
#include "StoryEvents.h"
#include "NpcActor.h"
#include "KinisActor.h"
#include "GodActor.h"
#include "MonsterActor.h"
#include "SparkActor.h"
#include "Dialogue.h"
#include "Cutscenes.h"
#include "KingPieceActor.h"
#include "QueenPieceActor.h"
#include "Progress.h"
#include "PlayerStatus.h"
#include "Regions.h"
#include "SceneGraph.h"
#include "WorldGrid.h"
#include "Tuning.h"
#include "Bosses.h"
#include <cmath>

namespace
{
	// Where Kinis waits on the battlefield, from its waystone.
	const float kKinisCol = 10.f;
	const float kKinisRow = -6.f;

	// Where the one to be saved is cornered, from the square's centre.
	const float kVictimCol = -3.f;
	const float kVictimRow = 1.f;

	// Where the two he saved meet again at the front, from its waystone.
	const float kReunionCol = 5.f;
	const float kReunionRow = -2.f;
	const float kReunionGap = 1.6f;


	const int kDustSparks = 16;
	const float kDustSpeed = 110.f;
	const float kDustLift = 40.f;
}

bool StoryEvents::IsNearGray() const
{
	return m_Region == Regions::Fides || m_Region == Regions::Spes
		|| m_Region == Regions::Requiem || m_Region == Regions::Luctus;
}

bool StoryEvents::IsLightSide() const
{
	return m_Region == Regions::Fides;
}

float StoryEvents::DistanceTo(const Actor& actor, float playerX, float playerY) const
{
	float col, row, playerCol, playerRow;
	WorldGrid::InverseIso(actor.WorldX(), actor.WorldY(), col, row);
	WorldGrid::InverseIso(playerX, playerY, playerCol, playerRow);
	float dc = playerCol - col;
	float dr = playerRow - row;
	return sqrtf(dc * dc + dr * dr);
}

int StoryEvents::RescueLeft() const
{
	return (m_Stage == Stage::Rescue) ? m_Left : -1;
}

int StoryEvents::TakeCutscene()
{
	int cutscene = m_Cutscene;
	m_Cutscene = -1;
	return cutscene;
}

void StoryEvents::RaiseCheckmate(SceneGraph& graph, const Progress& progress, float playerX, float playerY)
{
	// The pair of each nation left standing, close on either side of him and
	// before and behind.
	float col, row;
	WorldGrid::InverseIso(playerX, playerY, col, row);
	const float kClose = 1.8f;
	for (int nation = 0; nation < 2; nation++)
	{
		bool light = (nation == 0);
		if (progress.Has(light ? Story::LightPairBroken : Story::DarkPairBroken))
		{
			continue;
		}
		int region = light ? Regions::LumenCastle : Regions::TenebraeCastle;
		float side = light ? -1.f : 1.f;
		std::unique_ptr<KingPieceActor> king(new KingPieceActor(region, light, true));
		float x, y;
		WorldGrid::IsoPosition(col + side * kClose, row, x, y);
		king->SetLocalPosition(x, y);
		king->SetZone(kZone);
		graph.Spawn(std::move(king));

		std::unique_ptr<QueenPieceActor> queen(new QueenPieceActor(region, light, true));
		WorldGrid::IsoPosition(col, row + side * kClose, x, y);
		queen->SetLocalPosition(x, y);
		queen->SetZone(kZone);
		graph.Spawn(std::move(queen));
	}
}

void StoryEvents::EndInCheckmate()
{
	if (m_Stage == Stage::Checkmate)
	{
		m_Ending = Dialogue::Checkmate;
		m_Stage = Stage::Done;
	}
}

int StoryEvents::TakeTravel()
{
	int travel = m_Travel;
	m_Travel = -1;
	return travel;
}

int StoryEvents::TakeEnding()
{
	int ending = m_Ending;
	m_Ending = -1;
	return ending;
}

bool StoryEvents::HoldsSave() const
{
	return m_Stage == Stage::KinisTalked || m_Stage == Stage::KinisFight || m_Stage == Stage::KinisLastWords
		|| m_Stage == Stage::GodTalked || m_Stage == Stage::GodFight || m_Stage == Stage::GodFalling
		|| m_Stage == Stage::Checkmate || m_Stage == Stage::Done;
}

bool StoryEvents::FreeOfGod(const Progress& progress) const
{
	return progress.HasKingRecognition() && progress.Has(Story::EscapedGodInfluence);
}

bool StoryEvents::TrueEndReached(const Progress& progress) const
{
	return FreeOfGod(progress) && progress.HasBrokenBothPairs();
}

NpcActor* StoryEvents::SpawnPerson(SceneGraph& graph, NpcActor::Kind kind, Regions::Style style, int conversation, float col, float row)
{
	std::unique_ptr<NpcActor> person(new NpcActor(kind, style, conversation));
	float x, y;
	WorldGrid::IsoPosition(col, row, x, y);
	person->SetLocalPosition(x, y);
	person->SetZone(kZone);
	NpcActor* raw = person.get();
	graph.Spawn(std::move(person));
	return raw;
}

void StoryEvents::Dissolve(SceneGraph& graph, Actor& actor) const
{
	SparkActor::Burst(graph, actor.WorldX(), actor.WorldY(), SparkActor::Style::Dust, kDustSparks, kDustSpeed, kDustLift);
	actor.Destroy();
}

void StoryEvents::Leave(SceneGraph& graph)
{
	graph.ForEach<Actor>([&](Actor& actor)
	{
		if (actor.Zone() == kZone)
		{
			actor.Destroy();
		}
	});
	m_Stage = Stage::None;
	m_Figure = nullptr;
	m_Second = nullptr;
	m_Kinis = nullptr;
	m_God = nullptr;
	m_Left = 0;
}

void StoryEvents::Enter(int region, const Progress& progress, const Settlement::Layout& settlement, SceneGraph& graph,
	float centerCol, float centerRow)
{
	m_Region = region;
	m_Stage = Stage::None;
	m_Remembered = false;
	m_Left = 0;
	m_Figure = nullptr;
	m_Second = nullptr;
	m_Kinis = nullptr;
	m_God = nullptr;
	m_Ending = -1;
	m_Travel = -1;

	if (region == Regions::Realm)
	{
		std::unique_ptr<GodActor> god(new GodActor(region));
		float x, y;
		// Beyond the far corner of its board.
		WorldGrid::IsoPosition(centerCol + Tuning::God::kStandOffset, centerRow + Tuning::God::kStandOffset, x, y);
		god->SetLocalPosition(x, y);
		god->SetZone(kZone);
		m_God = god.get();
		graph.Spawn(std::move(god));
		m_Stage = Stage::GodWaiting;
		return;
	}

	if (region == Regions::Divisio && progress.Has(Story::EscapedGodInfluence) && !progress.Has(Story::SawRenewedWar))
	{
		float col = centerCol + kReunionCol;
		float row = centerRow + kReunionRow;
		m_Figure = SpawnPerson(graph, NpcActor::Kind::Adult, Regions::Style::Light, -1, col, row);
		m_Second = SpawnPerson(graph, NpcActor::Kind::Adult, Regions::Style::Dark, -1,
			col + kReunionGap, row - kReunionGap);
		m_Stage = Stage::Reunion;
		return;
	}

	if (region == Regions::Kinera)
	{
		std::unique_ptr<KinisActor> kinis(new KinisActor(region));
		float arenaCol, arenaRow;
		Bosses::Arena(region, arenaCol, arenaRow);
		float x, y;
		WorldGrid::IsoPosition(arenaCol, arenaRow, x, y);
		kinis->SetLocalPosition(x, y);
		kinis->SetZone(kZone);
		m_Kinis = kinis.get();
		graph.Spawn(std::move(kinis));
		m_Stage = Stage::KinisWaiting;
		return;
	}

	if (region == Regions::Bellum && !progress.Has(Story::KnowsCollusion))
	{
		float col = centerCol + kKinisCol;
		float row = centerRow + kKinisRow;
		std::unique_ptr<NpcActor> kinis(new NpcActor(NpcActor::Kind::Kinis, Regions::Style::Ashen, -1));
		float x, y;
		WorldGrid::IsoPosition(col, row, x, y);
		kinis->SetLocalPosition(x, y);
		kinis->SetZone(kZone);
		m_Figure = kinis.get();
		graph.Spawn(std::move(kinis));
		m_Stage = Stage::Kinis;
		return;
	}

	if (region == Regions::Fides || region == Regions::Requiem)
	{
		bool light = IsLightSide();
		bool saved = progress.Has(light ? Story::RescuedLight : Story::RescuedDark);

		// Dressed as the side they are from, in a village the gray has taken.
		float col = settlement.squareCol + kVictimCol;
		float row = settlement.squareRow + kVictimRow;
		int conversation = saved ? (light ? Dialogue::SurvivorLight : Dialogue::SurvivorDark) : -1;
		Regions::Style style = light ? Regions::Style::Light : Regions::Style::Dark;
		std::unique_ptr<NpcActor> victim(new NpcActor(NpcActor::Kind::Adult, style, conversation));
		float x, y;
		WorldGrid::IsoPosition(col, row, x, y);
		victim->SetLocalPosition(x, y);
		victim->SetZone(kZone);
		m_Figure = victim.get();
		graph.Spawn(std::move(victim));

		if (!saved)
		{
			SpawnRaiders(graph, col, row);
			m_Stage = Stage::Rescue;
		}
	}
}

void StoryEvents::SpawnRaiders(SceneGraph& graph, float col, float row) const
{
	int count = Tuning::Story::kRescueRaiders;
	for (int i = 0; i < count; i++)
	{
		float angle = (i + 0.5f) * 2.f * 3.14159265f / count;
		float raiderCol = col + cosf(angle) * Tuning::Story::kRescueRing;
		float raiderRow = row + sinf(angle) * Tuning::Story::kRescueRing;
		WorldGrid::ClampToMap(raiderCol, raiderRow);
		if (WorldGrid::IsBlocked(raiderCol, raiderRow))
		{
			raiderCol = col;
			raiderRow = row;
		}

		Alignment alignment = (i % 2 == 0) ? Alignment::Light : Alignment::Dark;
		MonsterActor::Kind kind = (i == count - 1) ? MonsterActor::Kind::Mage : MonsterActor::Kind::Knight;
		std::unique_ptr<MonsterActor> raider(new MonsterActor(kind, alignment));
		float x, y;
		WorldGrid::IsoPosition(raiderCol, raiderRow, x, y);
		raider->SetLocalPosition(x, y);
		raider->SetZone(kZone);
		graph.Spawn(std::move(raider));
	}
}

int StoryEvents::Update(SceneGraph& graph, Progress& progress, const PlayerStatus& status, float playerX, float playerY)
{
	switch (m_Stage)
	{
	case Stage::Kinis:
	{
		if (DistanceTo(*m_Figure, playerX, playerY) <= Tuning::Story::kKinisReach)
		{
			m_Stage = Stage::KinisLeaving;
			return Dialogue::KinisFirst;
		}
		return -1;
	}
	case Stage::KinisLeaving:
	{
		Dissolve(graph, *m_Figure);
		m_Figure = nullptr;
		m_Stage = Stage::None;
		return -1;
	}
	case Stage::Reunion:
	{
		if (DistanceTo(*m_Figure, playerX, playerY) <= Tuning::Story::kReunionReach)
		{
			m_Stage = Stage::ReunionOver;
			return Dialogue::RenewedWar;
		}
		return -1;
	}
	case Stage::ReunionOver:
	{
		Dissolve(graph, *m_Figure);
		Dissolve(graph, *m_Second);
		m_Figure = nullptr;
		m_Second = nullptr;
		m_Stage = Stage::None;
		return -1;
	}
	case Stage::KinisWaiting:
	{
		if (DistanceTo(*m_Kinis, playerX, playerY) <= Tuning::Story::kKinisTalk)
		{
			m_Stage = Stage::KinisTalked;
			return FreeOfGod(progress) ? Dialogue::KinisSilent : Dialogue::KinisOffer;
		}
		return -1;
	}
	case Stage::KinisTalked:
	{
		// He took the offer: no fight, and that is the end.
		if (progress.Has(Story::AcceptAshOffer))
		{
			m_Ending = Dialogue::Ash;
			m_Stage = Stage::Done;
			return -1;
		}
		m_Kinis->Provoke();
		m_Stage = Stage::KinisFight;
		return -1;
	}
	case Stage::KinisFight:
	{
		if (m_Kinis->HasFallen())
		{
			m_Stage = Stage::KinisLastWords;
			if (TrueEndReached(progress))
			{
				return Dialogue::KinisLastClear;
			}
			return FreeOfGod(progress) ? Dialogue::KinisLastUnchosen : Dialogue::KinisLastBroken;
		}
		return -1;
	}
	case Stage::KinisLastWords:
	{
		Dissolve(graph, *m_Kinis);
		m_Kinis = nullptr;
		m_Stage = Stage::Done;
		if (!FreeOfGod(progress))
		{
			m_Ending = Dialogue::Savior;
		}
		else if (!TrueEndReached(progress))
		{
			// A nation's pair left standing: they close in.
			m_Stage = Stage::Checkmate;
			m_Cutscene = Cutscenes::CheckmateEnd;
		}
		else if (progress.Has(Story::AcceptAsh))
		{
			// The gray taken up: on to the one who set the board.
			m_Travel = Regions::Realm;
		}
		else
		{
			m_Ending = Dialogue::Scheme;
		}
		return -1;
	}
	case Stage::GodWaiting:
	{
		m_Stage = Stage::GodTalked;
		return progress.Has(Story::MetTheGod) ? Dialogue::GodReturn : Dialogue::GodMeeting;
	}
	case Stage::GodTalked:
	{
		m_God->Provoke();
		m_Stage = Stage::GodFight;
		return -1;
	}
	case Stage::GodFight:
	{
		if (m_God->HasFallen())
		{
			m_Stage = Stage::GodFalling;
			return Dialogue::GodFall;
		}
		return -1;
	}
	case Stage::GodFalling:
	{
		Dissolve(graph, *m_God);
		m_God = nullptr;
		m_Stage = Stage::Done;
		m_Ending = Dialogue::True;
		return -1;
	}
	case Stage::Done:
	{
		return -1;
	}
	case Stage::Rescue:
	{
		// Over when no gray is left anywhere near them - and they speak once
		// he comes to them.
		float area = Tuning::Story::kRescueArea;
		float col, row;
		WorldGrid::InverseIso(m_Figure->WorldX(), m_Figure->WorldY(), col, row);
		m_Left = 0;
		graph.ForEach<MonsterActor>([&](MonsterActor& monster)
		{
			float monsterCol, monsterRow;
			WorldGrid::InverseIso(monster.WorldX(), monster.WorldY(), monsterCol, monsterRow);
			float dc = monsterCol - col;
			float dr = monsterRow - row;
			if (!monster.IsDestroyed() && dc * dc + dr * dr <= area * area)
			{
				m_Left++;
			}
		});

		if (m_Left == 0 && DistanceTo(*m_Figure, playerX, playerY) <= Tuning::Story::kRescueTalk)
		{
			m_Stage = Stage::Rescued;
			return IsLightSide() ? Dialogue::RescueLight : Dialogue::RescueDark;
		}
		return -1;
	}
	case Stage::Rescued:
	{
		m_Figure->SetConversation(IsLightSide() ? Dialogue::SurvivorLight : Dialogue::SurvivorDark);
		m_Stage = Stage::None;

		// Both prayers heard: what he makes of them.
		if (progress.Has(Story::RescuedLight) && progress.Has(Story::RescuedDark) && !progress.Has(Story::KnowsOneGod))
		{
			bool sawKings = progress.Has(Story::SawKingAnomaly);
			bool strong = status.lightTotal + status.darkTotal >= Tuning::Story::kChosenPower;
			if (sawKings && strong)
			{
				return Dialogue::EavesdropChosen;
			}
			return sawKings ? Dialogue::EavesdropClouded : Dialogue::EavesdropUnaware;
		}
		return -1;
	}
	default:
	{
		break;
	}
	}

	// Heard the prayers without seeing what they meant, and since become
	// sure of the kings and strong enough: near the gray's heart, it comes
	// back to him.
	if (!m_Remembered && IsNearGray() && progress.Has(Story::KnowsOneGod) && progress.Has(Story::SawKingAnomaly)
		&& !progress.Has(Story::EscapedGodInfluence)
		&& status.lightTotal + status.darkTotal >= Tuning::Story::kChosenPower)
	{
		m_Remembered = true;
		return Dialogue::ChosenLater;
	}
	return -1;
}
