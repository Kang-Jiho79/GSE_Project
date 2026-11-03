#pragma once

#include "Story.h"

struct Progress;

// Every conversation in the game, as data. All the words live in Dialogue.cpp
// so they can be read and changed in one place.
//
// A script is a run of lines, optionally ending in a choice. A choice can
// mark a story flag and lead on to another script.
namespace Dialogue
{
	enum class Style
	{
		Speech,  // said aloud
		Thought, // his own, not said aloud
		// Heard through the god's interference. Spans in [ ] break up into
		// noise; words in { } inside them always come through.
		Broken,
		// Said by someone in the god's grip who cannot hear the question:
		// the line jolts and glitches as it is spoken.
		Error,
	};

	// Who says a line: a name, kHero for him (shown as his name), or nullptr
	// for narration. An @ in the text itself is his name too.
	const char* const kHero = "@";

	struct Line
	{
		const char* speaker;
		const char* text;
		Style style;
	};

	// kEnd as the next script ends the conversation.
	const int kEnd = -1;

	struct Choice
	{
		const char* text;
		Story::Flag sets;
		int next;
	};

	struct Script
	{
		const Line* lines;
		int lineCount;
		const Choice* choices;
		int choiceCount;
	};

	enum Id
	{
		// The priests of the light's and the dark's temples.
		PriestLight,
		PriestOnGodLight,
		PriestErrorLight,
		PriestDark,
		PriestOnGodDark,
		PriestErrorDark,

		// Talk of the kings on the roads to their capitals.
		RumorLucius,
		RecognizedLucius,
		RumorNoctis,
		RecognizedNoctis,

		// The capitals' people.
		CitizenLumen,
		CitizenTenebrae,

		// The hidden village, before it was found.
		LiminaElder,
		LiminaChild,

		// The prologue: the village as it was, the gray coming, what is left.
		PrologueOpening,
		PrologueRaid,
		PrologueAftermath,

		// Kinis, met on the old battlefield: the kings' collusion.
		KinisFirst,

		// A king who has yielded to him, spoken to again - and asked about
		// the collusion, which he cannot hear. Before it can answer, the god
		// does, and he comes to on the old battlefield none the wiser.
		KingLucius,
		KingLuciusAsk,
		KingLuciusError,
		KingNoctis,
		KingNoctisAsk,
		KingNoctisError,
		Intervened,

		// Near the gray's heart: one of each side saved, and the prayer each
		// says afterward - word for word the same.
		RescueLight,
		RescueDark,
		SurvivorLight,
		SurvivorDark,

		// Having heard both prayers: seeing it, half seeing it, or not.
		EavesdropChosen,
		EavesdropClouded,
		EavesdropUnaware,
		// Seeing it later, once strong enough and sure of the kings.
		ChosenLater,

		// At the front, free of the god: the two he saved, enemies again.
		RenewedWar,

		// At the gray's heart. Short of the true ending's conditions Kinis
		// still talks, and offers; past them he cannot hear him at all.
		KinisOffer,
		KinisAccept,
		KinisRefuse,
		KinisSilent,
		// His last words: broken by the god's interference for one still in
		// its grip, whole for one who is not - with the last choice after.
		KinisLastBroken,
		KinisLastClear,

		// The god, at last: what he is, and what it says as it breaks. A
		// shorter word for each time he comes back after falling.
		GodMeeting,
		GodReturn,
		GodFall,

		// The endings, as pages of narration.
		EndingSavior,
		EndingAsh,
		EndingScheme,
		EndingTrue,
		// What became of everyone, after the true ending.
		EpilogueTrue,

		// The villages between: the border's keeper and a child, the
		// battlefield's guide and two neighbours who have never asked each
		// other's names, the pilgrims on the roads to the capitals, and the
		// refugees at the gray's edge.
		EveningKeeper,
		EveningKeeperAgain,
		VesperaChild,
		TreugaGuide,
		TreugaCapitals,
		TreugaNeighbours,
		PilgrimCandida,
		PilgrimObscura,
		RefugeeWidow,
		RefugeeElder,

		// The squares with no name: their people, and what is left written.
		HamletMother,
		HamletSmith,
		Wayfarer,
		PilgrimRest,
		PilgrimRestKnowing,
		HideoutNote,
		FallenKingPlaque,
		RuinedAltarWords,

		// What the notice boards' requests lead to.
		Signpost,
		MerchantBesieged,
		MerchantSaved,
		AlbaMother,
		AlbaMotherLetter,
		LostLantern,
		LanternOwner,
		LetterSender,

		// The queens, Solena of the light and Nocta of the dark: before and
		// after their kings yield, asked about the god (and, once he is free
		// of it, failing to hear), and fallen once their pair is broken.
		QueenSolena,
		QueenSolenaAfter,
		QueenSolenaOnGod,
		QueenSolenaError,
		QueenSolenaFallen,
		QueenNocta,
		QueenNoctaAfter,
		QueenNoctaOnGod,
		QueenNoctaError,
		QueenNoctaFallen,

		// Free of the god, back before each king: its words break off as the
		// god takes king and queen back as its pieces - and once they are
		// broken, the king can only repeat itself.
		KingLuciusRise,
		KingNoctisRise,
		KingLuciusFallen,
		KingNoctisFallen,

		// Kinis's last words to one free of the god who left a nation's king
		// and queen standing: whole, but no choice after - the pieces left on
		// the board move first.
		KinisLastUnchosen,

		// The ending when they do.
		EndingCheckmate,

		Count,
	};

	const Script& Get(int id);

	// Some conversations go differently once he knows more, or once a king
	// has yielded to him: the one to actually play.
	int Resolve(int id, const Progress& progress);

	// What having heard a script to its end changes in the story. Kept here,
	// beside the words, rather than wherever the script is started from.
	void Finish(int id, Progress& progress);

	// The four endings: the pages each is told in, its name, and the pages
	// of what came after (-1 for none).
	enum EndingId
	{
		Savior,
		Ash,
		Scheme,
		True,
		Checkmate,

		EndingCount,
	};

	struct Ending
	{
		int script;
		const char* name;
		int epilogue;
	};

	const Ending& GetEnding(int ending);
}
