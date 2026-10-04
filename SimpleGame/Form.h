#pragma once

#include "Alignment.h"

// Which power the player is fighting with right now. The keys stay the same
// in every form; the form decides what they do. The third, the gray, only
// exists once he has taken it up at the very end.
enum class Form
{
	Light, // ranged
	Dark,  // melee
	Ash,   // between
};

inline Alignment FormAlignment(Form form)
{
	switch (form)
	{
	case Form::Light:
	{
		return Alignment::Light;
	}
	case Form::Dark:
	{
		return Alignment::Dark;
	}
	default:
	{
		return Alignment::Ashen;
	}
	}
}
