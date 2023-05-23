#pragma once
#include "List.h"
#include "RingBuffer.h"
#include <ws2tcpip.h>
#include <iostream>

#define XMAX 81
#define YMAX 24


struct Player
{
	bool alive = false;
	SOCKET sock;
	RingBuffer recvBuf;
	RingBuffer sendBuf;

	__int32 ID;
	__int32 X = XMAX / 2;
	__int32 Y = YMAX / 2;
};

class IDGenerator
{
public:
	IDGenerator()
	{
		srand((unsigned int)time(NULL));
		ID = rand();
	}
	__int32 AllocID()
	{
		ID++;
		return ID;
	}

private:
	__int32 ID;

};

extern CList<Player*> gPlayerList;
extern IDGenerator gIDGenerator;
extern bool deleted;