#pragma once

// The turns the story takes, each remembered once it has happened. Named as
// in the design; kept in Progress so they are saved with everything else.
// (Both kings' recognition is kept there too, as its own pair.)
namespace Story
{
	enum Flag
	{
		NoFlag = -1,

		VillageRaided,           // the gray found the hidden village (the prologue)
		KnowsCollusion,          // Kinis told him the two kings plotted together
		AskedKingAboutCollusion, // he put it to a king
		SawKingAnomaly,          // and the king could not hear the question
		KnowsOneGod,             // he overheard that both nations keep one god
		TitleChosenByGod,        // "신에게 선택받은 자"
		EscapedGodInfluence,     // the god's hold on him is gone
		KnowsWorldTruth,         // Kinis's last words
		AcceptAshOffer,          // he took Kinis's offer before their fight
		AcceptAsh,               // he took the gray after it

		// The steps on the way to the ones above: each king asked in turn
		// (both asked is SawKingAnomaly), and each side's survivor saved
		// (both saved is when he overhears that their prayers are one).
		AskedLucius,
		AskedNoctis,
		RescuedLight,
		RescuedDark,
		// Free of the god, he saw the two he saved at each other's throats.
		SawRenewedWar,

		Count,
	};
}
