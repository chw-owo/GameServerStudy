#include "MemoryPoolT.h"
#include "ProfilerForSingleThread.h"
#include <stdio.h>
#include <ctime>

class Player1
{
public:
	Player1()
	{
		srand(time(NULL));
		ID = rand();
		ID++;
		constructCnt++;
	}

	~Player1()
	{
		srand(time(NULL));
		ID = rand();
		ID++;
		destructCnt++;
	}

private:
	int ID;

public:
	static int constructCnt;
	static int destructCnt;
};
class Player2
{
public:
	Player2()
	{
		srand(time(NULL));
		ID = rand();
		ID++;
		constructCnt++;
	}

	~Player2()
	{
		srand(time(NULL));
		ID = rand();
		ID++;
		destructCnt++;
	}

private:
	int ID;

public:
	static int constructCnt;
	static int destructCnt;
};
class Player3
{
public:
	Player3()
	{
		srand(time(NULL));
		ID = rand();
		ID++;
		constructCnt++;
	}

	~Player3()
	{
		srand(time(NULL));
		ID = rand();
		ID++;
		destructCnt++;
	}

private:
	int ID;

public:
	static int constructCnt;
	static int destructCnt;
};

int Player1::constructCnt = 0;
int Player1::destructCnt = 0;
int Player2::constructCnt = 0;
int Player2::destructCnt = 0;
int Player3::constructCnt = 0;
int Player3::destructCnt = 0;

#define NUM 500
void ProfileMemoryPoolAllocFalse()
{
	int i = 0;
	Player1* pPlayer1[NUM];
	CHUU::CMemoryPoolT<Player1> MemPoolT1(0, false);

	PRO_BEGIN(L"MP False - Total");

	PRO_BEGIN(L"MP False - 1st Alloc");
	for (; i < NUM; i++)
	{
		pPlayer1[i] = MemPoolT1.Alloc();
	}
	PRO_END(L"MP False - 1st Alloc");

	i = 0;
	PRO_BEGIN(L"MP False - 1st Free");
	for (; i < NUM; i++)
	{
		MemPoolT1.Free(pPlayer1[i]);
	}
	PRO_END(L"MP False - 1st Free");

	i = 0;
	PRO_BEGIN(L"MP False - 2nd Alloc");
	for (; i < NUM; i++)
	{
		pPlayer1[i] = MemPoolT1.Alloc();
	}
	PRO_END(L"MP False - 2nd Alloc");

	i = 0;
	PRO_BEGIN(L"MP False - 2nd Free");
	for (; i < NUM; i++)
	{
		MemPoolT1.Free(pPlayer1[i]);
	}
	PRO_END(L"MP False - 2nd Free");

	i = 0;
	PRO_BEGIN(L"MP False - 3rd Alloc");
	for (; i < NUM; i++)
	{
		pPlayer1[i] = MemPoolT1.Alloc();
	}
	PRO_END(L"MP False - 3rd Alloc");

	i = 0;
	PRO_BEGIN(L"MP False - 3rd Free");
	for (; i < NUM; i++)
	{
		MemPoolT1.Free(pPlayer1[i]);
	}
	PRO_END(L"MP False - 3rd Free");

	PRO_END(L"MP False - Total");
}

void ProfileMemoryPoolAllocTrue()
{
	int i = 0;
	Player2* pPlayer2[NUM];
	CHUU::CMemoryPoolT<Player2> MemPoolT2(0, true);

	PRO_BEGIN(L"MP True - Total");

	PRO_BEGIN(L"MP True - 1st Alloc");
	for (; i < NUM; i++)
	{
		pPlayer2[i] = MemPoolT2.Alloc();
	}
	PRO_END(L"MP True - 1st Alloc");

	i = 0;
	PRO_BEGIN(L"MP True - 1st Free");
	for (; i < NUM; i++)
	{
		MemPoolT2.Free(pPlayer2[i]);
	}
	PRO_END(L"MP True - 1st Free");

	i = 0;
	PRO_BEGIN(L"MP True - 2nd Alloc");
	for (; i < NUM; i++)
	{
		pPlayer2[i] = MemPoolT2.Alloc();
	}
	PRO_END(L"MP True - 2nd Alloc");

	i = 0;
	PRO_BEGIN(L"MP True - 2nd Free");
	for (; i < NUM; i++)
	{
		MemPoolT2.Free(pPlayer2[i]);
	}
	PRO_END(L"MP True - 2nd Free");

	i = 0;
	PRO_BEGIN(L"MP True - 3rd Alloc");
	for (; i < NUM; i++)
	{
		pPlayer2[i] = MemPoolT2.Alloc();
	}
	PRO_END(L"MP True - 3rd Alloc");

	i = 0;
	PRO_BEGIN(L"MP True - 3rd Free");
	for (; i < NUM; i++)
	{
		MemPoolT2.Free(pPlayer2[i]);
	}
	PRO_END(L"MP True - 3rd Free");

	PRO_END(L"MP True - Total");
}
void ProfileDefaultNew()
{
	int i = 0;
	Player3* pPlayer3[NUM];

	PRO_BEGIN(L"NEW - Total");

	i = 0;
	PRO_BEGIN(L"NEW - 1st Alloc");
	for (; i < NUM; i++)
	{
		pPlayer3[i] = new Player3;
	}
	PRO_END(L"NEW - 1st Alloc");

	i = 0;
	PRO_BEGIN(L"NEW - 1st Free");
	for (; i < NUM; i++)
	{
		delete(pPlayer3[i]);
	}
	PRO_END(L"NEW - 1st Free");

	i = 0;
	PRO_BEGIN(L"NEW - 2nd Alloc");
	for (; i < NUM; i++)
	{
		pPlayer3[i] = new Player3;
	}
	PRO_END(L"NEW - 2nd Alloc");

	i = 0;
	PRO_BEGIN(L"NEW - 2nd Free");
	for (; i < NUM; i++)
	{
		delete(pPlayer3[i]);
	}
	PRO_END(L"NEW - 2nd Free");

	i = 0;
	PRO_BEGIN(L"NEW - 3rd Alloc");
	for (; i < NUM; i++)
	{
		pPlayer3[i] = new Player3;
	}
	PRO_END(L"NEW - 3rd Alloc");

	i = 0;
	PRO_BEGIN(L"NEW - 3rd Free");
	for (; i < NUM; i++)
	{
		delete(pPlayer3[i]);
	}
	PRO_END(L"NEW - 3rd Free");

	PRO_END(L"NEW - Total");
}

int main()
{
	for (int i = 0; i < 20; i++)
	{
		ProfileMemoryPoolAllocFalse();
		ProfileMemoryPoolAllocTrue();
		ProfileDefaultNew();
	}

	printf("1: %d - %d\n2: %d - %d\n3: %d - %d\n",
	Player1::constructCnt, Player1::destructCnt,
	Player2::constructCnt, Player2::destructCnt,
	Player3::constructCnt, Player3::destructCnt);

	PRO_OUT(L"output.txt");
	return 0;
}