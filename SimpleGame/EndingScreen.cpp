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
}

void EndingScreen::Open(int ending)
{
	m_Open = true;
	m_Ending = ending;
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

int EndingScreen::PageCount() const
{
	const Dialogue::Ending& ending = Dialogue::GetEnding(m_Ending);
	return Dialogue::Get(ending.script).lineCount + 1;
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

	renderer->DrawSolidRect(0.f, 0.f, (float)windowSizeX, (float)windowSizeY,
		Palette::kVoid.r, Palette::kVoid.g, Palette::kVoid.b, 1.f);

	float alpha = (m_PageTime < kFadeIn) ? m_PageTime / kFadeIn : 1.f;
	const Dialogue::Ending& ending = Dialogue::GetEnding(m_Ending);
	const Dialogue::Script& script = Dialogue::Get(ending.script);

	if (m_Page < script.lineCount)
	{
		text->Draw(TextRenderer::Face::Body, script.lines[m_Page].text, -kWrap * 0.5f, 10.f, Palette::kUiBone, alpha,
			TextRenderer::Align::Left, kWrap);
	}
	else
	{
		text->Draw(TextRenderer::Face::Title, ending.name, 0.f, 20.f, Palette::kUiBone, alpha, TextRenderer::Align::Center);
		text->Draw(TextRenderer::Face::Small, "끝", 0.f, -20.f, Palette::kUiNotch, alpha, TextRenderer::Align::Center);
	}

	if (m_PageTime >= kFadeIn)
	{
		const char* prompt = (m_Page + 1 < PageCount()) ? "F ▼" : "F  처음으로";
		float pulse = 0.4f + 0.3f * sinf(m_PageTime * 3.f);
		text->Draw(TextRenderer::Face::Small, prompt, 0.f, -windowSizeY / 2.f + 30.f, Palette::kUiBone, pulse,
			TextRenderer::Align::Center);
	}
}
