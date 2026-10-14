#include "stdafx.h"
#include "LandmarkActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Palette.h"
#include "Alignment.h"
#include <cmath>

namespace
{
	const float kPi = 3.14159265f;

	// The landmark stands this far from the place's waystone, so the two
	// never crowd each other.
	const float kSpotCol = 7.f;
	const float kSpotRow = -7.f;

	const Palette::Color kStone = { 0.34f, 0.34f, 0.36f };
	const Palette::Color kPaleStone = { 0.58f, 0.58f, 0.60f };
	const Palette::Color kCanvas = { 0.42f, 0.38f, 0.30f };
	const Palette::Color kWood = { 0.20f, 0.16f, 0.12f };
	const Palette::Color kFire = { 1.00f, 0.62f, 0.25f };

	// The wreckage: broken spears, banners and shields, scattered.
	const int kWreckPieces = 16;
	const float kWreckSpread = 7.f;
	// The rift's pools.
	const int kRiftPools = 6;
	const float kRiftSpread = 9.f;

	// A steady pseudo-random number for a piece, 0..1.
	float Roll(int piece, int salt)
	{
		unsigned int h = (unsigned int)(piece * 2654435761u + salt * 40503u + 977u);
		h = (h ^ (h >> 13)) * 1274126177u;
		return ((h ^ (h >> 16)) % 1000) / 1000.f;
	}
}

void LandmarkActor::Spot(int region, float& col, float& row)
{
	Regions::Center(region, col, row);
	col += kSpotCol;
	row += kSpotRow;
}

LandmarkActor::LandmarkActor(int region, Regions::Feature feature)
	: m_Feature(feature)
{
	Spot(region, m_Col, m_Row);
	float x, y;
	WorldGrid::IsoPosition(m_Col, m_Row, x, y);
	SetLocalPosition(x, y);

	// What is big enough to stand in the way.
	if (feature == Regions::Feature::FallenKing)
	{
		WorldGrid::AddBlocker(m_Col - 4.f, m_Row - 1.2f, m_Col + 4.f, m_Row + 1.2f);
	}
	else if (feature == Regions::Feature::Hideout)
	{
		WorldGrid::AddBlocker(m_Col - 1.5f, m_Row - 1.5f, m_Col + 1.5f, m_Row + 1.5f);
	}
}

void LandmarkActor::OnUpdate(const ActorContext& ctx)
{
	m_Time += ctx.deltaTime;
}

bool LandmarkActor::Place(const ActorContext& ctx, float dc, float dr, float& x, float& y, float& depth) const
{
	float worldX, worldY;
	WorldGrid::IsoPosition(m_Col + dc, m_Row + dr, worldX, worldY);
	ctx.ToScreen(worldX, worldY, x, y);
	if (!ctx.IsOnScreen(x, y, 120.f, 160.f))
	{
		return false;
	}
	y += WorldGrid::GroundHeightAt(worldX, worldY);
	depth = WorldGrid::TileDepth(worldX, worldY) + 0.5f;
	return true;
}

void LandmarkActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 700.f, 500.f))
	{
		return;
	}

	switch (m_Feature)
	{
	case Regions::Feature::Camp:
	{
		DrawCamp(out, ctx);
		break;
	}
	case Regions::Feature::FallenKing:
	{
		DrawFallenKing(out, ctx);
		break;
	}
	case Regions::Feature::Wreckage:
	{
		DrawWreckage(out, ctx);
		break;
	}
	case Regions::Feature::Rift:
	{
		DrawRift(out, ctx);
		break;
	}
	case Regions::Feature::RuinedAltar:
	{
		DrawRuins(out, ctx);
		break;
	}
	case Regions::Feature::Hideout:
	{
		DrawHideout(out, ctx);
		break;
	}
	case Regions::Feature::PilgrimRest:
	{
		DrawPilgrimRest(out, ctx);
		break;
	}
	default:
	{
		break;
	}
	}
}

void LandmarkActor::DrawCamp(DrawList& out, const ActorContext& ctx) const
{
	float x, y, depth;
	// The fire, ringed with stones and burning low.
	if (Place(ctx, 0.f, 0.f, x, y, depth))
	{
		float flicker = 0.85f + 0.15f * sinf(m_Time * 9.f) * sinf(m_Time * 5.3f);
		out.AddGlow(depth - 0.02f, x, y, 70.f * flicker, kFire.r, kFire.g * 0.8f, kFire.b * 0.6f, 0.7f);
		for (int i = 0; i < 6; i++)
		{
			float a = i * kPi / 3.f;
			out.AddBox(depth, x + cosf(a) * 12.f, y + sinf(a) * 6.f, 0.12f, 0.12f, 3.f, kStone.r, kStone.g, kStone.b);
		}
		out.SetGlow(1.f);
		out.AddTaperedBox(depth + 0.01f, x, y, 0.25f, 0.25f, 12.f * flicker, 0.1f, 0.f, 0.f, kFire.r, kFire.g, kFire.b);
		out.SetGlow(0.f);
	}
	// Two tents, and a bedroll by the fire.
	for (int t = 0; t < 2; t++)
	{
		float dc = (t == 0) ? -2.5f : 1.5f;
		float dr = (t == 0) ? -1.f : -3.f;
		if (Place(ctx, dc, dr, x, y, depth))
		{
			out.AddShadow(depth - 0.01f, x, y, 60.f, 30.f);
			out.AddTaperedBox(depth, x, y, 1.5f, 1.2f, 26.f, 0.05f, 0.f, 0.f, kCanvas.r, kCanvas.g, kCanvas.b);
		}
	}
	if (Place(ctx, 1.2f, 1.2f, x, y, depth))
	{
		out.AddBox(depth, x, y, 0.9f, 0.35f, 3.f, 0.30f, 0.20f, 0.16f);
	}
}

void LandmarkActor::DrawFallenKing(DrawList& out, const ActorContext& ctx) const
{
	// A great king of stone lying along the board: its base at one end, the
	// body tapering away, the crown and its cross broken off at the other.
	float x, y, depth;
	for (int i = 0; i < 7; i++)
	{
		float dc = -3.5f + i * 1.15f;
		if (!Place(ctx, dc, 0.f, x, y, depth))
		{
			continue;
		}
		float foot = (i == 0) ? 2.1f : ((i < 5) ? 1.5f - i * 0.12f : 1.1f);
		float height = (i == 0) ? 30.f : 22.f - i * 1.5f;
		const Palette::Color& c = (i % 2 == 0) ? kPaleStone : kStone;
		out.AddShadow(depth - 0.01f, x, y, 70.f, 30.f);
		out.AddBox(depth, x, y, 1.2f, foot, height, c.r, c.g, c.b);
	}
	if (Place(ctx, 5.f, 1.5f, x, y, depth))
	{
		out.AddBox(depth, x, y, 1.6f, 0.35f, 6.f, kPaleStone.r, kPaleStone.g, kPaleStone.b);
		out.AddBox(depth + 0.01f, x, y, 0.35f, 1.1f, 6.f, kPaleStone.r, kPaleStone.g, kPaleStone.b);
	}
}

void LandmarkActor::DrawWreckage(DrawList& out, const ActorContext& ctx) const
{
	float lightR, lightG, lightB, darkR, darkG, darkB;
	AlignmentColor::Core(Alignment::Light, lightR, lightG, lightB);
	AlignmentColor::Core(Alignment::Dark, darkR, darkG, darkB);
	for (int i = 0; i < kWreckPieces; i++)
	{
		float angle = Roll(i, 1) * 2.f * kPi;
		float reach = kWreckSpread * sqrtf(Roll(i, 2));
		float x, y, depth;
		if (!Place(ctx, cosf(angle) * reach, sinf(angle) * reach, x, y, depth))
		{
			continue;
		}
		bool light = (i % 2 == 0);
		float r = light ? lightR : darkR;
		float g = light ? lightG : darkG;
		float b = light ? lightB : darkB;
		float lean = (Roll(i, 3) - 0.5f) * 30.f;
		switch (i % 3)
		{
		case 0:
		{
			// A banner, torn, on a leaning pole.
			out.AddTaperedBox(depth, x, y, 0.06f, 0.06f, 34.f, 1.f, lean, 0.f, kWood.r, kWood.g, kWood.b);
			out.AddRect(depth + 0.01f, x + lean * 0.8f + 7.f, y + 26.f, 14.f, 9.f, r * 0.8f, g * 0.8f, b * 0.8f, 1.f);
			break;
		}
		case 1:
		{
			// A spear broken off in the ground.
			out.AddTaperedBox(depth, x, y, 0.05f, 0.05f, 20.f, 0.6f, lean, 0.f, kStone.r, kStone.g, kStone.b);
			break;
		}
		default:
		{
			// A shield lying where it fell.
			out.AddDiamond(depth, x, y + 2.f, 16.f, 9.f, r * 0.6f, g * 0.6f, b * 0.6f, 1.f);
			break;
		}
		}
	}
}

void LandmarkActor::DrawRift(DrawList& out, const ActorContext& ctx) const
{
	float r, g, b;
	AlignmentColor::Core(Alignment::Ashen, r, g, b);
	for (int i = 0; i < kRiftPools; i++)
	{
		float angle = Roll(i, 4) * 2.f * kPi;
		float reach = kRiftSpread * sqrtf(Roll(i, 5));
		float dc = cosf(angle) * reach;
		float dr = sinf(angle) * reach;
		float x, y, depth;
		if (!Place(ctx, dc, dr, x, y, depth))
		{
			continue;
		}
		float worldX, worldY;
		WorldGrid::IsoPosition(m_Col + dc, m_Row + dr, worldX, worldY);
		float radius = 1.4f + Roll(i, 6) * 1.6f;
		float pulse = 0.75f + 0.25f * sinf(m_Time * 1.3f + i);
		out.SetGround(true);
		out.AddGroundArc(worldX, worldY, x, y + 1.f, 0.f, radius, 0.f, 2.f * kPi, 20, r * 0.5f, g * 0.5f, b * 0.5f, 0.7f);
		out.SetGround(false);
		out.SetGlow(0.4f);
		out.AddGroundArc(worldX, worldY, x, y + 2.f, radius * pulse - 0.12f, radius * pulse, 0.f, 2.f * kPi, 20, r, g, b, 0.6f);
		out.SetGlow(0.f);
		// The gray rising off it, slowly.
		for (int m = 0; m < 3; m++)
		{
			float rise = fmodf(m_Time * 0.4f + m * 0.33f + i * 0.17f, 1.f);
			out.AddDiamond(depth + 0.05f, x + (m - 1) * 10.f, y + 4.f + rise * 40.f, 4.f, 6.f, r, g, b, 0.6f * (1.f - rise));
		}
	}
}

void LandmarkActor::DrawRuins(DrawList& out, const ActorContext& ctx) const
{
	// Broken pillars in a ring where the temple's colonnade stood.
	for (int i = 0; i < 6; i++)
	{
		float a = i * kPi / 3.f + 0.3f;
		float x, y, depth;
		if (!Place(ctx, cosf(a) * 4.5f, sinf(a) * 4.5f, x, y, depth))
		{
			continue;
		}
		float height = 14.f + Roll(i, 7) * 24.f;
		out.AddShadow(depth - 0.01f, x, y, 26.f, 13.f);
		out.AddBox(depth, x, y, 0.55f, 0.55f, height, kStone.r, kStone.g, kStone.b);
	}
}

void LandmarkActor::DrawHideout(DrawList& out, const ActorContext& ctx) const
{
	// A heap of boulders with a low dark mouth in it, and the borderborn's
	// marks scratched beside it.
	float x, y, depth;
	const float offsets[4][3] = { { -1.f, -1.f, 26.f }, { 1.f, -1.f, 20.f }, { -1.f, 1.f, 18.f }, { 0.4f, 0.6f, 30.f } };
	for (int i = 0; i < 4; i++)
	{
		if (Place(ctx, offsets[i][0], offsets[i][1], x, y, depth))
		{
			out.AddShadow(depth - 0.01f, x, y, 50.f, 25.f);
			out.AddTaperedBox(depth, x, y, 1.2f, 1.1f, offsets[i][2], 0.6f, 0.f, 0.f, kStone.r, kStone.g, kStone.b);
		}
	}
	if (Place(ctx, 1.6f, 1.6f, x, y, depth))
	{
		out.AddRect(depth + 0.3f, x, y + 8.f, 18.f, 14.f, 0.02f, 0.02f, 0.03f, 1.f);
		float r, g, b;
		AlignmentColor::Core(Alignment::Light, r, g, b);
		out.AddRect(depth + 0.31f, x + 14.f, y + 14.f, 2.f, 6.f, r * 0.5f, g * 0.5f, b * 0.5f, 1.f);
		AlignmentColor::Core(Alignment::Dark, r, g, b);
		out.AddRect(depth + 0.31f, x + 18.f, y + 14.f, 2.f, 6.f, r, g, b, 1.f);
	}
}

void LandmarkActor::DrawPilgrimRest(DrawList& out, const ActorContext& ctx) const
{
	// Two benches set well apart, each under its own side's banner, and a
	// worn shrine stone between them that both face.
	float lightR, lightG, lightB, darkR, darkG, darkB;
	AlignmentColor::Core(Alignment::Light, lightR, lightG, lightB);
	AlignmentColor::Core(Alignment::Dark, darkR, darkG, darkB);
	float x, y, depth;
	for (int side = 0; side < 2; side++)
	{
		float dc = (side == 0) ? -3.5f : 3.5f;
		if (Place(ctx, dc, 0.f, x, y, depth))
		{
			out.AddShadow(depth - 0.01f, x, y, 50.f, 20.f);
			out.AddBox(depth, x, y, 0.4f, 1.6f, 7.f, kStone.r, kStone.g, kStone.b);
		}
		if (Place(ctx, dc, -1.6f, x, y, depth))
		{
			float r = (side == 0) ? lightR : darkR;
			float g = (side == 0) ? lightG : darkG;
			float b = (side == 0) ? lightB : darkB;
			out.AddTaperedBox(depth, x, y, 0.06f, 0.06f, 40.f, 1.f, 0.f, 0.f, kWood.r, kWood.g, kWood.b);
			out.SetGlow(side == 0 ? 0.4f : 0.f);
			out.AddRect(depth + 0.01f, x + 7.f, y + 32.f, 14.f, 10.f, r, g, b, 1.f);
			out.SetGlow(0.f);
		}
	}
	if (Place(ctx, 0.f, -2.5f, x, y, depth))
	{
		out.AddShadow(depth - 0.01f, x, y, 30.f, 15.f);
		out.AddTaperedBox(depth, x, y, 0.7f, 0.5f, 22.f, 0.7f, 0.f, 0.f, kPaleStone.r, kPaleStone.g, kPaleStone.b);
		out.SetGlow(0.6f);
		out.AddDiamond(depth + 0.02f, x, y + 30.f, 6.f, 8.f, Palette::kUiBone.r, Palette::kUiBone.g, Palette::kUiBone.b, 0.9f);
		out.SetGlow(0.f);
	}
}
