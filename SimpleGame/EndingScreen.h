#pragma once

class Renderer;
class TextRenderer;

// How the story ends, told on a dark screen a page at a time, then the
// ending's name. When he has read it through, the game goes back to the
// title.
class EndingScreen
{
public:
	// ending: a Dialogue::EndingId.
	void Open(int ending);
	void Close();
	bool IsOpen() const;
	// Read through to the ending's name, and past it.
	bool IsFinished() const;

	// Real time: each page fades in.
	void Update(float deltaTime);
	// The rest of the page's fade at once if it is still coming in, otherwise
	// the next page.
	void Advance();

	void Render(Renderer* renderer, TextRenderer* text, int windowSizeX, int windowSizeY) const;

private:
	int PageCount() const;

	bool m_Open = false;
	int m_Ending = 0;
	int m_Page = 0; // the last page is the ending's name
	float m_PageTime = 0.f;
	bool m_Finished = false;
};
