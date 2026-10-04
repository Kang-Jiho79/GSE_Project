#include "stdafx.h"
#include "DialogueLog.h"
#include "Renderer.h"
#include "TextRenderer.h"
#include "Palette.h"
#include "Tuning.h"
#include <cmath>

namespace
{
	const float kPanelWidth = 680.f;
	const float kPanelHeight = 460.f;
	const float kPanelBorder = 4.f;
	const float kMargin = 32.f;
	const float kEntryGap = 14.f;
}

void DialogueLog::Add(const std::string& speaker, const std::string& text)
{
	Entry entry = { speaker, text };
	m_Entries.push_back(entry);
	while ((int)m_Entries.size() > Tuning::Dialogue::kLogEntries)
	{
		m_Entries.pop_front();
	}
}

void DialogueLog::Clear()
{
	m_Entries.clear();
	m_Scroll = 0;
}

void DialogueLog::Open()
{
	m_Open = true;
	m_Scroll = 0;
}

void DialogueLog::Close()
{
	m_Open = false;
}

bool DialogueLog::IsOpen() const
{
	return m_Open;
}

void DialogueLog::Scroll(int entries)
{
	m_Scroll += entries;
	int most = (int)m_Entries.size() - 1;
	if (m_Scroll > most)
	{
		m_Scroll = most;
	}
	if (m_Scroll < 0)
	{
		m_Scroll = 0;
	}
}

void DialogueLog::Render(Renderer* renderer, TextRenderer* text, int windowSizeX, int windowSizeY) const
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
	float left = -kPanelWidth / 2.f + kMargin;
	float wrap = kPanelWidth - kMargin * 2.f;

	text->Draw(TextRenderer::Face::Title, "대화 기록", 0.f, top - 44.f, Palette::kUiBone, 1.f, TextRenderer::Align::Center);
	text->Draw(TextRenderer::Face::Small, "W/S: 넘기기   L / Esc: 닫기", 0.f, bottom + 14.f, Palette::kUiBone, 0.6f,
		TextRenderer::Align::Center);

	if (m_Entries.empty())
	{
		text->Draw(TextRenderer::Face::Body, "아직 나눈 대화가 없다.", 0.f, 0.f, Palette::kUiBone, 0.6f,
			TextRenderer::Align::Center);
		return;
	}

	// Newest at the bottom, filling upward until the panel runs out of room.
	float bodyLine = text->LineHeight(TextRenderer::Face::Body);
	float smallLine = text->LineHeight(TextRenderer::Face::Small);
	float lowest = bottom + 44.f;
	float highest = top - 72.f;
	float y = lowest;

	for (int i = (int)m_Entries.size() - 1 - m_Scroll; i >= 0; i--)
	{
		const Entry& entry = m_Entries[i];

		// Near enough how many lines the wrap will make of it.
		int lines = (int)ceilf(text->Measure(TextRenderer::Face::Body, entry.text.c_str()) / wrap);
		if (lines < 1)
		{
			lines = 1;
		}
		float height = lines * bodyLine + (entry.speaker.empty() ? 0.f : smallLine);
		if (y + height > highest)
		{
			break;
		}

		float baseline = y + height - (entry.speaker.empty() ? 0.f : smallLine) - bodyLine * 0.8f;
		if (!entry.speaker.empty())
		{
			text->Draw(TextRenderer::Face::Small, entry.speaker.c_str(), left, y + height - smallLine * 0.8f,
				Palette::kUiNotch, 1.f);
		}
		text->Draw(TextRenderer::Face::Body, entry.text.c_str(), left, baseline, Palette::kUiBone, 0.9f,
			TextRenderer::Align::Left, wrap);

		y += height + kEntryGap;
	}
}
