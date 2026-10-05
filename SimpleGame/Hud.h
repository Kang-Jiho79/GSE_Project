#pragma once

#include <string>

#include "Renderer.h"
#include "PlayerStatus.h"

class TextRenderer;

// Screen-space overlay, anchored to the top-left corner: one diamond split down
// the middle - dark fills the left half, light the right - with the health bar
// beside it, and under that the current form, how soon it can change, and
// how soon its E and Q are ready. Notices - a setting switched, say - show
// briefly across the top.
class Hud
{
public:
	void Render(Renderer* renderer, TextRenderer* text, const PlayerStatus& status, int windowSizeX, int windowSizeY);

	// Shows a line across the top for a moment, replacing any still showing.
	void Notify(const std::string& message);
	// Real time, so a notice still fades while the world is held still.
	void Tick(float deltaTime);

private:
	void DrawVessel(Renderer* renderer, const PlayerStatus& status, float centerX, float centerY);
	void DrawBalance(Renderer* renderer, const PlayerStatus& status, float centerX, float centerY);
	void DrawHealth(Renderer* renderer, TextRenderer* text, const PlayerStatus& status, float left, float centerY);
	void DrawUpgradePips(Renderer* renderer, const PlayerStatus& status, float left, float centerY);
	void DrawForm(Renderer* renderer, const PlayerStatus& status, float left, float centerY);
	void DrawSkills(Renderer* renderer, TextRenderer* text, const PlayerStatus& status, float left, float centerY);
	// locked: not learned yet - drawn empty, and said so.
	void DrawSkillSlot(Renderer* renderer, TextRenderer* text, const Cooldown& cooldown, bool diamond, bool locked,
		const char* key, float centerX, float centerY, float r, float g, float b);
	void DrawNotice(TextRenderer* text, float top);

	std::string m_Notice;
	float m_NoticeTimer = 0.f;
};
