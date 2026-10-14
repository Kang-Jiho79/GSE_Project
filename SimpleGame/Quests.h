#pragma once

#include <string>

struct Progress;
struct PlayerStatus;

// What he is asked to do, as data: the next step of the main road - worked
// out from how far the story has gone, never saved on its own, and never
// pointing at the choices that decide how it ends - and the requests pinned
// to the new villages' notice boards. A request is taken at its board, done
// out in the world, and reported back at the same board for its reward,
// which the board shows up front.
namespace Quests
{
	enum Id
	{
		WoodsKnights,     // Vespera: knights on the forest roads
		BrokenSignpost,   // Vespera: the old signpost on Crepus's hills
		LightSoldiers,    // Treuga: the light's eroded soldiers
		DarkSoldiers,     // Treuga: the dark's
		StrandedMerchant, // Treuga: a peddler hemmed in among the wreckage
		PilgrimLetter,    // Candida: a pilgrim's letter to his mother in Alba
		GateMages,        // Candida: mages on the road to the light's gate
		GraveKnights,     // Obscura: knights among Nox's graves
		LostLantern,      // Obscura: a lantern left by Crypta's tombs
		MarchHorde,       // Refugium: the gray on the march

		Count,
	};

	enum class Kind
	{
		Hunt,    // so many of the gray, in given places
		Find,    // something lying out in the world
		Rescue,  // someone hemmed in by the gray
		Deliver, // a word carried to someone
	};

	// Which of the gray a hunt counts.
	enum class Quarry
	{
		Any,
		Knight,
		Mage,
		Light,
		Dark,
	};

	// Where a request stands, as kept in Progress.
	enum Stage
	{
		Unseen, // not taken
		Active, // taken, under way
		Ready,  // done, to be reported at its board
		Done,   // reported and paid
	};

	struct Info
	{
		const char* key;         // its name in the save
		const char* title;
		const char* description;
		const char* goal;        // what to do, as the board and the log say it
		const char* boon;        // what it gives besides light and dark, or nullptr
		int board;               // the village whose notice board has it
		Kind kind;
		Quarry quarry;
		int zones[2];            // a hunt's places (-1 for none)
		int target;              // a hunt's count
		float light;             // paid on report
		float dark;
		int place;               // where taking it sets someone or something out, or -1
		int report;              // a conversation played on reporting, or -1
	};

	const Info& Get(int id);
	// What it pays, as the board shows it: "빛 +10 · 어둠 +10", say.
	std::string Reward(int id);

	// Whether a village's board has anything on it at all.
	bool HasBoard(int region);
	// The requests showing on a village's board, in order: up to max of them
	// into ids. A request for word of a king who has already acknowledged
	// him is never put up.
	int OnBoard(int region, const Progress& progress, int* ids, int max);

	Stage StageOf(const Progress& progress, int id);
	void Accept(int id, Progress& progress);
	// What a request under way stands at, for the board and the log: "3/5"
	// for a hunt, or what is left to do.
	std::string Standing(int id, const Progress& progress);

	// One of the gray has fallen: in which place, and what it was.
	void NoteKill(int zone, bool knight, bool light);
	// Something found, someone saved, a word given: the request is done, to
	// be reported. Ignored unless it is under way.
	void Fulfil(int id, Progress& progress);
	// The fallen counted into the hunts under way. Returns a line to show for
	// what moved this frame, or an empty one.
	std::string Tally(Progress& progress);
	// Paid out at the board: the line to show, and a conversation to play
	// after it (or -1).
	std::string Claim(int id, PlayerStatus& status, Progress& progress, int& script);

	// The main road's next step, as one line and as the log tells it - or
	// nullptr once there is nothing more to point at.
	const char* MainGoal(const Progress& progress);
	const char* MainDetail(const Progress& progress);
	// Where it points: up to two places, into regions. Returns how many.
	int MainTargets(const Progress& progress, int* regions);
}
