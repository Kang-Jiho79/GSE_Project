#pragma once

#include <deque>
#include <string>

class Renderer;
class TextRenderer;

// Everything said to him, as he heard it - so a line half lost to the god's
// interference can be read again, gaps and all. Opened with L; the world
// holds still while it is open.
class DialogueLog
{
public:
	// speaker: empty for narration.
	void Add(const std::string& speaker, const std::string& text);
	// Forgets everything - a new game.
	void Clear();

	void Open();
	void Close();
	bool IsOpen() const;
	// Back toward older lines (positive) or newer ones (negative).
	void Scroll(int entries);

	void Render(Renderer* renderer, TextRenderer* text, int windowSizeX, int windowSizeY) const;

private:
	struct Entry
	{
		std::string speaker;
		std::string text;
	};

	std::deque<Entry> m_Entries;
	bool m_Open = false;
	int m_Scroll = 0; // entries hidden below the bottom of the panel
};
