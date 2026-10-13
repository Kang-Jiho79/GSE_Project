#pragma once

// The cutscenes, as tables of shots. A shot says how long it lasts, where the
// camera looks and how close, whether the black bars are drawn, whether the
// screen washes black or white, how fast the world runs under it, a line of
// caption, and what the story should do as it begins (a cue) or say (a
// conversation it waits on).
//
// Positions are tiles from the cutscene's anchor - whatever the one who plays
// it puts it beside: a king, an altar, the waystone.
namespace Cutscenes
{
	enum Id
	{
		Test, // tried from a debug key: every kind of shot once

		Count,
	};

	// What a shot asks the story to do as it begins. Handled by the scene.
	enum Cue
	{
		NoCue = -1,
		CueTestBurst, // a spray of light at the anchor
	};

	// Where the camera looks over the shot.
	enum class Aim
	{
		Player, // on him, wherever he goes
		Anchor, // on the anchor, moved by the shot's offset
		Hold,   // wherever the last shot left it
	};

	// What the screen washes toward.
	enum class Wash
	{
		None,
		Black,
		White,
	};

	// How fast the world runs under the shot. Stopped is a held breath: the
	// world frozen while only the camera and the words move.
	enum class Clock
	{
		Run,
		Slow,
		Stop,
	};

	struct Shot
	{
		float seconds;

		Aim aim;
		float col; // tiles from the anchor, for Aim::Anchor
		float row;
		float zoom; // reached by the end of the shot; 0 keeps the last one
		bool cut;   // jump straight there instead of easing over the shot

		bool letterbox;
		Wash wash;
		float washFrom; // how far washed, 0..1, at the start and the end
		float washTo;
		Clock clock;

		const char* speaker; // nullptr: no name, a narrator's line
		const char* caption; // nullptr: nothing written

		int cue;          // Cue, or NoCue
		int conversation; // a Dialogue::Id opened as it begins, or -1
	};

	struct Script
	{
		const Shot* shots;
		int count;
	};

	const Script& Get(int id);
}
