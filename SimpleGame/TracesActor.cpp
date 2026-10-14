#include "stdafx.h"
#include "TracesActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Traces.h"
#include "Palette.h"
#include "Tuning.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
	// How far out from the middle of the screen to look, in tiles.
	int ViewRadius(const ActorContext& ctx)
	{
		float uMax = (ctx.windowSizeX / 2.f) / (WorldGrid::kTileWidth / 2.f) + 2.f;
		float vMax = (ctx.windowSizeY / 2.f) / (WorldGrid::kTileHeight / 2.f) + 4.f;
		return (int)std::ceil((uMax + vMax) / 2.f);
	}

	const Palette::Color& kGold = Palette::kLightCore;
	const Palette::Color& kViolet = Palette::kDarkCore;
	const Palette::Color kInk = { 0.07f, 0.06f, 0.09f };
	const Palette::Color kStone = { 0.36f, 0.36f, 0.38f };
	const Palette::Color kWood = { 0.22f, 0.18f, 0.14f };

	// Motes rise or sink this far, this fast, before they start again.
	const float kMoteTravel = 34.f;
	const float kMoteSpeed = 12.f;
}

void TracesActor::OnUpdate(const ActorContext& ctx)
{
	m_Time += ctx.deltaTime;
}

void TracesActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	int centerCol, centerRow;
	WorldGrid::WorldToTile(ctx.camX, ctx.camY, centerCol, centerRow);
	int radius = ViewRadius(ctx);

	// Ring by ring outward from the middle, so when there are more than may
	// be drawn, it is the far ones that go.
	int drawn = 0;
	int glowing = 0;
	for (int ring = 0; ring <= radius && drawn < Tuning::Trace::kMaxDrawn; ring++)
	{
		for (int dr = -ring; dr <= ring; dr++)
		{
			for (int dc = -ring; dc <= ring; dc++)
			{
				if (std::abs(dc) != ring && std::abs(dr) != ring)
				{
					continue;
				}
				int col = centerCol + dc;
				int row = centerRow + dr;
				if (col < 0 || row < 0 || col >= WorldGrid::MapCols() || row >= WorldGrid::MapRows())
				{
					continue;
				}
				if (drawn >= Tuning::Trace::kMaxDrawn)
				{
					break;
				}

				int variant;
				Traces::Kind kind = Traces::At(col, row, variant);
				if (kind == Traces::Kind::None)
				{
					continue;
				}
				bool glows = Traces::Glows(kind);
				if (glows && glowing >= Tuning::Trace::kMaxGlowing)
				{
					continue;
				}

				float worldX, worldY;
				WorldGrid::IsoPosition((float)col, (float)row, worldX, worldY);
				float x, y;
				ctx.ToScreen(worldX, worldY, x, y);
				if (!ctx.IsOnScreen(x, y, WorldGrid::kTileWidth, WorldGrid::kTileHeight * 3.f))
				{
					continue;
				}
				float groundY = y + WorldGrid::Elevation(col, row);
				float depth = (float)(col + row) + 0.3f;

				switch (kind)
				{
				case Traces::Kind::LightCrack:
				{
					DrawCrack(out, depth, x, groundY, variant, kGold.r, kGold.g, kGold.b);
					break;
				}
				case Traces::Kind::DarkCrack:
				{
					DrawCrack(out, depth, x, groundY, variant, kViolet.r, kViolet.g, kViolet.b);
					break;
				}
				case Traces::Kind::FlickerCrack:
				{
					// Gold, then violet, by turns.
					bool gold = fmodf(m_Time * 0.8f + variant * 0.37f, 2.f) < 1.f;
					const Palette::Color& c = gold ? kGold : kViolet;
					DrawCrack(out, depth, x, groundY, variant, c.r, c.g, c.b);
					break;
				}
				case Traces::Kind::Lamp:
				{
					DrawLamp(out, depth, x, groundY, variant);
					break;
				}
				case Traces::Kind::HaloStatue:
				{
					DrawHaloStatue(out, depth, x, groundY, variant);
					break;
				}
				case Traces::Kind::Banner:
				{
					DrawBanner(out, depth, x, groundY, variant);
					break;
				}
				case Traces::Kind::LightMotes:
				{
					DrawMotes(out, depth, x, groundY, variant, true);
					break;
				}
				case Traces::Kind::ShadowMotes:
				{
					DrawMotes(out, depth, x, groundY, variant, false);
					break;
				}
				case Traces::Kind::Thorns:
				{
					DrawThorns(out, depth, x, groundY, variant);
					break;
				}
				case Traces::Kind::Embers:
				{
					DrawEmbers(out, depth, x, groundY, variant);
					break;
				}
				case Traces::Kind::Crystal:
				{
					DrawCrystal(out, depth, x, groundY, variant);
					break;
				}
				case Traces::Kind::Wreck:
				{
					DrawWreck(out, depth, x, groundY, variant);
					break;
				}
				default:
				{
					break;
				}
				}
				out.ResetMaterial();

				drawn++;
				if (glows)
				{
					glowing++;
				}
			}
		}
	}
}

void TracesActor::DrawCrack(DrawList& out, float depth, float x, float y, int variant, float r, float g, float b) const
{
	// A zigzag of slivers lying flat, lit from below.
	out.SetGround(true);
	out.SetGlow(0.5f);
	float lean = (variant % 2 == 0) ? 1.f : -1.f;
	for (int i = -1; i <= 1; i++)
	{
		float offsetX = i * 9.f * lean;
		float offsetY = (i % 2 == 0) ? 0.f : 2.5f;
		out.AddDiamond(depth, x + offsetX, y + offsetY, 12.f, 3.f, r, g, b, 0.85f);
	}
	out.SetGround(false);
	out.SetGlow(0.f);
}

void TracesActor::DrawLamp(DrawList& out, float depth, float x, float y, int variant) const
{
	// A post, and a lamp on it guttering out.
	out.AddShadow(depth - 0.01f, x, y, 14.f, 7.f);
	out.AddBox(depth, x, y, 0.1f, 0.1f, 26.f, kStone.r * 0.7f, kStone.g * 0.7f, kStone.b * 0.7f);
	float flicker = 0.55f + 0.45f * sinf(m_Time * (5.f + variant) + variant);
	out.SetGlow(0.6f * flicker);
	out.AddBox(depth + 0.01f, x, y + 26.f, 0.18f, 0.18f, 6.f, kGold.r, kGold.g, kGold.b);
	out.AddGlow(depth + 0.005f, x, y + 29.f, 14.f, kGold.r, kGold.g, kGold.b, 0.35f * flicker);
	out.SetGlow(0.f);
}

void TracesActor::DrawHaloStatue(DrawList& out, float depth, float x, float y, int variant) const
{
	// A statue broken off at the chest, and over where its head was, only
	// the halo, still lit.
	float tilt = (variant % 2 == 0) ? 2.f : -2.f;
	out.AddShadow(depth - 0.01f, x, y, 20.f, 10.f);
	out.AddBox(depth, x, y, 0.4f, 0.4f, 6.f, kStone.r * 0.8f, kStone.g * 0.8f, kStone.b * 0.8f);
	out.AddTaperedBox(depth + 0.01f, x, y + 6.f, 0.3f, 0.26f, 18.f, 0.8f, tilt, 0.f, kStone.r, kStone.g, kStone.b);
	out.SetGlow(0.7f);
	out.AddDiamond(depth + 0.02f, x + tilt, y + 34.f, 16.f, 7.f, kGold.r, kGold.g, kGold.b, 0.9f);
	out.AddDiamond(depth + 0.021f, x + tilt, y + 34.f, 10.f, 3.5f, kStone.r * 0.5f, kStone.g * 0.5f, kStone.b * 0.5f, 1.f);
	out.SetGlow(0.f);
}

void TracesActor::DrawBanner(DrawList& out, float depth, float x, float y, int variant) const
{
	// A snapped pole, a scrap of gold cloth still on it.
	float lean = (variant % 2 == 0) ? 5.f : -5.f;
	out.AddTaperedBox(depth, x, y, 0.06f, 0.06f, 22.f, 1.f, lean, 0.f, kWood.r, kWood.g, kWood.b);
	float sway = sinf(m_Time * 1.6f + variant) * 1.5f;
	out.AddRect(depth + 0.01f, x + lean * 0.8f + 5.f + sway, y + 16.f, 9.f, 10.f, kGold.r * 0.8f, kGold.g * 0.8f,
		kGold.b * 0.8f, 1.f);
}

void TracesActor::DrawMotes(DrawList& out, float depth, float x, float y, int variant, bool light) const
{
	// A few motes drifting up from the ground - or, of the dark, settling
	// back down into it.
	const Palette::Color& c = light ? kGold : kInk;
	if (light)
	{
		out.SetGlow(0.6f);
	}
	for (int i = 0; i < 3; i++)
	{
		float phase = fmodf(m_Time * kMoteSpeed + (variant * 7 + i * 11) * 1.f, kMoteTravel);
		float rise = light ? phase : kMoteTravel - phase;
		float fade = 1.f - phase / kMoteTravel;
		float drift = sinf(m_Time * 1.3f + i * 2.f + variant) * 4.f;
		out.AddRect(depth + 0.02f, x + (i - 1) * 7.f + drift, y + 4.f + rise, 2.5f, 2.5f, c.r, c.g, c.b, light ? fade : 1.f - fade * 0.5f);
	}
	if (!light)
	{
		// Where they sink, a violet stain.
		out.SetGround(true);
		out.AddDiamond(depth, x, y, 18.f, 6.f, kViolet.r * 0.5f, kViolet.g * 0.5f, kViolet.b * 0.5f, 0.6f);
		out.SetGround(false);
	}
	out.SetGlow(0.f);
}

void TracesActor::DrawThorns(DrawList& out, float depth, float x, float y, int variant) const
{
	// Ink-black thorns and vine, splaying out of the ground.
	for (int i = 0; i < 4; i++)
	{
		float lean = (i - 1.5f) * 5.f + ((variant % 3) - 1) * 2.f;
		float height = 10.f + ((variant + i * 3) % 4) * 3.f;
		out.AddTaperedBox(depth + i * 0.002f, x + (i - 1.5f) * 4.f, y, 0.06f, 0.06f, height, 0.1f, lean, 0.f,
			kInk.r, kInk.g, kInk.b);
	}
	out.AddRect(depth + 0.01f, x, y + 3.f, 16.f, 2.f, kViolet.r * 0.35f, kViolet.g * 0.35f, kViolet.b * 0.35f, 1.f);
}

void TracesActor::DrawEmbers(DrawList& out, float depth, float x, float y, int variant) const
{
	// A heap of black cinders with violet still burning in it.
	out.AddTaperedBox(depth, x, y, 0.35f, 0.25f, 4.f, 0.6f, 0.f, 0.f, kInk.r * 1.5f, kInk.g * 1.5f, kInk.b * 1.5f);
	out.SetGlow(0.5f);
	for (int i = 0; i < 3; i++)
	{
		float pulse = 0.5f + 0.5f * sinf(m_Time * (3.f + i) + variant + i);
		out.AddRect(depth + 0.01f, x + (i - 1) * 5.f, y + 3.f + (i % 2) * 2.f, 2.5f, 2.f, kViolet.r, kViolet.g, kViolet.b,
			0.4f + 0.6f * pulse);
	}
	out.SetGlow(0.f);
}

void TracesActor::DrawCrystal(DrawList& out, float depth, float x, float y, int variant) const
{
	// A cluster of dark crystal, lit violet from within.
	out.AddShadow(depth - 0.01f, x, y, 16.f, 8.f);
	out.SetGlow(0.35f);
	float tall = 14.f + (variant % 3) * 4.f;
	out.AddTaperedBoxFaces(depth, x, y, 0.2f, 0.2f, tall, 0.1f, 1.5f, 0.f,
		kViolet.r, kViolet.g, kViolet.b,
		kViolet.r * 0.4f, kViolet.g * 0.4f, kViolet.b * 0.4f,
		kViolet.r * 0.2f, kViolet.g * 0.2f, kViolet.b * 0.2f);
	out.AddTaperedBoxFaces(depth + 0.005f, x + 6.f, y, 0.14f, 0.14f, tall * 0.6f, 0.1f, 3.f, 0.f,
		kViolet.r, kViolet.g, kViolet.b,
		kViolet.r * 0.4f, kViolet.g * 0.4f, kViolet.b * 0.4f,
		kViolet.r * 0.2f, kViolet.g * 0.2f, kViolet.b * 0.2f);
	out.SetGlow(0.f);
}

void TracesActor::DrawWreck(DrawList& out, float depth, float x, float y, int variant) const
{
	// A spear of each side broken across another, their colours still on
	// them, and a torn scrap of cloth.
	out.AddShadow(depth - 0.01f, x, y, 22.f, 9.f);
	out.AddTaperedBox(depth, x - 3.f, y, 0.05f, 0.05f, 20.f, 1.f, 8.f, 0.f, kGold.r * 0.6f, kGold.g * 0.6f, kGold.b * 0.6f);
	out.AddTaperedBox(depth + 0.005f, x + 3.f, y, 0.05f, 0.05f, 18.f, 1.f, -7.f, 0.f, kViolet.r * 0.6f, kViolet.g * 0.6f,
		kViolet.b * 0.6f);
	const Palette::Color& cloth = (variant % 2 == 0) ? kGold : kViolet;
	out.SetGround(true);
	out.AddDiamond(depth, x + 6.f, y + 1.f, 12.f, 4.f, cloth.r * 0.5f, cloth.g * 0.5f, cloth.b * 0.5f, 1.f);
	out.SetGround(false);
}
