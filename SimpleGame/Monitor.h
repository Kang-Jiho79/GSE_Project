#pragma once

#include <string>
#include <vector>

// The game's state, watched from a browser on this same machine. A tiny web
// server on 127.0.0.1 only: "/" answers with the monitor page itself, and
// "/events" stays open as a stream of server-sent events, each one a JSON
// snapshot of the world. The game only ever talks; the page only listens.
//
// No threads: the sockets never block, and everything happens in Poll, once
// a frame, where the world can be read safely. Off unless switched on.
class Monitor
{
public:
	~Monitor();

	// Opens the port. Returns whether it could.
	bool Start(int port);
	void Stop();
	bool IsRunning() const;

	// Lets anyone new in, answers what they asked, sends what is waiting,
	// and drops anyone gone. Returns true when it is time for a snapshot -
	// someone is watching, and the interval has passed.
	bool Poll(float deltaTime);
	// One snapshot to everyone watching.
	void Send(const std::string& json);

private:
	struct Client
	{
		unsigned long long socket;
		std::string request; // read so far, until the blank line ending it
		std::string outgoing; // written but not yet sent
		bool watching = false; // on the event stream
		bool closing = false;  // to be dropped once everything is sent
	};

	void Accept();
	void Read(Client& client);
	void Answer(Client& client);
	void Flush(Client& client);
	void Drop(size_t index);

	unsigned long long m_Listen = 0;
	bool m_Running = false;
	bool m_Started = false; // Winsock itself, which stays up once started
	std::vector<Client> m_Clients;
	float m_Timer = 0.f;
};
