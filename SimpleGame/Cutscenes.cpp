#include "stdafx.h"
#include "Cutscenes.h"

namespace
{
	using Cutscenes::Aim;
	using Cutscenes::Wash;
	using Cutscenes::Clock;

	// Anchored at the waystone: in close on him, across to the stone, back
	// to look down on the whole place, then a white wash that hides the cut
	// home.
	const Cutscenes::Shot kTest[] =
	{
		// seconds  aim           col   row   zoom  cut    bars   wash         from  to    clock         speaker  caption                                cue                        conversation
		{ 1.0f,     Aim::Player,  0.f,  0.f,  1.0f, false, true,  Wash::None,  0.f,  0.f,  Clock::Slow,  nullptr, nullptr,                               Cutscenes::NoCue,          -1 },
		{ 2.0f,     Aim::Player,  0.f,  0.f,  2.0f, false, true,  Wash::None,  0.f,  0.f,  Clock::Slow,  nullptr, "컷신 시험  ·  그에게 다가간다",         Cutscenes::NoCue,          -1 },
		{ 2.5f,     Aim::Anchor,  0.f,  0.f,  1.4f, false, true,  Wash::None,  0.f,  0.f,  Clock::Stop,  "시험",  "멈춘 세계 위로 카메라만 흘러간다",      Cutscenes::CueTestBurst,   -1 },
		{ 2.0f,     Aim::Anchor,  2.f,  2.f,  0.6f, false, true,  Wash::None,  0.f,  0.f,  Clock::Run,   nullptr, "물러나 내려다본다",                      Cutscenes::NoCue,          -1 },
		{ 1.0f,     Aim::Hold,    0.f,  0.f,  0.f,  false, true,  Wash::White, 0.f,  1.f,  Clock::Run,   nullptr, nullptr,                               Cutscenes::NoCue,          -1 },
		{ 1.2f,     Aim::Player,  0.f,  0.f,  1.0f, true,  false, Wash::White, 1.f,  0.f,  Clock::Run,   nullptr, nullptr,                               Cutscenes::NoCue,          -1 },
	};

	const Cutscenes::Script kScripts[Cutscenes::Count] =
	{
		{ kTest, sizeof(kTest) / sizeof(kTest[0]) },
	};
}

const Cutscenes::Script& Cutscenes::Get(int id)
{
	return kScripts[id];
}
