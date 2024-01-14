#pragma once
#include <synchapi.h>
#pragma comment(lib, "Synchronization.lib")

class CUser
{
public:
	CUser() {}

	CUser(__int64 sessionID, WCHAR* IP)
	{
		InitializeSRWLock(&_lock);
		_sessionID = sessionID;
		wcscpy_s(_IP, dfIP_LEN, IP);
		_accountNo = -1;
		_lastRecvTime = 0;
		_ID = new WCHAR[dfID_LEN]; 
		_nickname = new WCHAR[dfNICKNAME_LEN]; 
		_sessionKey = new CHAR[dfSESSIONKEY_LEN]; 
	}

	~CUser()
	{
		_sessionID = -1;
		memset(_IP, 0, sizeof(WCHAR) * dfIP_LEN);
		_accountNo = -1;
		_lastRecvTime = 0;
		delete[] _ID;
		delete[] _nickname;
		delete[] _sessionKey;
	}

public:
	__int64 _sessionID = -1;
	WCHAR _IP[dfIP_LEN] = { L"0" };
	SRWLOCK _lock;

public:
	__int64 _accountNo = -1;
	DWORD _lastRecvTime = 0;
	WCHAR* _ID;
	WCHAR* _nickname;
	CHAR* _sessionKey;
};