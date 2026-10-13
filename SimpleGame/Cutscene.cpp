#include "stdafx.h"
#include "Cutscene.h"
#include "Renderer.h"
#include "TextRenderer.h"
#include "WorldGrid.h"
#include "Palette.h"
#include "Tuning.h"
#include <string>

namespace
{
	float Clamp01(float value)
	{
		return (value < 0.f) ? 0.f : ((value > 1.f) ? 1.f : value);
	}

	// Slow out of the last shot, slow into the next.
	float Ease(float t)
	{
		t = Clamp01(t);
		return t * t * (3.f - 2.f * t);
	}

	float ClampZoom(float zoom)
	{
		if (zoom < Tuning::Cutscene::kMinZoom)
		{
			return Tuning::Cutscene::kMinZoom;
		}
		if (zoom > Tuning::Cutscene::kMaxZoom)
		{
			return Tuning::Cutscene::kMaxZoom;
		}
		return zoom;
	}

	// The skip's progress: a thin bar filling in the lower right.
	const float kSkipBarWidth = 90.f;
	const float kSkipBarHeight = 3.f;
	const float kSkipMargin = 40.f;
}

void Cutscene::Play(int id, float anchorX, float anchorY, float startX, float startY)
{
	m_Script = &Cutscenes::Get(id);
	m_Playing = m_Script->count > 0;
	m_AnchorX = anchorX;
	m_AnchorY = anchorY;
	m_PlayerX = startX;
	m_PlayerY = startY;
	m_X = startX;
	m_Y = startY;
	m_Zoom = 1.f;
	m_Letterbox = 0.f;
	m_SkipHeld = 0.f;
	m_Cues.clear();
	m_Conversation = -1;
	if (m_Playing)
	{
		BeginShot(0);
	}
}

bool Cutscene::IsPlaying() const
{
	return m_Playing;
}

const Cutscenes::Shot& Cutscene::Current() const
{
	return m_Script->shots[m_Index];
}

void Cutscene::Target(const Cutscenes::Shot& shot, float& x, float& y) const
{
	switch (shot.aim)
	{
	case Cutscenes::Aim::Player:
	{
		x = m_PlayerX;
		y = m_PlayerY;
		break;
	}
	case Cutscenes::Aim::Anchor:
	{
		float offsetX, offsetY;
		WorldGrid::IsoPosition(shot.col, shot.row, offsetX, offsetY);
		x = m_AnchorX + offsetX;
		y = m_AnchorY + offsetY;
		break;
	}
	default:
	{
		x = m_FromX;
		y = m_FromY;
		break;
	}
	}
}

void Cutscene::BeginShot(int index)
{
	if (index >= m_Script->count)
	{
		m_Playing = false;
		return;
	}

	m_Index = index;
	m_ShotTime = 0.f;
	m_FromX = m_X;
	m_FromY = m_Y;
	m_FromZoom = m_Zoom;

	const Cutscenes::Shot& shot = Current();
	if (shot.cut)
	{
		Target(shot, m_X, m_Y);
		m_FromX = m_X;
		m_FromY = m_Y;
		if (shot.zoom > 0.f)
		{
			m_Zoom = ClampZoom(shot.zoom);
			m_FromZoom = m_Zoom;
		}
	}

	if (shot.cue != Cutscenes::NoCue)
	{
		m_Cues.push_back(shot.cue);
	}
	if (shot.conversation >= 0)
	{
		m_Conversation = shot.conversation;
	}
}

void Cutscene::SkipAhead()
{
	for (int i = m_Index + 1; i < m_Script->count; i++)
	{
		const Cutscenes::Shot& shot = m_Script->shots[i];
		if (shot.conversation >= 0)
		{
			// A conversation is never skipped over - what is said in it can
			// change the story. Stop here and let it be read.
			BeginShot(i);
			return;
		}
		if (shot.cue != Cutscenes::NoCue)
		{
			m_Cues.push_back(shot.cue);
		}
	}
	m_Playing = false;
}

void Cutscene::Update(float deltaTime, float playerX, float playerY, bool skipHeld)
{
	if (!m_Playing)
	{
		return;
	}

	m_PlayerX = playerX;
	m_PlayerY = playerY;

	m_SkipHeld = skipHeld ? m_SkipHeld + deltaTime : 0.f;
	if (m_SkipHeld >= Tuning::Cutscene::kSkipHold)
	{
		m_SkipHeld = 0.f;
		SkipAhead();
		if (!m_Playing)
		{
			return;
		}
	}

	m_ShotTime += deltaTime;
	while (m_Playing && m_ShotTime >= Current().seconds)
	{
		float over = m_ShotTime - Current().seconds;
		BeginShot(m_Index + 1);
		m_ShotTime = over;
	}
	if (!m_Playing)
	{
		return;
	}

	const Cutscenes::Shot& shot = Current();
	float t = Ease(m_ShotTime / shot.seconds);
	float targetX, targetY;
	Target(shot, targetX, targetY);
	m_X = m_FromX + (targetX - m_FromX) * t;
	m_Y = m_FromY + (targetY - m_FromY) * t;
	if (shot.zoom > 0.f)
	{
		m_Zoom = m_FromZoom + (ClampZoom(shot.zoom) - m_FromZoom) * t;
	}

	float bars = shot.letterbox ? 1.f : 0.f;
	float step = deltaTime / Tuning::Cutscene::kLetterboxEase;
	if (m_Letterbox < bars)
	{
		m_Letterbox = (m_Letterbox + step > bars) ? bars : m_Letterbox + step;
	}
	else if (m_Letterbox > bars)
	{
		m_Letterbox = (m_Letterbox - step < bars) ? bars : m_Letterbox - step;
	}
}

float Cutscene::CameraX() const
{
	return m_X;
}

float Cutscene::CameraY() const
{
	return m_Y;
}

float Cutscene::Zoom() const
{
	return m_Playing ? m_Zoom : 1.f;
}

float Cutscene::TimeScale() const
{
	if (!m_Playing)
	{
		return 1.f;
	}
	switch (Current().clock)
	{
	case Cutscenes::Clock::Slow:
	{
		return Tuning::Cutscene::kSlowScale;
	}
	case Cutscenes::Clock::Stop:
	{
		return 0.f;
	}
	default:
	{
		return 1.f;
	}
	}
}

int Cutscene::TakeCue()
{
	if (m_Cues.empty())
	{
		return Cutscenes::NoCue;
	}
	int cue = m_Cues.front();
	m_Cues.erase(m_Cues.begin());
	return cue;
}

int Cutscene::TakeConversation()
{
	int conversation = m_Conversation;
	m_Conversation = -1;
	return conversation;
}

void Cutscene::Render(Renderer* renderer, int windowSizeX, int windowSizeY) const
{
	if (!m_Playing)
	{
		return;
	}

	const Cutscenes::Shot& shot = Current();
	if (shot.wash != Cutscenes::Wash::None)
	{
		float t = Clamp01(m_ShotTime / shot.seconds);
		float alpha = shot.washFrom + (shot.washTo - shot.washFrom) * t;
		const Palette::Color& color = (shot.wash == Cutscenes::Wash::White) ? Palette::kCutsceneWhite : Palette::kVoid;
		renderer->DrawSolidRect(0.f, 0.f, (float)windowSizeX, (float)windowSizeY, color.r, color.g, color.b, alpha);
	}

	if (m_Letterbox > 0.f)
	{
		// Each bar slides in from its edge.
		float height = windowSizeY * Tuning::Cutscene::kLetterboxShare * Ease(m_Letterbox);
		float edge = windowSizeY / 2.f;
		renderer->DrawSolidRect(0.f, edge - height / 2.f, (float)windowSizeX, height, 0.f, 0.f, 0.f, 1.f);
		renderer->DrawSolidRect(0.f, -edge + height / 2.f, (float)windowSizeX, height, 0.f, 0.f, 0.f, 1.f);
	}

	if (m_SkipHeld > 0.f)
	{
		float share = Clamp01(m_SkipHeld / Tuning::Cutscene::kSkipHold);
		float right = windowSizeX / 2.f - kSkipMargin;
		float y = -windowSizeY / 2.f + kSkipMargin;
		renderer->DrawSolidRect(right - kSkipBarWidth / 2.f, y, kSkipBarWidth, kSkipBarHeight,
			Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
		float filled = kSkipBarWidth * share;
		renderer->DrawSolidRect(right - kSkipBarWidth + filled / 2.f, y, filled, kSkipBarHeight,
			Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b, 1.f);
	}
}

void Cutscene::RenderText(TextRenderer* text, int windowSizeX, int windowSizeY) const
{
	if (!m_Playing)
	{
		return;
	}

	const Cutscenes::Shot& shot = Current();
	if (shot.caption != nullptr)
	{
		// In the middle of the lower bar, fading in over the shot's start.
		float bar = windowSizeY * Tuning::Cutscene::kLetterboxShare;
		float y = -windowSizeY / 2.f + bar / 2.f - 6.f;
		float alpha = Clamp01(m_ShotTime / Tuning::Cutscene::kCaptionFade);
		if (shot.speaker != nullptr)
		{
			std::string line = std::string(shot.speaker) + "  ·  " + shot.caption;
			text->Draw(TextRenderer::Face::Body, line.c_str(), 0.f, y, Palette::kUiBone, alpha, TextRenderer::Align::Center);
		}
		else
		{
			text->Draw(TextRenderer::Face::Body, shot.caption, 0.f, y, Palette::kUiBone, alpha, TextRenderer::Align::Center);
		}
	}

	if (m_SkipHeld > 0.f)
	{
		float right = windowSizeX / 2.f - kSkipMargin;
		float y = -windowSizeY / 2.f + kSkipMargin + 10.f;
		text->Draw(TextRenderer::Face::Small, "넘기기", right, y, Palette::kUiBone, 0.7f, TextRenderer::Align::Right);
	}
}
