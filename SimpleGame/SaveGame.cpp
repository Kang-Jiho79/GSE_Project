#include "stdafx.h"
#include "SaveGame.h"
#include "PlayerStatus.h"
#include "Progress.h"
#include "Regions.h"
#include "Story.h"
#include "Tuning.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
	const char* const kPath = "SaveData.txt";
	const int kVersion = 1;

	// Story flags are kept by name, so adding one later does not shift the
	// rest. In the order of Story::Flag.
	const char* const kStoryNames[] =
	{
		"VillageRaided",
		"KnowsCollusion",
		"AskedKingAboutCollusion",
		"SawKingAnomaly",
		"KnowsOneGod",
		"TitleChosenByGod",
		"EscapedGodInfluence",
		"KnowsWorldTruth",
		"AcceptAshOffer",
		"AcceptAsh",
		"AskedLucius",
		"AskedNoctis",
		"RescuedLight",
		"RescuedDark",
		"SawRenewedWar",
	};
	static_assert(sizeof(kStoryNames) / sizeof(kStoryNames[0]) == Story::Count, "a story flag has no name to save it by");

	void Put(std::ofstream& out, const char* key, const std::string& value)
	{
		out << key << '=' << value << '\n';
	}

	void Put(std::ofstream& out, const char* key, float value)
	{
		char text[32];
		snprintf(text, sizeof(text), "%.3f", value);
		Put(out, key, std::string(text));
	}

	void Put(std::ofstream& out, const char* key, int value)
	{
		Put(out, key, std::to_string(value));
	}

	void Put(std::ofstream& out, const char* key, bool value)
	{
		Put(out, key, std::string(value ? "1" : "0"));
	}

	// One character per region, 1 or 0.
	std::string Flags(const bool* flags, int count)
	{
		std::string text(count, '0');
		for (int i = 0; i < count; i++)
		{
			if (flags[i])
			{
				text[i] = '1';
			}
		}
		return text;
	}

	void ReadFlags(const std::string& text, bool* flags, int count)
	{
		for (int i = 0; i < count && i < (int)text.size(); i++)
		{
			flags[i] = (text[i] == '1');
		}
	}

	bool IsTrue(const std::string& value)
	{
		return value == "1";
	}

	float ToFloat(const std::string& value)
	{
		return (float)atof(value.c_str());
	}

	int ToInt(const std::string& value)
	{
		return atoi(value.c_str());
	}
}

bool SaveGame::Exists()
{
	std::ifstream in(kPath, std::ios::binary);
	return in.good();
}

bool SaveGame::Write(const PlayerStatus& status, const Progress& progress, int region)
{
	std::ofstream out(kPath, std::ios::binary | std::ios::trunc);
	if (!out)
	{
		std::cout << "Could not write " << kPath << "\n";
		return false;
	}

	Put(out, "version", kVersion);
	Put(out, "name", progress.heroName);
	Put(out, "region", region);

	Put(out, "light", status.light);
	Put(out, "dark", status.dark);
	Put(out, "capacity", status.capacity);
	Put(out, "lightTotal", status.lightTotal);
	Put(out, "darkTotal", status.darkTotal);
	Put(out, "health", status.health);
	Put(out, "maxHealth", status.maxHealth);
	Put(out, "upgrades", status.upgrades);
	Put(out, "form", std::string(status.form == Form::Light ? "light" : "dark"));
	Put(out, "lightBlessing", status.lightBlessing);
	Put(out, "darkBlessing", status.darkBlessing);

	Put(out, "visited", Flags(progress.visited, Regions::Count));
	Put(out, "bossDefeated", Flags(progress.bossDefeated, Regions::Count));
	Put(out, "respawnRegion", progress.respawnRegion);
	Put(out, "respawnAtAltar", progress.respawnAtAltar);
	Put(out, "recognizedByLucius", progress.recognizedByLucius);
	Put(out, "recognizedByNoctis", progress.recognizedByNoctis);

	std::string story;
	for (int i = 0; i < Story::Count; i++)
	{
		if (progress.story[i])
		{
			if (!story.empty())
			{
				story += ',';
			}
			story += kStoryNames[i];
		}
	}
	Put(out, "story", story);

	return out.good();
}

bool SaveGame::Read(PlayerStatus& status, Progress& progress, int& region)
{
	std::ifstream in(kPath, std::ios::binary);
	if (!in)
	{
		return false;
	}

	status = PlayerStatus();
	progress = Progress();
	region = Regions::Limina;
	bool ours = false;

	std::string line;
	while (std::getline(in, line))
	{
		// Mended by hand on Windows, a line may end in a carriage return.
		if (!line.empty() && line[line.size() - 1] == '\r')
		{
			line.erase(line.size() - 1);
		}

		size_t split = line.find('=');
		if (split == std::string::npos)
		{
			continue;
		}
		std::string key = line.substr(0, split);
		std::string value = line.substr(split + 1);

		if (key == "version")
		{
			ours = (ToInt(value) >= 1);
		}
		else if (key == "name")
		{
			progress.heroName = value;
		}
		else if (key == "region")
		{
			region = ToInt(value);
		}
		else if (key == "light")
		{
			status.light = ToFloat(value);
		}
		else if (key == "dark")
		{
			status.dark = ToFloat(value);
		}
		else if (key == "capacity")
		{
			status.capacity = ToFloat(value);
		}
		else if (key == "lightTotal")
		{
			status.lightTotal = ToFloat(value);
		}
		else if (key == "darkTotal")
		{
			status.darkTotal = ToFloat(value);
		}
		else if (key == "health")
		{
			status.health = ToFloat(value);
		}
		else if (key == "maxHealth")
		{
			status.maxHealth = ToFloat(value);
		}
		else if (key == "upgrades")
		{
			status.upgrades = ToInt(value);
		}
		else if (key == "form")
		{
			status.form = (value == "dark") ? Form::Dark : Form::Light;
		}
		else if (key == "lightBlessing")
		{
			status.lightBlessing = IsTrue(value);
		}
		else if (key == "darkBlessing")
		{
			status.darkBlessing = IsTrue(value);
		}
		else if (key == "visited")
		{
			ReadFlags(value, progress.visited, Regions::Count);
		}
		else if (key == "bossDefeated")
		{
			ReadFlags(value, progress.bossDefeated, Regions::Count);
		}
		else if (key == "respawnRegion")
		{
			progress.respawnRegion = ToInt(value);
		}
		else if (key == "respawnAtAltar")
		{
			progress.respawnAtAltar = IsTrue(value);
		}
		else if (key == "recognizedByLucius")
		{
			progress.recognizedByLucius = IsTrue(value);
		}
		else if (key == "recognizedByNoctis")
		{
			progress.recognizedByNoctis = IsTrue(value);
		}
		else if (key == "story")
		{
			for (int i = 0; i < Story::Count; i++)
			{
				// Whole names only: one name inside another must not count.
				std::string padded = "," + value + ",";
				std::string name = std::string(",") + kStoryNames[i] + ",";
				progress.story[i] = (padded.find(name) != std::string::npos);
			}
		}
	}

	if (!ours)
	{
		status = PlayerStatus();
		progress = Progress();
		return false;
	}

	// Never wake into a place that does not exist, or already fallen.
	if (region < 0 || region >= Regions::Count)
	{
		region = Regions::Limina;
	}
	if (progress.respawnRegion < 0 || progress.respawnRegion >= Regions::Count)
	{
		progress.respawnRegion = Regions::Limina;
		progress.respawnAtAltar = false;
	}
	if (status.maxHealth <= 0.f)
	{
		status.maxHealth = Tuning::Player::kStartHealth;
	}
	if (status.health <= 0.f || status.health > status.maxHealth)
	{
		status.health = status.maxHealth;
	}
	if (progress.heroName.empty())
	{
		progress.heroName = Progress().heroName;
	}
	progress.visited[region] = true;
	return true;
}

int SaveGame::Milestones(const Progress& progress)
{
	int count = 0;
	for (int i = 0; i < Regions::Count; i++)
	{
		count += progress.bossDefeated[i] ? 1 : 0;
	}
	for (int i = 0; i < Story::Count; i++)
	{
		count += progress.story[i] ? 1 : 0;
	}
	count += progress.recognizedByLucius ? 1 : 0;
	count += progress.recognizedByNoctis ? 1 : 0;
	return count;
}
