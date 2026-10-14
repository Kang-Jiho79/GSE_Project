#include "stdafx.h"
#include "WorldMapScreen.h"
#include "Renderer.h"
#include "TextRenderer.h"
#include "Palette.h"
#include <cmath>
#include <string>

namespace
{
	const float kPanelWidth = 780.f;
	const float kPanelHeight = 560.f;
	const float kPanelBorder = 4.f;
	const float kPanelY = 10.f;

	// The board: five squares across and seven down, centred in the panel
	// a little below its title.
	const int kColumns = 5;
	const int kRows = 7;
	const float kSquareWidth = 120.f;
	const float kSquareHeight = 58.f;
	const float kSquareGap = 3.f;
	const float kBoardDrop = 18.f;

	const float kWallWidth = 5.f;
	const float kFrame = 3.f;          // the ring round where he is, or where the cursor would take him
	const float kIcon = 9.f;
	const float kMarker = 12.f;

	// Unknown places are only a dark square; places within reach are shown,
	// those he has been to more strongly.
	const float kUnknownShade = 0.18f;
	const float kReachableShade = 0.55f;

	const Palette::Color& StyleColor(Regions::Style style)
	{
		switch (style)
		{
		case Regions::Style::Light:
		{
			return Palette::kMapLight;
		}
		case Regions::Style::Dark:
		{
			return Palette::kMapDark;
		}
		case Regions::Style::Ashen:
		{
			return Palette::kMapAshen;
		}
		case Regions::Style::Borderborn:
		{
			return Palette::kMapBorder;
		}
		default:
		{
			return Palette::kMapMixed;
		}
		}
	}

	// The place in a square of the board, or -1.
	int RegionInSquare(int col, int row)
	{
		for (int region = 0; region < Regions::Count; region++)
		{
			const Regions::Info& info = Regions::Get(region);
			if (!Regions::IsOffMap(region) && info.cellCol == col && info.cellRow == row)
			{
				return region;
			}
		}
		return -1;
	}
}

void WorldMapScreen::Open(int currentRegion, const bool* visited, float playerCol, float playerRow, bool travel)
{
	m_Open = true;
	m_Travel = travel;
	m_Current = currentRegion;
	m_PlayerCol = playerCol;
	m_PlayerRow = playerRow;
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
	if (Regions::IsOffMap(region))
	{
		return false;
	}
	return region == m_Current || m_Visited[region] || Regions::AreLinked(m_Current, region);
}

bool WorldMapScreen::CanTravelTo(int region) const
{
	if (!m_Travel || Regions::IsOffMap(region))
	{
		return false;
	}
	return region != m_Current && (m_Visited[region] || Regions::AreLinked(m_Current, region));
}

void WorldMapScreen::NodePosition(int region, float& x, float& y) const
{
	const Regions::Info& info = Regions::Get(region);
	x = (info.cellCol - (kColumns - 1) * 0.5f) * (kSquareWidth + kSquareGap);
	y = kPanelY - kBoardDrop - (info.cellRow - (kRows - 1) * 0.5f) * (kSquareHeight + kSquareGap);
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
		if (fabsf(x - nodeX) <= kSquareWidth * 0.5f && fabsf(y - nodeY) <= kSquareHeight * 0.5f)
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

	int hovered = RegionAt(mouseX, mouseY);
	for (int region = 0; region < Regions::Count; region++)
	{
		if (!Regions::IsOffMap(region))
		{
			DrawSquare(renderer, text, region, region == hovered);
		}
	}
	DrawWalls(renderer);
	DrawPlayer(renderer);

	float top = kPanelY + kPanelHeight / 2.f;
	float bottom = kPanelY - kPanelHeight / 2.f;
	text->Draw(TextRenderer::Face::Title, "세계", 0.f, top - 34.f, Palette::kUiBone, 1.f, TextRenderer::Align::Center);

	// Where he is, and what he can do here.
	std::string here = "지금 있는 곳  ·  ";
	here += Regions::Get(m_Current).name;
	text->Draw(TextRenderer::Face::Small, here.c_str(), -kPanelWidth / 2.f + 24.f, top - 30.f,
		Palette::kUiBone, 0.85f, TextRenderer::Align::Left);
	const char* hint = m_Travel ? "클릭: 이동   ·   M / F / Esc: 닫기"
		: "M / F / Esc: 닫기   ·   웨이스톤에서 열면 이동할 수 있다";
	text->Draw(TextRenderer::Face::Small, hint, 0.f, bottom + 12.f, Palette::kUiBone, 0.7f, TextRenderer::Align::Center);
}

void WorldMapScreen::DrawSquare(Renderer* renderer, TextRenderer* text, int region, bool hovered) const
{
	const Regions::Info& info = Regions::Get(region);
	float x, y;
	NodePosition(region, x, y);

	bool current = (region == m_Current);
	bool known = IsKnown(region);

	// Where he stands, and where the cursor would take him, are ringed.
	if (current || hovered)
	{
		renderer->DrawSolidRect(x, y, kSquareWidth + kFrame * 2.f, kSquareHeight + kFrame * 2.f,
			Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b, current ? 1.f : 0.6f);
	}

	float shade = 1.f;
	if (!known)
	{
		shade = kUnknownShade;
	}
	else if (!m_Visited[region] && !current)
	{
		shade = kReachableShade;
	}
	const Palette::Color& ground = StyleColor(info.style);
	renderer->DrawSolidRect(x, y, kSquareWidth, kSquareHeight, ground.r * shade, ground.g * shade, ground.b * shade, 1.f);

	if (!known)
	{
		text->Draw(TextRenderer::Face::Small, "?", x, y - 5.f, Palette::kUiBone, 0.35f, TextRenderer::Align::Center);
		return;
	}

	// What kind of place it is, in its corner: a village's diamond, a
	// capital's double one, a gate's bar.
	float iconX = x - kSquareWidth / 2.f + 10.f;
	float iconY = y + kSquareHeight / 2.f - 10.f;
	const Palette::Color& mark = Palette::kUiBone;
	switch (info.kind)
	{
	case Regions::Kind::Village:
	{
		renderer->DrawSolidDiamond(iconX, iconY, kIcon, kIcon, mark.r, mark.g, mark.b, 0.8f);
		break;
	}
	case Regions::Kind::Capital:
	{
		renderer->DrawSolidDiamond(iconX, iconY, kIcon * 1.6f, kIcon * 1.6f, mark.r, mark.g, mark.b, 0.9f);
		renderer->DrawSolidDiamond(iconX, iconY, kIcon * 0.6f, kIcon * 0.6f, Palette::kUiWell.r, Palette::kUiWell.g,
			Palette::kUiWell.b, 1.f);
		break;
	}
	case Regions::Kind::Gate:
	{
		renderer->DrawSolidRect(iconX, iconY, kIcon * 0.5f, kIcon * 1.4f, mark.r, mark.g, mark.b, 0.8f);
		break;
	}
	default:
	{
		break;
	}
	}

	text->Draw(TextRenderer::Face::Small, info.name, x, y - 5.f,
		Palette::kUiBone, (m_Visited[region] || current) ? 1.f : 0.7f, TextRenderer::Align::Center);
}

void WorldMapScreen::DrawWalls(Renderer* renderer) const
{
	// Between neighbouring squares he cannot walk across: a band of fog.
	for (int row = 0; row < kRows; row++)
	{
		for (int col = 0; col < kColumns; col++)
		{
			int here = RegionInSquare(col, row);
			if (here < 0)
			{
				continue;
			}
			float x, y;
			NodePosition(here, x, y);
			int east = RegionInSquare(col + 1, row);
			if (east >= 0 && !Regions::AreLinked(here, east))
			{
				renderer->DrawSolidRect(x + (kSquareWidth + kSquareGap) * 0.5f, y, kWallWidth, kSquareHeight + kSquareGap,
					Palette::kAshVeil.r, Palette::kAshVeil.g, Palette::kAshVeil.b, 1.f);
			}
			int south = RegionInSquare(col, row + 1);
			if (south >= 0 && !Regions::AreLinked(here, south))
			{
				renderer->DrawSolidRect(x, y - (kSquareHeight + kSquareGap) * 0.5f, kSquareWidth + kSquareGap, kWallWidth,
					Palette::kAshVeil.r, Palette::kAshVeil.g, Palette::kAshVeil.b, 1.f);
			}
		}
	}
}

void WorldMapScreen::DrawPlayer(Renderer* renderer) const
{
	if (Regions::IsOffMap(m_Current))
	{
		return;
	}

	// Exactly where in its square he stands.
	float minCol, minRow, maxCol, maxRow;
	Regions::Bounds(m_Current, minCol, minRow, maxCol, maxRow);
	float acrossShare = (m_PlayerCol - minCol) / (maxCol - minCol);
	float downShare = (m_PlayerRow - minRow) / (maxRow - minRow);
	acrossShare = (acrossShare < 0.f) ? 0.f : ((acrossShare > 1.f) ? 1.f : acrossShare);
	downShare = (downShare < 0.f) ? 0.f : ((downShare > 1.f) ? 1.f : downShare);

	float x, y;
	NodePosition(m_Current, x, y);
	x += (acrossShare - 0.5f) * kSquareWidth;
	y -= (downShare - 0.5f) * kSquareHeight;

	renderer->DrawSolidDiamond(x, y, kMarker + 5.f, kMarker + 5.f, Palette::kMapPlayerRim.r, Palette::kMapPlayerRim.g,
		Palette::kMapPlayerRim.b, 1.f);
	renderer->DrawSolidDiamond(x, y, kMarker, kMarker, Palette::kMapPlayer.r, Palette::kMapPlayer.g, Palette::kMapPlayer.b, 1.f);
}
