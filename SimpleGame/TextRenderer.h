#pragma once

#include <vector>

#include "Dependencies\glew.h"
#include "Shader.h"
#include "Font.h"
#include "Palette.h"

// Text on screen, in the same pixel space as the Renderer: origin at the
// window centre, +y up. Strings are UTF-8, so Korean goes straight in.
//
// Like the Renderer, nothing is drawn on the spot: glyphs are queued and
// drawn in one instanced call per font at Flush, after the shapes, so text
// always sits on top of whatever was drawn in the same pass.
class TextRenderer
{
public:
	enum class Face
	{
		Title, // large serif, for what must be read at a glance
		Body,  // serif, for dialogue and messages
		Small, // plain sans, for numbers and short labels
	};

	enum class Align
	{
		Left,
		Center,
		Right,
	};

	~TextRenderer();

	bool Initialize(int windowSizeX, int windowSizeY);

	// y is the baseline of the first line. A line longer than wrapWidth breaks
	// at the last space before it (or mid-word if it has none); 0 never
	// wraps. '\n' always breaks. The shadow sits a pixel down and right.
	void Draw(Face face, const char* utf8, float x, float y, const Palette::Color& color, float alpha,
		Align align = Align::Left, float wrapWidth = 0.f, bool shadow = true);

	// Width of the widest line, unwrapped.
	float Measure(Face face, const char* utf8);
	float LineHeight(Face face);

	void Flush();

private:
	static const int kFaceCount = 3;

	struct Instance
	{
		float rect[4]; // clip space: left, bottom, right, top
		float uv[4];   // texture: left, bottom, right, top
		float color[4];
	};

	Font& FontFor(Face face);
	void BreakLines(Font& font, const std::vector<unsigned int>& text, float wrapWidth,
		std::vector<std::vector<unsigned int> >& lines);
	float LineWidth(Font& font, const std::vector<unsigned int>& line);
	void QueueLine(Face face, const std::vector<unsigned int>& line, float x, float y,
		float r, float g, float b, float a);

	bool m_Initialized = false;
	int m_WindowSizeX = 0;
	int m_WindowSizeY = 0;

	Font m_Fonts[kFaceCount];
	std::vector<Instance> m_Pending[kFaceCount];

	Shader m_Shader;
	GLuint m_QuadVBO = 0;
	GLuint m_InstanceVBO = 0;
	GLuint m_VAO = 0;

	// Reused from call to call so drawing text does not allocate every frame.
	std::vector<unsigned int> m_Codepoints;
	std::vector<std::vector<unsigned int> > m_Lines;
};
