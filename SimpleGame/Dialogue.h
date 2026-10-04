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
		// the collusion, which he cannot hear.
		KingLucius,
		KingLuciusAsk,
		KingLuciusError,
		KingNoctis,
		KingNoctisAsk,
		KingNoctisError,

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

		// The endings, as pages of narration.
		EndingSavior,
		EndingAsh,
		EndingScheme,
		EndingTrue,

		Count,
	};

	const Script& Get(int id);

	// Some conversations go differently once he knows more, or once a king
	// has yielded to him: the one to actually play.
	int Resolve(int id, const Progress& progress);

	// What having heard a script to its end changes in the story. Kept here,
	// beside the words, rather than wherever the script is started from.
	void Finish(int id, Progress& progress);

	// The four endings: the pages each is told in, and its name.
	enum EndingId
	{
		Savior,
		Ash,
		Scheme,
		True,

		EndingCount,
	};

	struct Ending
	{
		int script;
		const char* name;
	};

	const Ending& GetEnding(int ending);
}
