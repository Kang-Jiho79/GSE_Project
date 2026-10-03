#pragma once

#include <unordered_map>
#include <vector>

#include "Dependencies\glew.h"

// One typeface at one size, drawn by Windows itself (GDI) and kept in a
// single texture. A glyph is only drawn the first time it is asked for, so a
// Korean font costs the handful of syllables actually used rather than all
// eleven thousand.
class Font
{
public:
	struct Glyph
	{
		// Where it sits in the texture: left, top, right, bottom.
		float u0 = 0.f;
		float v0 = 0.f;
		float u1 = 0.f;
		float v1 = 0.f;

		// Its inked box in pixels, and where that box sits from the pen on
		// the baseline: right by offsetX, its top up by offsetY.
		float width = 0.f;
		float height = 0.f;
		float offsetX = 0.f;
		float offsetY = 0.f;

		// How far the pen moves on after it.
		float advance = 0.f;
	};

	Font();
	~Font();

	// weight follows Windows: 400 regular, 700 bold.
	bool Initialize(const wchar_t* faceName, int pixelHeight, int weight);

	// Draws it into the texture the first time. A character the face does
	// not have comes back as an empty glyph that still advances.
	const Glyph& GetGlyph(unsigned int codepoint);

	float LineHeight() const;
	GLuint Texture() const;

private:
	bool RenderGlyph(unsigned int codepoint, Glyph& glyph);

	// Windows handles, kept opaque so this header does not drag in windows.h.
	void* m_DeviceContext = nullptr;
	void* m_FontHandle = nullptr;
	void* m_PreviousFont = nullptr;

	GLuint m_Texture = 0;
	float m_LineHeight = 0.f;
	float m_FallbackAdvance = 0.f;

	// Glyphs are packed in rows: along the current row until it is full,
	// then down to a new one.
	int m_PenX = 0;
	int m_PenY = 0;
	int m_RowHeight = 0;
	bool m_Full = false;

	std::unordered_map<unsigned int, Glyph> m_Glyphs;
	std::vector<unsigned char> m_Outline;
	std::vector<unsigned char> m_Pixels;
};
