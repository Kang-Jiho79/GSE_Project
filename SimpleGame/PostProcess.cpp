#include "stdafx.h"
#include "PostProcess.h"
#include "Palette.h"
#include <iostream>

namespace
{
	const float kBloomIntensity = 1.1f;
	// Each pass blurs across and then down once more, widening the halo.
	const int kBlurPasses = 2;
	// Taps spaced wider than a texel spread the halo further for the same cost.
	const float kBlurSpread = 1.5f;

	// Outline: this many pixels wide, laid this far over what is behind.
	const float kOutlineWidth = 1.5f;
	const float kOutlineStrength = 0.85f;
	// Rim: a band this many pixels in from the edge that faces the light,
	// which comes from up and to the right like the light on every box.
	const float kRimWidth = 2.f;
	const float kRimStrength = 0.22f;
	const float kRimDirX = 0.7071f;
	const float kRimDirY = 0.7071f;
}

PostProcess::~PostProcess()
{
	glDeleteFramebuffers(1, &m_SceneFramebuffer);
	glDeleteFramebuffers(2, m_BloomFramebuffer);
	glDeleteTextures(1, &m_SceneColor);
	glDeleteTextures(1, &m_SceneAux);
	glDeleteTextures(2, m_BloomTexture);
	glDeleteVertexArrays(1, &m_TriangleVAO);
	glDeleteBuffers(1, &m_TriangleVBO);
}

bool PostProcess::Initialize(int width, int height)
{
	m_Width = width;
	m_Height = height;
	m_BloomWidth = width / 2;
	m_BloomHeight = height / 2;

	bool loaded = m_Extract.Load("./Shaders/Fullscreen.vs", "./Shaders/BloomExtract.fs")
		&& m_Blur.Load("./Shaders/Fullscreen.vs", "./Shaders/Blur.fs")
		&& m_Composite.Load("./Shaders/Fullscreen.vs", "./Shaders/Composite.fs");
	if (!loaded)
	{
		std::cout << "Post-process shaders failed to load - drawing without bloom\n";
		return false;
	}

	if (!CreateTargets())
	{
		std::cout << "Post-process targets could not be created - drawing without bloom\n";
		return false;
	}

	CreateFullscreenTriangle();

	m_Ready = true;
	return true;
}

void PostProcess::SetEnabled(bool enabled)
{
	m_Enabled = enabled;
}

bool PostProcess::IsEnabled() const
{
	return m_Ready && m_Enabled;
}

void PostProcess::SetGrade(const Grade& grade)
{
	m_Grade = grade;
}

void PostProcess::SetOutlineEnabled(bool enabled)
{
	m_OutlineEnabled = enabled;
}

bool PostProcess::IsOutlineEnabled() const
{
	return m_OutlineEnabled;
}

GLuint PostProcess::CreateTexture(int width, int height, GLint internalFormat, GLenum format, GLenum type)
{
	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	return texture;
}

bool PostProcess::CheckFramebuffer(const char* name)
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Framebuffer '" << name << "' incomplete: 0x" << std::hex << status << std::dec << "\n";
		return false;
	}
	return true;
}

bool PostProcess::CreateTargets()
{
	// Half-float, so light added on light can go past white before the
	// composite brings it back down.
	m_SceneColor = CreateTexture(m_Width, m_Height, GL_RGBA16F, GL_RGBA, GL_FLOAT);
	m_SceneAux = CreateTexture(m_Width, m_Height, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);

	glGenFramebuffers(1, &m_SceneFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SceneColor, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_SceneAux, 0);
	const GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, drawBuffers);
	bool complete = CheckFramebuffer("scene");

	glGenFramebuffers(2, m_BloomFramebuffer);
	for (int i = 0; i < 2 && complete; i++)
	{
		m_BloomTexture[i] = CreateTexture(m_BloomWidth, m_BloomHeight, GL_RGBA16F, GL_RGBA, GL_FLOAT);
		glBindFramebuffer(GL_FRAMEBUFFER, m_BloomFramebuffer[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BloomTexture[i], 0);
		complete = CheckFramebuffer("bloom");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return complete;
}

void PostProcess::CreateFullscreenTriangle()
{
	static const float triangle[] =
	{
		-1.f, -1.f,
		3.f, -1.f,
		-1.f, 3.f,
	};

	glGenVertexArrays(1, &m_TriangleVAO);
	glBindVertexArray(m_TriangleVAO);

	glGenBuffers(1, &m_TriangleVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_TriangleVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);

	// Location 0 is fixed in Fullscreen.vs, so every pass shares this layout.
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

	glBindVertexArray(0);
}

void PostProcess::BindTexture(int unit, GLuint texture)
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture);
}

void PostProcess::BeginScene(float clearR, float clearG, float clearB)
{
	if (!IsEnabled())
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(clearR, clearG, clearB, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFramebuffer);
	glViewport(0, 0, m_Width, m_Height);

	const float clearColor[4] = { clearR, clearG, clearB, 1.f };
	const float clearAux[4] = { 0.f, 0.f, 0.f, 0.f };
	glClearBufferfv(GL_COLOR, 0, clearColor);
	glClearBufferfv(GL_COLOR, 1, clearAux);
}

void PostProcess::EndScene()
{
	if (!IsEnabled())
	{
		return;
	}

	// Every pass overwrites its whole target, so blending and clipping only
	// get in the way.
	glDisable(GL_BLEND);
	glDisable(GL_SCISSOR_TEST);
	glBindVertexArray(m_TriangleVAO);

	// Lift the glowing parts out at half size.
	glBindFramebuffer(GL_FRAMEBUFFER, m_BloomFramebuffer[0]);
	glViewport(0, 0, m_BloomWidth, m_BloomHeight);
	m_Extract.Use();
	BindTexture(0, m_SceneColor);
	BindTexture(1, m_SceneAux);
	glUniform1i(m_Extract.GetUniformLocation("u_Scene"), 0);
	glUniform1i(m_Extract.GetUniformLocation("u_Aux"), 1);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	// Blur them, across into the second target and back down into the first.
	m_Blur.Use();
	glUniform1i(m_Blur.GetUniformLocation("u_Source"), 0);
	GLint stepLocation = m_Blur.GetUniformLocation("u_Step");
	for (int pass = 0; pass < kBlurPasses; pass++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_BloomFramebuffer[1]);
		BindTexture(0, m_BloomTexture[0]);
		glUniform2f(stepLocation, kBlurSpread / m_BloomWidth, 0.f);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindFramebuffer(GL_FRAMEBUFFER, m_BloomFramebuffer[0]);
		BindTexture(0, m_BloomTexture[1]);
		glUniform2f(stepLocation, 0.f, kBlurSpread / m_BloomHeight);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	// Lay the world and its glow onto the window, graded.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, m_Width, m_Height);
	m_Composite.Use();
	BindTexture(0, m_SceneColor);
	BindTexture(1, m_BloomTexture[0]);
	BindTexture(2, m_SceneAux);
	glUniform1i(m_Composite.GetUniformLocation("u_Scene"), 0);
	glUniform1i(m_Composite.GetUniformLocation("u_Bloom"), 1);
	glUniform1i(m_Composite.GetUniformLocation("u_Aux"), 2);
	glUniform2f(m_Composite.GetUniformLocation("u_Texel"), 1.f / m_Width, 1.f / m_Height);
	glUniform1f(m_Composite.GetUniformLocation("u_Outline"), m_OutlineEnabled ? 1.f : 0.f);
	glUniform1f(m_Composite.GetUniformLocation("u_OutlineWidth"), kOutlineWidth);
	glUniform1f(m_Composite.GetUniformLocation("u_OutlineStrength"), kOutlineStrength);
	glUniform3f(m_Composite.GetUniformLocation("u_OutlineColor"),
		Palette::kOutline.r, Palette::kOutline.g, Palette::kOutline.b);
	glUniform2f(m_Composite.GetUniformLocation("u_RimOffset"), kRimDirX * kRimWidth, kRimDirY * kRimWidth);
	glUniform1f(m_Composite.GetUniformLocation("u_RimStrength"), kRimStrength);
	glUniform3f(m_Composite.GetUniformLocation("u_RimColor"), Palette::kRim.r, Palette::kRim.g, Palette::kRim.b);
	glUniform1f(m_Composite.GetUniformLocation("u_BloomIntensity"), kBloomIntensity);
	glUniform1f(m_Composite.GetUniformLocation("u_Saturation"), m_Grade.saturation);
	glUniform1f(m_Composite.GetUniformLocation("u_Contrast"), m_Grade.contrast);
	glUniform3f(m_Composite.GetUniformLocation("u_Tint"), m_Grade.tintR, m_Grade.tintG, m_Grade.tintB);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glBindVertexArray(0);
	BindTexture(2, 0);
	BindTexture(1, 0);
	BindTexture(0, 0);
	glEnable(GL_BLEND);
}
