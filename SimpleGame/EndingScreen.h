#pragma once

class Renderer;
class TextRenderer;

// How the story ends, told a page at a time - then, for the true ending,
// what became of everyone after - and last the ending's name. Each ending
// has its own dark behind the pages; the true one grows toward dawn as it
// goes. When he has read it through, the game goes back to the title.
class EndingScreen
{
public:
	// ending: a Dialogue::EndingId. firstTime: never reached before, and
	// marked as new on its last page.
	void Open(int ending, bool firstTime);
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
	int StoryPages() const;
	int EpiloguePages() const;
	int PageCount() const;
	// The page's words, or nullptr for the last page, the ending's name.
	const char* PageText(bool& epilogue) const;
	void Background(float& r, float& g, float& b) const;

	bool m_Open = false;
	int m_Ending = 0;
	bool m_FirstTime = false;
	int m_Page = 0; // the last page is the ending's name
	float m_PageTime = 0.f;
	bool m_Finished = false;
};
