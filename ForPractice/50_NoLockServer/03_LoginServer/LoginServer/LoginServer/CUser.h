#pragma once
#include <synchapi.h>
#pragma comment(lib, "Synchronization.lib")

class CUser
{
public:
	CUser() {}

	CUser(__int64 sessionID)
	{
		InitializeSRWLock(&_lock);
		_sessionID = sessionID;
		_accountNo = -1;
		_sessionKey = new char[dfSESSIONKEY_LEN];
		_lastRecvTime = 0;
	}

	~CUser()
	{
		_sessionID = -1;
		_accountNo = -1;
		delete[] _sessionKey;
		_lastRecvTime = 0;
	}

public:
	__int64 _sessionID = -1;
	__int64 _accountNo = -1;
	char* _sessionKey;
	DWORD _lastRecvTime = 0;
	SRWLOCK _lock;
};