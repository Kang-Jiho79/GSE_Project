#pragma once

struct Progress;

// The names he is known by. He begins already called the god's chosen - a
// fine-sounding name that is really the first hint of what he is - and each
// king who yields to him adds one of its own. When he slips the god's hold,
// the first is struck out and replaced: the chosen one is the one thing he
// no longer is.
namespace Titles
{
	const char* const kChosenByGod = "신에게 선택받은 자";
	const char* const kDarkestLight = "가장 어두운 빛";   // Lucius's
	const char* const kBrightestDark = "가장 밝은 어둠";  // Noctis's
	const char* const kOffTheBoard = "판을 벗어난 자";

	const int kMaxHeld = 3;

	// The names he holds right now, in the order he came by them. Returns
	// how many were written to held.
	int Held(const Progress& progress, const char* held[kMaxHeld]);

	// Off the board: the god's rule of balance no longer holds him, so
	// however far his powers lean, the gray does not take him.
	bool IsUnbound(const Progress& progress);
}
