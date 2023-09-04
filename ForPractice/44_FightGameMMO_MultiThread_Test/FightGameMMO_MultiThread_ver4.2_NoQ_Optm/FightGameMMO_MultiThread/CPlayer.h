#pragma once
#include "CSystemLog.h"
#include "CSector.h"
#include "CJobQueue.h"
#include <synchapi.h>
#include <tchar.h>

enum class PLAYER_STATE
{
	DISCONNECT,
	CONNECT,
	ALIVE,
	DEAD
};

class PLAYER_IDX
{
public:
	PLAYER_IDX() {};
	PLAYER_IDX(int num)
		: _num(num),
		_logicThreadNum(num / dfLOGIC_PLAYER_NUM),
		_logicThreadIdx(num % dfLOGIC_PLAYER_NUM) {};

public:
	int _num;
	int _logicThreadNum;
	int _logicThreadIdx;
};

class CPlayer
{
public:
	inline CPlayer(int num)
		: _idx(num), _state(PLAYER_STATE::DISCONNECT), _x(0), _y(0), _sessionID(-1), _playerID(-1), _pSector(nullptr),
		_move(false), _hp(dfMAX_HP), _direction(dfMOVE_DIR_LL), _moveDirection(dfMOVE_DIR_LL), _lastRecvTime(0)
	{
		InitializeSRWLock(&_lock);
	}

	inline CPlayer()
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Player() is called\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Player() is called\n", _T(__FUNCTION__), __LINE__);
	}

	void Initialize(__int64 sessionID, __int64 playerID)
	{
		_sessionID = sessionID;
		_playerID = playerID;
		
		_move = false; 
		_hp = dfMAX_HP; 
		_direction = dfMOVE_DIR_LL; 
		_moveDirection = dfMOVE_DIR_LL;
		_x = rand() % dfRANGE_MOVE_RIGHT;
		_y = rand() % dfRANGE_MOVE_BOTTOM;

		_pSector = nullptr;
		_lastRecvTime = GetTickCount64();
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
		
	PLAYER_IDX _idx;
	PLAYER_STATE _state;
	SRWLOCK _lock;
	CSector* _pSector;
	DWORD _lastRecvTime;
};