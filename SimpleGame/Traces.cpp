#include "stdafx.h"
#include "Traces.h"
#include "WorldGrid.h"
#include "Props.h"
#include "Tuning.h"
#include <vector>
#include <cmath>

namespace
{
	struct Place
	{
		float density = 0.f;
		float lightShare = 0.5f;
		float mixedShare = 0.f;
	};

	std::vector<Place> s_Places;

	const Place* PlaceAt(int zone)
	{
		return (zone >= 0 && zone < (int)s_Places.size()) ? &s_Places[zone] : nullptr;
	}
}

void Traces::Clear()
{
	s_Places.clear();
}

void Traces::SetPlace(int zone, float density, float lightShare, float mixedShare)
{
	if (zone < 0)
	{
		return;
	}
	if (zone >= (int)s_Places.size())
	{
		s_Places.resize(zone + 1);
	}
	s_Places[zone].density = density;
	s_Places[zone].lightShare = lightShare;
	s_Places[zone].mixedShare = mixedShare;
}

float Traces::LightLean(int col, int row)
{
	const Place* place = PlaceAt(WorldGrid::ZoneAt(col, row));
	float share = (place != nullptr) ? place->lightShare : 0.5f;
	// Patches swing widest where the two sides are evenly matched; in a land
	// that is mostly one side's, the other's shows only here and there.
	float even = 1.f - fabsf(share - 0.5f) * 2.f;
	float patch = WorldGrid::Noise((float)col + 5113.f, (float)row + 2741.f, Tuning::Trace::kPatchCell);
	float lean = share + (patch - 0.5f) * Tuning::Trace::kPatchSwing * even;
	// Pushed away from even, so each patch is plainly one side's.
	lean = 0.5f + (lean - 0.5f) * Tuning::Trace::kContrast;
	return (lean < 0.f) ? 0.f : ((lean > 1.f) ? 1.f : lean);
}

Traces::Kind Traces::At(int col, int row, int& variant)
{
	variant = 0;
	const Place* place = PlaceAt(WorldGrid::ZoneAt(col, row));
	if (place == nullptr || place->density <= 0.f)
	{
		return Kind::None;
	}

	unsigned int hash = WorldGrid::Hash(col + 9241, row + 6007);
	float roll = (hash % 1000) / 1000.f;
	if (roll >= Tuning::Trace::kChance * place->density)
	{
		return Kind::None;
	}
	// Not in a clearing, nor where something already stands.
	if (Props::IsClear(col, row) || Props::At(col, row) != Props::Kind::None)
	{
		return Kind::None;
	}

	variant = (hash / 1000) % 8;
	float side = ((hash / 8000) % 1000) / 1000.f;
	int pick = (hash / 8000000) % 5;

	if (side < place->mixedShare)
	{
		return (pick < 3) ? Kind::Wreck : Kind::FlickerCrack;
	}
	// Which side, by the patch it lies in.
	float lean = LightLean(col, row);
	float which = ((hash / 64) % 997) / 997.f;
	if (which < lean)
	{
		switch (pick)
		{
		case 0:
		{
			return Kind::LightCrack;
		}
		case 1:
		{
			return Kind::Lamp;
		}
		case 2:
		{
			return Kind::HaloStatue;
		}
		case 3:
		{
			return Kind::Banner;
		}
		default:
		{
			return Kind::LightMotes;
		}
		}
	}
	switch (pick)
	{
	case 0:
	{
		return Kind::DarkCrack;
	}
	case 1:
	{
		return Kind::Thorns;
	}
	case 2:
	{
		return Kind::Embers;
	}
	case 3:
	{
		return Kind::Crystal;
	}
	default:
	{
		return Kind::ShadowMotes;
	}
	}
}

bool Traces::Glows(Kind kind)
{
	switch (kind)
	{
	case Kind::LightCrack:
	case Kind::Lamp:
	case Kind::HaloStatue:
	case Kind::LightMotes:
	case Kind::DarkCrack:
	case Kind::Embers:
	case Kind::Crystal:
	case Kind::FlickerCrack:
	{
		return true;
	}
	default:
	{
		return false;
	}
	}
}
