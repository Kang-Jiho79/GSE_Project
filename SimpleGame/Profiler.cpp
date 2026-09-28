#include "stdafx.h"
#include "Profiler.h"
#include <cstdio>
#include <cstring>

namespace
{
	const float kReportInterval = 1.f; // seconds
}

Profiler& Profiler::Get()
{
	static Profiler instance;
	return instance;
}

void Profiler::AddFrame(float deltaTime, unsigned int drawCalls, unsigned int quads)
{
	if (m_Frames == 0 || quads < m_QuadMin)
	{
		m_QuadMin = quads;
	}
	if (quads > m_QuadMax)
	{
		m_QuadMax = quads;
	}

	m_DrawCallTotal += drawCalls;
	m_QuadTotal += quads;
	m_Frames++;
	m_Elapsed += deltaTime;

	if (m_Elapsed >= kReportInterval)
	{
		Report();
	}
}

void Profiler::AddPhaseMs(const char* name, float ms)
{
	FindPhase(name).totalMs += ms;
}

void Profiler::SetGauge(const char* name, int value)
{
	FindGauge(name).value = value;
}

Profiler::Phase& Profiler::FindPhase(const char* name)
{
	for (size_t i = 0; i < m_Phases.size(); i++)
	{
		if (strcmp(m_Phases[i].name, name) == 0)
		{
			return m_Phases[i];
		}
	}

	Phase phase;
	phase.name = name;
	phase.totalMs = 0.f;
	m_Phases.push_back(phase);
	return m_Phases.back();
}

Profiler::Gauge& Profiler::FindGauge(const char* name)
{
	for (size_t i = 0; i < m_Gauges.size(); i++)
	{
		if (strcmp(m_Gauges[i].name, name) == 0)
		{
			return m_Gauges[i];
		}
	}

	Gauge gauge;
	gauge.name = name;
	gauge.value = 0;
	m_Gauges.push_back(gauge);
	return m_Gauges.back();
}

void Profiler::Report()
{
	float fps = m_Frames / m_Elapsed;
	float frameMs = (m_Elapsed * 1000.f) / m_Frames;
	unsigned int drawAverage = m_DrawCallTotal / (unsigned int)m_Frames;
	unsigned int quadAverage = m_QuadTotal / (unsigned int)m_Frames;

	printf("[perf] fps %.1f | frame %.2f ms | draws %u | quads %u (%u-%u)\n",
		fps, frameMs, drawAverage, quadAverage, m_QuadMin, m_QuadMax);

	if (!m_Phases.empty())
	{
		printf("[perf]   ms:");
		for (size_t i = 0; i < m_Phases.size(); i++)
		{
			printf("%s %s %.3f", (i == 0) ? "" : " |", m_Phases[i].name, m_Phases[i].totalMs / m_Frames);
			m_Phases[i].totalMs = 0.f;
		}
		printf("\n");
	}

	if (!m_Gauges.empty())
	{
		printf("[perf]   count:");
		for (size_t i = 0; i < m_Gauges.size(); i++)
		{
			printf("%s %s %d", (i == 0) ? "" : " |", m_Gauges[i].name, m_Gauges[i].value);
		}
		printf("\n");
	}

	m_Elapsed = 0.f;
	m_Frames = 0;
	m_DrawCallTotal = 0;
	m_QuadTotal = 0;
	m_QuadMin = 0;
	m_QuadMax = 0;
}

ProfileScope::ProfileScope(const char* name)
	: m_Name(name)
	, m_Start(std::chrono::steady_clock::now())
{
}

ProfileScope::~ProfileScope()
{
	std::chrono::duration<float, std::milli> elapsed = std::chrono::steady_clock::now() - m_Start;
	Profiler::Get().AddPhaseMs(m_Name, elapsed.count());
}
