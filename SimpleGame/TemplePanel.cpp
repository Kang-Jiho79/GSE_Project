#include "stdafx.h"
#include "TemplePanel.h"
#include "Renderer.h"
#include "TextRenderer.h"
#include "PlayerStatus.h"
#include "Palette.h"
#include "Tuning.h"
#include <cmath>
#include <cstdio>

namespace
{
	const float kPanelWidth = 560.f;
	const float kPanelHeight = 330.f;
	const float kPanelBorder = 4.f;

	const float kBarWidth = 380.f;
	const float kBarHeight = 12.f;
	const float kNeedleWidth = 3.f;
	const float kNeedleHeight = 22.f;

	// How it describes the lean: balanced below the first, leaning below the
	// warning, perilous below the allowance, the gray beyond it.
	const float kEvenShare = 0.08f;
}

void TemplePanel::Open(const std::string& placeName, bool ruined)
{
	m_Open = true;
	m_PlaceName = placeName;
	m_Ruined = ruined;
}

void TemplePanel::Close()
{
	m_Open = false;
}

bool TemplePanel::IsOpen() const
{
	return m_Open;
}

void TemplePanel::Render(Renderer* renderer, TextRenderer* text, const PlayerStatus& status,
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
	float left = -kPanelWidth / 2.f + 40.f;
	float right = kPanelWidth / 2.f - 40.f;

	std::string title = m_PlaceName + "의 신전";
	text->Draw(TextRenderer::Face::Title, title.c_str(), 0.f, top - 52.f, Palette::kUiBone, 1.f, TextRenderer::Align::Center);

	const char* opening = m_Ruined
		? "무너진 제단이지만, 신은 여전히 힘의 균형을 비춰준다."
		: "제단에 손을 얹자, 신이 힘의 균형을 비춰준다.";
	text->Draw(TextRenderer::Face::Body, opening, 0.f, top - 92.f, Palette::kUiBone, 0.85f, TextRenderer::Align::Center);

	// Dark on the left and light on the right, as in the vessel.
	char amount[64];
	snprintf(amount, sizeof(amount), "어둠의 힘  %d", (int)status.darkTotal);
	text->Draw(TextRenderer::Face::Body, amount, left, 20.f, Palette::kVesselDarkFull, 1.f, TextRenderer::Align::Left);
	snprintf(amount, sizeof(amount), "빛의 힘  %d", (int)status.lightTotal);
	text->Draw(TextRenderer::Face::Body, amount, right, 20.f, Palette::kVesselLightFull, 1.f, TextRenderer::Align::Right);

	DrawBalanceBar(renderer, status, -10.f);

	float lean = fabsf(status.BalanceRatio());
	const char* side = (status.Balance() > 0.f) ? "빛" : "어둠";
	char reading[128];
	if (lean < kEvenShare)
	{
		snprintf(reading, sizeof(reading), "두 힘이 고르게 맞서 있다.");
	}
	else if (lean < Tuning::Balance::kWarningShare)
	{
		snprintf(reading, sizeof(reading), "%s 쪽으로 조금 기울어 있다.", side);
	}
	else if (lean <= 1.f)
	{
		snprintf(reading, sizeof(reading), "%s 쪽으로 위태롭게 기울어 있다.", side);
	}
	else
	{
		snprintf(reading, sizeof(reading), "균형이 무너져, 잿빛이 스며들고 있다.");
	}
	const Palette::Color& readingColor = (lean > 1.f) ? Palette::kAsh : Palette::kUiBone;
	text->Draw(TextRenderer::Face::Body, reading, 0.f, -56.f, readingColor, 1.f, TextRenderer::Align::Center);

	text->Draw(TextRenderer::Face::Body, "상처가 아물고 여정이 기록되었다. 쓰러지면 이곳에서 다시 일어선다.",
		0.f, bottom + 52.f, Palette::kUiBone, 0.7f, TextRenderer::Align::Center);
	text->Draw(TextRenderer::Face::Small, "F / Esc: 닫기", 0.f, bottom + 16.f, Palette::kUiBone, 0.6f, TextRenderer::Align::Center);
}

void TemplePanel::DrawBalanceBar(Renderer* renderer, const PlayerStatus& status, float centerY) const
{
	float half = kBarWidth / 2.f;

	// A dim violet half and a dim gold half, meeting at the even point.
	renderer->DrawSolidRect(0.f, centerY, kBarWidth + 4.f, kBarHeight + 4.f,
		Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
	renderer->DrawSolidRect(-half / 2.f, centerY, half, kBarHeight,
		Palette::kVesselDark.r * 0.5f, Palette::kVesselDark.g * 0.5f, Palette::kVesselDark.b * 0.5f, 1.f);
	renderer->DrawSolidRect(half / 2.f, centerY, half, kBarHeight,
		Palette::kVesselLight.r * 0.5f, Palette::kVesselLight.g * 0.5f, Palette::kVesselLight.b * 0.5f, 1.f);

	// Marks where the warning starts on either side, and the even point.
	float warning = half * Tuning::Balance::kWarningShare;
	renderer->DrawSolidRect(-warning, centerY, 2.f, kBarHeight + 6.f,
		Palette::kUiNotch.r, Palette::kUiNotch.g, Palette::kUiNotch.b, 1.f);
	renderer->DrawSolidRect(warning, centerY, 2.f, kBarHeight + 6.f,
		Palette::kUiNotch.r, Palette::kUiNotch.g, Palette::kUiNotch.b, 1.f);
	renderer->DrawSolidRect(0.f, centerY, 1.f, kBarHeight,
		Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);

	float ratio = status.BalanceRatio();
	if (ratio > 1.f)
	{
		ratio = 1.f;
	}
	else if (ratio < -1.f)
	{
		ratio = -1.f;
	}
	float needleX = ratio * half;
	renderer->DrawSolidRect(needleX, centerY, kNeedleWidth + 2.f, kNeedleHeight + 2.f,
		Palette::kUiInk.r, Palette::kUiInk.g, Palette::kUiInk.b, 1.f);
	renderer->DrawSolidRect(needleX, centerY, kNeedleWidth, kNeedleHeight,
		Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b, 1.f);
}
