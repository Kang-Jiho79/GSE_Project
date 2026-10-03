#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "Font.h"
#include <iostream>

namespace
{
	// Room for every Latin character and several hundred Hangul syllables at
	// dialogue size, before anything has to give.
	const int kAtlasSize = 1024;
	const int kGlyphPadding = 1;

	// GetGlyphOutline's 8-bit gray runs 0..64, not 0..255.
	const int kGrayLevels = 64;
}

Font::Font()
{
}

Font::~Font()
{
	HDC dc = (HDC)m_DeviceContext;
	if (dc != nullptr)
	{
		SelectObject(dc, (HGDIOBJ)m_PreviousFont);
		DeleteDC(dc);
	}
	if (m_FontHandle != nullptr)
	{
		DeleteObject((HGDIOBJ)m_FontHandle);
	}
	glDeleteTextures(1, &m_Texture);
}

bool Font::Initialize(const wchar_t* faceName, int pixelHeight, int weight)
{
	HDC dc = CreateCompatibleDC(nullptr);
	if (dc == nullptr)
	{
		std::cout << "Font: could not create a device context\n";
		return false;
	}

	// A negative height asks for the character height itself, not the cell.
	HFONT font = CreateFontW(-pixelHeight, 0, 0, 0, weight, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, faceName);
	if (font == nullptr)
	{
		std::wcout << L"Font: could not create '" << faceName << L"'\n";
		DeleteDC(dc);
		return false;
	}

	m_DeviceContext = dc;
	m_FontHandle = font;
	m_PreviousFont = SelectObject(dc, font);

	TEXTMETRICW metrics;
	GetTextMetricsW(dc, &metrics);
	m_LineHeight = (float)(metrics.tmHeight + metrics.tmExternalLeading);
	m_FallbackAdvance = (float)metrics.tmAveCharWidth;

	glGenTextures(1, &m_Texture);
	glBindTexture(GL_TEXTURE_2D, m_Texture);
	std::vector<unsigned char> blank(kAtlasSize * kAtlasSize, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, kAtlasSize, kAtlasSize, 0, GL_RED, GL_UNSIGNED_BYTE, blank.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

const Font::Glyph& Font::GetGlyph(unsigned int codepoint)
{
	std::unordered_map<unsigned int, Glyph>::iterator found = m_Glyphs.find(codepoint);
	if (found != m_Glyphs.end())
	{
		return found->second;
	}

	Glyph glyph;
	if (!RenderGlyph(codepoint, glyph))
	{
		glyph = Glyph();
		glyph.advance = m_FallbackAdvance;
	}

	return m_Glyphs.emplace(codepoint, glyph).first->second;
}

bool Font::RenderGlyph(unsigned int codepoint, Glyph& glyph)
{
	HDC dc = (HDC)m_DeviceContext;
	if (dc == nullptr || codepoint > 0xFFFF)
	{
		return false;
	}

	GLYPHMETRICS metrics;
	const MAT2 identity = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };

	DWORD size = GetGlyphOutlineW(dc, codepoint, GGO_GRAY8_BITMAP, &metrics, 0, nullptr, &identity);
	if (size == GDI_ERROR)
	{
		return false;
	}

	glyph.advance = (float)metrics.gmCellIncX;

	// A space and its kind: nothing to draw, only somewhere to move on to.
	if (size == 0)
	{
		return true;
	}

	m_Outline.resize(size);
	GetGlyphOutlineW(dc, codepoint, GGO_GRAY8_BITMAP, &metrics, size, m_Outline.data(), &identity);

	int width = (int)metrics.gmBlackBoxX;
	int height = (int)metrics.gmBlackBoxY;
	int pitch = (width + 3) & ~3; // rows come padded to four bytes

	if (m_PenX + width + kGlyphPadding > kAtlasSize)
	{
		m_PenX = 0;
		m_PenY += m_RowHeight + kGlyphPadding;
		m_RowHeight = 0;
	}
	if (m_PenY + height > kAtlasSize)
	{
		if (!m_Full)
		{
			std::cout << "Font: glyph texture is full - further characters will be blank\n";
			m_Full = true;
		}
		return false;
	}

	m_Pixels.resize(width * height);
	for (int row = 0; row < height; row++)
	{
		for (int column = 0; column < width; column++)
		{
			int gray = m_Outline[row * pitch + column];
			m_Pixels[row * width + column] = (unsigned char)(gray * 255 / kGrayLevels);
		}
	}

	glBindTexture(GL_TEXTURE_2D, m_Texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, m_PenX, m_PenY, width, height, GL_RED, GL_UNSIGNED_BYTE, m_Pixels.data());
	glBindTexture(GL_TEXTURE_2D, 0);

	// Rows were uploaded top first, so the top of the glyph is at v0.
	glyph.u0 = (float)m_PenX / kAtlasSize;
	glyph.v0 = (float)m_PenY / kAtlasSize;
	glyph.u1 = (float)(m_PenX + width) / kAtlasSize;
	glyph.v1 = (float)(m_PenY + height) / kAtlasSize;
	glyph.width = (float)width;
	glyph.height = (float)height;
	glyph.offsetX = (float)metrics.gmptGlyphOrigin.x;
	glyph.offsetY = (float)metrics.gmptGlyphOrigin.y;

	m_PenX += width + kGlyphPadding;
	if (height > m_RowHeight)
	{
		m_RowHeight = height;
	}

	return true;
}

float Font::LineHeight() const
{
	return m_LineHeight;
}

GLuint Font::Texture() const
{
	return m_Texture;
}
