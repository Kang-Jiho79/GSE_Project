#include "stdafx.h"
#include "NameEntryScreen.h"
#include "Renderer.h"
#include "TextRenderer.h"
#include "Palette.h"
#include <cmath>

namespace
{
	const int kNameLength = 8;

	const float kPanelWidth = 560.f;
	const float kPanelHeight = 300.f;
	const float kPanelBorder = 4.f;
	const float kFieldWidth = 300.f;
	const float kFieldHeight = 44.f;
	const float kFieldY = -10.f;

	const unsigned char kTab = 9;
	const unsigned char kBackspace = 8;
	const unsigned char kEnter = 13;
}

NameEntryScreen::NameEntryScreen()
	: m_Input(kNameLength)
{
}

void NameEntryScreen::Open(bool overwrites)
{
	m_Open = true;
	m_Overwrites = overwrites;
	m_Refused = false;
	m_Input.Clear();
}

void NameEntryScreen::Close()
{
	m_Open = false;
}

bool NameEntryScreen::IsOpen() const
{
	return m_Open;
}

std::string NameEntryScreen::Name() const
{
	return m_Input.Text();
}

bool NameEntryScreen::Key(unsigned char key)
{
	switch (key)
	{
	case kTab:
	{
		m_Input.ToggleKorean();
		return false;
	}
	case kBackspace:
	{
		m_Input.Backspace();
		return false;
	}
	case kEnter:
	{
		if (m_Input.Length() == 0)
		{
			m_Refused = true;
			return false;
		}
		return true;
	}
	default:
	{
		m_Refused = false;
		m_Input.Type(key);
		return false;
	}
	}
}

void NameEntryScreen::Render(Renderer* renderer, TextRenderer* text, int windowSizeX, int windowSizeY, float time) const
{
	if (!m_Open)
	{
		return;
	}

	renderer->DrawSolidRect(0.f, 0.f, (float)windowSizeX, (float)windowSizeY,
		Palette::kVoid.r, Palette::kVoid.g, Palette::kVoid.b, 0.9f);
	renderer->DrawSolidRect(0.f, 0.f, kPanelWidth + kPanelBorder * 2.f, kPanelHeight + kPanelBorder * 2.f,
		Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
	renderer->DrawSolidRect(0.f, 0.f, kPanelWidth, kPanelHeight,
		Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 0.97f);

	float top = kPanelHeight / 2.f;
	float bottom = -kPanelHeight / 2.f;

	text->Draw(TextRenderer::Face::Body, "숲에 버려져 있던 아이를, 숨은 마을 사람들이 거두어 키웠다.",
		0.f, top - 50.f, Palette::kUiBone, 0.85f, TextRenderer::Align::Center);
	text->Draw(TextRenderer::Face::Body, "그들이 아이에게 지어 준 이름은 —",
		0.f, top - 82.f, Palette::kUiBone, 0.85f, TextRenderer::Align::Center);

	renderer->DrawSolidRect(0.f, kFieldY, kFieldWidth + 4.f, kFieldHeight + 4.f,
		Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
	renderer->DrawSolidRect(0.f, kFieldY, kFieldWidth, kFieldHeight,
		Palette::kVoid.r, Palette::kVoid.g, Palette::kVoid.b, 1.f);

	// What is settled, then the syllable still being built, underlined, then
	// the caret - all centred together.
	std::string closed = m_Input.ClosedText();
	std::string open = m_Input.OpenText();
	float closedWidth = text->Measure(TextRenderer::Face::Title, closed.c_str());
	float openWidth = text->Measure(TextRenderer::Face::Title, open.c_str());
	float left = -(closedWidth + openWidth) * 0.5f;
	float baseline = kFieldY - 10.f;

	text->Draw(TextRenderer::Face::Title, closed.c_str(), left, baseline, Palette::kUiBone, 1.f);
	if (!open.empty())
	{
		text->Draw(TextRenderer::Face::Title, open.c_str(), left + closedWidth, baseline, Palette::kUiBone, 1.f);
		renderer->DrawSolidRect(left + closedWidth + openWidth * 0.5f, baseline - 6.f, openWidth, 1.5f,
			Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b, 0.8f);
	}
	if (fmodf(time, 1.f) < 0.55f)
	{
		renderer->DrawSolidRect(left + closedWidth + openWidth + 3.f, kFieldY, 2.f, kFieldHeight * 0.6f,
			Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b, 0.9f);
	}

	const char* mode = m_Input.IsKorean() ? "한글  (Tab: 영문으로)" : "영문  (Tab: 한글로)";
	text->Draw(TextRenderer::Face::Small, mode, 0.f, kFieldY - kFieldHeight * 0.5f - 22.f, Palette::kUiNotch, 1.f,
		TextRenderer::Align::Center);

	if (m_Overwrites && !m_Refused)
	{
		text->Draw(TextRenderer::Face::Small, "새로 시작하면 이전 기록은 덮어쓴다.", 0.f, bottom + 62.f, Palette::kAsh, 1.f,
			TextRenderer::Align::Center);
	}
	if (m_Refused)
	{
		text->Draw(TextRenderer::Face::Body, "이름 없이 떠날 수는 없다.", 0.f, bottom + 62.f, Palette::kAsh, 1.f,
			TextRenderer::Align::Center);
	}

	text->Draw(TextRenderer::Face::Small, "Enter: 결정 · Backspace: 지우기 · Esc: 돌아가기 · 최대 8자",
		0.f, bottom + 20.f, Palette::kUiBone, 0.6f, TextRenderer::Align::Center);
}
