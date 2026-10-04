#pragma once

class Renderer;
class TextRenderer;

// The first thing he sees: the name of the game over the hidden village, and
// a short menu. Continuing is offered only when there is a game to continue.
class TitleScreen
{
public:
	enum class Pick
	{
		None,
		NewGame,
		Continue,
		Quit,
	};

	// endingsSeen: how many of the endings have ever been reached, shown
	// once there is at least one.
	void Open(bool canContinue, int endingsSeen);
	void Close();
	bool IsOpen() const;

	// Up or down the menu, skipping what cannot be picked.
	void Move(int step);
	// Selects the item under a screen point, if there is one. Returns whether
	// there was.
	bool SelectAt(float x, float y);
	// What the selected item does.
	Pick Confirm() const;

	void Render(Renderer* renderer, TextRenderer* text, int windowSizeX, int windowSizeY, float time) const;

private:
	static const int kItemCount = 3;

	bool IsEnabled(int item) const;
	float ItemY(int item) const;

	bool m_Open = false;
	bool m_CanContinue = false;
	int m_EndingsSeen = 0;
	int m_Selected = 0;
};
