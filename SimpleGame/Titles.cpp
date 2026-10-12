#include "stdafx.h"
#include "Titles.h"
#include "Progress.h"

int Titles::Held(const Progress& progress, const char* held[kMaxHeld])
{
	int count = 0;
	held[count++] = IsUnbound(progress) ? kOffTheBoard : kChosenByGod;
	if (progress.recognizedByLucius)
	{
		held[count++] = kDarkestLight;
	}
	if (progress.recognizedByNoctis)
	{
		held[count++] = kBrightestDark;
	}
	return count;
}

bool Titles::IsUnbound(const Progress& progress)
{
	return progress.Has(Story::EscapedGodInfluence);
}
