#pragma once

#include "Renderer.h"
#include "PlayerStatus.h"

// Screen-space overlay, anchored to the top-left corner: one diamond split down
// the middle - dark fills the left half, light the right - with the health bar
// beside it.
class Hud
{
public:
	void Render(Renderer* renderer, const PlayerStatus& status, int windowSizeX, int windowSizeY);

private:
	void DrawVessel(Renderer* renderer, const PlayerStatus& status, float centerX, float centerY);
	void DrawHealth(Renderer* renderer, const PlayerStatus& status, float left, float centerY);
	void DrawUpgradePips(Renderer* renderer, const PlayerStatus& status, float left, float centerY);
};
