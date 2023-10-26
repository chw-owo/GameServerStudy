#pragma once
#include "Protocol.h"

class CPlayer
{
public:
	CPlayer() {};
	CPlayer(__int64 sessionID, __int64 playerID) : 
	_sessionID(sessionID), _playerID(playerID), _sectorX(0), _sectorY(0)
	{
		_ID = new wchar_t[dfID_LEN];
		_nickname = new wchar_t[dfNICKNAME_LEN];
		_sessionKey = new char[dfSESSIONKEY_LEN];
		_lastRecvTime = timeGetTime();
	}

	~CPlayer()
	{
		delete[] _ID;
		delete[] _nickname;
		delete[] _sessionKey;
	}

public:
	__int64 _sessionID;
	__int64 _playerID;

	__int64 _accountNo = -1;
	wchar_t* _ID;
	wchar_t* _nickname;
	char* _sessionKey;

	WORD _sectorX;
	WORD _sectorY;
	DWORD _lastRecvTime;
};