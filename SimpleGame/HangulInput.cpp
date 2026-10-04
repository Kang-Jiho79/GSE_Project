#include "stdafx.h"
#include "HangulInput.h"
#include "Utf8.h"

namespace
{
	// Consonants are numbered as lead consonants (초성):
	// ㄱ ㄲ ㄴ ㄷ ㄸ ㄹ ㅁ ㅂ ㅃ ㅅ ㅆ ㅇ ㅈ ㅉ ㅊ ㅋ ㅌ ㅍ ㅎ
	// and vowels as medial vowels (중성), in Unicode's order.
	const int kLeadCount = 19;
	const int kVowelCount = 21;
	const int kTailCount = 28;

	// A consonant standing alone, as the compatibility jamo.
	const unsigned int kLeadAlone[kLeadCount] =
	{
		0x3131, 0x3132, 0x3134, 0x3137, 0x3138, 0x3139, 0x3141, 0x3142, 0x3143, 0x3145,
		0x3146, 0x3147, 0x3148, 0x3149, 0x314A, 0x314B, 0x314C, 0x314D, 0x314E,
	};
	const unsigned int kVowelAlone = 0x314F;
	const unsigned int kSyllableBase = 0xAC00;

	// Each consonant as a tail (종성), or -1 for the three that cannot be one.
	const int kLeadToTail[kLeadCount] =
	{
		1, 2, 4, 7, -1, 8, 16, 17, -1, 19, 20, 21, 22, -1, 23, 24, 25, 26, 27,
	};

	struct Pair
	{
		int first;
		int second;
		int result;
	};

	// Two consonants that make one tail: ㄳ ㄵ ㄶ ㄺ ㄻ ㄼ ㄽ ㄾ ㄿ ㅀ ㅄ.
	const Pair kTailPairs[] =
	{
		{ 0, 9, 3 }, { 2, 12, 5 }, { 2, 18, 6 }, { 5, 0, 9 }, { 5, 6, 10 }, { 5, 7, 11 },
		{ 5, 9, 12 }, { 5, 16, 13 }, { 5, 17, 14 }, { 5, 18, 15 }, { 7, 9, 18 },
	};

	// Two vowels that make one: ㅘ ㅙ ㅚ ㅝ ㅞ ㅟ ㅢ.
	const Pair kVowelPairs[] =
	{
		{ 8, 0, 9 }, { 8, 1, 10 }, { 8, 20, 11 }, { 13, 4, 14 }, { 13, 5, 15 }, { 13, 20, 16 }, { 18, 20, 19 },
	};

	template <size_t N>
	int Combine(const Pair(&pairs)[N], int first, int second)
	{
		for (size_t i = 0; i < N; i++)
		{
			if (pairs[i].first == first && pairs[i].second == second)
			{
				return pairs[i].result;
			}
		}
		return -1;
	}

	// The two-set layout. A consonant comes back as its lead number, a vowel
	// as kVowelKey plus its number, anything else as -1.
	const int kVowelKey = 100;

	int JamoForKey(unsigned char key)
	{
		switch (key)
		{
		case 'Q': return 8;
		case 'W': return 13;
		case 'E': return 4;
		case 'R': return 1;
		case 'T': return 10;
		case 'O': return kVowelKey + 3;
		case 'P': return kVowelKey + 7;
		default: break;
		}

		switch ((key >= 'A' && key <= 'Z') ? key - 'A' + 'a' : key)
		{
		case 'q': return 7;
		case 'w': return 12;
		case 'e': return 3;
		case 'r': return 0;
		case 't': return 9;
		case 'a': return 6;
		case 's': return 2;
		case 'd': return 11;
		case 'f': return 5;
		case 'g': return 18;
		case 'z': return 15;
		case 'x': return 16;
		case 'c': return 14;
		case 'v': return 17;
		case 'y': return kVowelKey + 12;
		case 'u': return kVowelKey + 6;
		case 'i': return kVowelKey + 2;
		case 'o': return kVowelKey + 1;
		case 'p': return kVowelKey + 5;
		case 'h': return kVowelKey + 8;
		case 'j': return kVowelKey + 4;
		case 'k': return kVowelKey + 0;
		case 'l': return kVowelKey + 20;
		case 'b': return kVowelKey + 17;
		case 'n': return kVowelKey + 13;
		case 'm': return kVowelKey + 18;
		default: return -1;
		}
	}

	bool IsLatinOrDigit(unsigned char key)
	{
		return (key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9');
	}
}

HangulInput::HangulInput(int maxLength)
	: m_MaxLength(maxLength)
{
}

void HangulInput::Clear()
{
	m_Closed.clear();
	m_Open = Syllable();
	m_History.clear();
}

bool HangulInput::IsKorean() const
{
	return m_Korean;
}

void HangulInput::ToggleKorean()
{
	Close();
	m_Korean = !m_Korean;
}

bool HangulInput::IsOpen() const
{
	return m_Open.lead >= 0 || m_Open.vowel >= 0;
}

int HangulInput::Length() const
{
	return (int)m_Closed.size() + (IsOpen() ? 1 : 0);
}

bool HangulInput::HasRoom() const
{
	return Length() < m_MaxLength;
}

unsigned int HangulInput::Compose(const Syllable& syllable) const
{
	if (syllable.lead >= 0 && syllable.vowel >= 0)
	{
		int tail = (syllable.tail >= 0) ? syllable.tail : 0;
		return kSyllableBase + (syllable.lead * kVowelCount + syllable.vowel) * kTailCount + tail;
	}
	if (syllable.lead >= 0)
	{
		return kLeadAlone[syllable.lead];
	}
	return kVowelAlone + syllable.vowel;
}

void HangulInput::Change(const Syllable& next)
{
	m_History.push_back(m_Open);
	m_Open = next;
}

void HangulInput::Close()
{
	if (IsOpen())
	{
		m_Closed.push_back(Compose(m_Open));
	}
	m_Open = Syllable();
	m_History.clear();
}

void HangulInput::StartNew(const Syllable& next)
{
	m_History.clear();
	m_History.push_back(Syllable());
	m_Open = next;
}

void HangulInput::Type(unsigned char key)
{
	int jamo = m_Korean ? JamoForKey(key) : -1;
	if (jamo >= kVowelKey)
	{
		TypeVowel(jamo - kVowelKey);
		return;
	}
	if (jamo >= 0)
	{
		TypeConsonant(jamo);
		return;
	}

	if (IsLatinOrDigit(key))
	{
		Close();
		if (HasRoom())
		{
			m_Closed.push_back(key);
		}
	}
}

void HangulInput::TypeConsonant(int consonant)
{
	Syllable next = m_Open;

	// After a full lead and vowel, it may join as the tail - or join the tail
	// already there to make a double one.
	if (m_Open.lead >= 0 && m_Open.vowel >= 0)
	{
		if (m_Open.tail < 0 && kLeadToTail[consonant] >= 0)
		{
			next.tail = kLeadToTail[consonant];
			next.tailFirst = consonant;
			Change(next);
			return;
		}
		if (m_Open.tail >= 0 && m_Open.tailSecond < 0)
		{
			int pair = Combine(kTailPairs, m_Open.tailFirst, consonant);
			if (pair >= 0)
			{
				next.tail = pair;
				next.tailSecond = consonant;
				Change(next);
				return;
			}
		}
	}

	// Otherwise it begins the next syllable, if there is room for one.
	if (!HasRoom())
	{
		return;
	}
	Close();
	Syllable fresh;
	fresh.lead = consonant;
	StartNew(fresh);
}

void HangulInput::TypeVowel(int vowel)
{
	Syllable next = m_Open;

	// A lead waiting for its vowel.
	if (m_Open.lead >= 0 && m_Open.vowel < 0)
	{
		next.vowel = vowel;
		Change(next);
		return;
	}

	// A vowel with no tail yet may join it to make a double vowel.
	if (m_Open.vowel >= 0 && m_Open.tail < 0)
	{
		int pair = Combine(kVowelPairs, m_Open.vowel, vowel);
		if (pair >= 0)
		{
			next.vowel = pair;
			Change(next);
			return;
		}
	}

	if (!HasRoom())
	{
		return;
	}

	// A vowel after a tail takes the tail's last consonant with it into the
	// next syllable: 갑 then ㅏ is 가바.
	if (m_Open.tail >= 0)
	{
		Syllable kept = m_Open;
		int moved;
		if (m_Open.tailSecond >= 0)
		{
			moved = m_Open.tailSecond;
			kept.tail = kLeadToTail[m_Open.tailFirst];
			kept.tailSecond = -1;
		}
		else
		{
			moved = m_Open.tailFirst;
			kept.tail = -1;
			kept.tailFirst = -1;
		}
		m_Open = kept;
		Close();

		Syllable lead;
		lead.lead = moved;
		StartNew(lead);
		Syllable full = lead;
		full.vowel = vowel;
		Change(full);
		return;
	}

	Close();
	Syllable alone;
	alone.vowel = vowel;
	StartNew(alone);
}

void HangulInput::Backspace()
{
	if (IsOpen())
	{
		if (!m_History.empty())
		{
			m_Open = m_History.back();
			m_History.pop_back();
		}
		else
		{
			m_Open = Syllable();
		}
		return;
	}

	if (!m_Closed.empty())
	{
		m_Closed.pop_back();
	}
}

std::string HangulInput::ClosedText() const
{
	std::string text;
	for (size_t i = 0; i < m_Closed.size(); i++)
	{
		Utf8::Append(text, m_Closed[i]);
	}
	return text;
}

std::string HangulInput::OpenText() const
{
	std::string text;
	if (IsOpen())
	{
		Utf8::Append(text, Compose(m_Open));
	}
	return text;
}

std::string HangulInput::Text() const
{
	return ClosedText() + OpenText();
}
