#include "stdafx.h"
#include "Hud.h"
#include "Tuning.h"
#include "Palette.h"
#include <cmath>

namespace
{
	const float kVesselWidth = 84.f;
	const float kVesselHeight = 104.f;
	const float kVesselBorder = 5.f;

	const float kHealthWidth = 250.f;
	const float kHealthHeight = 16.f;

	// The balance needle across the middle of the vessel: centred is even,
	// the vessel's left and right tips are the allowance.
	const float kNeedleWidth = 3.f;
	const float kNeedleHeight = 34.f;
	const float kNotchWidth = 2.f;
	const float kNotchHeight = 6.f;

	// The gray that creeps into the HUD as the balance goes pulses at this rate
	// once it is eating him.
	const float kAshPulseRate = 6.f;

	const float kPipWidth = 9.f;
	const float kPipHeight = 12.f;
	const float kPipSpacing = 16.f;
	const int kPipCount = 6;

	const float kFormMarkWidth = 12.f;
	const float kFormMarkHeight = 16.f;
	const float kFormBarWidth = 72.f;
	const float kFormBarHeight = 6.f;

	// E is a square, Q a diamond - told apart by shape, since the HUD has no
	// lettering yet.
	const float kSkillSize = 18.f;
	const float kSkillSpacing = 28.f;

	// Mixes a colour toward the ash gray by amount (0..1).
	void Ash(float amount, float& r, float& g, float& b)
	{
		r += (Palette::kAsh.r - r) * amount;
		g += (Palette::kAsh.g - g) * amount;
		b += (Palette::kAsh.b - b) * amount;
	}

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
	DrawBalance(renderer, status, vesselCenterX, vesselCenterY);
	DrawHealth(renderer, status, left + 132.f, vesselCenterY + 20.f);
	DrawUpgradePips(renderer, status, left + 132.f, vesselCenterY - 12.f);
	DrawForm(renderer, status, left + 132.f, vesselCenterY - 38.f);
	DrawSkills(renderer, status, left + 238.f, vesselCenterY - 38.f);
}

void Hud::DrawVessel(Renderer* renderer, const PlayerStatus& status, float centerX, float centerY)
{
	float bottom = centerY - kVesselHeight / 2.f;
	float halfWidth = kVesselWidth / 2.f;

	float darkFraction = Clamp01(status.dark / status.capacity);
	float lightFraction = Clamp01(status.light / status.capacity);

	// The rim greys over with the warning, and pulses once the gray is
	// actually eating him.
	float rimR = Palette::kUiFrame.r;
	float rimG = Palette::kUiFrame.g;
	float rimB = Palette::kUiFrame.b;
	float ash = status.WarningLevel();
	if (status.IsEroding())
	{
		ash = 0.6f + 0.4f * sinf(status.erosionTime * kAshPulseRate);
	}
	Ash(ash, rimR, rimG, rimB);

	renderer->DrawSolidDiamond(centerX, centerY,
		kVesselWidth + kVesselBorder * 2.f, kVesselHeight + kVesselBorder * 2.f, rimR, rimG, rimB, 1.f);
	renderer->DrawSolidDiamond(centerX, centerY, kVesselWidth, kVesselHeight, Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 1.f);

	// Each half is the whole diamond clipped to its side and to how far it has filled.
	if (darkFraction > 0.f)
	{
		const Palette::Color& fill = (darkFraction >= 1.f) ? Palette::kVesselDarkFull : Palette::kVesselDark;
		renderer->SetClip(centerX - halfWidth, bottom, halfWidth, kVesselHeight * darkFraction);
		renderer->DrawSolidDiamond(centerX, centerY, kVesselWidth, kVesselHeight, fill.r, fill.g, fill.b, 1.f);
	}

	if (lightFraction > 0.f)
	{
		const Palette::Color& fill = (lightFraction >= 1.f) ? Palette::kVesselLightFull : Palette::kVesselLight;
		renderer->SetClip(centerX, bottom, halfWidth, kVesselHeight * lightFraction);
		renderer->DrawSolidDiamond(centerX, centerY, kVesselWidth, kVesselHeight, fill.r, fill.g, fill.b, 1.f);
	}

	renderer->ClearClip();

	renderer->DrawSolidRect(centerX, centerY, 1.5f, kVesselHeight, Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
}

void Hud::DrawBalance(Renderer* renderer, const PlayerStatus& status, float centerX, float centerY)
{
	float halfWidth = kVesselWidth / 2.f;

	// Notches where the warning starts, on the vessel's outline at that width.
	float notchX = halfWidth * Tuning::Balance::kWarningShare;
	float notchY = (kVesselHeight / 2.f) * (1.f - Tuning::Balance::kWarningShare);
	for (int side = -1; side <= 1; side += 2)
	{
		renderer->DrawSolidRect(centerX + side * notchX, centerY + notchY, kNotchWidth, kNotchHeight,
			Palette::kUiNotch.r, Palette::kUiNotch.g, Palette::kUiNotch.b, 1.f);
		renderer->DrawSolidRect(centerX + side * notchX, centerY - notchY, kNotchWidth, kNotchHeight,
			Palette::kUiNotch.r, Palette::kUiNotch.g, Palette::kUiNotch.b, 1.f);
	}

	// Pinned to the tip once past the allowance, so it never leaves the vessel.
	float ratio = status.BalanceRatio();
	if (ratio > 1.f)
	{
		ratio = 1.f;
	}
	else if (ratio < -1.f)
	{
		ratio = -1.f;
	}
	float needleX = centerX + ratio * halfWidth;

	// White while he is safe, ash once he is warned.
	float r = Palette::kUiBone.r;
	float g = Palette::kUiBone.g;
	float b = Palette::kUiBone.b;
	Ash(status.WarningLevel(), r, g, b);

	renderer->DrawSolidRect(needleX, centerY, kNeedleWidth + 2.f, kNeedleHeight + 2.f, Palette::kUiInk.r, Palette::kUiInk.g, Palette::kUiInk.b, 1.f);
	renderer->DrawSolidRect(needleX, centerY, kNeedleWidth, kNeedleHeight, r, g, b, 1.f);
}

void Hud::DrawHealth(Renderer* renderer, const PlayerStatus& status, float left, float centerY)
{
	float centerX = left + kHealthWidth / 2.f;
	float fraction = Clamp01(status.health / status.maxHealth);
	float fillWidth = kHealthWidth * fraction;

	renderer->DrawSolidRect(centerX, centerY, kHealthWidth + 6.f, kHealthHeight + 6.f, Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
	renderer->DrawSolidRect(centerX, centerY, kHealthWidth, kHealthHeight, Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 1.f);

	// Gray while erosion is what is draining it, so the cause is plain.
	float r = Palette::kHealth.r;
	float g = Palette::kHealth.g;
	float b = Palette::kHealth.b;
	if (status.IsEroding())
	{
		Ash(0.75f, r, g, b);
	}

	if (fillWidth > 0.f)
	{
		renderer->DrawSolidRect(left + fillWidth / 2.f, centerY, fillWidth, kHealthHeight, r, g, b, 1.f);
	}
}

void Hud::DrawUpgradePips(Renderer* renderer, const PlayerStatus& status, float left, float centerY)
{
	for (int i = 0; i < kPipCount; i++)
	{
		bool earned = i < status.upgrades;
		float x = left + kPipWidth / 2.f + i * kPipSpacing;

		const Palette::Color& pip = earned ? Palette::kPipEarned : Palette::kPipEmpty;
		renderer->DrawSolidDiamond(x, centerY, kPipWidth, kPipHeight, pip.r, pip.g, pip.b, 1.f);
	}
}

void Hud::DrawForm(Renderer* renderer, const PlayerStatus& status, float left, float centerY)
{
	float r, g, b;
	AlignmentColor::Core(FormAlignment(status.form), r, g, b);

	float markX = left + kFormMarkWidth / 2.f;
	renderer->DrawSolidDiamond(markX, centerY, kFormMarkWidth + 4.f, kFormMarkHeight + 4.f, Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
	renderer->DrawSolidDiamond(markX, centerY, kFormMarkWidth, kFormMarkHeight, r, g, b, 1.f);

	// Refills while the switch cools down; full means Space will work.
	float ready = 1.f - Clamp01(status.formCooldown / Tuning::FormSwitch::kSwitchCooldown);
	float barLeft = left + kFormMarkWidth + 8.f;
	float fillWidth = kFormBarWidth * ready;
	float shade = (ready >= 1.f) ? 1.f : 0.45f;

	renderer->DrawSolidRect(barLeft + kFormBarWidth / 2.f, centerY,
		kFormBarWidth + 4.f, kFormBarHeight + 4.f, Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
	renderer->DrawSolidRect(barLeft + kFormBarWidth / 2.f, centerY,
		kFormBarWidth, kFormBarHeight, Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 1.f);

	if (fillWidth > 0.f)
	{
		renderer->DrawSolidRect(barLeft + fillWidth / 2.f, centerY, fillWidth, kFormBarHeight,
			r * shade, g * shade, b * shade, 1.f);
	}
}

void Hud::DrawSkills(Renderer* renderer, const PlayerStatus& status, float left, float centerY)
{
	float r, g, b;
	AlignmentColor::Core(FormAlignment(status.form), r, g, b);

	float x = left + kSkillSize / 2.f;
	DrawSkillSlot(renderer, status.SkillCooldown(status.form), false, x, centerY, r, g, b);
	DrawSkillSlot(renderer, status.SpecialCooldown(status.form), true, x + kSkillSpacing, centerY, r, g, b);
}

void Hud::DrawSkillSlot(Renderer* renderer, const Cooldown& cooldown, bool diamond,
	float centerX, float centerY, float r, float g, float b)
{
	float size = diamond ? kSkillSize * 1.3f : kSkillSize;
	float half = size / 2.f;

	if (diamond)
	{
		renderer->DrawSolidDiamond(centerX, centerY, size + 6.f, size + 6.f, Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
		renderer->DrawSolidDiamond(centerX, centerY, size, size, Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 1.f);
	}
	else
	{
		renderer->DrawSolidRect(centerX, centerY, size + 4.f, size + 4.f, Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
		renderer->DrawSolidRect(centerX, centerY, size, size, Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 1.f);
	}

	// Refills from the bottom while it recharges; full and bright once ready.
	float progress = Clamp01(cooldown.Progress());
	float shade = cooldown.Ready() ? 1.f : 0.45f;

	renderer->SetClip(centerX - half, centerY - half, size, size * progress);
	if (diamond)
	{
		renderer->DrawSolidDiamond(centerX, centerY, size, size, r * shade, g * shade, b * shade, 1.f);
	}
	else
	{
		renderer->DrawSolidRect(centerX, centerY, size, size, r * shade, g * shade, b * shade, 1.f);
	}
	renderer->ClearClip();
}
