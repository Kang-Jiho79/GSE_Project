#pragma once

class Renderer;
class TextRenderer;
struct PlayerStatus;
struct Progress;

// What he can look up about himself at any time (Tab): his name and the
// names he is known by, the vessel and its upgrades, both lifetime totals
// and how far they lean, the kings' blessings, and the gray if he holds it.
// The world holds still while it is open.
class StatusScreen
{
public:
	void Open();
	void Close();
	bool IsOpen() const;

	void Render(Renderer* renderer, TextRenderer* text, const PlayerStatus& status, const Progress& progress,
		int windowSizeX, int windowSizeY) const;

private:
	bool m_Open = false;
};
