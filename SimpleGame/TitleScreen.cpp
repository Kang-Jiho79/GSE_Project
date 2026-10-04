#include "stdafx.h"
#include "TitleScreen.h"
#include "Renderer.h"
#include "TextRenderer.h"
#include "Palette.h"
#include "Dialogue.h"
#include <string>
#include <cmath>

namespace
{
	const char* const kItems[] = { "새로 하기", "이어하기", "끝내기" };

	const float kTitleY = 120.f;
	const float kMenuTop = -30.f;
	const float kItemSpacing = 44.f;
	const float kItemWidth = 240.f;
	const float kItemHeight = 36.f;
	const float kVeil = 0.82f;
}

void TitleScreen::Open(bool canContinue, int endingsSeen)
{
	m_Open = true;
	m_CanContinue = canContinue;
	m_EndingsSeen = endingsSeen;
	m_Selected = canContinue ? 1 : 0;
}

void TitleScreen::Close()
{
	m_Open = false;
}

bool TitleScreen::IsOpen() const
{
	return m_Open;
}

bool TitleScreen::IsEnabled(int item) const
{
	return item != 1 || m_CanContinue;
}

float TitleScreen::ItemY(int item) const
{
	return kMenuTop - item * kItemSpacing;
}

void TitleScreen::Move(int step)
{
	for (int tries = 0; tries < kItemCount; tries++)
	{
		m_Selected = ((m_Selected + step) % kItemCount + kItemCount) % kItemCount;
		if (IsEnabled(m_Selected))
		{
			return;
		}
	}
}

bool TitleScreen::SelectAt(float x, float y)
{
	for (int i = 0; i < kItemCount; i++)
	{
		if (IsEnabled(i) && fabsf(x) <= kItemWidth * 0.5f && fabsf(y - ItemY(i)) <= kItemHeight * 0.5f)
		{
			m_Selected = i;
			return true;
		}
	}
	return false;
}

TitleScreen::Pick TitleScreen::Confirm() const
{
	switch (m_Selected)
	{
	case 0:
	{
		return Pick::NewGame;
	}
	case 1:
	{
		return m_CanContinue ? Pick::Continue : Pick::None;
	}
	default:
	{
		return Pick::Quit;
	}
	}
}

void TitleScreen::Render(Renderer* renderer, TextRenderer* text, int windowSizeX, int windowSizeY, float time) const
{
	if (!m_Open)
	{
		return;
	}

	// The village shows faintly through the dark.
	renderer->DrawSolidRect(0.f, 0.f, (float)windowSizeX, (float)windowSizeY,
		Palette::kVoid.r, Palette::kVoid.g, Palette::kVoid.b, kVeil);

	text->Draw(TextRenderer::Face::Title, "가장 어두운 빛", 0.f, kTitleY, Palette::kUiBone, 1.f, TextRenderer::Align::Center);
	text->Draw(TextRenderer::Face::Small, "THE DARKEST LIGHT", 0.f, kTitleY - 34.f, Palette::kUiNotch, 1.f,
		TextRenderer::Align::Center);

	for (int i = 0; i < kItemCount; i++)
	{
		bool selected = (i == m_Selected);
		float alpha = !IsEnabled(i) ? 0.25f : (selected ? 1.f : 0.55f);
		if (selected)
		{
			float pulse = 0.6f + 0.25f * sinf(time * 3.f);
			renderer->DrawSolidRect(0.f, ItemY(i) + kItemHeight * 0.5f - 2.f, kItemWidth, 1.5f,
				Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b, pulse * 0.5f);
			renderer->DrawSolidRect(0.f, ItemY(i) - kItemHeight * 0.5f + 4.f, kItemWidth, 1.5f,
				Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b, pulse * 0.5f);
		}
		text->Draw(TextRenderer::Face::Body, kItems[i], 0.f, ItemY(i) - 6.f, Palette::kUiBone, alpha,
			TextRenderer::Align::Center);
	}

	if (m_EndingsSeen > 0)
	{
		std::string seen = "본 결말  " + std::to_string(m_EndingsSeen) + " / " + std::to_string((int)Dialogue::EndingCount);
		text->Draw(TextRenderer::Face::Small, seen.c_str(), 0.f, ItemY(kItemCount - 1) - 50.f, Palette::kUiNotch, 1.f,
			TextRenderer::Align::Center);
	}

	text->Draw(TextRenderer::Face::Small, "W/S 고르기 · F/Enter 결정", 0.f, -windowSizeY / 2.f + 24.f,
		Palette::kUiBone, 0.5f, TextRenderer::Align::Center);
}
