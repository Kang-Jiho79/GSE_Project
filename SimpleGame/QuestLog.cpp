#include "stdafx.h"
#include "QuestLog.h"
#include "Quests.h"
#include "Regions.h"
#include "Renderer.h"
#include "TextRenderer.h"
#include "Progress.h"
#include "Palette.h"
#include <string>

namespace
{
	const float kPanelWidth = 640.f;
	const float kPanelHeight = 600.f;
	const float kPanelBorder = 4.f;
	const float kMargin = 44.f;

	const float kRowSpacing = 28.f;
}

void QuestLog::Open()
{
	m_Open = true;
}

void QuestLog::Close()
{
	m_Open = false;
}

bool QuestLog::IsOpen() const
{
	return m_Open;
}

void QuestLog::Render(Renderer* renderer, TextRenderer* text, const Progress& progress, int windowSizeX, int windowSizeY) const
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
	float right = kPanelWidth / 2.f - kMargin;

	text->Draw(TextRenderer::Face::Title, "의뢰", 0.f, top - 50.f, Palette::kUiBone, 1.f, TextRenderer::Align::Center);

	// The main road.
	float y = top - 100.f;
	text->Draw(TextRenderer::Face::Small, "목표", left, y, Palette::kMapGoal, 1.f, TextRenderer::Align::Left);
	y -= kRowSpacing;
	const char* goal = Quests::MainGoal(progress);
	if (goal != nullptr)
	{
		text->Draw(TextRenderer::Face::Body, goal, left, y, Palette::kUiBone, 1.f, TextRenderer::Align::Left);
		y -= kRowSpacing;
		text->Draw(TextRenderer::Face::Small, Quests::MainDetail(progress), left, y, Palette::kUiBone, 0.75f,
			TextRenderer::Align::Left, right - left);
		y -= kRowSpacing * 2.f;
	}
	else
	{
		text->Draw(TextRenderer::Face::Body, "가야 할 곳은 스스로 정해야 한다.", left, y, Palette::kUiBone, 0.8f,
			TextRenderer::Align::Left);
		y -= kRowSpacing * 2.f;
	}

	// The requests under way, and those waiting to be reported.
	text->Draw(TextRenderer::Face::Small, "진행 중인 의뢰", left, y, Palette::kUiNotch, 1.f, TextRenderer::Align::Left);
	y -= kRowSpacing;
	int open = 0;
	int done = 0;
	for (int id = 0; id < Quests::Count; id++)
	{
		Quests::Stage stage = Quests::StageOf(progress, id);
		if (stage == Quests::Done)
		{
			done++;
			continue;
		}
		if (stage != Quests::Active && stage != Quests::Ready)
		{
			continue;
		}
		const Quests::Info& info = Quests::Get(id);
		std::string line = std::string(info.title) + "  ·  " + info.goal;
		text->Draw(TextRenderer::Face::Body, line.c_str(), left, y, Palette::kUiBone, 1.f, TextRenderer::Align::Left);
		std::string standing = (stage == Quests::Ready)
			? std::string(Regions::Get(info.board).name) + " 게시판에 보고"
			: Quests::Standing(id, progress);
		const Palette::Color& color = (stage == Quests::Ready) ? Palette::kMapGoal : Palette::kUiNotch;
		text->Draw(TextRenderer::Face::Small, standing.c_str(), right, y, color, 1.f, TextRenderer::Align::Right);
		y -= kRowSpacing;
		open++;
	}
	if (open == 0)
	{
		text->Draw(TextRenderer::Face::Body, "없음. 마을 게시판에서 의뢰를 받을 수 있다.", left, y, Palette::kUiBone, 0.6f,
			TextRenderer::Align::Left);
		y -= kRowSpacing;
	}

	std::string finished = "끝낸 의뢰  " + std::to_string(done) + " / " + std::to_string((int)Quests::Count);
	text->Draw(TextRenderer::Face::Small, finished.c_str(), left, bottom + 52.f, Palette::kUiNotch, 1.f,
		TextRenderer::Align::Left);
	text->Draw(TextRenderer::Face::Small, "J / F / Esc: 닫기", 0.f, bottom + 16.f, Palette::kUiBone, 0.6f,
		TextRenderer::Align::Center);
}
