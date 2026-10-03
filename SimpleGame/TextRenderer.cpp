#include "stdafx.h"
#include "TextRenderer.h"
#include <cmath>
#include <cstddef>
#include <iostream>

namespace
{
	// Batang for anything read as words - its brushed serifs suit the world -
	// and Malgun Gothic where small numbers have to stay legible.
	const wchar_t* const kSerifFace = L"Batang";
	const wchar_t* const kSansFace = L"Malgun Gothic";

	const int kTitleSize = 36;
	const int kBodySize = 18;
	const int kSmallSize = 13;

	const int kRegular = 400;
	const int kBold = 700;

	const float kShadowAlpha = 0.85f;

	// Decodes UTF-8 into code points. A broken sequence becomes U+FFFD rather
	// than taking the rest of the string down with it.
	void DecodeUtf8(const char* text, std::vector<unsigned int>& out)
	{
		out.clear();
		const unsigned char* p = (const unsigned char*)text;
		while (*p != 0)
		{
			unsigned int c = *p;
			int extra = 0;
			if (c < 0x80)
			{
				extra = 0;
			}
			else if ((c & 0xE0) == 0xC0)
			{
				c &= 0x1F;
				extra = 1;
			}
			else if ((c & 0xF0) == 0xE0)
			{
				c &= 0x0F;
				extra = 2;
			}
			else if ((c & 0xF8) == 0xF0)
			{
				c &= 0x07;
				extra = 3;
			}
			else
			{
				out.push_back(0xFFFD);
				p++;
				continue;
			}

			p++;
			bool broken = false;
			for (int i = 0; i < extra; i++)
			{
				if ((*p & 0xC0) != 0x80)
				{
					broken = true;
					break;
				}
				c = (c << 6) | (*p & 0x3F);
				p++;
			}
			out.push_back(broken ? 0xFFFD : c);
		}
	}
}

TextRenderer::~TextRenderer()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_QuadVBO);
	glDeleteBuffers(1, &m_InstanceVBO);
}

bool TextRenderer::Initialize(int windowSizeX, int windowSizeY)
{
	m_WindowSizeX = windowSizeX;
	m_WindowSizeY = windowSizeY;

	bool fonts = m_Fonts[(int)Face::Title].Initialize(kSerifFace, kTitleSize, kBold)
		&& m_Fonts[(int)Face::Body].Initialize(kSerifFace, kBodySize, kRegular)
		&& m_Fonts[(int)Face::Small].Initialize(kSansFace, kSmallSize, kBold);
	if (!fonts || !m_Shader.Load("./Shaders/Text.vs", "./Shaders/Text.fs"))
	{
		std::cout << "Text could not be set up - drawing without it\n";
		return false;
	}

	static const float quad[] =
	{
		-0.5f, -0.5f, 0.f,  -0.5f, 0.5f, 0.f,  0.5f, 0.5f, 0.f,
		-0.5f, -0.5f, 0.f,   0.5f, 0.5f, 0.f,  0.5f, -0.5f, 0.f,
	};

	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	GLint position = m_Shader.GetAttribLocation("a_Position");
	GLint rect = m_Shader.GetAttribLocation("a_Rect");
	GLint uv = m_Shader.GetAttribLocation("a_Uv");
	GLint color = m_Shader.GetAttribLocation("a_Color");

	glGenBuffers(1, &m_QuadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glEnableVertexAttribArray(position);
	glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

	glGenBuffers(1, &m_InstanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);

	glEnableVertexAttribArray(rect);
	glVertexAttribPointer(rect, 4, GL_FLOAT, GL_FALSE, sizeof(Instance), (const void*)offsetof(Instance, rect));
	glVertexAttribDivisor(rect, 1);

	glEnableVertexAttribArray(uv);
	glVertexAttribPointer(uv, 4, GL_FLOAT, GL_FALSE, sizeof(Instance), (const void*)offsetof(Instance, uv));
	glVertexAttribDivisor(uv, 1);

	glEnableVertexAttribArray(color);
	glVertexAttribPointer(color, 4, GL_FLOAT, GL_FALSE, sizeof(Instance), (const void*)offsetof(Instance, color));
	glVertexAttribDivisor(color, 1);

	glBindVertexArray(0);

	m_Initialized = true;
	return true;
}

Font& TextRenderer::FontFor(Face face)
{
	return m_Fonts[(int)face];
}

float TextRenderer::LineHeight(Face face)
{
	return FontFor(face).LineHeight();
}

float TextRenderer::LineWidth(Font& font, const std::vector<unsigned int>& line)
{
	float width = 0.f;
	for (size_t i = 0; i < line.size(); i++)
	{
		width += font.GetGlyph(line[i]).advance;
	}
	return width;
}

void TextRenderer::BreakLines(Font& font, const std::vector<unsigned int>& text, float wrapWidth,
	std::vector<std::vector<unsigned int> >& lines)
{
	lines.clear();
	lines.push_back(std::vector<unsigned int>());

	float width = 0.f;
	int lastSpace = -1; // index in the current line

	for (size_t i = 0; i < text.size(); i++)
	{
		unsigned int c = text[i];
		std::vector<unsigned int>* line = &lines.back();

		if (c == '\n')
		{
			lines.push_back(std::vector<unsigned int>());
			width = 0.f;
			lastSpace = -1;
			continue;
		}

		float advance = font.GetGlyph(c).advance;
		if (wrapWidth > 0.f && width + advance > wrapWidth && !line->empty() && c != ' ')
		{
			// Break at the last space if there is one, carrying the word that
			// overran onto the next line; otherwise break right here.
			std::vector<unsigned int> carried;
			if (lastSpace >= 0)
			{
				carried.assign(line->begin() + lastSpace + 1, line->end());
				line->resize(lastSpace);
			}
			lines.push_back(carried);
			line = &lines.back();
			width = LineWidth(font, *line);
			lastSpace = -1;
		}

		if (c == ' ')
		{
			if (line->empty() && lines.size() > 1)
			{
				continue; // no line starts with the space it wrapped on
			}
			lastSpace = (int)line->size();
		}

		line->push_back(c);
		width += advance;
	}
}

float TextRenderer::Measure(Face face, const char* utf8)
{
	if (!m_Initialized)
	{
		return 0.f;
	}

	Font& font = FontFor(face);
	DecodeUtf8(utf8, m_Codepoints);
	BreakLines(font, m_Codepoints, 0.f, m_Lines);

	float widest = 0.f;
	for (size_t i = 0; i < m_Lines.size(); i++)
	{
		float width = LineWidth(font, m_Lines[i]);
		if (width > widest)
		{
			widest = width;
		}
	}
	return widest;
}

void TextRenderer::Draw(Face face, const char* utf8, float x, float y, const Palette::Color& color, float alpha,
	Align align, float wrapWidth, bool shadow)
{
	if (!m_Initialized)
	{
		return;
	}

	Font& font = FontFor(face);
	DecodeUtf8(utf8, m_Codepoints);
	BreakLines(font, m_Codepoints, wrapWidth, m_Lines);

	float lineY = y;
	for (size_t i = 0; i < m_Lines.size(); i++)
	{
		float width = LineWidth(font, m_Lines[i]);
		float lineX = x;
		if (align == Align::Center)
		{
			lineX = x - width * 0.5f;
		}
		else if (align == Align::Right)
		{
			lineX = x - width;
		}

		if (shadow)
		{
			QueueLine(face, m_Lines[i], lineX + 1.f, lineY - 1.f,
				Palette::kUiInk.r, Palette::kUiInk.g, Palette::kUiInk.b, alpha * kShadowAlpha);
		}
		QueueLine(face, m_Lines[i], lineX, lineY, color.r, color.g, color.b, alpha);

		lineY -= font.LineHeight();
	}
}

void TextRenderer::QueueLine(Face face, const std::vector<unsigned int>& line, float x, float y,
	float r, float g, float b, float a)
{
	Font& font = FontFor(face);
	std::vector<Instance>& pending = m_Pending[(int)face];

	float scaleX = 2.f / m_WindowSizeX;
	float scaleY = 2.f / m_WindowSizeY;

	// Snapped to whole pixels, so the glyphs stay as crisp as Windows drew them.
	float penX = floorf(x + 0.5f);
	float baseline = floorf(y + 0.5f);

	for (size_t i = 0; i < line.size(); i++)
	{
		const Font::Glyph& glyph = font.GetGlyph(line[i]);
		if (glyph.width > 0.f)
		{
			float left = penX + glyph.offsetX;
			float top = baseline + glyph.offsetY;

			Instance instance;
			instance.rect[0] = left * scaleX;
			instance.rect[1] = (top - glyph.height) * scaleY;
			instance.rect[2] = (left + glyph.width) * scaleX;
			instance.rect[3] = top * scaleY;
			instance.uv[0] = glyph.u0;
			instance.uv[1] = glyph.v1;
			instance.uv[2] = glyph.u1;
			instance.uv[3] = glyph.v0;
			instance.color[0] = r;
			instance.color[1] = g;
			instance.color[2] = b;
			instance.color[3] = a;
			pending.push_back(instance);
		}
		penX += glyph.advance;
	}
}

void TextRenderer::Flush()
{
	if (!m_Initialized)
	{
		return;
	}

	m_Shader.Use();
	glUniform1i(m_Shader.GetUniformLocation("u_Atlas"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);

	for (int face = 0; face < kFaceCount; face++)
	{
		std::vector<Instance>& pending = m_Pending[face];
		if (pending.empty())
		{
			continue;
		}

		glBindTexture(GL_TEXTURE_2D, m_Fonts[face].Texture());
		glBufferData(GL_ARRAY_BUFFER, pending.size() * sizeof(Instance), pending.data(), GL_STREAM_DRAW);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)pending.size());
		pending.clear();
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
