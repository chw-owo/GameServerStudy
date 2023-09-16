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
		InitializeSRWLock(&_lock);
		InitializeCriticalSection(&_csDebug);
		// _debugDataArray.reserve(1000);
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

private:
	class CPlayerDebugData
	{
		friend CPlayer;

	private:
		CPlayerDebugData() :
			_line(-1), _state(-1), _playerID(-1),
			_threadID(-1), _min(-1), _sec(-1) {}

		CPlayerDebugData(int line, int state, __int64 playerID ,
			int threadID, int min, int sec)
			: _line(line), _state(state), _playerID(playerID),
			_threadID(-1), _min(-1), _sec(-1) {}

	private:
		int _line;
		int _state;
		__int64 _playerID;
		int _threadID;
		int _min;
		int _sec;
	};

private:
	CRITICAL_SECTION _csDebug;
	vector<CPlayerDebugData*> _debugDataArray;

public:
	void PushStateForDebug(int line)
	{
		/*
		EnterCriticalSection(&_csDebug);
		SYSTEMTIME stTime;
		GetLocalTime(&stTime);

		CPlayerDebugData* data = new CPlayerDebugData(
			line, (int)_state, _playerID, GetCurrentThreadId(), 
			stTime.wMinute, stTime.wSecond);
		_debugDataArray.push_back(data);

		LeaveCriticalSection(&_csDebug);
		*/
	}

	void PrintStateForDebug()
	{
		/*
		EnterCriticalSection(&_csDebug);	
		::wprintf(L"\n<%lld>\n", _debugDataArray.size());		
		for (int i = 0; i < _debugDataArray.size(); i++)
		{
			::wprintf(L"%d: %lld, %d (%d - %02d:%02d)\n", 
				_debugDataArray[i]->_line, _debugDataArray[i]->_playerID,
				_debugDataArray[i]->_state, _debugDataArray[i]->_threadID,
				_debugDataArray[i]->_min, _debugDataArray[i]->_sec);
		}
		LeaveCriticalSection(&_csDebug);
		*/
	}
};
