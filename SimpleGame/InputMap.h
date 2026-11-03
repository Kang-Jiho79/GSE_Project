#pragma once

// GLFW's key and button codes only - none of the GL headers, which glew.h
// brings in wherever they are needed.
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include "Dependencies\GLFW\include\GLFW\glfw3.h"

// Which physical key does what. Letters are stored lower-case and compared
// after lowering the incoming key, so Shift (run) does not change their meaning.
// Keys that type something arrive as the character (SimpleGame turns GLFW's
// key codes into them); the rest - function keys, arrows, Shift - as GLFW's
// own codes.
namespace InputMap
{
	const unsigned char kMoveUp = 'w';
	const unsigned char kMoveDown = 's';
	const unsigned char kMoveLeft = 'a';
	const unsigned char kMoveRight = 'd';

	const unsigned char kSwitchForm = ' ';
	const unsigned char kSkill = 'e';
	const unsigned char kSpecial = 'q';
	// The form's special, once its gauge is full.
	const unsigned char kLimit = 'r';
	const unsigned char kInteract = 'f';
	const unsigned char kDialogueLog = 'l';
	// What he has set out to do: the main road and the requests taken.
	const unsigned char kQuestLog = 'j';
	// The world map, to look at anywhere; at a waystone F opens it to travel.
	const unsigned char kWorldMap = 'm';
	const unsigned char kStatus = 9; // Tab
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
	// Debug: the special's gauge filled.
	const unsigned char kDebugLimit = '0';
	// Debug: the gray taken up (light and dark given up for it) or not.
	const unsigned char kDebugAsh = '8';
	// Debug: straight into the god's place, the gray already taken up.
	const unsigned char kDebugToRealm = '9';
	const unsigned char kQuit = 27; // Esc

	// Either Shift, reported among the keys that type nothing.
	const int kRunKeyLeft = GLFW_KEY_LEFT_SHIFT;
	const int kRunKeyRight = GLFW_KEY_RIGHT_SHIFT;

	// Debug: bloom and grading on and off, to compare and to measure the cost.
	const int kTogglePostProcess = GLFW_KEY_F1;
	const int kToggleOutline = GLFW_KEY_F2;
	const int kToggleEffects = GLFW_KEY_F3;
	// Debug: the monitor's little server on and off, to watch from a browser.
	const int kToggleMonitor = GLFW_KEY_F4;
	// Debug: the test cutscene, to see the camera, bars and washes at work.
	const int kDebugCutscene = GLFW_KEY_F5;

	// Moving between choices and through the log, besides W and S.
	const int kMenuUp = GLFW_KEY_UP;
	const int kMenuDown = GLFW_KEY_DOWN;

	const int kAttackButton = GLFW_MOUSE_BUTTON_LEFT;
	const int kDodgeButton = GLFW_MOUSE_BUTTON_RIGHT;
	// A button's state as it changes: pressed, or let go.
	const int kButtonDown = GLFW_PRESS;
}
