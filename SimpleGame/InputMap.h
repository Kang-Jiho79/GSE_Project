#pragma once

#include "Dependencies\freeglut.h"

// Which physical key does what. Letters are stored lower-case and compared
// after lowering the incoming key, so Shift (run) does not change their meaning.
namespace InputMap
{
	const unsigned char kMoveUp = 'w';
	const unsigned char kMoveDown = 's';
	const unsigned char kMoveLeft = 'a';
	const unsigned char kMoveRight = 'd';

	const unsigned char kSwitchForm = ' ';
	const unsigned char kSkill = 'e';
	const unsigned char kSpecial = 'q';
	const unsigned char kInteract = 'f';

	// Debug: take a hit, or absorb one kill's worth of light or dark.
	const unsigned char kDebugDamage = 'c';
	const unsigned char kDebugAddLight = '1';
	const unsigned char kDebugAddDark = '2';
	const unsigned char kQuit = 27; // Esc

	// Either Shift; freeglut reports them through the special-key callbacks.
	const int kRunKeyLeft = GLUT_KEY_SHIFT_L;
	const int kRunKeyRight = GLUT_KEY_SHIFT_R;

	// Debug: bloom and grading on and off, to compare and to measure the cost.
	const int kTogglePostProcess = GLUT_KEY_F1;
	const int kToggleOutline = GLUT_KEY_F2;
	const int kToggleEffects = GLUT_KEY_F3;

	const int kAttackButton = GLUT_LEFT_BUTTON;
	const int kDodgeButton = GLUT_RIGHT_BUTTON;
}
