#include "stdafx.h"
#include "Regions.h"

namespace
{
	using Regions::Kind;
	using Regions::Style;

	// Field and gate names are working names - the design does not name them yet.
	const Regions::Info kRegions[Regions::Count] =
	{
		// Settlements stand on flat ground. The borderborn's hidden village has
		// no temple - hunted by both nations, they kept no faith with either.
		//
		// name           kind            style              temple size levels props  count light  mage   mapX    mapY
		{ "리미나",        Kind::Village, Style::Borderborn, false,  40,  1,  0.4f,   0, 0.5f, 0.5f, -0.85f, -0.50f },
		{ "리미나 숲",     Kind::Field,   Style::Borderborn, false,  80,  3,  1.6f,  14, 0.5f, 0.3f, -0.65f, -0.30f },
		{ "크레푸스",      Kind::Field,   Style::Mixed,      false, 100,  4,  1.0f,  20, 0.5f, 0.5f, -0.40f, -0.12f },
		{ "벨룸 평원",     Kind::Field,   Style::Mixed,      false, 120,  2,  0.6f,  26, 0.5f, 0.5f,  0.00f,  0.00f },

		{ "알바",          Kind::Village, Style::Light,      true,   40,  1,  0.3f,   0, 0.8f, 0.5f, -0.12f,  0.30f },
		{ "솔라",          Kind::Field,   Style::Light,      false, 100,  4,  0.8f,  22, 0.8f, 0.5f, -0.24f,  0.55f },
		{ "아우라",        Kind::Village, Style::Light,      true,   44,  1,  0.3f,   0, 0.8f, 0.5f, -0.04f,  0.76f },
		{ "빛의 관문",     Kind::Gate,    Style::Light,      false,  50,  3,  0.5f,   8, 0.8f, 0.5f,  0.20f,  0.86f },
		{ "루멘",          Kind::Capital, Style::Light,      true,   60,  1,  0.0f,   0, 0.8f, 0.5f,  0.45f,  0.90f },

		{ "움브라",        Kind::Village, Style::Dark,       true,   40,  1,  0.3f,   0, 0.2f, 0.5f,  0.12f, -0.30f },
		{ "녹스",          Kind::Field,   Style::Dark,       false, 100,  4,  0.8f,  22, 0.2f, 0.5f,  0.24f, -0.55f },
		{ "크립타",        Kind::Village, Style::Dark,       true,   44,  1,  0.3f,   0, 0.2f, 0.5f,  0.04f, -0.76f },
		{ "어둠의 관문",   Kind::Gate,    Style::Dark,       false,  50,  3,  0.5f,   8, 0.2f, 0.5f, -0.20f, -0.86f },
		{ "테네브라에",    Kind::Capital, Style::Dark,       true,   60,  1,  0.0f,   0, 0.2f, 0.5f, -0.45f, -0.90f },

		{ "잿빛 변경",     Kind::Field,   Style::Ashen,      false, 110,  4,  1.0f,  24, 0.5f, 0.5f,  0.40f,  0.00f },
		{ "피데스",        Kind::Village, Style::Ashen,      true,   40,  1,  0.5f,   6, 0.7f, 0.5f,  0.55f,  0.30f },
		{ "스페스",        Kind::Village, Style::Ashen,      true,   40,  1,  0.5f,   6, 0.7f, 0.5f,  0.76f,  0.36f },
		{ "레퀴엠",        Kind::Village, Style::Ashen,      true,   40,  1,  0.5f,   6, 0.3f, 0.5f,  0.55f, -0.30f },
		{ "루크투스",      Kind::Village, Style::Ashen,      true,   40,  1,  0.5f,   6, 0.3f, 0.5f,  0.76f, -0.36f },
		{ "디비시오",      Kind::Field,   Style::Ashen,      false, 100,  3,  1.0f,  26, 0.5f, 0.5f,  0.70f,  0.00f },
		{ "키네라",        Kind::Field,   Style::Ashen,      false,  70,  4,  0.4f,  16, 0.5f, 0.5f,  0.92f,  0.00f },
		{ "신의 영역",     Kind::Field,   Style::Ashen,      false,  44,  1,  0.0f,   0, 0.5f, 0.5f,  0.00f,  0.00f },
	};

	struct Link
	{
		int a;
		int b;
	};

	// The roads, following the route in the design: the border leads to the
	// battlefield, and from there the light's land, the dark's, and the gray's.
	const Link kLinks[] =
	{
		{ Regions::Limina, Regions::LiminaWoods },
		{ Regions::LiminaWoods, Regions::Crepus },
		{ Regions::Crepus, Regions::Bellum },

		{ Regions::Bellum, Regions::Alba },
		{ Regions::Alba, Regions::Sola },
		{ Regions::Sola, Regions::Aura },
		{ Regions::Aura, Regions::LightGate },
		{ Regions::LightGate, Regions::Lumen },

		{ Regions::Bellum, Regions::Umbra },
		{ Regions::Umbra, Regions::Nox },
		{ Regions::Nox, Regions::Crypta },
		{ Regions::Crypta, Regions::DarkGate },
		{ Regions::DarkGate, Regions::Tenebrae },

		{ Regions::Bellum, Regions::AshMarch },
		{ Regions::AshMarch, Regions::Fides },
		{ Regions::AshMarch, Regions::Spes },
		{ Regions::AshMarch, Regions::Requiem },
		{ Regions::AshMarch, Regions::Luctus },
		{ Regions::AshMarch, Regions::Divisio },
		{ Regions::Divisio, Regions::Kinera },
	};

	const int kLinkCount = sizeof(kLinks) / sizeof(kLinks[0]);
}

const Regions::Info& Regions::Get(int region)
{
	return kRegions[region];
}

bool Regions::IsOffMap(int region)
{
	return region == Realm;
}

const Regions::Atmosphere& Regions::AtmosphereOf(int region)
{
	//                                 fog                       light  start  end   sat   contrast  tint
	static const Atmosphere kBorder = { { 0.050f, 0.046f, 0.042f }, 160.f, 220.f, 620.f, 0.85f, 1.00f, { 1.03f, 1.00f, 0.95f } };
	static const Atmosphere kWar = { { 0.045f, 0.043f, 0.050f }, 150.f, 210.f, 620.f, 0.70f, 1.00f, { 1.00f, 0.99f, 0.98f } };
	static const Atmosphere kLightLand = { { 0.060f, 0.052f, 0.040f }, 170.f, 240.f, 660.f, 0.90f, 1.04f, { 1.05f, 1.01f, 0.92f } };
	static const Atmosphere kLightCapital = { { 0.085f, 0.075f, 0.055f }, 260.f, 330.f, 780.f, 1.15f, 1.10f, { 1.07f, 1.03f, 0.94f } };
	static const Atmosphere kDarkLand = { { 0.040f, 0.032f, 0.065f }, 140.f, 200.f, 580.f, 0.90f, 1.06f, { 0.95f, 0.93f, 1.06f } };
	static const Atmosphere kDarkCapital = { { 0.050f, 0.035f, 0.085f }, 220.f, 290.f, 700.f, 1.10f, 1.12f, { 0.97f, 0.92f, 1.10f } };
	static const Atmosphere kAshen = { { 0.050f, 0.050f, 0.052f }, 140.f, 200.f, 560.f, 0.50f, 0.95f, { 0.97f, 0.97f, 0.98f } };
	// The heart of the gray: almost no colour left at all, and the dark close.
	static const Atmosphere kHeart = { { 0.055f, 0.055f, 0.056f }, 120.f, 170.f, 480.f, 0.30f, 0.90f, { 0.96f, 0.96f, 0.97f } };

	// The god's place: the dark pressed close and colour turned up hard, as
	// if everything were lit for someone watching.
	static const Atmosphere kRealm = { { 0.018f, 0.018f, 0.024f }, 220.f, 280.f, 620.f, 0.75f, 1.18f, { 1.00f, 1.00f, 1.02f } };

	if (region == Kinera)
	{
		return kHeart;
	}
	if (region == Realm)
	{
		return kRealm;
	}

	const Info& info = Get(region);
	bool capital = (info.kind == Kind::Capital);
	switch (info.style)
	{
	case Style::Light:
	{
		return capital ? kLightCapital : kLightLand;
	}
	case Style::Dark:
	{
		return capital ? kDarkCapital : kDarkLand;
	}
	case Style::Ashen:
	{
		return kAshen;
	}
	case Style::Borderborn:
	{
		return kBorder;
	}
	default:
	{
		return kWar;
	}
	}
}

unsigned int Regions::Seed(int region)
{
	return (unsigned int)region * 7919u + 13u;
}

bool Regions::AreLinked(int a, int b)
{
	for (int i = 0; i < kLinkCount; i++)
	{
		if ((kLinks[i].a == a && kLinks[i].b == b) || (kLinks[i].a == b && kLinks[i].b == a))
		{
			return true;
		}
	}
	return false;
}

int Regions::LinkCount()
{
	return kLinkCount;
}

void Regions::GetLink(int index, int& a, int& b)
{
	a = kLinks[index].a;
	b = kLinks[index].b;
}
