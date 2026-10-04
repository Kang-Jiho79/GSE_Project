#include "stdafx.h"
#include "NpcActor.h"
#include "DrawList.h"
#include "WorldGrid.h"
#include "Alignment.h"
#include "Palette.h"
#include "Random.h"
#include <cmath>

namespace
{
	// He is noticed from this far off, in tiles.
	const float kNoticeRange = 4.f;

	const float kChildScale = 0.68f;
	const float kElderStoop = 4.f;

	const Palette::Color& ClothFor(Regions::Style style)
	{
		switch (style)
		{
		case Regions::Style::Light:
		{
			return Palette::kClothLight;
		}
		case Regions::Style::Dark:
		{
			return Palette::kClothDark;
		}
		case Regions::Style::Borderborn:
		{
			return Palette::kClothBorderborn;
		}
		default:
		{
			return Palette::kClothAshen;
		}
		}
	}
}

NpcActor::NpcActor(Kind kind, Regions::Style style, int conversation)
	: m_Kind(kind)
	, m_Style(style)
	, m_Conversation(conversation)
{
	m_Time = Random::Unit() * 10.f;
}

int NpcActor::Conversation() const
{
	return m_Conversation;
}

void NpcActor::SetConversation(int conversation)
{
	m_Conversation = conversation;
}

void NpcActor::OnUpdate(const ActorContext& ctx)
{
	m_Time += ctx.deltaTime;

	float col, row, playerCol, playerRow;
	WorldGrid::InverseIso(WorldX(), WorldY(), col, row);
	WorldGrid::InverseIso(ctx.playerX, ctx.playerY, playerCol, playerRow);
	float dc = playerCol - col;
	float dr = playerRow - row;
	if (dc * dc + dr * dr <= kNoticeRange * kNoticeRange && fabsf(ctx.playerX - WorldX()) > 1.f)
	{
		m_Facing = (ctx.playerX < WorldX()) ? -1.f : 1.f;
	}
}

void NpcActor::OnDraw(DrawList& out, const ActorContext& ctx) const
{
	float x, y;
	ctx.ToScreen(WorldX(), WorldY(), x, y);
	if (!ctx.IsOnScreen(x, y, 40.f, 80.f))
	{
		return;
	}

	float groundY = y + WorldGrid::GroundHeightAt(WorldX(), WorldY());
	float depth = WorldGrid::TileDepth(WorldX(), WorldY()) + 0.6f;

	if (m_Kind == Kind::Kinis)
	{
		DrawKinis(out, depth, x, groundY);
		return;
	}

	float s = (m_Kind == Kind::Child) ? kChildScale : 1.f;
	float f = m_Facing;
	float breathe = sinf(m_Time * 1.7f) * 0.5f;
	float stoop = (m_Kind == Kind::Elder) ? f * kElderStoop : 0.f;

	const Palette::Color& cloth = ClothFor(m_Style);
	const Palette::Color& robe = (m_Style == Regions::Style::Dark) ? Palette::kRobeDark : Palette::kRobeLight;
	const Palette::Color& skin = Palette::kSkin;

	out.AddShadow(depth - 0.01f, x, groundY, 26.f * s, 13.f * s);
	out.SetFigure(true);

	float shoulderY;
	if (m_Kind == Kind::Priest)
	{
		// One long robe to the ground, widening toward the hem.
		const float kRobeHeight = 34.f;
		out.AddTaperedBox(depth, x, groundY, 0.32f, 0.26f, kRobeHeight + breathe, 0.55f, 0.f, 0.f,
			robe.r, robe.g, robe.b);
		shoulderY = groundY + kRobeHeight + breathe;

		// The mark of the one his side keeps - the light's shines, the dark's
		// does not.
		bool light = (m_Style != Regions::Style::Dark);
		float mr, mg, mb;
		AlignmentColor::Core(light ? Alignment::Light : Alignment::Dark, mr, mg, mb);
		out.SetGlow(light ? 0.5f : 0.f);
		out.AddDiamond(depth + 0.01f, x + f * 2.f, groundY + kRobeHeight * 0.62f, 6.f, 8.f, mr, mg, mb, 1.f);
		out.SetGlow(0.f);
	}
	else
	{
		// Legs, then a plain tunic.
		const float kLegHeight = 13.f;
		out.AddTaperedBox(depth, x - 4.f * s, groundY, 0.12f * s, 0.12f * s, kLegHeight * s, 0.9f, 0.f, 0.f,
			cloth.r * 0.7f, cloth.g * 0.7f, cloth.b * 0.7f);
		out.AddTaperedBox(depth, x + 4.f * s, groundY, 0.12f * s, 0.12f * s, kLegHeight * s, 0.9f, 0.f, 0.f,
			cloth.r * 0.7f, cloth.g * 0.7f, cloth.b * 0.7f);

		const float kTorsoHeight = 18.f;
		float torsoBase = groundY + kLegHeight * s + breathe;
		out.AddTaperedBox(depth + 0.01f, x, torsoBase, 0.26f * s, 0.22f * s, kTorsoHeight * s, 0.85f, stoop, 0.f,
			cloth.r, cloth.g, cloth.b);
		shoulderY = torsoBase + kTorsoHeight * s;
	}

	// Arms hanging at the sides.
	for (int side = -1; side <= 1; side += 2)
	{
		out.AddTaperedBox(depth + ((side == (int)f) ? 0.02f : 0.005f), x + side * 9.f * s + stoop, shoulderY - 15.f * s,
			0.08f * s, 0.08f * s, 14.f * s, 1.2f, 0.f, 0.f, skin.r * 0.9f, skin.g * 0.9f, skin.b * 0.9f);
	}

	// The head, bowed a little on the old, and hair or a hood over it.
	float headX = x + f * 1.f + stoop * 1.4f;
	float headBase = shoulderY - (m_Kind == Kind::Elder ? 2.f : 0.f);
	out.AddTaperedBox(depth + 0.03f, headX, headBase, 0.18f * s, 0.18f * s, 10.f * s, 0.9f, 0.f, 0.f,
		skin.r, skin.g, skin.b);
	const Palette::Color& hair = (m_Kind == Kind::Priest) ? robe : cloth;
	float hairShade = (m_Kind == Kind::Elder) ? 1.6f : 0.6f;
	out.AddTaperedBox(depth + 0.035f, headX - f * 1.f, headBase + 6.f * s, 0.2f * s, 0.2f * s, 5.f * s, 0.7f, 0.f, 0.f,
		hair.r * hairShade, hair.g * hairShade, hair.b * hairShade);
	out.AddRect(depth + 0.04f, headX + f * 3.f * s, headBase + 5.f * s, 1.5f, 1.5f, Palette::kOutline.r, Palette::kOutline.g, Palette::kOutline.b, 1.f);
}

void NpcActor::DrawKinis(DrawList& out, float depth, float x, float groundY) const
{
	// Tall and gray from hood to hem, the gray pooling round his feet - and
	// through the cracks in him, both powers he once held, still there.
	const Palette::Color& ash = Palette::kAsh;
	float f = m_Facing;
	float breathe = sinf(m_Time * 1.1f) * 0.6f;
	float sway = sinf(m_Time * 0.7f) * 1.5f;

	out.AddGlow(depth - 0.02f, x, groundY, 60.f, Palette::kAshVeil.r, Palette::kAshVeil.g, Palette::kAshVeil.b, 0.5f);
	out.AddShadow(depth - 0.01f, x, groundY, 30.f, 15.f);
	out.SetFigure(true);

	const float kRobeHeight = 42.f;
	out.AddTaperedBox(depth, x, groundY, 0.36f, 0.3f, kRobeHeight + breathe, 0.5f, sway, 0.f,
		ash.r * 0.55f, ash.g * 0.55f, ash.b * 0.57f);
	float shoulderY = groundY + kRobeHeight + breathe;

	// The hood, drawn low, the face in it all shadow but the eyes.
	float headX = x + f * 1.5f + sway;
	out.AddTaperedBox(depth + 0.03f, headX, shoulderY - 2.f, 0.24f, 0.24f, 15.f, 0.55f, 0.f, 0.f,
		ash.r * 0.45f, ash.g * 0.45f, ash.b * 0.47f);
	out.AddRect(depth + 0.035f, headX + f * 3.f, shoulderY + 4.f, 7.f, 6.f, Palette::kShade.r, Palette::kShade.g, Palette::kShade.b, 1.f);

	// One eye gold, one violet: the darkest light and the brightest dark.
	float lr, lg, lb, dr, dg, db;
	AlignmentColor::Core(Alignment::Light, lr, lg, lb);
	AlignmentColor::Core(Alignment::Dark, dr, dg, db);
	out.SetGlow(0.7f);
	out.AddRect(depth + 0.04f, headX + f * 1.5f, shoulderY + 5.f, 1.6f, 1.6f, lr, lg, lb, 1.f);
	out.SetGlow(0.f);
	out.AddRect(depth + 0.04f, headX + f * 4.5f, shoulderY + 5.f, 1.6f, 1.6f, dr, dg, db, 1.f);

	// The cracks down the robe, one of each.
	float pulse = 0.7f + 0.3f * sinf(m_Time * 2.3f);
	out.SetGlow(0.6f * pulse);
	out.AddRect(depth + 0.02f, x - 5.f + sway * 0.6f, groundY + 20.f, 1.6f, 12.f, lr * pulse, lg * pulse, lb * pulse, 1.f);
	out.SetGlow(0.f);
	out.AddRect(depth + 0.02f, x + 6.f + sway * 0.6f, groundY + 14.f, 1.6f, 10.f, dr * pulse, dg * pulse, db * pulse, 1.f);
}
