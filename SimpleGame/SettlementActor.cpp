#include "stdafx.h"
#include "SettlementActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Alignment.h"
#include "Palette.h"
#include <cmath>

namespace
{
	// Half a tile's width and height on screen: one tile of footprint moves a
	// corner this far.
	const float kHalfW = WorldGrid::kTileWidth / 2.f;
	const float kHalfH = WorldGrid::kTileHeight / 2.f;

	const float kCapitalScale = 1.3f;
	const float kDoorShade = 0.05f;
	const float kAltarBobRate = 1.8f;

	struct Look
	{
		Palette::Color wall;
		Palette::Color roof;
		Palette::Color trim;
		Palette::Color paving;
	};

	// The trims carry what is left of each side's colour, the way the eroded's
	// emblems do.
	Look LookFor(Regions::Style style)
	{
		Look look;
		switch (style)
		{
		case Regions::Style::Light:
		{
			look.wall = { 0.44f, 0.43f, 0.42f };
			look.roof = { 0.40f, 0.36f, 0.27f };
			look.trim = { 0.62f, 0.52f, 0.30f };
			look.paving = { 0.26f, 0.25f, 0.25f };
			break;
		}
		case Regions::Style::Dark:
		{
			look.wall = { 0.17f, 0.16f, 0.20f };
			look.roof = { 0.11f, 0.10f, 0.13f };
			look.trim = { 0.36f, 0.28f, 0.55f };
			look.paving = { 0.13f, 0.12f, 0.15f };
			break;
		}
		case Regions::Style::Borderborn:
		{
			look.wall = { 0.23f, 0.19f, 0.15f };
			look.roof = { 0.26f, 0.23f, 0.16f };
			look.trim = { 0.40f, 0.38f, 0.34f };
			look.paving = { 0.17f, 0.16f, 0.15f };
			break;
		}
		default:
		{
			look.wall = { 0.27f, 0.27f, 0.28f };
			look.roof = { 0.22f, 0.22f, 0.23f };
			look.trim = { 0.35f, 0.35f, 0.36f };
			look.paving = { 0.18f, 0.18f, 0.19f };
			break;
		}
		}
		return look;
	}

	void Box(DrawList& out, float depth, float x, float baseY, float foot, float height, const Palette::Color& c)
	{
		out.AddBox(depth, x, baseY, foot, foot, height, c.r, c.g, c.b);
	}

	void Spike(DrawList& out, float depth, float x, float baseY, float height, float lean, const Palette::Color& c)
	{
		out.AddTaperedBox(depth, x, baseY, 0.12f, 0.12f, height, 0.1f, lean, 0.f, c.r, c.g, c.b);
	}

	// The middle of a footprint's right-hand face on screen, at ground level.
	void RightFace(float x, float baseY, float foot, float& outX, float& outY)
	{
		outX = x + foot * kHalfW * 0.5f;
		outY = baseY - foot * kHalfH * 0.5f;
	}

	void LeftFace(float x, float baseY, float foot, float& outX, float& outY)
	{
		outX = x - foot * kHalfW * 0.5f;
		outY = baseY - foot * kHalfH * 0.5f;
	}
}

SettlementActor::SettlementActor(const Settlement::Layout& layout, Regions::Style style, bool capital)
	: m_Layout(layout)
	, m_Style(style)
	, m_Capital(capital)
{
}

void SettlementActor::OnUpdate(const ActorContext& ctx)
{
	m_Time += ctx.deltaTime;
}

void SettlementActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	DrawSquare(out, ctx);

	for (size_t i = 0; i < m_Layout.buildings.size(); i++)
	{
		const Settlement::Building& building = m_Layout.buildings[i];

		float worldX, worldY;
		WorldGrid::IsoPosition(building.col, building.row, worldX, worldY);
		float x, y;
		ctx.ToScreen(worldX, worldY, x, y);

		float reach = building.footprint * kHalfW + 40.f;
		if (!ctx.IsOnScreen(x, y, reach, reach * 2.f))
		{
			continue;
		}

		float groundY = y + WorldGrid::GroundHeightAt(worldX, worldY);
		// Sorted by its nearest corner, so anyone walking in front of it is
		// drawn over it and anyone behind it is hidden.
		float depth = building.col + building.row + building.footprint;

		switch (building.type)
		{
		case Settlement::BuildingType::Temple:
		{
			DrawTemple(out, depth, x, groundY, building);
			break;
		}
		case Settlement::BuildingType::NoticeBoard:
		{
			DrawBoard(out, depth, x, groundY);
			break;
		}
		default:
		{
			DrawHouse(out, depth, x, groundY, building);
			break;
		}
		}
	}

	if (m_Layout.hasTemple)
	{
		DrawAltar(out, ctx);
	}
}

void SettlementActor::DrawSquare(DrawList& out, const ActorContext& ctx) const
{
	Look look = LookFor(m_Style);
	int radius = (int)ceilf(m_Layout.squareRadius);
	int centerCol = (int)(m_Layout.squareCol + 0.5f);
	int centerRow = (int)(m_Layout.squareRow + 0.5f);

	for (int row = centerRow - radius; row <= centerRow + radius; row++)
	{
		for (int col = centerCol - radius; col <= centerCol + radius; col++)
		{
			float dc = col - m_Layout.squareCol;
			float dr = row - m_Layout.squareRow;
			if (dc * dc + dr * dr > m_Layout.squareRadius * m_Layout.squareRadius)
			{
				continue;
			}

			unsigned int hash = WorldGrid::Hash(col + 919, row + 271);
			// In the gray's villages the paving is broken up, flags missing.
			if (m_Style == Regions::Style::Ashen && hash % 10 < 3)
			{
				continue;
			}

			float worldX, worldY;
			WorldGrid::IsoPosition((float)col, (float)row, worldX, worldY);
			float x, y;
			ctx.ToScreen(worldX, worldY, x, y);
			if (!ctx.IsOnScreen(x, y, kHalfW, kHalfH))
			{
				continue;
			}

			float grain = 0.9f + (hash % 20) * 0.01f;
			float groundY = y + WorldGrid::GroundHeightAt(worldX, worldY);
			// A little smaller than the tile, so the joints between flags show.
			out.AddDiamond((float)(col + row) + 0.02f, x, groundY, WorldGrid::kTileWidth - 4.f, WorldGrid::kTileHeight - 2.f,
				look.paving.r * grain, look.paving.g * grain, look.paving.b * grain, 1.f);
		}
	}
}

void SettlementActor::DrawHouse(DrawList& out, float depth, float x, float groundY, const Settlement::Building& building) const
{
	Look look = LookFor(m_Style);
	float f = building.footprint;
	int variant = building.variant;
	float scale = m_Capital ? kCapitalScale : 1.f;
	const Palette::Color door = { kDoorShade, kDoorShade, kDoorShade * 1.2f };

	float doorX, doorY;
	RightFace(x, groundY, f, doorX, doorY);

	switch (m_Style)
	{
	case Regions::Style::Light:
	{
		// Pale stone, tall, under a steep pointed roof with a gilt finial.
		float height = (20.f + (variant % 3) * 4.f) * scale;
		Box(out, depth, x, groundY, f, height, look.wall);
		out.AddRect(depth + 0.005f, doorX, doorY + 6.f, 6.f, 12.f, door.r, door.g, door.b, 1.f);
		float windowX, windowY;
		LeftFace(x, groundY, f, windowX, windowY);
		out.AddRect(depth + 0.005f, windowX, windowY + height * 0.6f, 4.f, 6.f, look.trim.r, look.trim.g, look.trim.b, 1.f);

		float roofHeight = f * 7.f;
		out.AddTaperedBox(depth + 0.01f, x, groundY + height, f * 1.12f, f * 1.12f, roofHeight, 0.1f, 0.f, 0.f,
			look.roof.r, look.roof.g, look.roof.b);
		out.AddDiamond(depth + 0.02f, x, groundY + height + roofHeight + 2.f, 4.f, 6.f,
			look.trim.r, look.trim.g, look.trim.b, 1.f);
		break;
	}
	case Regions::Style::Dark:
	{
		// Black stone, low and heavy, a flat slab for a roof with spikes at its
		// corners and a slit of old violet for a window.
		float height = (14.f + (variant % 3) * 3.f) * scale;
		Box(out, depth, x, groundY, f, height, look.wall);
		out.AddRect(depth + 0.005f, doorX, doorY + 5.f, 7.f, 10.f, door.r, door.g, door.b, 1.f);
		out.AddRect(depth + 0.005f, doorX + 6.f, doorY + height * 0.6f, 1.5f, 6.f, look.trim.r, look.trim.g, look.trim.b, 1.f);

		float slabTop = groundY + height + 3.f;
		Box(out, depth + 0.01f, x, groundY + height, f * 1.18f, 3.f, look.roof);
		Spike(out, depth + 0.02f, x - f * kHalfW * 0.95f, slabTop - 1.f, 9.f, -3.f, look.roof);
		Spike(out, depth + 0.02f, x + f * kHalfW * 0.95f, slabTop - 1.f, 9.f, 3.f, look.roof);
		break;
	}
	case Regions::Style::Borderborn:
	{
		// Scavenged boards under a sagging thatch, leaning as if hastily built.
		float height = (12.f + (variant % 3) * 2.f) * scale;
		Box(out, depth, x, groundY, f, height, look.wall);
		out.AddRect(depth + 0.005f, doorX, doorY + 5.f, 5.f, 10.f, door.r, door.g, door.b, 1.f);
		out.AddTaperedBox(depth + 0.01f, x + 2.f, groundY + height, f * 1.2f, f * 1.2f, 9.f, 0.35f, 3.f, 0.f,
			look.roof.r, look.roof.g, look.roof.b);
		break;
	}
	default:
	{
		// What the gray left: a broken stump of wall, two jagged corners still
		// standing, and the rest fallen.
		float stump = 5.f + (variant % 3) * 3.f;
		Box(out, depth, x, groundY, f, stump, look.wall);
		out.AddTaperedBox(depth + 0.01f, x - f * kHalfW * 0.8f, groundY - 2.f, 0.35f, 0.35f, 14.f + (variant % 4) * 3.f,
			0.6f, -2.f, 0.f, look.wall.r, look.wall.g, look.wall.b);
		out.AddTaperedBox(depth + 0.01f, x + f * kHalfW * 0.6f, groundY - 4.f, 0.3f, 0.3f, 10.f + (variant % 3) * 4.f,
			0.5f, 3.f, 0.f, look.wall.r, look.wall.g, look.wall.b);
		Box(out, depth + 0.02f, x + 8.f, groundY - f * 4.f, 0.4f, 4.f, look.roof);
		break;
	}
	}
}

void SettlementActor::DrawTemple(DrawList& out, float depth, float x, float groundY, const Settlement::Building& building) const
{
	Look look = LookFor(m_Style);
	float f = building.footprint;
	float scale = m_Capital ? kCapitalScale : 1.f;
	const Palette::Color door = { kDoorShade, kDoorShade, kDoorShade * 1.2f };

	float doorX, doorY;
	RightFace(x, groundY, f, doorX, doorY);

	const Palette::Color plinth = { look.wall.r * 0.85f, look.wall.g * 0.85f, look.wall.b * 0.85f };

	switch (m_Style)
	{
	case Regions::Style::Light:
	{
		// Raised on a plinth, tall, with a stepped roof and a spire.
		float wall = 30.f * scale;
		Box(out, depth, x, groundY, f * 1.15f, 4.f, plinth);
		Box(out, depth + 0.01f, x, groundY + 4.f, f, wall, look.wall);
		out.AddRect(depth + 0.015f, doorX, doorY + 4.f + 11.f * scale, 9.f, 22.f * scale, door.r, door.g, door.b, 1.f);

		float roofBase = groundY + 4.f + wall;
		out.AddTaperedBox(depth + 0.02f, x, roofBase, f * 1.08f, f * 1.08f, 10.f * scale, 0.5f, 0.f, 0.f,
			look.roof.r, look.roof.g, look.roof.b);
		out.AddTaperedBox(depth + 0.03f, x, roofBase + 10.f * scale, 0.35f, 0.35f, 28.f * scale, 0.1f, 0.f, 0.f,
			look.trim.r, look.trim.g, look.trim.b);

		// A sun-sign over the door, still holding a little of its light.
		out.SetGlow(0.4f);
		out.AddDiamond(depth + 0.02f, doorX, doorY + 4.f + 26.f * scale, 6.f, 8.f, look.trim.r, look.trim.g, look.trim.b, 1.f);
		out.SetGlow(0.f);
		break;
	}
	case Regions::Style::Dark:
	{
		// Low and broad, sunk behind a heavy slab, spiked at every corner,
		// the doorway a deep black mouth.
		float wall = 18.f * scale;
		Box(out, depth, x, groundY, f * 1.2f, 3.f, plinth);
		Box(out, depth + 0.01f, x, groundY + 3.f, f, wall, look.wall);
		out.AddRect(depth + 0.015f, doorX, doorY + 3.f + 8.f * scale, 12.f, 16.f * scale, 0.f, 0.f, 0.f, 1.f);

		float slabBase = groundY + 3.f + wall;
		Box(out, depth + 0.02f, x, slabBase, f * 1.2f, 4.f, look.roof);
		float reach = f * kHalfW * 1.15f;
		Spike(out, depth + 0.03f, x - reach, slabBase + 2.f, 12.f * scale, -3.f, look.roof);
		Spike(out, depth + 0.03f, x + reach, slabBase + 2.f, 12.f * scale, 3.f, look.roof);
		Spike(out, depth + 0.03f, x, slabBase + 2.f + f * kHalfH * 1.15f, 14.f * scale, 0.f, look.roof);
		Spike(out, depth + 0.035f, x, slabBase + 2.f - f * kHalfH * 1.15f, 10.f * scale, 0.f, look.roof);

		out.AddRect(depth + 0.016f, doorX, doorY + 3.f + 20.f * scale, 1.6f, 7.f, look.trim.r, look.trim.g, look.trim.b, 1.f);
		break;
	}
	default:
	{
		// Fallen in: the walls broken off low, three pillars still standing at
		// different heights. The altar in front of it is untouched.
		Box(out, depth, x, groundY, f * 1.15f, 3.f, plinth);
		Box(out, depth + 0.01f, x, groundY + 3.f, f, 8.f, look.wall);
		float reach = f * kHalfW * 0.9f;
		out.AddTaperedBox(depth + 0.02f, x - reach, groundY + 3.f, 0.4f, 0.4f, 26.f, 0.75f, -1.f, 0.f,
			look.wall.r, look.wall.g, look.wall.b);
		out.AddTaperedBox(depth + 0.02f, x + reach, groundY + 3.f, 0.4f, 0.4f, 18.f, 0.7f, 2.f, 0.f,
			look.wall.r, look.wall.g, look.wall.b);
		out.AddTaperedBox(depth + 0.02f, x, groundY + 3.f + f * kHalfH * 0.9f, 0.4f, 0.4f, 11.f, 0.6f, 0.f, 0.f,
			look.wall.r, look.wall.g, look.wall.b);
		break;
	}
	}
}

void SettlementActor::DrawAltar(DrawList& out, const ActorContext& ctx) const
{
	float worldX, worldY;
	WorldGrid::IsoPosition(m_Layout.altarCol, m_Layout.altarRow, worldX, worldY);
	float x, y;
	ctx.ToScreen(worldX, worldY, x, y);
	if (!ctx.IsOnScreen(x, y, 30.f, 40.f))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(worldX, worldY);
	float depth = WorldGrid::TileDepth(worldX, worldY) + 0.5f;

	out.AddShadow(depth - 0.01f, x, groundY, 26.f, 13.f);
	out.AddBox(depth, x, groundY, 0.6f, 0.6f, 7.f, 0.36f, 0.36f, 0.37f);
	out.AddBox(depth + 0.01f, x, groundY + 7.f, 0.75f, 0.75f, 2.f, 0.42f, 0.42f, 0.43f);

	// The same pale mark hangs over every altar, in every land.
	float bob = sinf(m_Time * kAltarBobRate) * 1.5f;
	out.SetGlow(0.6f);
	out.AddDiamond(depth + 0.02f, x, groundY + 16.f + bob, 6.f, 8.f,
		Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b, 1.f);
	out.SetGlow(0.f);
}

void SettlementActor::DrawBoard(DrawList& out, float depth, float x, float groundY) const
{
	Look look = LookFor(m_Style);
	const Palette::Color wood = { 0.20f, 0.17f, 0.14f };

	out.AddBox(depth, x - 9.f, groundY, 0.08f, 0.08f, 15.f, wood.r, wood.g, wood.b);
	out.AddBox(depth, x + 9.f, groundY, 0.08f, 0.08f, 15.f, wood.r, wood.g, wood.b);
	out.AddBox(depth + 0.01f, x, groundY + 7.f, 0.7f, 0.1f, 9.f, wood.r * 1.2f, wood.g * 1.2f, wood.b * 1.2f);

	// Notices pinned to it, gone pale.
	out.AddRect(depth + 0.02f, x - 3.f, groundY + 13.f, 5.f, 6.f, look.trim.r * 0.7f + 0.2f, look.trim.g * 0.7f + 0.2f, look.trim.b * 0.7f + 0.2f, 1.f);
	out.AddRect(depth + 0.02f, x + 4.f, groundY + 12.f, 4.f, 5.f, 0.55f, 0.54f, 0.52f, 1.f);
}
