#pragma once
#include "Protocol.h"
#include "CLockFreeQueue.h"
#include "CJob.h"

class PLAYER_IDX
{
public:
	PLAYER_IDX() {};
	PLAYER_IDX(int idx, int playerPerThread)
		: _idx(idx),
		_threadIdx(idx / playerPerThread),
		_playerIdx(idx % playerPerThread) {};

public:
	int _idx;
	int _threadIdx;
	int _playerIdx;
};

class CPlayer
{
public:
	CPlayer() {}

	void Setting(int idx, int playerPerThread)  
	{
		_idx = PLAYER_IDX(idx, playerPerThread);
		InitializeSRWLock(&_lock);
	}

	void Initialize(__int64 sessionID, __int64 playerID) 
	{
		_sessionID = sessionID;
		_playerID = playerID;
		_sectorX = -1; 
		_sectorY = -1;

		_accountNo = -1;
		_lastRecvTime = timeGetTime();
		_ID = new wchar_t[dfID_LEN];
		_nickname = new wchar_t[dfNICKNAME_LEN];
		_sessionKey = new char[dfSESSIONKEY_LEN];
	}

	int CleanUp()
	{
		_sessionID = -1;
		_playerID = -1;
		_sectorX = -1;
		_sectorY = -1;

		_accountNo = -1;
		_lastRecvTime = -1;
		delete[] _ID;
		delete[] _nickname;
		delete[] _sessionKey;

		return  _idx._idx;
	}

public:
	PLAYER_IDX _idx;

	__int64 _sessionID;
	__int64 _playerID;
	WORD _sectorX;
	WORD _sectorY;

	__int64 _accountNo;
	wchar_t* _ID;
	wchar_t* _nickname;
	char* _sessionKey;
	DWORD _lastRecvTime;
	
public:
	SRWLOCK _lock;
	CLockFreeQueue<CJob*> _jobQ;
	
};