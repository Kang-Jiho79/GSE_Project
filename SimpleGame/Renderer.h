#pragma once

#include <vector>

#include "Dependencies\glew.h"
#include "Shader.h"
#include "MeshCache.h"

// All positions and sizes are in pixels, with the origin at the window center,
// +x to the right and +y up.
//
// Nothing is drawn on the spot. Every shape is queued as one instance of the
// shared unit quad, and the queue is drawn in a single instanced call at the
// next state change (a clip) or at EndFrame. Queue order is draw order, so a
// depth-sorted list stays depth-sorted - it just costs one draw call instead
// of one per shape.
class Renderer
{
public:
	Renderer(int windowSizeX, int windowSizeY);
	~Renderer();

	bool IsInitialized();

	void DrawSolidRect(float x, float y, float width, float height, float r, float g, float b, float a);
	// Rhombus measured corner to corner - the ground tile shape a quarter view needs,
	// and the shape of the vessel gauge.
	void DrawSolidDiamond(float x, float y, float width, float height, float r, float g, float b, float a);
	// Quad spanning two arbitrary edge vectors from its center. Slanted edges are what
	// make the side faces of an isometric box, so this is what gives the world volume.
	void DrawSolidQuad(float x, float y, float edgeAX, float edgeAY, float edgeBX, float edgeBY,
		float r, float g, float b, float a);
	// Four corners given outright, wound bottom-left, top-left, top-right,
	// bottom-right. Lets a face taper, which is what keeps the world from being
	// made entirely of right angles.
	void DrawSolidCorners(const float corners[8], float r, float g, float b, float a);

	// How much everything queued from now on glows under bloom, 0..1. Sticks
	// until changed; the HUD leaves it at 0.
	void SetGlow(float glow);
	// Whether everything queued from now on is part of a figure - someone
	// standing in the world, to be outlined. Sticks until changed.
	void SetFigure(bool figure);

	// Confines drawing to a rectangle so a shape can be drawn partially filled.
	// Both flush whatever is queued first, since a clip is a state change.
	void SetClip(float left, float bottom, float width, float height);
	void ClearClip();

	// Draws whatever is still queued. Must be called before the buffers swap.
	void EndFrame();

	// Draw calls is the number of instanced draws actually issued; quads is how
	// many shapes went into them. After batching the first should stay small and
	// flat while the second is the real measure of the frame's work.
	unsigned int DrawCallCount() const;
	unsigned int QuadCount() const;
	void ResetCounts();

private:
	// Exactly what the vertex shader reads per instance, laid out in order.
	struct Instance
	{
		float cornerLo[4]; // bottom-left, bottom-right
		float cornerHi[4]; // top-left, top-right
		float color[4];
		float material[2]; // glow, figure
	};

	void Initialize(int windowSizeX, int windowSizeY);
	void CreateVertexBufferObjects();
	void CacheShaderLocations();
	void CreateVertexArray();
	// corners are in pixels, wound bottom-left, top-left, top-right, bottom-right
	void QueueCorners(const float corners[8], float r, float g, float b, float a);
	void Flush();

	bool m_Initialized = false;

	int m_WindowSizeX = 0;
	int m_WindowSizeY = 0;

	Shader m_Shader;
	MeshCache m_MeshCache;
	GLuint m_VBORect = 0;
	GLuint m_VBOInstances = 0;
	GLuint m_VAO = 0;

	GLint m_AttribPosition = -1;
	GLint m_AttribCornerLo = -1;
	GLint m_AttribCornerHi = -1;
	GLint m_AttribColor = -1;
	GLint m_AttribMaterial = -1;

	float m_Glow = 0.f;
	float m_Figure = 0.f;

	std::vector<Instance> m_Pending;

	unsigned int m_DrawCallCount = 0;
	unsigned int m_QuadCount = 0;
};
