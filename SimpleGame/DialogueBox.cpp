#include "stdafx.h"
#include "DialogueBox.h"
#include "DialogueLog.h"
#include "Renderer.h"
#include "TextRenderer.h"
#include "Progress.h"
#include "Palette.h"
#include "Tuning.h"
#include "Utf8.h"
#include <cmath>
#include <cstring>

namespace
{
	const float kBoxWidth = 760.f;
	const float kBoxHeight = 150.f;
	const float kBoxBorder = 4.f;
	const float kBoxRise = 36.f; // its bottom, above the bottom of the window
	const float kMargin = 28.f;

	const float kNameWidth = 180.f;
	const float kNameHeight = 30.f;

	const float kChoiceWidth = 380.f;
	const float kChoiceHeight = 34.f;
	const float kChoiceGap = 8.f;

	// What a lost character turns into: static, mostly, and some silence.
	const unsigned int kNoise[] = { 0x2592, 0x2591, 0x00B7, 0x2500, 0x0020, 0x0020 };
	const int kNoiseCount = sizeof(kNoise) / sizeof(kNoise[0]);

	// In the god's grip: this share of characters glitch at any moment, and
	// the line shakes by up to this much.
	const unsigned int kErrorGlitchPercent = 8;
	const float kErrorJitter = 2.5f;
	const float kBrokenJitter = 1.f;

	unsigned int Hash(unsigned int a, unsigned int b)
	{
		unsigned int h = a * 2654435761u ^ (b + 0x9E3779B9u) * 40503u;
		h ^= h >> 13;
		h *= 0x5BD1E995u;
		h ^= h >> 15;
		return h;
	}
}

void DialogueBox::Open(int script, const Progress& progress)
{
	m_Open = true;
	m_Script = Dialogue::Resolve(script, progress);
	Utf8::Decode(progress.heroName.c_str(), m_HeroName);
	m_Line = 0;
	m_Choice = 0;
	StartLine();
}

void DialogueBox::Close()
{
	m_Open = false;
}

bool DialogueBox::IsOpen() const
{
	return m_Open;
}

const Dialogue::Script& DialogueBox::Current() const
{
	return Dialogue::Get(m_Script);
}

const Dialogue::Line& DialogueBox::CurrentLine() const
{
	return Current().lines[m_Line];
}

void DialogueBox::StartLine()
{
	m_Written = 0.f;
	m_Glyphs.clear();

	// [ ] marks what the interference takes; { } inside it, what it cannot.
	bool lost = (CurrentLine().style == Dialogue::Style::Broken);
	bool inBroken = false;
	bool kept = false;
	Utf8::Decode(CurrentLine().text, m_Decoded);
	for (size_t i = 0; i < m_Decoded.size(); i++)
	{
		unsigned int c = m_Decoded[i];
		switch (c)
		{
		case '[':
		{
			inBroken = true;
			break;
		}
		case ']':
		{
			inBroken = false;
			break;
		}
		case '{':
		{
			kept = true;
			break;
		}
		case '}':
		{
			kept = false;
			break;
		}
		case '@':
		{
			for (size_t n = 0; n < m_HeroName.size(); n++)
			{
				Glyph glyph = { m_HeroName[n], false };
				m_Glyphs.push_back(glyph);
			}
			break;
		}
		default:
		{
			Glyph glyph = { c, lost && inBroken && !kept };
			m_Glyphs.push_back(glyph);
			break;
		}
		}
	}
}

bool DialogueBox::IsLineWritten() const
{
	return m_Written >= (float)m_Glyphs.size();
}

bool DialogueBox::IsChoosing() const
{
	return m_Open && IsLineWritten() && m_Line == Current().lineCount - 1 && Current().choiceCount > 0;
}

void DialogueBox::Update(float deltaTime)
{
	if (!m_Open)
	{
		return;
	}

	m_Time += deltaTime;
	if (!IsLineWritten())
	{
		m_Written += Tuning::Dialogue::kTypeRate * deltaTime;
	}
}

void DialogueBox::Advance(Progress& progress, DialogueLog& log)
{
	if (!m_Open)
	{
		return;
	}

	if (!IsLineWritten())
	{
		m_Written = (float)m_Glyphs.size();
		return;
	}

	Log(progress, log);

	const Dialogue::Script& script = Current();
	if (m_Line < script.lineCount - 1)
	{
		m_Line++;
		StartLine();
		return;
	}

	// Heard to its end: whatever that changes, before anything follows it.
	Dialogue::Finish(m_Script, progress);

	if (script.choiceCount == 0)
	{
		Close();
		return;
	}

	// His answer goes in the log as his own words.
	const Dialogue::Choice& choice = script.choices[m_Choice];
	log.Add(progress.heroName, choice.text);
	progress.Set(choice.sets);
	if (choice.next == Dialogue::kEnd)
	{
		Close();
		return;
	}
	Open(choice.next, progress);
}

void DialogueBox::MoveChoice(int step)
{
	if (!IsChoosing())
	{
		return;
	}

	int count = Current().choiceCount;
	m_Choice = ((m_Choice + step) % count + count) % count;
}

void DialogueBox::ChoiceCenter(int index, int windowSizeY, float& x, float& y) const
{
	// Stacked above the box, the first at the top.
	float boxTop = -windowSizeY / 2.f + kBoxRise + kBoxHeight + kBoxBorder;
	int count = Current().choiceCount;
	x = 0.f;
	y = boxTop + kChoiceGap * 2.f + kChoiceHeight * 0.5f + (count - 1 - index) * (kChoiceHeight + kChoiceGap);
}

bool DialogueBox::SelectChoiceAt(float x, float y, int windowSizeY)
{
	if (!IsChoosing())
	{
		return false;
	}

	for (int i = 0; i < Current().choiceCount; i++)
	{
		float centerX, centerY;
		ChoiceCenter(i, windowSizeY, centerX, centerY);
		if (fabsf(x - centerX) <= kChoiceWidth * 0.5f && fabsf(y - centerY) <= kChoiceHeight * 0.5f)
		{
			m_Choice = i;
			return true;
		}
	}
	return false;
}

std::string DialogueBox::SpeakerName(const Progress& progress) const
{
	const char* speaker = CurrentLine().speaker;
	if (speaker == nullptr)
	{
		return std::string();
	}
	if (strcmp(speaker, Dialogue::kHero) == 0)
	{
		return progress.heroName;
	}
	return speaker;
}

std::string DialogueBox::LoggedText() const
{
	std::string text;
	bool inGap = false;
	for (size_t i = 0; i < m_Glyphs.size(); i++)
	{
		if (m_Glyphs[i].broken)
		{
			if (!inGap)
			{
				text += "……";
				inGap = true;
			}
			continue;
		}
		inGap = false;
		Utf8::Append(text, m_Glyphs[i].codepoint);
	}
	return text;
}

void DialogueBox::Log(const Progress& progress, DialogueLog& log) const
{
	std::string text = LoggedText();
	if (CurrentLine().style == Dialogue::Style::Thought)
	{
		text = "(" + text + ")";
	}
	log.Add(SpeakerName(progress), text);
}

void DialogueBox::BuildShown(int count) const
{
	Dialogue::Style style = CurrentLine().style;
	unsigned int step = (unsigned int)(m_Time * Tuning::Dialogue::kGlitchRate);

	m_Shown.clear();
	for (int i = 0; i < count; i++)
	{
		const Glyph& glyph = m_Glyphs[i];
		unsigned int roll = Hash((unsigned int)i, step);
		bool glitch = glyph.broken
			|| (style == Dialogue::Style::Error && glyph.codepoint != ' ' && roll % 100 < kErrorGlitchPercent);
		Utf8::Append(m_Shown, glitch ? kNoise[(roll >> 8) % kNoiseCount] : glyph.codepoint);
	}
}

void DialogueBox::Render(Renderer* renderer, TextRenderer* text, const Progress& progress,
	int windowSizeX, int windowSizeY) const
{
	if (!m_Open)
	{
		return;
	}

	const Dialogue::Line& line = CurrentLine();
	unsigned int step = (unsigned int)(m_Time * Tuning::Dialogue::kGlitchRate);
	bool error = (line.style == Dialogue::Style::Error);
	bool broken = (line.style == Dialogue::Style::Broken);

	// The god's grip shows in the frame too: it flickers toward ash.
	const Palette::Color& frame = (error && Hash(step, 7u) % 3 == 0) ? Palette::kAsh : Palette::kUiFrame;

	float centerY = -windowSizeY / 2.f + kBoxRise + kBoxHeight / 2.f;
	float top = centerY + kBoxHeight / 2.f;
	float bottom = centerY - kBoxHeight / 2.f;
	float left = -kBoxWidth / 2.f;
	float right = kBoxWidth / 2.f;

	renderer->DrawSolidRect(0.f, centerY, kBoxWidth + kBoxBorder * 2.f, kBoxHeight + kBoxBorder * 2.f,
		frame.r, frame.g, frame.b, 1.f);
	renderer->DrawSolidRect(0.f, centerY, kBoxWidth, kBoxHeight,
		Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 0.95f);

	std::string speaker = SpeakerName(progress);
	if (!speaker.empty())
	{
		float nameX = left + kMargin + kNameWidth / 2.f;
		float nameY = top + kNameHeight / 2.f - 4.f;
		renderer->DrawSolidRect(nameX, nameY, kNameWidth + kBoxBorder * 2.f, kNameHeight + kBoxBorder * 2.f,
			frame.r, frame.g, frame.b, 1.f);
		renderer->DrawSolidRect(nameX, nameY, kNameWidth, kNameHeight,
			Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 1.f);
		text->Draw(TextRenderer::Face::Body, speaker.c_str(), nameX, nameY - 7.f, Palette::kUiBone, 1.f,
			TextRenderer::Align::Center);
	}

	int count = (int)m_Written;
	if (count > (int)m_Glyphs.size())
	{
		count = (int)m_Glyphs.size();
	}
	BuildShown(count);

	float jitterX = 0.f;
	float jitterY = 0.f;
	if (error || broken)
	{
		float reach = error ? kErrorJitter : kBrokenJitter;
		unsigned int roll = Hash(step, 99u);
		jitterX = ((int)(roll % 5) - 2) * reach * 0.5f;
		jitterY = ((int)((roll >> 4) % 3) - 1) * reach * 0.5f;
	}

	const Palette::Color& ink = (line.style == Dialogue::Style::Thought) ? Palette::kUiNotch
		: ((error && Hash(step, 3u) % 4 == 0) ? Palette::kAsh : Palette::kUiBone);
	text->Draw(TextRenderer::Face::Body, m_Shown.c_str(), left + kMargin + jitterX, top - 44.f + jitterY,
		ink, 1.f, TextRenderer::Align::Left, kBoxWidth - kMargin * 2.f);

	if (IsChoosing())
	{
		for (int i = 0; i < Current().choiceCount; i++)
		{
			float x, y;
			ChoiceCenter(i, windowSizeY, x, y);
			bool chosen = (i == m_Choice);
			const Palette::Color& edge = chosen ? Palette::kUiBone : Palette::kUiFrame;
			renderer->DrawSolidRect(x, y, kChoiceWidth + kBoxBorder, kChoiceHeight + kBoxBorder, edge.r, edge.g, edge.b, 1.f);
			renderer->DrawSolidRect(x, y, kChoiceWidth, kChoiceHeight,
				Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 0.97f);
			text->Draw(TextRenderer::Face::Body, Current().choices[i].text, x, y - 7.f, Palette::kUiBone,
				chosen ? 1.f : 0.6f, TextRenderer::Align::Center);
		}
		text->Draw(TextRenderer::Face::Small, "W/S 고르기 · F 결정", right - kMargin, bottom + 14.f,
			Palette::kUiBone, 0.6f, TextRenderer::Align::Right);
	}
	else if (IsLineWritten())
	{
		// Waiting on him: a mark that breathes.
		float alpha = 0.45f + 0.45f * sinf(m_Time * 5.f);
		text->Draw(TextRenderer::Face::Small, "F ▼", right - kMargin, bottom + 14.f, Palette::kUiBone, alpha,
			TextRenderer::Align::Right);
	}
}
