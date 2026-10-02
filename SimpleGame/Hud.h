#pragma once

#include "Renderer.h"
#include "PlayerStatus.h"

// Screen-space overlay, anchored to the top-left corner: one diamond split down
// the middle - dark fills the left half, light the right - with the health bar
// beside it, and under that the current form, how soon it can change, and
// how soon its E and Q are ready.
class Hud
{
public:
	void Render(Renderer* renderer, const PlayerStatus& status, int windowSizeX, int windowSizeY);

private:
	void DrawVessel(Renderer* renderer, const PlayerStatus& status, float centerX, float centerY);
	void DrawHealth(Renderer* renderer, const PlayerStatus& status, float left, float centerY);
	void DrawUpgradePips(Renderer* renderer, const PlayerStatus& status, float left, float centerY);
	void DrawForm(Renderer* renderer, const PlayerStatus& status, float left, float centerY);
	void DrawSkills(Renderer* renderer, const PlayerStatus& status, float left, float centerY);
	void DrawSkillSlot(Renderer* renderer, const Cooldown& cooldown, bool diamond,
		float centerX, float centerY, float r, float g, float b);
};
