#pragma once

#include "Alignment.h"

// Which of the two powers the player is fighting with right now. The keys stay
// the same in both; the form decides what they do.
enum class Form
{
	Light, // ranged
	Dark,  // melee
};

inline Alignment FormAlignment(Form form)
{
	return (form == Form::Light) ? Alignment::Light : Alignment::Dark;
}
