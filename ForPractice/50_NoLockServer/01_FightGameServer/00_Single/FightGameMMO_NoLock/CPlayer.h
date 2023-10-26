#pragma once
#include "CSystemLog.h"
#include "CSector.h"
#include <synchapi.h>
#include <tchar.h>
#include <vector>
using namespace std;

class CPlayer
{
public:
	inline CPlayer(__int64 sessionID, __int64 playerID)
	{
		_sessionID = sessionID;
		_playerID = playerID;

		_move = false;
		_hp = dfMAX_HP;
		_direction = dfMOVE_DIR_LL;
		_moveDirection = dfMOVE_DIR_LL;
		_x = rand() % dfRANGE_MOVE_RIGHT; // Test: 100
		_y = rand() % dfRANGE_MOVE_BOTTOM; // Test: 100

		_pSector = nullptr;
		_lastRecvTime = timeGetTime();
	}

	inline CPlayer()
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Player() is called\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Player() is called\n", _T(__FUNCTION__), __LINE__);
	}

public:
	__int64 _sessionID;
	__int64 _playerID;

	short _x;
	short _y;
	char _hp;
	bool _move;
	unsigned char _direction;
	unsigned char _moveDirection;

	CSector* _pSector;
	DWORD _lastRecvTime;
};
