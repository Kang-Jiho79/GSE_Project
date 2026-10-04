#pragma once

#include <string>
#include <vector>

// Typing Korean without the system's input method, which GLUT does not pass
// through: the keys of a standard two-set (두벌식) keyboard are read as jamo
// and put together into syllables here, the way the input method would.
// Tab switches between Korean and plain Latin letters.
//
// The syllable being built stays open - another consonant may still join it,
// or move on to the next syllable - until something closes it.
class HangulInput
{
public:
	explicit HangulInput(int maxLength);

	void Clear();
	bool IsKorean() const;
	void ToggleKorean();

	// A key as typed, with Shift already applied (capitals). Letters become
	// jamo in Korean mode; digits and Latin letters are taken as they are.
	// Anything else is ignored.
	void Type(unsigned char key);
	// Takes back the last jamo of the open syllable, or else the last character.
	void Backspace();

	// Everything typed, the open syllable included.
	std::string Text() const;
	// The part that is settled, and the syllable still being built (empty if
	// none), so the second can be shown as unfinished.
	std::string ClosedText() const;
	std::string OpenText() const;
	// How many characters there are, the open syllable included.
	int Length() const;

private:
	// The open syllable: a lead consonant, a vowel and a tail, each as an
	// index into its own table, -1 for none. A tail made of two consonants
	// keeps both, so it can be split again.
	struct Syllable
	{
		int lead = -1;
		int vowel = -1;
		int tail = -1;     // index into the tail table
		int tailFirst = -1; // the consonants it was made of, if it was two
		int tailSecond = -1;
	};

	void TypeConsonant(int consonant);
	void TypeVowel(int vowel);
	void Change(const Syllable& next);
	void Close();
	void StartNew(const Syllable& next);
	bool IsOpen() const;
	unsigned int Compose(const Syllable& syllable) const;
	bool HasRoom() const;

	int m_MaxLength;
	bool m_Korean = true;
	std::vector<unsigned int> m_Closed;
	Syllable m_Open;
	// What the open syllable was before each jamo went into it, for Backspace.
	std::vector<Syllable> m_History;
};
