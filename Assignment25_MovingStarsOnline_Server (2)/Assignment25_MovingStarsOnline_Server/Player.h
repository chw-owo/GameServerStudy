#pragma once
#include "List.h"
#include "RingBuffer.h"
#include <ws2tcpip.h>
#include <iostream>

#define XMAX 81
#define YMAX 24

enum PlayerState
{
	DEAD = 0,
	BEFORE_SETTING = 1,
	ALIVE = 2
};

struct Player
{
	PlayerState state;
	SOCKET sock;
	RingBuffer recvBuf;
	RingBuffer sendBuf;

	bool updated = false;
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