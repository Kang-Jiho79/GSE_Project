#include "stdafx.h"
#include "Profiler.h"
#include <cstdio>

namespace
{
	const float kReportInterval = 1.f; // seconds
}

void Profiler::AddFrame(float deltaTime, unsigned int drawCalls)
{
	if (m_Frames == 0 || drawCalls < m_DrawCallMin)
	{
		m_DrawCallMin = drawCalls;
	}
	if (drawCalls > m_DrawCallMax)
	{
		m_DrawCallMax = drawCalls;
	}

	m_DrawCallTotal += drawCalls;
	m_Frames++;
	m_Elapsed += deltaTime;

	if (m_Elapsed >= kReportInterval)
	{
		Report();
	}
}

void Profiler::Report()
{
	float fps = m_Frames / m_Elapsed;
	float frameMs = (m_Elapsed * 1000.f) / m_Frames;
	unsigned int drawAverage = m_DrawCallTotal / (unsigned int)m_Frames;

	printf("[perf] fps %.1f | frame %.2f ms | draws/frame %u (min %u, max %u)\n",
		fps, frameMs, drawAverage, m_DrawCallMin, m_DrawCallMax);

	m_Elapsed = 0.f;
	m_Frames = 0;
	m_DrawCallTotal = 0;
	m_DrawCallMin = 0;
	m_DrawCallMax = 0;
}
