#include "stdafx.h"
#include "Renderer.h"
#include <cstddef>

namespace
{
	// Enough for a full screen of terrain plus everything on it, so the queue
	// does not regrow mid-frame.
	const size_t kPendingReserve = 4096;
}

Renderer::Renderer(int windowSizeX, int windowSizeY)
{
	Initialize(windowSizeX, windowSizeY);
}

Renderer::~Renderer()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBOInstances);
}

void Renderer::Initialize(int windowSizeX, int windowSizeY)
{
	m_WindowSizeX = windowSizeX;
	m_WindowSizeY = windowSizeY;

	bool shaderLoaded = m_Shader.Load("./Shaders/SolidRect.vs", "./Shaders/SolidRect.fs");
	CreateVertexBufferObjects();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (shaderLoaded && m_VBORect > 0)
	{
		CacheShaderLocations();
		CreateVertexArray();
		m_Pending.reserve(kPendingReserve);
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
	m_AttribPosition = m_Shader.GetAttribLocation("a_Position");
	m_AttribCornerLo = m_Shader.GetAttribLocation("a_CornerLo");
	m_AttribCornerHi = m_Shader.GetAttribLocation("a_CornerHi");
	m_AttribColor = m_Shader.GetAttribLocation("a_Color");
}

void Renderer::CreateVertexArray()
{
	// The VAO records the whole attribute layout once. Flush only has to bind
	// it, upload the new instances and draw - the pointers stay valid because
	// they refer to the buffer objects, not to any particular upload.
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBORect);
	glEnableVertexAttribArray(m_AttribPosition);
	glVertexAttribPointer(m_AttribPosition, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

	glGenBuffers(1, &m_VBOInstances);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOInstances);

	// Divisor 1: advance these once per instance rather than once per vertex.
	glEnableVertexAttribArray(m_AttribCornerLo);
	glVertexAttribPointer(m_AttribCornerLo, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
		(const void*)offsetof(Instance, cornerLo));
	glVertexAttribDivisor(m_AttribCornerLo, 1);

	glEnableVertexAttribArray(m_AttribCornerHi);
	glVertexAttribPointer(m_AttribCornerHi, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
		(const void*)offsetof(Instance, cornerHi));
	glVertexAttribDivisor(m_AttribCornerHi, 1);

	glEnableVertexAttribArray(m_AttribColor);
	glVertexAttribPointer(m_AttribColor, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
		(const void*)offsetof(Instance, color));
	glVertexAttribDivisor(m_AttribColor, 1);

	glBindVertexArray(0);
}

void Renderer::DrawSolidRect(float x, float y, float width, float height, float r, float g, float b, float a)
{
	DrawSolidQuad(x, y, width, 0.f, 0.f, height, r, g, b, a);
}

void Renderer::DrawSolidDiamond(float x, float y, float width, float height, float r, float g, float b, float a)
{
	// local +x maps to the lower-right edge, local +y to the lower-left edge
	DrawSolidQuad(x, y, width / 2.f, height / 2.f, -width / 2.f, height / 2.f, r, g, b, a);
}

void Renderer::DrawSolidQuad(float x, float y, float edgeAX, float edgeAY, float edgeBX, float edgeBY,
	float r, float g, float b, float a)
{
	float halfAX = edgeAX / 2.f;
	float halfAY = edgeAY / 2.f;
	float halfBX = edgeBX / 2.f;
	float halfBY = edgeBY / 2.f;

	float corners[8] =
	{
		x - halfAX - halfBX, y - halfAY - halfBY, // bottom-left
		x - halfAX + halfBX, y - halfAY + halfBY, // top-left
		x + halfAX + halfBX, y + halfAY + halfBY, // top-right
		x + halfAX - halfBX, y + halfAY - halfBY, // bottom-right
	};

	QueueCorners(corners, r, g, b, a);
}

void Renderer::DrawSolidCorners(const float corners[8], float r, float g, float b, float a)
{
	QueueCorners(corners, r, g, b, a);
}

void Renderer::QueueCorners(const float corners[8], float r, float g, float b, float a)
{
	float scaleX = 2.f / m_WindowSizeX;
	float scaleY = 2.f / m_WindowSizeY;

	Instance instance;
	instance.cornerLo[0] = corners[0] * scaleX; // bottom-left
	instance.cornerLo[1] = corners[1] * scaleY;
	instance.cornerLo[2] = corners[6] * scaleX; // bottom-right
	instance.cornerLo[3] = corners[7] * scaleY;
	instance.cornerHi[0] = corners[2] * scaleX; // top-left
	instance.cornerHi[1] = corners[3] * scaleY;
	instance.cornerHi[2] = corners[4] * scaleX; // top-right
	instance.cornerHi[3] = corners[5] * scaleY;
	instance.color[0] = r;
	instance.color[1] = g;
	instance.color[2] = b;
	instance.color[3] = a;

	m_Pending.push_back(instance);
	m_QuadCount++;
}

void Renderer::Flush()
{
	if (m_Pending.empty())
	{
		return;
	}

	m_Shader.Use();
	glBindVertexArray(m_VAO);

	// STREAM_DRAW and a fresh glBufferData each flush: the driver orphans the
	// old storage instead of waiting for the GPU to finish reading it.
	glBindBuffer(GL_ARRAY_BUFFER, m_VBOInstances);
	glBufferData(GL_ARRAY_BUFFER, m_Pending.size() * sizeof(Instance), m_Pending.data(), GL_STREAM_DRAW);

	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)m_Pending.size());
	m_DrawCallCount++;

	glBindVertexArray(0);
	m_Pending.clear();
}

void Renderer::SetClip(float left, float bottom, float width, float height)
{
	Flush();

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
	Flush();
	glDisable(GL_SCISSOR_TEST);
}

void Renderer::EndFrame()
{
	Flush();
}

unsigned int Renderer::DrawCallCount() const
{
	return m_DrawCallCount;
}

unsigned int Renderer::QuadCount() const
{
	return m_QuadCount;
}

void Renderer::ResetCounts()
{
	m_DrawCallCount = 0;
	m_QuadCount = 0;
}
