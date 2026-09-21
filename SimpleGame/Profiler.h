#pragma once

// Rolls per-frame numbers up into one console line a second.
//
// Deliberately not one line per frame: console output costs more than the frame
// it would be reporting on, so printing every frame would mostly measure the
// printing. Averaging over a window also works around the frame timer only
// having millisecond resolution - a single fast frame reads as 0 ms on its own.
class Profiler
{
public:
	void AddFrame(float deltaTime, unsigned int drawCalls);

private:
	void Report();

	float m_Elapsed = 0.f;
	int m_Frames = 0;

	unsigned int m_DrawCallTotal = 0;
	unsigned int m_DrawCallMin = 0;
	unsigned int m_DrawCallMax = 0;
};
