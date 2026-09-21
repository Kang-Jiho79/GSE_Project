#include "stdafx.h"
#include "Renderer.h"

Renderer::Renderer(int windowSizeX, int windowSizeY)
{
	Initialize(windowSizeX, windowSizeY);
}

void Renderer::Initialize(int windowSizeX, int windowSizeY)
{
	m_WindowSizeX = windowSizeX;
	m_WindowSizeY = windowSizeY;

	bool shaderLoaded = m_SolidRectShader.Load("./Shaders/SolidRect.vs", "./Shaders/SolidRect.fs");
	CreateVertexBufferObjects();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (shaderLoaded && m_VBORect > 0)
	{
		CacheShaderLocations();
		m_Initialized = true;
	}
}

bool Renderer::IsInitialized()
{
	return m_Initialized;
}

void Renderer::CreateVertexBufferObjects()
{
	// A unit quad centered on the origin. Every shape is this quad under a 2x2 transform.
	static const float rect[] =
	{
		-0.5f, -0.5f, 0.f,  -0.5f, 0.5f, 0.f,  0.5f, 0.5f, 0.f,
		-0.5f, -0.5f, 0.f,   0.5f, 0.5f, 0.f,  0.5f, -0.5f, 0.f,
	};

	m_VBORect = m_MeshCache.GetOrCreate("unit_quad", rect, sizeof(rect));
}

void Renderer::CacheShaderLocations()
{
	// Looked up once: the scene submits hundreds of quads per frame.
	m_LocTrans = m_SolidRectShader.GetUniformLocation("u_Trans");
	m_LocTransform = m_SolidRectShader.GetUniformLocation("u_Transform");
	m_LocColor = m_SolidRectShader.GetUniformLocation("u_Color");
	m_AttribPosition = m_SolidRectShader.GetAttribLocation("a_Position");
}

void Renderer::DrawSolidRect(float x, float y, float width, float height, float r, float g, float b, float a)
{
	float transform[4] = { width, 0.f, 0.f, height };
	SubmitQuad(x, y, transform, r, g, b, a);
}

void Renderer::DrawSolidDiamond(float x, float y, float width, float height, float r, float g, float b, float a)
{
	// local +x maps to the lower-right edge, local +y to the lower-left edge
	float transform[4] = { width / 2.f, height / 2.f, -width / 2.f, height / 2.f };
	SubmitQuad(x, y, transform, r, g, b, a);
}

void Renderer::DrawSolidQuad(float x, float y, float edgeAX, float edgeAY, float edgeBX, float edgeBY,
	float r, float g, float b, float a)
{
	float transform[4] = { edgeAX, edgeAY, edgeBX, edgeBY };
	SubmitQuad(x, y, transform, r, g, b, a);
}

void Renderer::SubmitQuad(float x, float y, const float transform[4], float r, float g, float b, float a)
{
	float scaleX = 2.f / m_WindowSizeX;
	float scaleY = 2.f / m_WindowSizeY;

	float ndc[4] =
	{
		transform[0] * scaleX, transform[1] * scaleY,
		transform[2] * scaleX, transform[3] * scaleY,
	};

	m_SolidRectShader.Use();
	glUniform2f(m_LocTrans, x * scaleX, y * scaleY);
	glUniformMatrix2fv(m_LocTransform, 1, GL_FALSE, ndc);
	glUniform4f(m_LocColor, r, g, b, a);

	glEnableVertexAttribArray(m_AttribPosition);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBORect);
	glVertexAttribPointer(m_AttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	m_DrawCallCount++;

	glDisableVertexAttribArray(m_AttribPosition);
}

unsigned int Renderer::DrawCallCount() const
{
	return m_DrawCallCount;
}

void Renderer::ResetDrawCallCount()
{
	m_DrawCallCount = 0;
}

void Renderer::SetClip(float left, float bottom, float width, float height)
{
	int clipWidth = (int)(width + 0.5f);
	int clipHeight = (int)(height + 0.5f);
	if (clipWidth < 0)
	{
		clipWidth = 0;
	}
	if (clipHeight < 0)
	{
		clipHeight = 0;
	}

	glEnable(GL_SCISSOR_TEST);
	glScissor((int)(m_WindowSizeX / 2.f + left), (int)(m_WindowSizeY / 2.f + bottom), clipWidth, clipHeight);
}

void Renderer::ClearClip()
{
	glDisable(GL_SCISSOR_TEST);
}
