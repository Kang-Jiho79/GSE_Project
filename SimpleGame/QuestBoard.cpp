#include "stdafx.h"
#include "QuestBoard.h"
#include "Quests.h"
#include "Regions.h"
#include "Renderer.h"
#include "TextRenderer.h"
#include "PlayerStatus.h"
#include "Progress.h"
#include "Palette.h"

namespace
{
	const float kPanelWidth = 680.f;
	const float kPanelHeight = 500.f;
	const float kPanelBorder = 4.f;
	const float kMargin = 40.f;

	// Each request's card, top to bottom.
	const float kFirstCardY = 120.f; // from the panel's centre
	const float kCardHeight = 110.f;
	const float kCardGap = 8.f;
	const float kCardInset = 14.f;
}

void QuestBoard::Open(int region, const Progress& progress)
{
	m_Open = true;
	m_Region = region;
	m_Count = Quests::OnBoard(region, progress, m_Ids, kMaxRequests);
	m_Selected = 0;
}

void QuestBoard::Close()
{
	m_Open = false;
}

bool QuestBoard::IsOpen() const
{
	return m_Open;
}

void QuestBoard::Move(int step)
{
	if (m_Count == 0)
	{
		return;
	}
	m_Selected = (m_Selected + step + m_Count) % m_Count;
}

bool QuestBoard::Confirm(PlayerStatus& status, Progress& progress, std::string& notice, int& script, int& refresh)
{
	script = -1;
	refresh = -1;
	if (m_Count == 0)
	{
		return false;
	}
	int id = m_Ids[m_Selected];
	switch (Quests::StageOf(progress, id))
	{
	case Quests::Unseen:
	{
		Quests::Accept(id, progress);
		notice = std::string("의뢰 수락: ") + Quests::Get(id).title;
		refresh = Quests::Get(id).place;
		return true;
	}
	case Quests::Ready:
	{
		notice = Quests::Claim(id, status, progress, script);
		return true;
	}
	default:
	{
		return false;
	}
	}
}

void QuestBoard::Render(Renderer* renderer, TextRenderer* text, const Progress& progress, int windowSizeX, int windowSizeY) const
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

	std::string title = std::string(Regions::Get(m_Region).name) + " 게시판";
	text->Draw(TextRenderer::Face::Title, title.c_str(), 0.f, top - 50.f, Palette::kUiBone, 1.f, TextRenderer::Align::Center);

	if (m_Count == 0)
	{
		text->Draw(TextRenderer::Face::Body, "붙은 의뢰가 없다.", 0.f, 0.f, Palette::kUiBone, 0.8f, TextRenderer::Align::Center);
	}

	for (int i = 0; i < m_Count; i++)
	{
		int id = m_Ids[i];
		const Quests::Info& info = Quests::Get(id);
		Quests::Stage stage = Quests::StageOf(progress, id);
		float cardY = kFirstCardY - i * (kCardHeight + kCardGap);

		// The one picked stands out; one paid off sinks back.
		bool picked = (i == m_Selected);
		if (picked)
		{
			renderer->DrawSolidRect(0.f, cardY - kCardHeight / 2.f + 18.f, kPanelWidth - kMargin + 8.f, kCardHeight + 4.f,
				Palette::kUiNotch.r, Palette::kUiNotch.g, Palette::kUiNotch.b, 1.f);
		}
		renderer->DrawSolidRect(0.f, cardY - kCardHeight / 2.f + 18.f, kPanelWidth - kMargin, kCardHeight,
			Palette::kUiInk.r, Palette::kUiInk.g, Palette::kUiInk.b, 1.f);

		float fade = (stage == Quests::Done) ? 0.45f : 1.f;
		float textLeft = left + kCardInset - 20.f;
		float textRight = right - kCardInset + 20.f;
		text->Draw(TextRenderer::Face::Body, info.title, textLeft, cardY, Palette::kUiBone, fade, TextRenderer::Align::Left);

		std::string standing;
		switch (stage)
		{
		case Quests::Unseen:
		{
			standing = "수락 (F)";
			break;
		}
		case Quests::Ready:
		{
			standing = "보고 (F)";
			break;
		}
		case Quests::Active:
		{
			standing = "진행 중  " + Quests::Standing(id, progress);
			break;
		}
		default:
		{
			standing = "완료";
			break;
		}
		}
		const Palette::Color& standingColor = (stage == Quests::Ready) ? Palette::kMapGoal : Palette::kUiNotch;
		text->Draw(TextRenderer::Face::Small, standing.c_str(), textRight, cardY, standingColor, fade, TextRenderer::Align::Right);

		text->Draw(TextRenderer::Face::Small, info.description, textLeft, cardY - 26.f, Palette::kUiBone, 0.8f * fade,
			TextRenderer::Align::Left, textRight - textLeft);
		std::string goal = std::string("목표  ·  ") + info.goal;
		if (info.kind == Quests::Kind::Hunt)
		{
			goal += " " + std::to_string(info.target);
		}
		text->Draw(TextRenderer::Face::Small, goal.c_str(), textLeft, cardY - 66.f, Palette::kUiBone, 0.9f * fade,
			TextRenderer::Align::Left);
		std::string reward = "보상  ·  " + Quests::Reward(id);
		text->Draw(TextRenderer::Face::Small, reward.c_str(), textLeft, cardY - 88.f, Palette::kMapGoal, 0.9f * fade,
			TextRenderer::Align::Left);
	}

	text->Draw(TextRenderer::Face::Small, "W / S: 고르기   ·   F: 수락 / 보고   ·   Esc: 닫기", 0.f, bottom + 16.f,
		Palette::kUiBone, 0.6f, TextRenderer::Align::Center);
}
