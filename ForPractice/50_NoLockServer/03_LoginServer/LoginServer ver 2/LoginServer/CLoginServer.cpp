#include "CLoginServer.h"
#include <tchar.h>
#include <wchar.h>

// #define _TIMEOUT

bool CLoginServer::Initialize()
{
	InitializeSRWLock(&_mapLock);
	InitializeSRWLock(&_setLock);
	_pUserPool = new CTlsPool<CUser>(dfUSER_MAX, true);
	
	// Set IP Address
	_WanIPs = new WCHAR[dfIP_LEN];
	wcscpy_s(_WanIPs, dfIP_LEN, L"127.0.0.1");

	WCHAR* lanIP1_key = new WCHAR[dfIP_LEN];
	WCHAR* lanIP1_val = new WCHAR[dfIP_LEN];
	WCHAR* lanIP2_key = new WCHAR[dfIP_LEN];
	WCHAR* lanIP2_val = new WCHAR[dfIP_LEN];
	wcscpy_s(lanIP1_key, dfIP_LEN, L"10.0.1.2");
	wcscpy_s(lanIP1_val, dfIP_LEN, L"10.0.1.1");
	wcscpy_s(lanIP2_key, dfIP_LEN, L"10.0.2.2");
	wcscpy_s(lanIP2_val, dfIP_LEN, L"10.0.2.1");
	_LanIPs.insert(make_pair(lanIP1_key, lanIP1_val));
	_LanIPs.insert(make_pair(lanIP2_key, lanIP2_val));

	// Set Network Library
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	int cpuCount = (int)si.dwNumberOfProcessors;
	int networkThreadCnt = 0;

	::wprintf(L"CPU total: %d\n", cpuCount);
	::wprintf(L"Login Thread Count (Except Accept Thread) (recommend under %d): ", cpuCount - 1);
	::scanf_s("%d", &networkThreadCnt);

	if (!NetworkInitialize(dfSERVER_IP, dfLOGIN_PORT, networkThreadCnt, false))
	{
		Terminate();
		return false;
	}

	// Initialize Compelete
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Server Initialize\n");
	::wprintf(L"Server Initialize\n\n");

#ifdef _TIMEOUT
	// Set Timeout Thread
	_timeoutThread = (HANDLE)_beginthreadex(NULL, 0, TimeoutThread, this, 0, nullptr);
	if (_timeoutThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Timeout Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Timeout Thread Error\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
#endif

	return true;
}

void CLoginServer::Terminate()
{
	delete[] _WanIPs;
	map<WCHAR*, WCHAR*, cmp>::iterator it = _LanIPs.begin();
	for (; it != _LanIPs.end(); it++)
	{
		delete[] it->first;
		delete[] it->second;
	}
}

void CLoginServer::OnInitialize()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Library Initialize\n");
	::wprintf(L"Network Library Initialize\n");
}

void CLoginServer::OnTerminate()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Library Terminate\n");
	::wprintf(L"Network Library Terminate\n");
}

void CLoginServer::OnThreadTerminate(wchar_t* threadName)
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s Terminate\n", threadName);
	::wprintf(L"%s Terminate\n", threadName);
}

void CLoginServer::OnError(int errorCode, wchar_t* errorMsg)
{
	LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s (%d)\n", errorMsg, errorCode);
	::wprintf(L"%s (%d)\n", errorMsg, errorCode);
}

void CLoginServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
	// LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s (%d)\n", debugMsg, debugCode);
	// ::wprintf(L"%s (%d)\n", debugMsg, debugCode);
}

bool CLoginServer::OnConnectRequest(WCHAR addr[dfADDRESS_LEN])
{
	return true;
}

void CLoginServer::OnAcceptClient(unsigned __int64 sessionID, WCHAR addr[dfADDRESS_LEN])
{
	AcquireSRWLockExclusive(&_mapLock);

	WCHAR* IP = nullptr;
	WCHAR* next = nullptr;
	IP = wcstok_s(addr, L":", &next);

	CUser* user = _pUserPool->Alloc(sessionID, IP);
	auto ret = _usersMap.insert(make_pair(sessionID, user));
	if (ret.second == false)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d] Already Exist SessionID: %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d] Already Exist SessionID: %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
	}

	ReleaseSRWLockExclusive(&_mapLock);
}

void CLoginServer::OnReleaseClient(unsigned __int64 sessionID)
{
	AcquireSRWLockExclusive(&_mapLock);

	unordered_map<unsigned __int64, CUser*>::iterator mapIter = _usersMap.find(sessionID);
	if (mapIter == _usersMap.end())
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		ReleaseSRWLockExclusive(&_mapLock);
		return;
	}
	CUser* user = mapIter->second;
	_usersMap.erase(mapIter);

	AcquireSRWLockShared(&user->_lock);
	ReleaseSRWLockExclusive(&_mapLock);
	__int64 accountNo = user->_accountNo;
	ReleaseSRWLockShared(&user->_lock);
	_pUserPool->Free(user);

	AcquireSRWLockExclusive(&_setLock);
	unordered_set<__int64>::iterator setIter = _accountNos.find(accountNo);
	if (setIter == _accountNos.end())
	{
		ReleaseSRWLockExclusive(&_setLock);
		return;
	}
	_accountNos.erase(setIter);
	ReleaseSRWLockExclusive(&_setLock);
}

void CLoginServer::OnRecv(unsigned __int64 sessionID, CPacket* packet)
{
	AcquireSRWLockShared(&_mapLock);
	unordered_map<unsigned __int64, CUser*>::iterator mapIter = _usersMap.find(sessionID);
	if (mapIter == _usersMap.end())
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		CPacket::Free(packet);
		ReleaseSRWLockShared(&_mapLock);
		return;
	}

	CUser* user = mapIter->second;
	AcquireSRWLockExclusive(&user->_lock);
	ReleaseSRWLockShared(&_mapLock);

	user->_lastRecvTime = timeGetTime();
	short msgType = -1;
	*packet >> msgType;

	try
	{
		switch (msgType)
		{
		case en_PACKET_CS_LOGIN_REQ_LOGIN:
			HandleCSPacket_REQ_LOGIN(packet, user);
			break;

		default:
			Disconnect(sessionID);
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
			::wprintf(L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
			break;
		}

		InterlockedIncrement(&_handlePacketTPS);
	}
	catch (int packetError)
	{
		if (packetError == ERR_PACKET)
		{
			Disconnect(sessionID);
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL,L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
			::wprintf(L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
		}
	}
	ReleaseSRWLockExclusive(&user->_lock);
}

void CLoginServer::OnSend(unsigned __int64 sessionID, int sendSize)
{
	
}

unsigned int __stdcall CLoginServer::TimeoutThread(void* arg)
{
	CLoginServer* pServer = (CLoginServer*)arg;

	while (pServer->_serverAlive)
	{
		AcquireSRWLockExclusive(&pServer->_mapLock);
		DWORD time = timeGetTime();
		unordered_map<unsigned __int64, CUser*>::iterator it = pServer->_usersMap.begin();
		for (; it != pServer->_usersMap.end(); it++)
		{
			if (it->second->_lastRecvTime - time > dfTIMEOUT)
				pServer->Disconnect(it->first);
		}
		ReleaseSRWLockExclusive(&pServer->_mapLock);
		Sleep(dfTIMEOUT);
	}

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Timeout Thread (%d) Terminate\n", GetCurrentThreadId());
	::wprintf(L"Timeout Thread (%d) Terminate\n", GetCurrentThreadId());

	return 0;
}

void CLoginServer::ReqSendUnicast(CPacket* packet, unsigned __int64 sessionID)
{
	packet->AddUsageCount(1);
	if (!SendPacket(sessionID, packet))
	{
		CPacket::Free(packet);
	}
}

void CLoginServer::GetDataFromMysql()
{
	// TO-DO
	// Sleep(100);
}

void CLoginServer::SetDataToRedis()
{
	// TO-DO
	// Sleep(50);
}

void CLoginServer::GetIPforClient(CUser* user, WCHAR* gameIP, WCHAR* chatIP)
{
	map<WCHAR*, WCHAR*, cmp>::iterator it = _LanIPs.find(user->_IP);
	if (it != _LanIPs.end())
	{
		wcscpy_s(gameIP, dfIP_LEN, it->second);
		wcscpy_s(chatIP, dfIP_LEN, it->second);
		// ::wprintf(L"%s => %s\n", user->_IP, it->second);
	}
	else
	{
		wcscpy_s(gameIP, dfIP_LEN, _WanIPs);
		wcscpy_s(chatIP, dfIP_LEN, _WanIPs);
		// ::wprintf(L"%s => %s\n", user->_IP, _WanIPs);
	}	
}

void CLoginServer::GetDataforUser(__int64 accountNo, WCHAR*& ID, WCHAR*& nickname)
{
	int a = (int)accountNo / 10000;
	int b = (int)accountNo % 10000;

	if (a > 0)
	{
		if (b == 0)
		{
			swprintf_s(ID, dfID_LEN, L"ID_%d", a);
			swprintf_s(nickname, dfNICKNAME_LEN, L"NICK_%d", a);
		}
		else
		{
			swprintf_s(ID, dfID_LEN, L"ID_%d%d", a, b + 1);
			swprintf_s(nickname, dfNICKNAME_LEN, L"NICK_%d%d", a, b + 1);
		}
	}
	else if (a == 0)
	{
		if (b == 1)
		{
			swprintf_s(ID, dfID_LEN, L"ID_%d", a);
			swprintf_s(nickname, dfNICKNAME_LEN, L"NICK_%d", a);
		}
		else
		{
			swprintf_s(ID, dfID_LEN, L"ID_%d%d", a, b);
			swprintf_s(nickname, dfNICKNAME_LEN, L"NICK_%d%d", a, b);
		}
	}

	// ::wprintf(L"%s\n", ID);
}

BYTE CLoginServer::CheckAccountNoValid(__int64 accountNo)
{
	if (accountNo < 0 || accountNo > 95000)
		return dfLOGIN_STATUS_ACCOUNT_MISS;

	AcquireSRWLockExclusive(&_setLock);
	auto ret = _accountNos.insert(accountNo);
	ReleaseSRWLockExclusive(&_setLock);
	if (ret.second == false)
		return dfLOGIN_STATUS_GAME;

	return dfLOGIN_STATUS_OK;
}

inline void CLoginServer::HandleCSPacket_REQ_LOGIN(CPacket* CSpacket, CUser* user)
{
	__int64 accountNo;
	char* sessionKey = new char[dfSESSIONKEY_LEN];
	GetCSPacket_REQ_LOGIN(CSpacket, accountNo, sessionKey);

	BYTE status = CheckAccountNoValid(accountNo);
	if (status != 1)
	{
		Disconnect(user->_sessionID);
		// LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Account No is Wrong: %lld\n", _T(__FUNCTION__), __LINE__,accountNo);
		// ::wprintf(L"%s[%d] Account No is Wrong: %lld\n", _T(__FUNCTION__), __LINE__, accountNo);
		return;
	}

	user->_accountNo = accountNo;
	memcpy_s(user->_sessionKey, dfSESSIONKEY_LEN, sessionKey, dfSESSIONKEY_LEN);
	delete[] sessionKey;
	
	GetDataFromMysql();
	SetDataToRedis();

	CPacket* SCpacket = CPacket::Alloc();
	SCpacket->Clear();
	
	WCHAR* ID = new WCHAR[dfID_LEN];
	WCHAR* nickname = new WCHAR[dfNICKNAME_LEN];
	GetDataforUser(accountNo, ID, nickname);

	WCHAR* gameIP = new WCHAR[dfIP_LEN];
	WCHAR* chatIP = new WCHAR[dfIP_LEN];
	GetIPforClient(user, gameIP, chatIP);

	// ::printf("(Login Server) Login Req: %lld, %lld, %d\n", accountNo, user->_accountNo, status);
	SetSCPacket_RES_LOGIN(SCpacket, user->_accountNo, status, ID, nickname, gameIP, dfGAME_PORT, chatIP, dfCHAT_PORT);
	ReqSendUnicast(SCpacket, user->_sessionID);

	delete[] ID;
	delete[] nickname;
	delete[] gameIP;
	delete[] chatIP;
}

inline void CLoginServer::GetCSPacket_REQ_LOGIN(CPacket* packet, __int64& accountNo, char*& sessionKey)
{
	*packet >> accountNo;
	packet->GetPayloadData((char*)sessionKey, dfSESSIONKEY_LEN);
}

inline void CLoginServer::SetSCPacket_RES_LOGIN(CPacket* packet, __int64 accountNo, BYTE status, WCHAR*& ID, WCHAR*& nickname, WCHAR*& gameIP, unsigned short gamePort, WCHAR*& chatIP, unsigned short chatPort)
{
	WORD type = en_PACKET_CS_LOGIN_RES_LOGIN;
	*packet << type;

	*packet << accountNo;
	*packet << status;
	packet->PutPayloadData((char*)ID, dfID_LEN * sizeof(wchar_t));
	packet->PutPayloadData((char*)nickname, dfNICKNAME_LEN * sizeof(wchar_t));

	packet->PutPayloadData((char*)gameIP, dfIP_LEN * sizeof(wchar_t));
	*packet << gamePort;
	packet->PutPayloadData((char*)chatIP, dfIP_LEN * sizeof(wchar_t));
	*packet << chatPort;
}


