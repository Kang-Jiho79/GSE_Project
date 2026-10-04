#pragma once

#include <string>

#include "HangulInput.h"

class Renderer;
class TextRenderer;

// Where he names the child the hidden village took in. Korean by default,
// put together from the keys as typed; Tab switches to Latin letters.
class NameEntryScreen
{
public:
	NameEntryScreen();

	// overwrites: a saved journey exists, which beginning anew will replace.
	void Open(bool overwrites);
	void Close();
	bool IsOpen() const;

	// A key as typed (Shift already applied). Returns true once Enter accepts
	// a name; Esc is left to the caller.
	bool Key(unsigned char key);

	std::string Name() const;

	void Render(Renderer* renderer, TextRenderer* text, int windowSizeX, int windowSizeY, float time) const;

private:
	bool m_Open = false;
	bool m_Refused = false; // Enter on an empty name: say so
	bool m_Overwrites = false;
	HangulInput m_Input;
};
