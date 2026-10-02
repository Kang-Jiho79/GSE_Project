#include "stdafx.h"
#include "Hud.h"
#include "Tuning.h"

namespace
{
	const float kVesselWidth = 84.f;
	const float kVesselHeight = 104.f;
	const float kVesselBorder = 5.f;

	const float kHealthWidth = 250.f;
	const float kHealthHeight = 16.f;

	const float kPipWidth = 9.f;
	const float kPipHeight = 12.f;
	const float kPipSpacing = 16.f;
	const int kPipCount = 6;

	const float kFormMarkWidth = 12.f;
	const float kFormMarkHeight = 16.f;
	const float kFormBarWidth = 72.f;
	const float kFormBarHeight = 6.f;

	float Clamp01(float value)
	{
		if (value < 0.f)
		{
			return 0.f;
		}
		if (value > 1.f)
		{
			return 1.f;
		}
		return value;
	}
}

void Hud::Render(Renderer* renderer, const PlayerStatus& status, int windowSizeX, int windowSizeY)
{
	float left = -windowSizeX / 2.f;
	float top = windowSizeY / 2.f;

	float vesselCenterX = left + 74.f;
	float vesselCenterY = top - 78.f;

	DrawVessel(renderer, status, vesselCenterX, vesselCenterY);
	DrawHealth(renderer, status, left + 132.f, vesselCenterY + 20.f);
	DrawUpgradePips(renderer, status, left + 132.f, vesselCenterY - 12.f);
	DrawForm(renderer, status, left + 132.f, vesselCenterY - 38.f);
}

void Hud::DrawVessel(Renderer* renderer, const PlayerStatus& status, float centerX, float centerY)
{
	float bottom = centerY - kVesselHeight / 2.f;
	float halfWidth = kVesselWidth / 2.f;

	float darkFraction = Clamp01(status.dark / status.capacity);
	float lightFraction = Clamp01(status.light / status.capacity);

	renderer->DrawSolidDiamond(centerX, centerY,
		kVesselWidth + kVesselBorder * 2.f, kVesselHeight + kVesselBorder * 2.f, 0.32f, 0.31f, 0.34f, 1.f);
	renderer->DrawSolidDiamond(centerX, centerY, kVesselWidth, kVesselHeight, 0.10f, 0.10f, 0.12f, 1.f);

	// Each half is the whole diamond clipped to its side and to how far it has filled.
	if (darkFraction > 0.f)
	{
		bool full = darkFraction >= 1.f;
		renderer->SetClip(centerX - halfWidth, bottom, halfWidth, kVesselHeight * darkFraction);
		renderer->DrawSolidDiamond(centerX, centerY, kVesselWidth, kVesselHeight,
			full ? 0.58f : 0.42f, full ? 0.40f : 0.28f, full ? 0.88f : 0.68f, 1.f);
	}

	if (lightFraction > 0.f)
	{
		bool full = lightFraction >= 1.f;
		renderer->SetClip(centerX, bottom, halfWidth, kVesselHeight * lightFraction);
		renderer->DrawSolidDiamond(centerX, centerY, kVesselWidth, kVesselHeight,
			full ? 0.98f : 0.82f, full ? 0.80f : 0.62f, full ? 0.38f : 0.24f, 1.f);
	}

	renderer->ClearClip();

	renderer->DrawSolidRect(centerX, centerY, 1.5f, kVesselHeight, 0.32f, 0.31f, 0.34f, 1.f);
}

void Hud::DrawHealth(Renderer* renderer, const PlayerStatus& status, float left, float centerY)
{
	float centerX = left + kHealthWidth / 2.f;
	float fraction = Clamp01(status.health / status.maxHealth);
	float fillWidth = kHealthWidth * fraction;

	renderer->DrawSolidRect(centerX, centerY, kHealthWidth + 6.f, kHealthHeight + 6.f, 0.32f, 0.31f, 0.34f, 1.f);
	renderer->DrawSolidRect(centerX, centerY, kHealthWidth, kHealthHeight, 0.10f, 0.10f, 0.12f, 1.f);

	if (fillWidth > 0.f)
	{
		renderer->DrawSolidRect(left + fillWidth / 2.f, centerY, fillWidth, kHealthHeight, 0.62f, 0.16f, 0.18f, 1.f);
	}
}

void Hud::DrawUpgradePips(Renderer* renderer, const PlayerStatus& status, float left, float centerY)
{
	for (int i = 0; i < kPipCount; i++)
	{
		bool earned = i < status.upgrades;
		float x = left + kPipWidth / 2.f + i * kPipSpacing;

		renderer->DrawSolidDiamond(x, centerY, kPipWidth, kPipHeight,
			earned ? 0.72f : 0.20f, earned ? 0.70f : 0.20f, earned ? 0.64f : 0.22f, 1.f);
	}
}

void Hud::DrawForm(Renderer* renderer, const PlayerStatus& status, float left, float centerY)
{
	float r, g, b;
	AlignmentColor::Core(FormAlignment(status.form), r, g, b);

	float markX = left + kFormMarkWidth / 2.f;
	renderer->DrawSolidDiamond(markX, centerY, kFormMarkWidth + 4.f, kFormMarkHeight + 4.f, 0.32f, 0.31f, 0.34f, 1.f);
	renderer->DrawSolidDiamond(markX, centerY, kFormMarkWidth, kFormMarkHeight, r, g, b, 1.f);

	// Refills while the switch cools down; full means Space will work.
	float ready = 1.f - Clamp01(status.formCooldown / Tuning::FormSwitch::kSwitchCooldown);
	float barLeft = left + kFormMarkWidth + 8.f;
	float fillWidth = kFormBarWidth * ready;
	float shade = (ready >= 1.f) ? 1.f : 0.45f;

	renderer->DrawSolidRect(barLeft + kFormBarWidth / 2.f, centerY,
		kFormBarWidth + 4.f, kFormBarHeight + 4.f, 0.32f, 0.31f, 0.34f, 1.f);
	renderer->DrawSolidRect(barLeft + kFormBarWidth / 2.f, centerY,
		kFormBarWidth, kFormBarHeight, 0.10f, 0.10f, 0.12f, 1.f);

	if (fillWidth > 0.f)
	{
		renderer->DrawSolidRect(barLeft + fillWidth / 2.f, centerY, fillWidth, kFormBarHeight,
			r * shade, g * shade, b * shade, 1.f);
	}
}
