#pragma once
#include "CSystemLog.h"
#include "CSector.h"
#include "CJobQueue.h"
#include <synchapi.h>
#include <tchar.h>

class PLAYER_NUM
{
public:
	PLAYER_NUM() {};
	PLAYER_NUM(int num)
		: _num(num), 
		_threadNum(num / dfPLAYER_PER_THREAD), 
		_threadIdx(num % dfPLAYER_PER_THREAD) {};

public:
	int _num;
	int _threadNum;
	int _threadIdx;
};

enum PLAYER_STATE
{
	BEFORE_CONNECT,
	ALIVE,
	DEAD,
	DELETED,
	AFTER_DISCONNECT
};

class CPlayer
{
public:
	inline CPlayer(int num)
		: _playerNum(num), _playerState(BEFORE_CONNECT), _sessionID(-1), _playerID(-1), _pSector(nullptr),
		_move(false), _hp(dfMAX_HP), _direction(dfMOVE_DIR_LL), _moveDirection(dfMOVE_DIR_LL), _lastRecvTime(0)
	{
		_x = rand() % dfRANGE_MOVE_RIGHT;
		_y = rand() % dfRANGE_MOVE_BOTTOM;
		InitializeSRWLock(&_lock);
	}

	inline CPlayer()
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Player() is called\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Player() is called\n", _T(__FUNCTION__), __LINE__);
	}

	void Initialize(int sessionID, int playerID)
	{
		_playerState = ALIVE;
		_playerID = playerID;
		_sessionID = sessionID;
		
		_move = false; 
		_hp = dfMAX_HP; 
		_direction = dfMOVE_DIR_LL; 
		_moveDirection = dfMOVE_DIR_LL;
		_x = rand() % dfRANGE_MOVE_RIGHT;
		_y = rand() % dfRANGE_MOVE_BOTTOM;

		_pSector = nullptr;
		_lastRecvTime = GetTickCount64();
		while(!_jobQ._queue.empty())
			_jobQ._queue.pop();
	}

public:
	int _playerID;
	__int64 _sessionID;
	short _x;
	short _y;
	char _hp;
	bool _move;
	unsigned char _direction;
	unsigned char _moveDirection;
		
	PLAYER_NUM _playerNum;
	PLAYER_STATE _playerState;
	CSector* _pSector;
	DWORD _lastRecvTime;
	SRWLOCK _lock;
	CJobQueue _jobQ;
	};
