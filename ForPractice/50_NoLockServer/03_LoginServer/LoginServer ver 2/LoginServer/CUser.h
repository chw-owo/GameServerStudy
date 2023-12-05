#pragma once
class CUser
{
public:
	CUser() {}

	CUser(__int64 sessionID, WCHAR* IP)
	{
		_sessionID = sessionID;
		_accountNo = -1;
		_lastRecvTime = 0;
		_sessionKey = new char[dfSESSIONKEY_LEN];
		wcscpy_s(_IP, dfIP_LEN, IP);
	}

	~CUser()
	{
		_sessionID = -1;
		_accountNo = -1;
		_lastRecvTime = 0;
		delete[] _sessionKey;
		memset(_IP, 0, sizeof(WCHAR) * dfIP_LEN);
	}

public:
	__int64 _sessionID = -1;
	__int64 _accountNo = -1;
	WCHAR _IP[dfIP_LEN] = { L"0" };
	char* _sessionKey;
	DWORD _lastRecvTime = 0;

};