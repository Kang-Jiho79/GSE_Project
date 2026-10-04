#pragma once

struct PlayerStatus;
struct Progress;

// His journey, kept between sessions in a plain text file beside the game:
// one "key=value" per line, readable and easy to mend by hand. What it keeps
// is what outlives a region - his powers and health, his blessings, where he
// has been, what he has beaten, what he knows, and his name - and the region
// he was in. Nothing in the middle of a region is kept: loading puts him at
// that region's waystone, or at its altar if that is where he last rested.
//
// A line it does not know is skipped, and anything missing keeps its starting
// value, so a file from an older version still loads.
namespace SaveGame
{
	bool Exists();
	bool Write(const PlayerStatus& status, const Progress& progress, int region);
	// Fills in a fresh status and progress. Returns false if there is no file
	// or it is not one of ours.
	bool Read(PlayerStatus& status, Progress& progress, int& region);

	// The endings ever reached, kept in a file of their own beside the save,
	// so beginning anew does not forget them.
	bool HasSeenEnding(int ending);
	void RecordEnding(int ending);
	int EndingsSeen();

	// How far along the story he is, as one number that only ever grows -
	// bosses beaten, kings won over, things learned. When it changes, there is
	// something worth saving.
	int Milestones(const Progress& progress);
}
