#pragma once
#include "CLockFreeQueue.h"
#include "CTlsPool.h"
#include "CommonProtocol.h"
#include "CNetJob.h"

struct ID
{
	wchar_t _id[dfID_LEN];
};

struct Nickname
{
	wchar_t _nickname[dfNICKNAME_LEN];
};

struct SessionKey
{
	char _key[dfSESSIONKEY_LEN];
};

extern CTlsPool<ID> g_IDPool;
extern CTlsPool<Nickname> g_NicknamePool;
extern CTlsPool<SessionKey> g_SessionKey;

class PLAYER_IDX
{
public:
	PLAYER_IDX() {};
	PLAYER_IDX(int idx, int playerPerThread)
		: _idx(idx),
		_threadIdx(idx / playerPerThread),
		_playerIdx(idx% playerPerThread) {};

public:
	int _idx;
	int _threadIdx;
	int _playerIdx;
};

class CPlayer
{
public:

	void Setting(int idx, int playerPerThread)
	{
		_idx = PLAYER_IDX(idx, playerPerThread);
		InitializeSRWLock(&_lock);
	}

	inline void Initialize(unsigned __int64 sessionID, __int64 playerID)
	{
		_id = g_IDPool.Alloc();
		_nickname = g_NicknamePool.Alloc();
		_key = g_SessionKey.Alloc();

		_sessionID = sessionID;
		_playerID = playerID;

		_accountNo = -1;
		_sectorX = -1;
		_sectorY = -1;
		_lastRecvTime = timeGetTime();
	}

	inline int Terminate()
	{
		g_IDPool.Free(_id);
		g_NicknamePool.Free(_nickname);
		g_SessionKey.Free(_key);

		_sessionID = MAXULONGLONG;
		_playerID = -1;

		_accountNo = -1;		
		_sectorX = -1;
		_sectorY = -1;
		_lastRecvTime = 0;

		return  _idx._idx;
	}

public:
	PLAYER_IDX _idx;

	unsigned __int64 _sessionID = MAXULONGLONG;
	__int64 _playerID = -1;
	__int64 _accountNo = -1;

	ID* _id;
	Nickname* _nickname;
	SessionKey* _key;

	WORD _sectorX = -1;
	WORD _sectorY = -1;
	DWORD _lastRecvTime = 0;

public:
	SRWLOCK _lock;
	CLockFreeQueue<CNetJob*> _jobQ;
};