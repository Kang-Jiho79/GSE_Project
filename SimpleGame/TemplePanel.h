#pragma once

#include <string>

class Renderer;
class TextRenderer;
struct PlayerStatus;

// What the altar shows him when he rests at it: the god "lighting up the
// balance of his powers" - both totals, how far they lean, and how close he
// stands to the gray. The world holds still while it is open.
class TemplePanel
{
public:
	// ruined: the temple has fallen to the gray, though its altar still answers.
	void Open(const std::string& placeName, bool ruined);
	void Close();
	bool IsOpen() const;

	void Render(Renderer* renderer, TextRenderer* text, const PlayerStatus& status,
		int windowSizeX, int windowSizeY) const;

private:
	void DrawBalanceBar(Renderer* renderer, const PlayerStatus& status, float centerY) const;

	bool m_Open = false;
	bool m_Ruined = false;
	std::string m_PlaceName;
};
