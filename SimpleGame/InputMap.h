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
	const unsigned char kDialogueLog = 'l';
	// Going on in a conversation, besides F.
	const unsigned char kConfirm = 13; // Enter

	// Debug: take a hit, or absorb one kill's worth of light or dark.
	const unsigned char kDebugDamage = 'c';
	const unsigned char kDebugAddLight = '1';
	const unsigned char kDebugAddDark = '2';
	// Debug: straight to a capital and its king, without walking the road.
	const unsigned char kDebugToLumen = '3';
	const unsigned char kDebugToTenebrae = '4';
	// Debug: as if he had already slipped the god's hold, to hear the priests
	// fail to answer him.
	const unsigned char kDebugEscapeGod = '5';
	// Debug: every region open on the map and both kings already won over,
	// and a large dose of both powers, to reach the later story quickly.
	const unsigned char kDebugOpenWorld = '6';
	const unsigned char kDebugAddBoth = '7';
	// Debug: the gray held or let go, to try the third form.
	const unsigned char kDebugAsh = '8';
	// Debug: straight into the god's place, the gray already taken up.
	const unsigned char kDebugToRealm = '9';
	const unsigned char kQuit = 27; // Esc

	// Either Shift; freeglut reports them through the special-key callbacks.
	const int kRunKeyLeft = GLUT_KEY_SHIFT_L;
	const int kRunKeyRight = GLUT_KEY_SHIFT_R;

	// Debug: bloom and grading on and off, to compare and to measure the cost.
	const int kTogglePostProcess = GLUT_KEY_F1;
	const int kToggleOutline = GLUT_KEY_F2;
	const int kToggleEffects = GLUT_KEY_F3;

	// Moving between choices and through the log, besides W and S.
	const int kMenuUp = GLUT_KEY_UP;
	const int kMenuDown = GLUT_KEY_DOWN;

	const int kAttackButton = GLUT_LEFT_BUTTON;
	const int kDodgeButton = GLUT_RIGHT_BUTTON;
}
