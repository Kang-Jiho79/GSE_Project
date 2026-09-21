#pragma once

#include "Dependencies\glew.h"
#include "Shader.h"
#include "MeshCache.h"

// All positions and sizes are in pixels, with the origin at the window center,
// +x to the right and +y up.
class Renderer
{
public:
	Renderer(int windowSizeX, int windowSizeY);

	bool IsInitialized();

	void DrawSolidRect(float x, float y, float width, float height, float r, float g, float b, float a);
	// Rhombus measured corner to corner - the ground tile shape a quarter view needs,
	// and the shape of the vessel gauge.
	void DrawSolidDiamond(float x, float y, float width, float height, float r, float g, float b, float a);
	// Quad spanning two arbitrary edge vectors from its center. Slanted edges are what
	// make the side faces of an isometric box, so this is what gives the world volume.
	void DrawSolidQuad(float x, float y, float edgeAX, float edgeAY, float edgeBX, float edgeBY,
		float r, float g, float b, float a);

	// Confines drawing to a rectangle so a shape can be drawn partially filled.
	void SetClip(float left, float bottom, float width, float height);
	void ClearClip();

	// Every shape drawn anywhere funnels through one glDraw call here, so this
	// count is the exact number of draw calls issued since the last reset.
	unsigned int DrawCallCount() const;
	void ResetDrawCallCount();

private:
	void Initialize(int windowSizeX, int windowSizeY);
	void CreateVertexBufferObjects();
	void CacheShaderLocations();
	// transform is a column-major 2x2 in pixel space: { m00, m10, m01, m11 }
	void SubmitQuad(float x, float y, const float transform[4], float r, float g, float b, float a);

	bool m_Initialized = false;

	int m_WindowSizeX = 0;
	int m_WindowSizeY = 0;

	Shader m_SolidRectShader;
	MeshCache m_MeshCache;
	GLuint m_VBORect = 0;

	GLint m_LocTrans = -1;
	GLint m_LocTransform = -1;
	GLint m_LocColor = -1;
	GLint m_AttribPosition = -1;

	unsigned int m_DrawCallCount = 0;
};
