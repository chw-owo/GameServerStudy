#pragma once
#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include "RingBuffer.h"

class Session
{
public:
	Session(int ID) { _ID = ID; }

public:
	void SetSessionAlive() { _alive = true; }
	void SetSessionDead() { _alive = false; }
	bool GetSessionAlive() { return _alive; }

private:
	bool _alive = false;
	int _ID;

public:
	SOCKET _socket;
	SOCKADDR_IN _addr;
	RingBuffer _recvBuf;
	RingBuffer _sendBuf;
};
