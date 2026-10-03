#pragma once

#include "Dependencies\glew.h"
#include "Shader.h"

// Draws the world off screen, then lays it onto the window with what makes
// it glow: the parts drawn as light are lifted out, blurred wide at half size
// and added back over it, and the whole frame is colour-graded on the way.
//
// The HUD is drawn after, straight to the window, so it never blooms.
class PostProcess
{
public:
	// How the finished frame is tinted. Neutral as constructed.
	struct Grade
	{
		float saturation = 1.f;
		float contrast = 1.f;
		float tintR = 1.f;
		float tintG = 1.f;
		float tintB = 1.f;
	};

	~PostProcess();

	// Needs a live GL context. On failure it says why and stays switched off,
	// so the game still draws, just without the effect.
	bool Initialize(int width, int height);

	// Off draws the world straight to the window and skips every pass.
	void SetEnabled(bool enabled);
	bool IsEnabled() const;

	void SetGrade(const Grade& grade);

	// The ink outline and rim light round figures. Part of the composite, so
	// it only shows while post-processing is on.
	void SetOutlineEnabled(bool enabled);
	bool IsOutlineEnabled() const;

	// The world is drawn between these. BeginScene clears to the given colour;
	// EndScene composites onto the window. Whatever the renderer has queued
	// must be flushed before EndScene.
	void BeginScene(float clearR, float clearG, float clearB);
	void EndScene();

private:
	bool CreateTargets();
	GLuint CreateTexture(int width, int height, GLint internalFormat, GLenum format, GLenum type);
	bool CheckFramebuffer(const char* name);
	void CreateFullscreenTriangle();
	void BindTexture(int unit, GLuint texture);

	bool m_Ready = false;
	bool m_Enabled = true;
	bool m_OutlineEnabled = true;

	int m_Width = 0;
	int m_Height = 0;
	int m_BloomWidth = 0;
	int m_BloomHeight = 0;

	// The world, and alongside it what each pixel is: glow in red, figure in
	// green.
	GLuint m_SceneFramebuffer = 0;
	GLuint m_SceneColor = 0;
	GLuint m_SceneAux = 0;

	// Two half-size targets the blur ping-pongs between.
	GLuint m_BloomFramebuffer[2] = { 0, 0 };
	GLuint m_BloomTexture[2] = { 0, 0 };

	GLuint m_TriangleVAO = 0;
	GLuint m_TriangleVBO = 0;

	Shader m_Extract;
	Shader m_Blur;
	Shader m_Composite;

	Grade m_Grade;
};
