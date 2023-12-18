#pragma once
#include "CommonProtocol.h"

class CPlayer
{
public:
	CPlayer() { __debugbreak(); };
	CPlayer(unsigned __int64 sessionID, __int64 playerID)
	{
		_sessionID = sessionID;
		_playerID = playerID;

		_accountNo = -1;
		_ID = new wchar_t[dfID_LEN];
		_nickname = new wchar_t[dfNICKNAME_LEN];
		_sessionKey = new char[dfSESSIONKEY_LEN];

		_sectorX = -1;
		_sectorY = -1;
		_lastRecvTime = timeGetTime();
	}

	~CPlayer()
	{
		_sessionID = MAXULONGLONG;
		_playerID = -1;

		_accountNo = -1;		
		delete[] _ID;
		delete[] _nickname;
		delete[] _sessionKey;

		_sectorX = -1;
		_sectorY = -1;
		_lastRecvTime = 0;
	}

public:
	unsigned __int64 _sessionID = MAXULONGLONG;
	__int64 _playerID = -1;

	__int64 _accountNo = -1;
	wchar_t* _ID;
	wchar_t* _nickname;
	char* _sessionKey;

	WORD _sectorX = -1;
	WORD _sectorY = -1;
	DWORD _lastRecvTime = 0;
};