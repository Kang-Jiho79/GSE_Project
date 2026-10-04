#include "stdafx.h"
#include "EndingScreen.h"
#include "Renderer.h"
#include "TextRenderer.h"
#include "Dialogue.h"
#include "Palette.h"
#include <cmath>

namespace
{
	const float kFadeIn = 1.2f;
	const float kWrap = 640.f;

	struct Mood
	{
		const Palette::Color* back;
		const Palette::Color* name;
	};

	// In the order of Dialogue::EndingId.
	const Mood kMoods[Dialogue::EndingCount] =
	{
		{ &Palette::kEndingSaviorBack, &Palette::kEndingSaviorName },
		{ &Palette::kEndingAshBack, &Palette::kEndingAshName },
		{ &Palette::kEndingSchemeBack, &Palette::kEndingSchemeName },
		{ &Palette::kEndingTrueBack, &Palette::kEndingTrueName },
	};
}

void EndingScreen::Open(int ending, bool firstTime)
{
	m_Open = true;
	m_Ending = ending;
	m_FirstTime = firstTime;
	m_Page = 0;
	m_PageTime = 0.f;
	m_Finished = false;
}

void EndingScreen::Close()
{
	m_Open = false;
}

bool EndingScreen::IsOpen() const
{
	return m_Open;
}

bool EndingScreen::IsFinished() const
{
	return m_Finished;
}

int EndingScreen::StoryPages() const
{
	return Dialogue::Get(Dialogue::GetEnding(m_Ending).script).lineCount;
}

int EndingScreen::EpiloguePages() const
{
	int epilogue = Dialogue::GetEnding(m_Ending).epilogue;
	return (epilogue >= 0) ? Dialogue::Get(epilogue).lineCount : 0;
}

int EndingScreen::PageCount() const
{
	return StoryPages() + EpiloguePages() + 1;
}

const char* EndingScreen::PageText(bool& epilogue) const
{
	const Dialogue::Ending& ending = Dialogue::GetEnding(m_Ending);
	epilogue = false;
	if (m_Page < StoryPages())
	{
		return Dialogue::Get(ending.script).lines[m_Page].text;
	}
	if (m_Page < StoryPages() + EpiloguePages())
	{
		epilogue = true;
		return Dialogue::Get(ending.epilogue).lines[m_Page - StoryPages()].text;
	}
	return nullptr;
}

void EndingScreen::Background(float& r, float& g, float& b) const
{
	const Palette::Color& back = *kMoods[m_Ending].back;
	r = back.r;
	g = back.g;
	b = back.b;

	// The true ending lightens page by page, toward a dawn by its end.
	if (m_Ending == Dialogue::True)
	{
		float pages = (float)(PageCount() - 1);
		float progress = (pages > 0.f) ? (m_Page + ((m_PageTime < kFadeIn) ? m_PageTime / kFadeIn : 1.f)) / pages : 1.f;
		if (progress > 1.f)
		{
			progress = 1.f;
		}
		const Palette::Color& dawn = Palette::kEndingTrueDawn;
		r += (dawn.r - r) * progress;
		g += (dawn.g - g) * progress;
		b += (dawn.b - b) * progress;
	}
}

void EndingScreen::Update(float deltaTime)
{
	if (m_Open)
	{
		m_PageTime += deltaTime;
	}
}

void EndingScreen::Advance()
{
	if (!m_Open)
	{
		return;
	}
	if (m_PageTime < kFadeIn)
	{
		m_PageTime = kFadeIn;
		return;
	}
	if (m_Page + 1 < PageCount())
	{
		m_Page++;
		m_PageTime = 0.f;
		return;
	}
	m_Finished = true;
}

void EndingScreen::Render(Renderer* renderer, TextRenderer* text, int windowSizeX, int windowSizeY) const
{
	if (!m_Open)
	{
		return;
	}

	float r, g, b;
	Background(r, g, b);
	renderer->DrawSolidRect(0.f, 0.f, (float)windowSizeX, (float)windowSizeY, r, g, b, 1.f);

	float alpha = (m_PageTime < kFadeIn) ? m_PageTime / kFadeIn : 1.f;
	const Dialogue::Ending& ending = Dialogue::GetEnding(m_Ending);
	bool epilogue = false;
	const char* page = PageText(epilogue);

	if (page != nullptr)
	{
		if (epilogue)
		{
			text->Draw(TextRenderer::Face::Small, "그 뒤", 0.f, 70.f, Palette::kUiNotch, alpha, TextRenderer::Align::Center);
		}
		text->Draw(TextRenderer::Face::Body, page, -kWrap * 0.5f, 10.f, Palette::kUiBone, alpha,
			TextRenderer::Align::Left, kWrap);
	}
	else
	{
		text->Draw(TextRenderer::Face::Title, ending.name, 0.f, 30.f, *kMoods[m_Ending].name, alpha,
			TextRenderer::Align::Center);
		text->Draw(TextRenderer::Face::Small, "끝", 0.f, -6.f, Palette::kUiNotch, alpha, TextRenderer::Align::Center);
		if (m_FirstTime)
		{
			text->Draw(TextRenderer::Face::Small, "새로운 결말", 0.f, 66.f, *kMoods[m_Ending].name, alpha * 0.9f,
				TextRenderer::Align::Center);
		}
		text->Draw(TextRenderer::Face::Body, "플레이해 주셔서 감사합니다", 0.f, -70.f, Palette::kUiBone, alpha * 0.7f,
			TextRenderer::Align::Center);
	}

	if (m_PageTime >= kFadeIn)
	{
		const char* prompt = (m_Page + 1 < PageCount()) ? "F ▼" : "F  처음으로";
		float pulse = 0.4f + 0.3f * sinf(m_PageTime * 3.f);
		text->Draw(TextRenderer::Face::Small, prompt, 0.f, -windowSizeY / 2.f + 30.f, Palette::kUiBone, pulse,
			TextRenderer::Align::Center);
	}
}
