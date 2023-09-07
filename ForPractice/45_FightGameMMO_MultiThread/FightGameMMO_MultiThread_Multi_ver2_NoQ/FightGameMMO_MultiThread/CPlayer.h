#pragma once
#include "CSystemLog.h"
#include "CSector.h"
#include "CJobQueue.h"
#include <synchapi.h>
#include <tchar.h>
#include <vector>
using namespace std;

enum class PLAYER_STATE
{
	DISCONNECT,
	CONNECT,
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
		: _idx(num), _state(PLAYER_STATE::DISCONNECT), 
		_x(-1), _y(-1), _sessionID(-1), _playerID(-1), _pSector(nullptr),
		_move(false), _hp(-1), _direction(-1), _moveDirection(-1), _lastRecvTime(-1)
	{
		/*
		_debugLineQ.reserve(100);
		_debugStateQ.reserve(100);
		_debugPlayerIDQ.reserve(100);
		_debugThreadIDQ.reserve(100);
		_debugMinQ.reserve(100);
		_debugSecQ.reserve(100);
		*/
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
		_state = PLAYER_STATE::CONNECT;
		_sessionID = sessionID;
		_playerID = playerID;
		
		_move = false; 
		_hp = dfMAX_HP; 
		_direction = dfMOVE_DIR_LL; 
		_moveDirection = dfMOVE_DIR_LL;
		_x = rand() % dfRANGE_MOVE_RIGHT;
		_y = rand() % dfRANGE_MOVE_BOTTOM;

		_pSector = nullptr;
		_lastRecvTime = timeGetTime();
	}

	void CleanUp()
	{
		_state = PLAYER_STATE::DISCONNECT;
		_sessionID = -1;
		_playerID = -1;

		_move = false;
		_hp = -1;
		_direction = -1;
		_moveDirection = -1;
		_x = -1;
		_y = -1;

		_pSector = nullptr;
		_lastRecvTime = -1;
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

public:
	/*
	vector<int> _debugLineQ;
	vector<int> _debugStateQ;
	vector<__int64> _debugPlayerIDQ;
	vector<int> _debugThreadIDQ;
	vector<int> _debugMinQ;
	vector<int> _debugSecQ;
	SRWLOCK _debugQLock;
	*/

	void PushStateForDebug(int line)
	{
		/*
		AcquireSRWLockExclusive(&_debugQLock);
		SYSTEMTIME stTime;
		GetLocalTime(&stTime);
		_debugLineQ.push_back(line);
		_debugStateQ.push_back((int)_state);
		_debugPlayerIDQ.push_back(_playerID);
		_debugThreadIDQ.push_back(GetCurrentThreadId());
		_debugMinQ.push_back(stTime.wMinute);
		_debugSecQ.push_back(stTime.wSecond);
		ReleaseSRWLockExclusive(&_debugQLock);
		*/
	}

	void PrintStateForDebug()
	{
		/*
		AcquireSRWLockExclusive(&_debugQLock);
		
		::printf("\n<%lld>\n", _debugLineQ.size());
		
		
		for (int i = 0; i < _debugThreadIDQ.size(); i++)
		{
			::printf("%d: %lld, %d (%d - %02d:%02d)\n", 
				_debugLineQ[i], _debugPlayerIDQ[i], 
				_debugStateQ[i], _debugThreadIDQ[i],
				_debugMinQ[i], _debugSecQ[i]);
		}

		ReleaseSRWLockExclusive(&_debugQLock);
		*/
	}
};
