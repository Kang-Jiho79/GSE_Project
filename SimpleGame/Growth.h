#pragma once

struct PlayerStatus;

// What each upgrade of the vessel brings. He starts with only his two basic
// attacks, the roll and the form switch; every time both halves of the
// vessel fill, something more opens up - a form's own dodge, the E and Q of
// both forms, the swap skills - and then each of those grows stronger in
// turn. Which upgrade brings what is set in Tuning::Growth.
//
// Once he holds the gray, at the very end, everything is open.
namespace Growth
{
	// Whether he has reached the upgrade that brings a gift.
	bool Has(const PlayerStatus& status, int upgrade);

	// What reaching this upgrade brings, said in a few words for the notice
	// as it happens; nullptr if it brings nothing new.
	const char* GiftOf(int upgrade);
}
