#include "stdafx.h"
#include "Cutscenes.h"
#include "Dialogue.h"

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
		// seconds  aim           col   row   zoom  cut    bars   wash         from  to    clock         speaker  caption                                cue                        conversation  portrait                   gray
		{ 1.0f,     Aim::Player,  0.f,  0.f,  1.0f, false, true,  Wash::None,  0.f,  0.f,  Clock::Slow,  nullptr, nullptr,                               Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
		{ 2.0f,     Aim::Player,  0.f,  0.f,  2.0f, false, true,  Wash::None,  0.f,  0.f,  Clock::Slow,  nullptr, "컷신 시험  ·  그에게 다가간다",         Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
		{ 2.5f,     Aim::Anchor,  0.f,  0.f,  1.4f, false, true,  Wash::None,  0.f,  0.f,  Clock::Stop,  "시험",  "멈춘 세계 위로 카메라만 흘러간다",      Cutscenes::CueTestBurst,   -1, Cutscenes::NoPortrait, false },
		{ 2.0f,     Aim::Anchor,  2.f,  2.f,  0.6f, false, true,  Wash::None,  0.f,  0.f,  Clock::Run,   nullptr, "물러나 내려다본다",                      Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
		{ 1.0f,     Aim::Hold,    0.f,  0.f,  0.f,  false, true,  Wash::White, 0.f,  1.f,  Clock::Run,   nullptr, nullptr,                               Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
		{ 1.2f,     Aim::Player,  0.f,  0.f,  1.0f, true,  false, Wash::White, 1.f,  0.f,  Clock::Run,   nullptr, nullptr,                               Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
	};

	// Anchored at the king he has just asked. In on it as its words break
	// off; the mark comes and turns over it; then everything goes white.
	const Cutscenes::Shot kIntervention[] =
	{
		// seconds  aim           col   row   zoom  cut    bars   wash         from  to    clock         speaker  caption  cue                        conversation  portrait                   gray
		{ 0.8f,     Aim::Anchor,  0.f,  0.f,  1.6f, false, true,  Wash::None,  0.f,  0.f,  Clock::Run,   nullptr, nullptr, Cutscenes::CueGodMark,     -1, Cutscenes::NoPortrait, false },
		{ 1.8f,     Aim::Anchor,  0.f,  0.f,  1.9f, false, true,  Wash::None,  0.f,  0.f,  Clock::Run,   nullptr, nullptr, Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
		{ 1.0f,     Aim::Hold,    0.f,  0.f,  2.1f, false, true,  Wash::White, 0.f,  1.f,  Clock::Run,   nullptr, nullptr, Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
		{ 0.4f,     Aim::Hold,    0.f,  0.f,  0.f,  false, true,  Wash::White, 1.f,  1.f,  Clock::Run,   nullptr, nullptr, Cutscenes::CueIntervene,   -1, Cutscenes::NoPortrait, false },
	};

	// At Bellum, the white thinning off him; he does not know how he came
	// to be here.
	const Cutscenes::Shot kIntervened[] =
	{
		// seconds  aim           col   row   zoom  cut    bars   wash         from  to    clock         speaker  caption  cue                        conversation  portrait                   gray
		{ 1.6f,     Aim::Player,  0.f,  0.f,  1.5f, true,  true,  Wash::White, 1.f,  0.f,  Clock::Run,   nullptr, nullptr, Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
		{ 0.4f,     Aim::Player,  0.f,  0.f,  0.f,  false, true,  Wash::None,  0.f,  0.f,  Clock::Run,   nullptr, nullptr, Cutscenes::NoCue,          Dialogue::Intervened, Cutscenes::NoPortrait, false },
		{ 0.8f,     Aim::Player,  0.f,  0.f,  1.0f, false, false, Wash::None,  0.f,  0.f,  Clock::Run,   nullptr, nullptr, Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
	};

	// The specials' cut-ins: in on him as the world stops and darkens, his
	// eyes across the screen, then back out as it runs again and the special
	// goes off. About a second, since they come often.
	const Cutscenes::Shot kLimitLight[] =
	{
		// seconds  aim           col   row   zoom  cut    bars   wash         from   to     clock         speaker  caption  cue                        conversation  portrait                   gray
		{ 0.15f,    Aim::Player,  0.f,  0.f,  1.3f, false, true,  Wash::Black, 0.f,   0.45f, Clock::Stop,  nullptr, nullptr, Cutscenes::NoCue,          -1,           Cutscenes::PortraitLight,  false },
		{ 0.7f,     Aim::Player,  0.f,  0.f,  1.4f, false, true,  Wash::Black, 0.45f, 0.45f, Clock::Stop,  nullptr, nullptr, Cutscenes::NoCue,          -1,           Cutscenes::PortraitLight,  false },
		{ 0.25f,    Aim::Player,  0.f,  0.f,  1.0f, false, false, Wash::Black, 0.45f, 0.f,   Clock::Run,   nullptr, nullptr, Cutscenes::CueLimit,       -1,           Cutscenes::NoPortrait,     false },
	};

	const Cutscenes::Shot kLimitDark[] =
	{
		// seconds  aim           col   row   zoom  cut    bars   wash         from   to     clock         speaker  caption  cue                        conversation  portrait                   gray
		{ 0.15f,    Aim::Player,  0.f,  0.f,  1.3f, false, true,  Wash::Black, 0.f,   0.45f, Clock::Stop,  nullptr, nullptr, Cutscenes::NoCue,          -1,           Cutscenes::PortraitDark,   false },
		{ 0.7f,     Aim::Player,  0.f,  0.f,  1.4f, false, true,  Wash::Black, 0.45f, 0.45f, Clock::Stop,  nullptr, nullptr, Cutscenes::NoCue,          -1,           Cutscenes::PortraitDark,   false },
		{ 0.25f,    Aim::Player,  0.f,  0.f,  1.0f, false, false, Wash::Black, 0.45f, 0.f,   Clock::Run,   nullptr, nullptr, Cutscenes::CueLimit,       -1,           Cutscenes::NoPortrait,     false },
	};

	// The gray's: the colour goes with the cut-in, and the special goes off
	// while the world is still stopped - he crosses it, and only the cuts
	// he leaves are seen, until it runs again and they burst.
	const Cutscenes::Shot kLimitAsh[] =
	{
		// seconds  aim           col   row   zoom  cut    bars   wash         from   to     clock         speaker  caption  cue                        conversation  portrait                   gray
		{ 0.15f,    Aim::Player,  0.f,  0.f,  1.3f, false, true,  Wash::Black, 0.f,   0.45f, Clock::Stop,  nullptr, nullptr, Cutscenes::NoCue,          -1,           Cutscenes::PortraitAsh,    true },
		{ 0.7f,     Aim::Player,  0.f,  0.f,  1.4f, false, true,  Wash::Black, 0.45f, 0.45f, Clock::Stop,  nullptr, nullptr, Cutscenes::NoCue,          -1,           Cutscenes::PortraitAsh,    true },
		{ 0.6f,     Aim::Player,  0.f,  0.f,  1.1f, false, true,  Wash::Black, 0.45f, 0.15f, Clock::Stop,  nullptr, nullptr, Cutscenes::CueLimit,       -1,           Cutscenes::NoPortrait,     true },
		{ 0.25f,    Aim::Player,  0.f,  0.f,  1.0f, false, false, Wash::Black, 0.15f, 0.f,   Clock::Run,   nullptr, nullptr, Cutscenes::NoCue,          -1,           Cutscenes::NoPortrait,     false },
	};

	// Anchored at the king he has come back to. The mark over it, the white
	// - and out of it, a king and a queen of the board.
	const Cutscenes::Shot kTransform[] =
	{
		// seconds  aim           col   row   zoom  cut    bars   wash         from  to    clock         speaker  caption               cue                        conversation  portrait                   gray
		{ 0.8f,     Aim::Anchor,  1.f,  0.f,  1.5f, false, true,  Wash::None,  0.f,  0.f,  Clock::Run,   nullptr, nullptr,              Cutscenes::CueGodMark,     -1, Cutscenes::NoPortrait, false },
		{ 1.6f,     Aim::Anchor,  1.f,  0.f,  1.8f, false, true,  Wash::None,  0.f,  0.f,  Clock::Run,   nullptr, nullptr,              Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
		{ 1.0f,     Aim::Hold,    0.f,  0.f,  0.f,  false, true,  Wash::White, 0.f,  1.f,  Clock::Run,   nullptr, nullptr,              Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
		{ 0.3f,     Aim::Hold,    0.f,  0.f,  0.f,  false, true,  Wash::White, 1.f,  1.f,  Clock::Run,   nullptr, nullptr,              Cutscenes::CueTransform,   -1, Cutscenes::NoPortrait, false },
		{ 1.8f,     Aim::Anchor,  1.f,  0.f,  1.2f, false, true,  Wash::White, 1.f,  0.f,  Clock::Run,   nullptr, "판이 말을 되찾는다.",  Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
		{ 0.6f,     Aim::Player,  0.f,  0.f,  1.0f, false, false, Wash::None,  0.f,  0.f,  Clock::Run,   nullptr, nullptr,              Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
	};

	// Anchored at the king of the board as it falls: slowed, named, and gone
	// in the white.
	const Cutscenes::Shot kCheckmate[] =
	{
		// seconds  aim           col   row   zoom  cut    bars   wash         from  to    clock         speaker  caption       cue                        conversation  portrait                   gray
		{ 1.6f,     Aim::Anchor,  0.f,  0.f,  1.7f, false, true,  Wash::None,  0.f,  0.f,  Clock::Slow,  nullptr, "체크메이트",   Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
		{ 1.3f,     Aim::Anchor,  0.f,  0.f,  1.4f, false, true,  Wash::None,  0.f,  0.f,  Clock::Run,   nullptr, "체크메이트",   Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
		{ 0.5f,     Aim::Hold,    0.f,  0.f,  0.f,  false, true,  Wash::White, 0.f,  1.f,  Clock::Run,   nullptr, nullptr,      Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
		{ 0.2f,     Aim::Hold,    0.f,  0.f,  0.f,  false, true,  Wash::White, 1.f,  1.f,  Clock::Run,   nullptr, nullptr,      Cutscenes::CueRestorePair, -1, Cutscenes::NoPortrait, false },
		{ 1.0f,     Aim::Player,  0.f,  0.f,  1.0f, true,  false, Wash::White, 1.f,  0.f,  Clock::Run,   nullptr, nullptr,      Cutscenes::NoCue,          -1, Cutscenes::NoPortrait, false },
	};

	// Anchored at him, at the gray's heart: the white, and out of it the
	// pieces left on the board, close round him.
	const Cutscenes::Shot kCheckmateEnd[] =
	{
		// seconds  aim           col   row   zoom  cut    bars   wash         from  to    clock         speaker  caption       cue                         conversation  portrait                   gray
		{ 0.7f,     Aim::Player,  0.f,  0.f,  1.3f, false, true,  Wash::White, 0.f,  1.f,  Clock::Run,   nullptr, nullptr,      Cutscenes::NoCue,           -1, Cutscenes::NoPortrait, false },
		{ 0.2f,     Aim::Player,  0.f,  0.f,  0.f,  false, true,  Wash::White, 1.f,  1.f,  Clock::Run,   nullptr, nullptr,      Cutscenes::CueCheckmateRise, -1, Cutscenes::NoPortrait, false },
		{ 1.4f,     Aim::Player,  0.f,  0.f,  1.5f, false, true,  Wash::White, 1.f,  0.f,  Clock::Run,   nullptr, "체크.",       Cutscenes::NoCue,           -1, Cutscenes::NoPortrait, false },
		{ 1.8f,     Aim::Player,  0.f,  0.f,  1.9f, false, true,  Wash::None,  0.f,  0.f,  Clock::Stop,  nullptr, "체크메이트.",  Cutscenes::NoCue,           -1, Cutscenes::NoPortrait, false },
		{ 1.2f,     Aim::Hold,    0.f,  0.f,  0.f,  false, true,  Wash::Black, 0.f,  1.f,  Clock::Stop,  nullptr, nullptr,      Cutscenes::NoCue,           -1, Cutscenes::NoPortrait, false },
		{ 0.3f,     Aim::Hold,    0.f,  0.f,  0.f,  false, true,  Wash::Black, 1.f,  1.f,  Clock::Stop,  nullptr, nullptr,      Cutscenes::CueCheckmateEnd,  -1, Cutscenes::NoPortrait, false },
	};

	const Cutscenes::Script kScripts[Cutscenes::Count] =
	{
		{ kTest, sizeof(kTest) / sizeof(kTest[0]) },
		{ kIntervention, sizeof(kIntervention) / sizeof(kIntervention[0]) },
		{ kIntervened, sizeof(kIntervened) / sizeof(kIntervened[0]) },
		{ kLimitLight, sizeof(kLimitLight) / sizeof(kLimitLight[0]) },
		{ kLimitDark, sizeof(kLimitDark) / sizeof(kLimitDark[0]) },
		{ kLimitAsh, sizeof(kLimitAsh) / sizeof(kLimitAsh[0]) },
		{ kTransform, sizeof(kTransform) / sizeof(kTransform[0]) },
		{ kCheckmate, sizeof(kCheckmate) / sizeof(kCheckmate[0]) },
		{ kCheckmateEnd, sizeof(kCheckmateEnd) / sizeof(kCheckmateEnd[0]) },
	};
}

const Cutscenes::Script& Cutscenes::Get(int id)
{
	return kScripts[id];
}
