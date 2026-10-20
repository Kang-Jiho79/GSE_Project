#include "stdafx.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Monitor.h"
#include "MonitorPage.h"
#include "Tuning.h"
#include <iostream>

namespace
{
	// More than this unread and a request is not one this answers.
	const size_t kMaxRequest = 4096;
	// Read a little at a time.
	const int kReadChunk = 1024;
	// More than this waiting to be sent, and the watcher is too slow: dropped.
	const size_t kMaxBacklog = 1 << 20;

	SOCKET AsSocket(unsigned long long socket)
	{
		return (SOCKET)socket;
	}
}

Monitor::~Monitor()
{
	Stop();
	if (m_Started)
	{
		WSACleanup();
	}
}

bool Monitor::Start(int port)
{
	if (m_Running)
	{
		return true;
	}
	if (!m_Started)
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
		{
			std::cout << "Monitor: Winsock would not start\n";
			return false;
		}
		m_Started = true;
	}

	SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listener == INVALID_SOCKET)
	{
		std::cout << "Monitor: no socket\n";
		return false;
	}

	// This machine only.
	sockaddr_in address = {};
	address.sin_family = AF_INET;
	address.sin_port = htons((u_short)port);
	inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
	if (bind(listener, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR || listen(listener, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "Monitor: port " << port << " is taken\n";
		closesocket(listener);
		return false;
	}
	u_long nonBlocking = 1;
	ioctlsocket(listener, FIONBIO, &nonBlocking);

	m_Listen = (unsigned long long)listener;
	m_Running = true;
	m_Timer = 0.f;
	std::cout << "Monitor: http://127.0.0.1:" << port << "\n";
	return true;
}

void Monitor::Stop()
{
	while (!m_Clients.empty())
	{
		Drop(m_Clients.size() - 1);
	}
	if (m_Running)
	{
		closesocket(AsSocket(m_Listen));
		m_Running = false;
	}
}

bool Monitor::IsRunning() const
{
	return m_Running;
}

bool Monitor::Poll(float deltaTime)
{
	if (!m_Running)
	{
		return false;
	}

	Accept();
	for (size_t i = 0; i < m_Clients.size(); i++)
	{
		Read(m_Clients[i]);
		Flush(m_Clients[i]);
	}
	// Anyone gone, or done with.
	for (size_t i = m_Clients.size(); i > 0; i--)
	{
		const Client& client = m_Clients[i - 1];
		if (client.closing && client.outgoing.empty())
		{
			Drop(i - 1);
		}
	}

	bool watched = false;
	for (size_t i = 0; i < m_Clients.size(); i++)
	{
		watched = watched || m_Clients[i].watching;
	}
	m_Timer += deltaTime;
	if (!watched || m_Timer < Tuning::Monitor::kInterval)
	{
		return false;
	}
	m_Timer = 0.f;
	return true;
}

void Monitor::Send(const std::string& json)
{
	std::string event = "data: " + json + "\n\n";
	for (size_t i = 0; i < m_Clients.size(); i++)
	{
		Client& client = m_Clients[i];
		if (!client.watching || client.closing)
		{
			continue;
		}
		if (client.outgoing.size() > kMaxBacklog)
		{
			client.closing = true;
			client.outgoing.clear();
			continue;
		}
		client.outgoing += event;
		Flush(client);
	}
}

void Monitor::Accept()
{
	for (;;)
	{
		SOCKET socket = accept(AsSocket(m_Listen), nullptr, nullptr);
		if (socket == INVALID_SOCKET)
		{
			return;
		}
		u_long nonBlocking = 1;
		ioctlsocket(socket, FIONBIO, &nonBlocking);
		Client client;
		client.socket = (unsigned long long)socket;
		m_Clients.push_back(client);
	}
}

void Monitor::Read(Client& client)
{
	if (client.closing)
	{
		return;
	}
	char chunk[kReadChunk];
	for (;;)
	{
		int got = recv(AsSocket(client.socket), chunk, sizeof(chunk), 0);
		if (got == 0)
		{
			// Gone.
			client.closing = true;
			client.outgoing.clear();
			return;
		}
		if (got < 0)
		{
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				client.closing = true;
				client.outgoing.clear();
			}
			break;
		}
		// Once on the stream, anything more it sends is ignored.
		if (!client.watching)
		{
			client.request.append(chunk, got);
		}
	}

	if (client.watching)
	{
		return;
	}
	if (client.request.find("\r\n\r\n") != std::string::npos)
	{
		Answer(client);
	}
	else if (client.request.size() > kMaxRequest)
	{
		client.closing = true;
	}
}

void Monitor::Answer(Client& client)
{
	// Only the path matters: "GET /path HTTP/1.1".
	std::string path;
	size_t start = client.request.find(' ');
	if (start != std::string::npos)
	{
		size_t end = client.request.find(' ', start + 1);
		path = client.request.substr(start + 1, (end == std::string::npos) ? std::string::npos : end - start - 1);
	}
	client.request.clear();

	if (path == "/events")
	{
		client.watching = true;
		client.outgoing += "HTTP/1.1 200 OK\r\n"
			"Content-Type: text/event-stream; charset=utf-8\r\n"
			"Cache-Control: no-cache\r\n"
			"Connection: keep-alive\r\n\r\n"
			"retry: 2000\n\n";
		// The first snapshot without waiting out the interval.
		m_Timer = Tuning::Monitor::kInterval;
		return;
	}

	std::string body;
	std::string status;
	std::string type;
	if (path == "/" || path == "/index.html")
	{
		body = kMonitorPage;
		status = "200 OK";
		type = "text/html; charset=utf-8";
	}
	else
	{
		body = "Not found";
		status = "404 Not Found";
		type = "text/plain; charset=utf-8";
	}
	client.outgoing += "HTTP/1.1 " + status + "\r\nContent-Type: " + type + "\r\nContent-Length: "
		+ std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
	client.closing = true;
}

void Monitor::Flush(Client& client)
{
	while (!client.outgoing.empty())
	{
		int sent = send(AsSocket(client.socket), client.outgoing.data(), (int)client.outgoing.size(), 0);
		if (sent <= 0)
		{
			if (sent < 0 && WSAGetLastError() != WSAEWOULDBLOCK)
			{
				client.closing = true;
				client.outgoing.clear();
			}
			return;
		}
		client.outgoing.erase(0, (size_t)sent);
	}
}

void Monitor::Drop(size_t index)
{
	closesocket(AsSocket(m_Clients[index].socket));
	m_Clients.erase(m_Clients.begin() + index);
}
