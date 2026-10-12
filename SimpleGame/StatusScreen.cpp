#include "stdafx.h"
#include "StatusScreen.h"
#include "Renderer.h"
#include "TextRenderer.h"
#include "PlayerStatus.h"
#include "Progress.h"
#include "Titles.h"
#include "Palette.h"
#include "Tuning.h"
#include <cmath>
#include <cstdio>
#include <string>

namespace
{
	const float kPanelWidth = 600.f;
	const float kPanelHeight = 440.f;
	const float kPanelBorder = 4.f;
	const float kMargin = 48.f;

	// The rows under the name: a label on the left, its value on the right.
	const float kFirstRowY = 60.f; // from the panel's centre
	const float kRowSpacing = 34.f;

	// Below this share of the allowance the totals read as even.
	const float kEvenShare = 0.08f;

	void Row(TextRenderer* text, float y, const char* label, const char* value, const Palette::Color& color)
	{
		text->Draw(TextRenderer::Face::Body, label, -kPanelWidth / 2.f + kMargin, y, Palette::kUiNotch, 1.f,
			TextRenderer::Align::Left);
		text->Draw(TextRenderer::Face::Body, value, kPanelWidth / 2.f - kMargin, y, color, 1.f,
			TextRenderer::Align::Right);
	}
}

void StatusScreen::Open()
{
	m_Open = true;
}

void StatusScreen::Close()
{
	m_Open = false;
}

bool StatusScreen::IsOpen() const
{
	return m_Open;
}

void StatusScreen::Render(Renderer* renderer, TextRenderer* text, const PlayerStatus& status, const Progress& progress,
	int windowSizeX, int windowSizeY) const
{
	if (!m_Open)
	{
		return;
	}

	renderer->DrawSolidRect(0.f, 0.f, (float)windowSizeX, (float)windowSizeY,
		Palette::kVoid.r, Palette::kVoid.g, Palette::kVoid.b, 0.55f);
	renderer->DrawSolidRect(0.f, 0.f, kPanelWidth + kPanelBorder * 2.f, kPanelHeight + kPanelBorder * 2.f,
		Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
	renderer->DrawSolidRect(0.f, 0.f, kPanelWidth, kPanelHeight,
		Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 0.97f);

	float top = kPanelHeight / 2.f;
	float bottom = -kPanelHeight / 2.f;

	// His name, and under it every name he is known by.
	text->Draw(TextRenderer::Face::Title, progress.heroName.c_str(), 0.f, top - 56.f, Palette::kUiBone, 1.f,
		TextRenderer::Align::Center);
	const char* held[Titles::kMaxHeld];
	int count = Titles::Held(progress, held);
	std::string titles;
	for (int i = 0; i < count; i++)
	{
		if (i > 0)
		{
			titles += "  ·  ";
		}
		titles += held[i];
	}
	text->Draw(TextRenderer::Face::Body, titles.c_str(), 0.f, top - 94.f, Palette::kUiBone, 0.85f,
		TextRenderer::Align::Center);

	char value[128];
	float y = kFirstRowY;

	snprintf(value, sizeof(value), "%d / %d", (int)ceilf(status.health), (int)status.maxHealth);
	Row(text, y, "체력", value, Palette::kUiBone);
	y -= kRowSpacing;

	snprintf(value, sizeof(value), "%d단계  (그릇 용량 %d)", status.upgrades, (int)status.capacity);
	Row(text, y, "강화", value, Palette::kUiBone);
	y -= kRowSpacing;

	// Dark on the left and light on the right, as in the vessel.
	snprintf(value, sizeof(value), "어둠 %d / %d   ·   빛 %d / %d",
		(int)status.dark, (int)status.capacity, (int)status.light, (int)status.capacity);
	Row(text, y, "그릇", value, Palette::kUiBone);
	y -= kRowSpacing;

	snprintf(value, sizeof(value), "%d", (int)status.darkTotal);
	Row(text, y, "어둠의 힘 (누적)", value, Palette::kVesselDarkFull);
	y -= kRowSpacing;

	snprintf(value, sizeof(value), "%d", (int)status.lightTotal);
	Row(text, y, "빛의 힘 (누적)", value, Palette::kVesselLightFull);
	y -= kRowSpacing;

	// How far they lean, and against what - unless nothing holds him to it.
	float balance = status.Balance();
	float lean = fabsf(status.BalanceRatio());
	const char* side = (balance > 0.f) ? "빛" : "어둠";
	const Palette::Color* color = &Palette::kUiBone;
	if (status.balanceUnbound)
	{
		if (lean < kEvenShare)
		{
			snprintf(value, sizeof(value), "고름  ·  제한 없음");
		}
		else
		{
			snprintf(value, sizeof(value), "%s 쪽 %d  ·  제한 없음", side, (int)fabsf(balance));
		}
	}
	else
	{
		if (lean < kEvenShare)
		{
			snprintf(value, sizeof(value), "고름  (허용 %d)", (int)status.BalanceAllowance());
		}
		else
		{
			snprintf(value, sizeof(value), "%s 쪽 %d  (허용 %d)", side, (int)fabsf(balance), (int)status.BalanceAllowance());
		}
		if (lean > 1.f)
		{
			color = &Palette::kAsh;
		}
	}
	Row(text, y, "균형", value, *color);
	y -= kRowSpacing;

	std::string blessings;
	if (status.lightBlessing)
	{
		snprintf(value, sizeof(value), "루키우스 (빛 공격 +%d%%)", (int)(Tuning::Duel::kLightBlessing * 100.f + 0.5f));
		blessings += value;
	}
	if (status.darkBlessing)
	{
		if (!blessings.empty())
		{
			blessings += "   ";
		}
		snprintf(value, sizeof(value), "녹티스 (피흡 +%d%%p)", (int)(Tuning::Duel::kLifestealBlessing * 100.f + 0.5f));
		blessings += value;
	}
	if (blessings.empty())
	{
		blessings = "없음";
	}
	Row(text, y, "왕의 축복", blessings.c_str(), Palette::kUiBone);
	y -= kRowSpacing;

	if (status.ashUnlocked)
	{
		Row(text, y, "잿빛", "받아들임", Palette::kAsh);
	}

	text->Draw(TextRenderer::Face::Small, "Tab / Esc: 닫기", 0.f, bottom + 16.f, Palette::kUiBone, 0.6f,
		TextRenderer::Align::Center);
}
