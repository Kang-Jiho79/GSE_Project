#pragma once

#include <string>

class Renderer;
class TextRenderer;
struct PlayerStatus;
struct Progress;

// A village's notice board, read up close: its requests, each with what it
// asks, what it pays and where it stands. One is picked with W/S; F takes it
// up, or once it is done, reports it and is paid. The world holds still
// while it is open.
class QuestBoard
{
public:
	void Open(int region, const Progress& progress);
	void Close();
	bool IsOpen() const;

	void Move(int step);
	// F on the request picked: taken up if it was not, paid if it is done.
	// notice: the line to show; script: a conversation to play after it, or
	// -1; refresh: a place whose errands must be set out afresh, or -1.
	// Returns whether anything happened.
	bool Confirm(PlayerStatus& status, Progress& progress, std::string& notice, int& script, int& refresh);

	void Render(Renderer* renderer, TextRenderer* text, const Progress& progress, int windowSizeX, int windowSizeY) const;

private:
	static const int kMaxRequests = 4;

	bool m_Open = false;
	int m_Region = 0;
	int m_Ids[kMaxRequests] = {};
	int m_Count = 0;
	int m_Selected = 0;
};
