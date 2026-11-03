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
		// A king asked about the collusion: the pale mark over it, the
		// world washing white - and, anchored at Bellum's waystone, him
		// coming to there.
		Intervention,
		Intervened,
		// The specials' cut-ins (R), anchored at him: his eyes across the
		// screen, the world stopped, and the special going off as it ends.
		LimitLight,
		LimitDark,
		LimitAsh,
		// Free of the god, before a king: the mark over king and queen, the
		// world washing white, and the two of them standing as pieces of its
		// board. Anchored at the king.
		Transform,
		// The king of the board fallen: the world slowing on it, checkmate,
		// and the white again, taking the pieces away. Anchored at the king.
		Checkmate,
		// The very end, one nation's pair left standing: they rise round him
		// out of the white, check, and checkmate. Anchored at him.
		CheckmateEnd,

		Count,
	};

	// What a shot asks the story to do as it begins. Handled by the scene.
	enum Cue
	{
		NoCue = -1,
		CueTestBurst, // a spray of light at the anchor
		CueGodMark,   // the pale mark over the king
		CueIntervene, // the world gone white: on to Bellum
		CueLimit,     // the cut-in over: his special goes off
		CueTransform,   // the white at its height: king and queen become pieces
		CueRestorePair, // the pieces gone: king and queen left fallen
		CueCheckmateRise, // the pieces left standing close round him
		CueCheckmateEnd,  // and that is the end
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

	// His eyes in a band across the screen, in a form's colour - the
	// specials' cut-in.
	enum Portrait
	{
		NoPortrait = -1,
		PortraitLight,
		PortraitDark,
		PortraitAsh,
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

		int portrait;     // Portrait, or NoPortrait
		bool gray;        // the world drained of colour
	};

	struct Script
	{
		const Shot* shots;
		int count;
	};

	const Script& Get(int id);
}
