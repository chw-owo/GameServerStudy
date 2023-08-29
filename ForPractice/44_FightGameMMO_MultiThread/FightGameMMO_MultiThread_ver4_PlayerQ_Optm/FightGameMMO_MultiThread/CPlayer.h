#pragma once
#include "CSystemLog.h"
#include "CSector.h"
#include "CJobQueue.h"
#include <synchapi.h>
#include <tchar.h>

class PLAYER_IDX
{
public:
	PLAYER_IDX() {};
	PLAYER_IDX(int num)
		: _num(num),
		_getPacketThreadNum(num / dfGETPACKET_PLAYER_NUM),
		_getPacketThreadIdx(num % dfGETPACKET_PLAYER_NUM),
		_logicThreadNum(num / dfLOGIC_PLAYER_NUM),
		_logicThreadIdx(num % dfLOGIC_PLAYER_NUM) {};

public:
	int _num;
	int _getPacketThreadNum;
	int _getPacketThreadIdx;
	int _logicThreadNum;
	int _logicThreadIdx;
};

enum PLAYER_STATE
{
	BEFORE_CONNECT,
	CONNECT,
	ALIVE,
	DEAD,
	DELETED,
	AFTER_DISCONNECT
};

class CPlayer
{
public:
	inline CPlayer(int num)
		: _idx(num), _state(BEFORE_CONNECT), _sessionID(-1), _playerID(-1), _pSector(nullptr),
		_move(false), _hp(dfMAX_HP), _direction(dfMOVE_DIR_LL), _moveDirection(dfMOVE_DIR_LL), _lastRecvTime(0)
	{
		_x = rand() % dfRANGE_MOVE_RIGHT;
		_y = rand() % dfRANGE_MOVE_BOTTOM;

		InitializeSRWLock(&_stateLock);
		InitializeSRWLock(&_recvTimeLock);
		InitializeSRWLock(&_xLock);
		InitializeSRWLock(&_yLock);
		InitializeSRWLock(&_hpLock);
		InitializeSRWLock(&_moveLock);
		InitializeSRWLock(&_dirLock);
		InitializeSRWLock(&_moveDirLock);
		InitializeSRWLock(&_sectorLock);
	}

	inline CPlayer()
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Player() is called\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Player() is called\n", _T(__FUNCTION__), __LINE__);
	}

	void Initialize(int sessionID, int playerID)
	{
		_state = ALIVE;
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
		
	PLAYER_IDX _idx;
	PLAYER_STATE _state;
	CSector* _pSector;
	DWORD _lastRecvTime;
	CJobQueue _jobQ;

public:
	SRWLOCK _stateLock;
	SRWLOCK _recvTimeLock;
	SRWLOCK _xLock;
	SRWLOCK _yLock;
	SRWLOCK _hpLock;
	SRWLOCK _moveLock;
	SRWLOCK _dirLock;
	SRWLOCK _moveDirLock;
	SRWLOCK _sectorLock;

};
