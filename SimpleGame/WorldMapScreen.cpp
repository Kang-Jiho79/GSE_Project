#include "stdafx.h"
#include "WorldMapScreen.h"
#include "Renderer.h"
#include "TextRenderer.h"
#include "Palette.h"
#include <cmath>

namespace
{
	const float kPanelWidth = 780.f;
	const float kPanelHeight = 520.f;
	const float kPanelBorder = 4.f;
	const float kPanelY = 10.f;

	// How far the map's -1..1 spreads across the panel.
	const float kSpanX = 340.f;
	const float kSpanY = 200.f;

	const float kHitRadius = 16.f;
	const float kLinkWidth = 2.f;
	const float kLabelDrop = 24.f;

	// Unknown places are only a faint mark; places within reach are shown,
	// those he has been to more strongly.
	const float kUnknownShade = 0.3f;
	const float kReachableShade = 0.7f;

	const Palette::Color& StyleColor(Regions::Style style)
	{
		switch (style)
		{
		case Regions::Style::Light:
		{
			return Palette::kVesselLight;
		}
		case Regions::Style::Dark:
		{
			return Palette::kVesselDark;
		}
		case Regions::Style::Ashen:
		{
			return Palette::kAsh;
		}
		case Regions::Style::Borderborn:
		{
			return Palette::kUiBone;
		}
		default:
		{
			return Palette::kPipEarned;
		}
		}
	}
}

void WorldMapScreen::Open(int currentRegion, const bool* visited)
{
	m_Open = true;
	m_Current = currentRegion;
	for (int i = 0; i < Regions::Count; i++)
	{
		m_Visited[i] = visited[i];
	}
}

void WorldMapScreen::Close()
{
	m_Open = false;
}

bool WorldMapScreen::IsOpen() const
{
	return m_Open;
}

bool WorldMapScreen::IsKnown(int region) const
{
	return region == m_Current || m_Visited[region] || Regions::AreLinked(m_Current, region);
}

bool WorldMapScreen::CanTravelTo(int region) const
{
	return region != m_Current && (m_Visited[region] || Regions::AreLinked(m_Current, region));
}

void WorldMapScreen::NodePosition(int region, float& x, float& y) const
{
	const Regions::Info& info = Regions::Get(region);
	x = info.mapX * kSpanX;
	y = kPanelY + info.mapY * kSpanY;
}

int WorldMapScreen::RegionAt(float x, float y) const
{
	for (int region = 0; region < Regions::Count; region++)
	{
		if (!CanTravelTo(region))
		{
			continue;
		}

		float nodeX, nodeY;
		NodePosition(region, nodeX, nodeY);
		float dx = x - nodeX;
		float dy = y - nodeY;
		if (dx * dx + dy * dy <= kHitRadius * kHitRadius)
		{
			return region;
		}
	}
	return -1;
}

void WorldMapScreen::Render(Renderer* renderer, TextRenderer* text, int windowSizeX, int windowSizeY,
	float mouseX, float mouseY) const
{
	if (!m_Open)
	{
		return;
	}

	// The world behind sinks away, and the map lies over it.
	renderer->DrawSolidRect(0.f, 0.f, (float)windowSizeX, (float)windowSizeY,
		Palette::kVoid.r, Palette::kVoid.g, Palette::kVoid.b, 0.55f);
	renderer->DrawSolidRect(0.f, kPanelY, kPanelWidth + kPanelBorder * 2.f, kPanelHeight + kPanelBorder * 2.f,
		Palette::kUiFrame.r, Palette::kUiFrame.g, Palette::kUiFrame.b, 1.f);
	renderer->DrawSolidRect(0.f, kPanelY, kPanelWidth, kPanelHeight,
		Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 0.97f);

	for (int i = 0; i < Regions::LinkCount(); i++)
	{
		int a, b;
		Regions::GetLink(i, a, b);
		DrawLink(renderer, a, b);
	}

	int hovered = RegionAt(mouseX, mouseY);
	for (int region = 0; region < Regions::Count; region++)
	{
		DrawNode(renderer, text, region, region == hovered);
	}

	float top = kPanelY + kPanelHeight / 2.f;
	float bottom = kPanelY - kPanelHeight / 2.f;
	text->Draw(TextRenderer::Face::Title, "세계", 0.f, top - 46.f, Palette::kUiBone, 1.f, TextRenderer::Align::Center);
	text->Draw(TextRenderer::Face::Small, "클릭: 이동   ·   F / Esc: 닫기", 0.f, bottom + 14.f,
		Palette::kUiBone, 0.7f, TextRenderer::Align::Center);
}

void WorldMapScreen::DrawLink(Renderer* renderer, int a, int b) const
{
	float ax, ay, bx, by;
	NodePosition(a, ax, ay);
	NodePosition(b, bx, by);

	float dx = bx - ax;
	float dy = by - ay;
	float length = sqrtf(dx * dx + dy * dy);
	if (length < 0.001f)
	{
		return;
	}

	// A road between two places he knows is drawn plainly; one leading into
	// the unknown only faintly.
	bool known = IsKnown(a) && IsKnown(b);
	float shade = known ? 1.6f : 0.9f;
	float alpha = known ? 1.f : 0.5f;

	renderer->DrawSolidQuad((ax + bx) * 0.5f, (ay + by) * 0.5f, dx, dy,
		-dy / length * kLinkWidth, dx / length * kLinkWidth,
		Palette::kUiFrame.r * shade, Palette::kUiFrame.g * shade, Palette::kUiFrame.b * shade, alpha);
}

void WorldMapScreen::DrawNode(Renderer* renderer, TextRenderer* text, int region, bool hovered) const
{
	const Regions::Info& info = Regions::Get(region);
	float x, y;
	NodePosition(region, x, y);

	bool current = (region == m_Current);
	bool known = IsKnown(region);

	float shade = 1.f;
	if (!known)
	{
		shade = kUnknownShade;
	}
	else if (!m_Visited[region] && !current)
	{
		shade = kReachableShade;
	}

	const Palette::Color& style = StyleColor(info.style);
	float r = style.r * shade;
	float g = style.g * shade;
	float b = style.b * shade;

	// Where he stands, and where the cursor would take him, are ringed.
	if (current || hovered)
	{
		renderer->DrawSolidDiamond(x, y, 34.f, 34.f, Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b,
			current ? 1.f : 0.6f);
		renderer->DrawSolidDiamond(x, y, 29.f, 29.f, Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 1.f);
	}

	switch (info.kind)
	{
	case Regions::Kind::Village:
	{
		renderer->DrawSolidDiamond(x, y, 14.f, 14.f, r, g, b, 1.f);
		break;
	}
	case Regions::Kind::Field:
	{
		renderer->DrawSolidRect(x, y, 11.f, 11.f, r, g, b, 1.f);
		break;
	}
	case Regions::Kind::Gate:
	{
		renderer->DrawSolidRect(x, y, 7.f, 16.f, r, g, b, 1.f);
		break;
	}
	case Regions::Kind::Capital:
	{
		renderer->DrawSolidDiamond(x, y, 22.f, 22.f, r, g, b, 1.f);
		renderer->DrawSolidDiamond(x, y, 9.f, 9.f, Palette::kUiWell.r, Palette::kUiWell.g, Palette::kUiWell.b, 1.f);
		break;
	}
	}

	if (known)
	{
		text->Draw(TextRenderer::Face::Small, info.name, x, y - kLabelDrop,
			Palette::kUiBone, (m_Visited[region] || current) ? 1.f : 0.7f, TextRenderer::Align::Center);
	}
}
