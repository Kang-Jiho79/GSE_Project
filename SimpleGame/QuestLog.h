#pragma once

class Renderer;
class TextRenderer;
struct Progress;

// What he has set out to do (J): the main road's next step and what is said
// of it, every request taken up and where it stands, and those already
// paid. The world holds still while it is open.
class QuestLog
{
public:
	void Open();
	void Close();
	bool IsOpen() const;

	void Render(Renderer* renderer, TextRenderer* text, const Progress& progress, int windowSizeX, int windowSizeY) const;

private:
	bool m_Open = false;
};
