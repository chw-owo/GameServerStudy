#pragma once
#include "List.h"

#define XMAX 81
#define YMAX 24

struct Player
{
	__int32 ID;
	__int32 X;
	__int32 Y;
};

extern __int32 ID;
extern Player* gMyPlayer;
extern CList<Player*> gPlayerList;
