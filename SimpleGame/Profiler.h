#pragma once

#include <chrono>
#include <vector>

// Rolls per-frame numbers up into a few console lines a second.
//
// Deliberately not one line per frame: console output costs more than the frame
// it would be reporting on, so printing every frame would mostly measure the
// printing. Averaging over a window also works around GLUT's frame timer only
// having millisecond resolution - a single fast frame reads as 0 ms on its own.
//
// A global by design: instrumentation should be one line to add anywhere,
// not a reference threaded through every call.
class Profiler
{
public:
	static Profiler& Get();

	// Once per rendered frame.
	void AddFrame(float deltaTime, unsigned int drawCalls, unsigned int quads);

	// Time spent in a named phase this frame. Summed within the frame, averaged
	// across the window, reported in the order the names were first seen.
	void AddPhaseMs(const char* name, float ms);

	// An instantaneous count (actors alive, etc). The last value seen in the
	// window is what gets reported.
	void SetGauge(const char* name, int value);

private:
	struct Phase
	{
		const char* name;
		float totalMs;
	};

	struct Gauge
	{
		const char* name;
		int value;
	};

	void Report();
	Phase& FindPhase(const char* name);
	Gauge& FindGauge(const char* name);

	float m_Elapsed = 0.f;
	int m_Frames = 0;

	unsigned int m_DrawCallTotal = 0;
	unsigned int m_QuadTotal = 0;
	unsigned int m_QuadMin = 0;
	unsigned int m_QuadMax = 0;

	std::vector<Phase> m_Phases;
	std::vector<Gauge> m_Gauges;
};

// Times the scope it lives in and hands the result to the profiler, so any
// stretch of code can be measured by wrapping it in braces and one line.
class ProfileScope
{
public:
	explicit ProfileScope(const char* name);
	~ProfileScope();

private:
	const char* m_Name;
	std::chrono::steady_clock::time_point m_Start;
};
