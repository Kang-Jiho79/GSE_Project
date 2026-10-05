#include "stdafx.h"
#include "Growth.h"
#include "PlayerStatus.h"
#include "Tuning.h"

bool Growth::Has(const PlayerStatus& status, int upgrade)
{
	return status.upgrades >= upgrade || status.ashUnlocked;
}

const char* Growth::GiftOf(int upgrade)
{
	if (upgrade == Tuning::Growth::kFormDodge)
	{
		return "폼마다 다른 회피 · 빛은 섬광 이동, 어둠은 그림자 돌진";
	}
	if (upgrade == Tuning::Growth::kSkills)
	{
		return "E · Q 해금";
	}
	if (upgrade == Tuning::Growth::kAttackI)
	{
		return "기본 공격 강화 · 빛 탄 두 갈래, 어둠 내려찍기에 칼날";
	}
	if (upgrade == Tuning::Growth::kSwap)
	{
		return "교대 스킬 해금 · 폼을 바꾸는 순간 터진다";
	}
	if (upgrade == Tuning::Growth::kSkillsII)
	{
		return "E · Q 강화";
	}
	if (upgrade == Tuning::Growth::kDodgeII)
	{
		return "회피 강화 · 떠난 자리가 터지고, 지나간 자리를 벤다";
	}
	if (upgrade == Tuning::Growth::kAttackII)
	{
		return "기본 공격 강화 · 빛 탄 세 갈래, 어둠 4연타";
	}
	if (upgrade == Tuning::Growth::kSwapII)
	{
		return "교대 스킬 강화";
	}
	return nullptr;
}
