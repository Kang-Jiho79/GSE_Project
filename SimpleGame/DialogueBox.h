#pragma once

#include <string>
#include <vector>

#include "Dialogue.h"

class Renderer;
class TextRenderer;
class DialogueLog;
struct Progress;

// A conversation under way, low across the screen: who is speaking, the line
// writing itself out, and at the end of some scripts a choice. The world
// holds still while it is open.
//
// Lines heard through the god's interference break up as they are written,
// all but the words that matter; lines from someone in the god's grip jolt and
// glitch. Each line goes into the log once he has read it, as he heard it.
class DialogueBox
{
public:
	// Starts a conversation - the version of it for what he knows by now.
	void Open(int script, const Progress& progress);
	void Close();
	bool IsOpen() const;

	// Real time: the line writes itself out, and broken words churn.
	void Update(float deltaTime);

	// F, Space, Enter or a click. The rest of the line at once if it is still
	// being written; otherwise on to the next line, or the choice taken.
	void Advance(Progress& progress, DialogueLog& log);
	// The last line is written out and the choices are showing.
	bool IsChoosing() const;
	// Up or down the choices, once they are showing.
	void MoveChoice(int step);
	// Picks the choice at a screen point, if there is one there. Returns
	// whether there was.
	bool SelectChoiceAt(float x, float y, int windowSizeY);

	void Render(Renderer* renderer, TextRenderer* text, const Progress& progress,
		int windowSizeX, int windowSizeY) const;

private:
	struct Glyph
	{
		unsigned int codepoint;
		bool broken; // lost to the interference
	};

	const Dialogue::Script& Current() const;
	const Dialogue::Line& CurrentLine() const;
	void StartLine();
	bool IsLineWritten() const;

	// What is on screen of the line right now, its first count characters.
	void BuildShown(int count) const;
	// The line as it goes in the log: plain, with every broken stretch a gap.
	std::string LoggedText() const;
	std::string SpeakerName(const Progress& progress) const;
	void Log(const Progress& progress, DialogueLog& log) const;
	void ChoiceCenter(int index, int windowSizeY, float& x, float& y) const;

	bool m_Open = false;
	int m_Script = 0;
	int m_Line = 0;
	float m_Written = 0.f; // characters of the line shown so far
	int m_Choice = 0;
	float m_Time = 0.f;

	std::vector<Glyph> m_Glyphs;
	std::vector<unsigned int> m_Decoded;
	// His name, for every @ in a line.
	std::vector<unsigned int> m_HeroName;
	mutable std::string m_Shown;
};
