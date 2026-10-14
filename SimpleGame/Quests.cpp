#include "stdafx.h"
#include "Quests.h"
#include "Progress.h"
#include "PlayerStatus.h"
#include "Regions.h"
#include "Dialogue.h"
#include "Tuning.h"

namespace
{
	using Quests::Info;
	using Quests::Kind;
	using Quests::Quarry;

	// In the order of Quests::Id.
	const Info kQuests[Quests::Count] =
	{
		{ "WoodsKnights", "숲길의 기사",
			"리미나 숲과 크레푸스 숲길에 잿빛 기사가 돌아다닌다. 다섯만 쓰러뜨려 다오.",
			"리미나 숲·크레푸스에서 기사 처치", nullptr,
			Regions::Vespera, Kind::Hunt, Quarry::Knight, { Regions::LiminaWoods, Regions::Crepus },
			Tuning::Quest::kWoodsKnights, Tuning::Quest::kWoodsReward, Tuning::Quest::kWoodsReward, -1, -1 },
		{ "BrokenSignpost", "부서진 이정표",
			"크레푸스 언덕 어딘가에 옛 이정표가 쓰러져 있다. 벨룸으로 가는 길이 적혀 있었다던데.",
			"크레푸스의 이정표 찾기", "벨룸 평원·트레우가 빠른 이동 개방",
			Regions::Vespera, Kind::Find, Quarry::Any, { -1, -1 },
			0, 0.f, 0.f, -1, -1 },
		{ "LightSoldiers", "빛 쪽 침식병",
			"빛의 갑옷을 입은 잿빛 병사들이 마을 서쪽을 넘본다.",
			"벨룸 평원·전쟁터 잔해에서 빛 소속 처치", nullptr,
			Regions::Treuga, Kind::Hunt, Quarry::Light, { Regions::Bellum, Regions::Wreckage },
			Tuning::Quest::kSoldiers, 0.f, Tuning::Quest::kSoldiersReward, -1, -1 },
		{ "DarkSoldiers", "어둠 쪽 침식병",
			"어둠의 갑옷을 입은 잿빛 병사들이 마을 동쪽을 넘본다.",
			"벨룸 평원·전쟁터 잔해에서 어둠 소속 처치", nullptr,
			Regions::Treuga, Kind::Hunt, Quarry::Dark, { Regions::Bellum, Regions::Wreckage },
			Tuning::Quest::kSoldiers, Tuning::Quest::kSoldiersReward, 0.f, -1, -1 },
		{ "StrandedMerchant", "갇힌 행상",
			"전쟁터 잔해에서 행상 하나가 잿빛 무리에 둘러싸였다.",
			"전쟁터 잔해의 행상 구하기", nullptr,
			Regions::Treuga, Kind::Rescue, Quarry::Any, { -1, -1 },
			0, Tuning::Quest::kMerchantReward, Tuning::Quest::kMerchantReward, Regions::Wreckage, -1 },
		{ "PilgrimLetter", "순례자의 편지",
			"알바에 사는 어머니께 이 편지를 전해 주시오.",
			"알바의 어머니에게 편지 전하기", "루키우스에 대한 소문",
			Regions::Candida, Kind::Deliver, Quarry::Any, { -1, -1 },
			0, 0.f, 0.f, -1, Dialogue::LetterSender },
		{ "GateMages", "관문 길목의 마법사",
			"빛의 관문으로 가는 솔라 길목에 잿빛 마법사가 진을 쳤소.",
			"솔라에서 마법사 처치", nullptr,
			Regions::Candida, Kind::Hunt, Quarry::Mage, { Regions::Sola, -1 },
			Tuning::Quest::kGateMages, Tuning::Quest::kGateMagesReward, 0.f, -1, -1 },
		{ "GraveKnights", "묘지의 기사",
			"녹스 묘지에 기사 넷이 서성인다. 무덤 파는 놈들은 아니지만 거슬려.",
			"녹스에서 기사 처치", nullptr,
			Regions::Obscura, Kind::Hunt, Quarry::Knight, { Regions::Nox, -1 },
			Tuning::Quest::kGraveKnights, 0.f, Tuning::Quest::kGraveKnightsReward, -1, -1 },
		{ "LostLantern", "잃어버린 등불",
			"크립타 묘실 근처에 등불 하나를 두고 왔어. 가져다주면 녹티스 얘기 하나 해 주지.",
			"크립타의 등불 찾기", "녹티스에 대한 소문",
			Regions::Obscura, Kind::Find, Quarry::Any, { -1, -1 },
			0, 0.f, 0.f, Regions::Crypta, Dialogue::LanternOwner },
		{ "MarchHorde", "변경의 잿빛 무리",
			"피난민들이 물 길으러 못 나간다. 변경의 잿빛 무리를 몰아내 주시오.",
			"잿빛 변경에서 처치", nullptr,
			Regions::Refugium, Kind::Hunt, Quarry::Any, { Regions::AshMarch, -1 },
			Tuning::Quest::kMarchHorde, Tuning::Quest::kMarchHordeReward, Tuning::Quest::kMarchHordeReward, -1, -1 },
	};

	// The gray fallen since the last tally.
	struct Kill
	{
		int zone;
		bool knight;
		bool light;
	};
	const int kMaxKills = 64;
	Kill s_Kills[kMaxKills];
	int s_KillCount = 0;

	// A request fulfilled since the last tally, to be said aloud.
	int s_Fulfilled = -1;

	bool Counts(const Info& info, const Kill& kill)
	{
		if (kill.zone < 0 || (kill.zone != info.zones[0] && kill.zone != info.zones[1]))
		{
			return false;
		}
		switch (info.quarry)
		{
		case Quarry::Knight:
		{
			return kill.knight;
		}
		case Quarry::Mage:
		{
			return !kill.knight;
		}
		case Quarry::Light:
		{
			return kill.light;
		}
		case Quarry::Dark:
		{
			return !kill.light;
		}
		default:
		{
			return true;
		}
		}
	}

	std::string ReadyLine(int id)
	{
		return std::string("의뢰 달성: ") + kQuests[id].title + "  ·  게시판에 보고하라";
	}

	// How far along the main road he is, from where he has been and what has
	// happened: 0 the raid, then the border's village, the battlefield's,
	// the capitals, the villages by the gray, its heart, and nothing past it.
	int MainStep(const Progress& progress)
	{
		if (!progress.Has(Story::VillageRaided))
		{
			return 0;
		}
		if (progress.Has(Story::KnowsWorldTruth) || progress.Has(Story::AcceptAshOffer))
		{
			return 6;
		}
		if (progress.Has(Story::RescuedLight) && progress.Has(Story::RescuedDark))
		{
			return 5;
		}
		bool nearGray = progress.visited[Regions::Fides] || progress.visited[Regions::Spes]
			|| progress.visited[Regions::Requiem] || progress.visited[Regions::Luctus];
		if (progress.recognizedByLucius || progress.recognizedByNoctis || nearGray)
		{
			return 4;
		}
		if (progress.visited[Regions::Treuga] || progress.Has(Story::KnowsCollusion))
		{
			return 3;
		}
		if (progress.visited[Regions::Vespera] || progress.visited[Regions::Bellum])
		{
			return 2;
		}
		return 1;
	}

	const char* const kMainGoals[] =
	{
		"마을을 지켜라",
		"크레푸스의 베스페라로 가라",
		"벨룸 평원의 트레우가에 들러라",
		"솔라 또는 녹스의 수도로 가라",
		"잿빛 지대 근처의 마을을 찾아라",
		"키네라로 가라",
	};

	const char* const kMainDetails[] =
	{
		"잿빛 무리가 리미나에 들이닥쳤다.",
		"숲을 나서면 크레푸스. 그 서쪽에 경계의 쉼터가 있다고 했다.",
		"옛 전쟁터 한가운데, 빛과 어둠이 휴전한 마을. 모든 길이 거기서 갈라진다.",
		"왕을 꺾고 인정받은 자는 축복을 받는다는 소문이 있다. 서쪽은 빛의 수도 루멘, 동쪽은 어둠의 수도 테네브라에.",
		"잿빛 변경 너머에도 아직 사람이 사는 마을이 있다고 한다.",
		"잿빛의 한가운데. 모든 것이 시작된 곳.",
	};
}

const Quests::Info& Quests::Get(int id)
{
	return kQuests[id];
}

std::string Quests::Reward(int id)
{
	const Info& info = kQuests[id];
	std::string reward;
	if (info.light > 0.f)
	{
		reward += "빛 +" + std::to_string((int)info.light);
	}
	if (info.dark > 0.f)
	{
		if (!reward.empty())
		{
			reward += " · ";
		}
		reward += "어둠 +" + std::to_string((int)info.dark);
	}
	if (info.boon != nullptr)
	{
		if (!reward.empty())
		{
			reward += " · ";
		}
		reward += info.boon;
	}
	return reward;
}

bool Quests::HasBoard(int region)
{
	for (int id = 0; id < Count; id++)
	{
		if (kQuests[id].board == region)
		{
			return true;
		}
	}
	return false;
}

int Quests::OnBoard(int region, const Progress& progress, int* ids, int max)
{
	int count = 0;
	for (int id = 0; id < Count && count < max; id++)
	{
		if (kQuests[id].board != region)
		{
			continue;
		}
		// Word of a king is no use to one he has already acknowledged.
		bool unseen = (StageOf(progress, id) == Unseen);
		if (unseen && id == PilgrimLetter && progress.recognizedByLucius)
		{
			continue;
		}
		if (unseen && id == LostLantern && progress.recognizedByNoctis)
		{
			continue;
		}
		ids[count++] = id;
	}
	return count;
}

Quests::Stage Quests::StageOf(const Progress& progress, int id)
{
	return (Stage)progress.questStage[id];
}

void Quests::Accept(int id, Progress& progress)
{
	if (StageOf(progress, id) == Unseen)
	{
		progress.questStage[id] = Active;
		progress.questCount[id] = 0;
	}
}

std::string Quests::Standing(int id, const Progress& progress)
{
	const Info& info = kQuests[id];
	switch (StageOf(progress, id))
	{
	case Active:
	{
		if (info.kind == Kind::Hunt)
		{
			return std::to_string(progress.questCount[id]) + "/" + std::to_string(info.target);
		}
		return "진행 중";
	}
	case Ready:
	{
		return "보고하라";
	}
	case Done:
	{
		return "완료";
	}
	default:
	{
		return "";
	}
	}
}

void Quests::NoteKill(int zone, bool knight, bool light)
{
	if (s_KillCount < kMaxKills)
	{
		s_Kills[s_KillCount++] = { zone, knight, light };
	}
}

void Quests::Fulfil(int id, Progress& progress)
{
	if (StageOf(progress, id) == Active)
	{
		progress.questStage[id] = Ready;
		s_Fulfilled = id;
	}
}

std::string Quests::Tally(Progress& progress)
{
	std::string line;
	if (s_Fulfilled >= 0)
	{
		line = ReadyLine(s_Fulfilled);
		s_Fulfilled = -1;
	}

	for (int i = 0; i < s_KillCount; i++)
	{
		for (int id = 0; id < Count; id++)
		{
			const Info& info = kQuests[id];
			if (info.kind != Kind::Hunt || StageOf(progress, id) != Active || !Counts(info, s_Kills[i]))
			{
				continue;
			}
			progress.questCount[id]++;
			if (progress.questCount[id] >= info.target)
			{
				progress.questCount[id] = info.target;
				progress.questStage[id] = Ready;
				line = ReadyLine(id);
			}
			else
			{
				line = std::string(info.title) + "  " + Standing(id, progress);
			}
		}
	}
	s_KillCount = 0;
	return line;
}

std::string Quests::Claim(int id, PlayerStatus& status, Progress& progress, int& script)
{
	script = -1;
	if (StageOf(progress, id) != Ready)
	{
		return "";
	}
	const Info& info = kQuests[id];
	if (info.light > 0.f)
	{
		status.AddLight(info.light);
	}
	if (info.dark > 0.f)
	{
		status.AddDark(info.dark);
	}
	// The way to the battlefield, known: both its places open to travel.
	if (id == BrokenSignpost)
	{
		progress.visited[Regions::Bellum] = true;
		progress.visited[Regions::Treuga] = true;
	}
	progress.questStage[id] = Done;
	script = info.report;
	return std::string("의뢰 완료: ") + info.title + "  ·  " + Reward(id);
}

const char* Quests::MainGoal(const Progress& progress)
{
	int step = MainStep(progress);
	return (step < 6) ? kMainGoals[step] : nullptr;
}

const char* Quests::MainDetail(const Progress& progress)
{
	int step = MainStep(progress);
	return (step < 6) ? kMainDetails[step] : nullptr;
}

int Quests::MainTargets(const Progress& progress, int* regions)
{
	switch (MainStep(progress))
	{
	case 0:
	{
		regions[0] = Regions::Limina;
		return 1;
	}
	case 1:
	{
		regions[0] = Regions::Vespera;
		return 1;
	}
	case 2:
	{
		regions[0] = Regions::Treuga;
		return 1;
	}
	case 3:
	{
		regions[0] = Regions::Lumen;
		regions[1] = Regions::Tenebrae;
		return 2;
	}
	case 4:
	{
		// The villages where someone still waits to be saved.
		int count = 0;
		if (!progress.Has(Story::RescuedLight))
		{
			regions[count++] = Regions::Fides;
		}
		if (!progress.Has(Story::RescuedDark))
		{
			regions[count++] = Regions::Requiem;
		}
		return count;
	}
	case 5:
	{
		regions[0] = Regions::Kinera;
		return 1;
	}
	default:
	{
		return 0;
	}
	}
}
