#pragma once
#include "CSystemLog.h"
#include "CSector.h"
#include <tchar.h>

enum PLAYER_STATE
{
	ALIVE,
	DEAD,
	DELETED
};

class CPlayer
{
public:
	inline CPlayer(int sessionID, int playerID)
		: _state(ALIVE), _sessionID(sessionID), _playerID(playerID), _pSector(nullptr),
		_move(false), _hp(dfMAX_HP), _direction(dfMOVE_DIR_LL), _moveDirection(dfMOVE_DIR_LL)
	{
		_x = rand() % dfRANGE_MOVE_RIGHT;
		_y = rand() % dfRANGE_MOVE_BOTTOM;
	}

	inline CPlayer()
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Player() is called\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Player() is called\n", _T(__FUNCTION__), __LINE__);
	}

	public:
		PLAYER_STATE _state;
		__int64 _sessionID;
		int _playerID;
		bool _move;
		char _hp;
		short _x;
		short _y;
		unsigned char _direction;
		unsigned char _moveDirection;
		CSector* _pSector;
		DWORD _lastRecvTime;
	};
