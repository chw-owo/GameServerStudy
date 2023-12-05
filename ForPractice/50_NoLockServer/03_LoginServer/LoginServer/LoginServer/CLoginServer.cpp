#include "CLoginServer.h"
#include <tchar.h>
#include <wchar.h>

// #define _MONITOR
// #define _TIMEOUT

bool CLoginServer::Initialize()
{
	InitializeSRWLock(&_mapLock);
	InitializeSRWLock(&_setLock);
	_pUserPool = new CTlsPool<CUser>(dfUSER_MAX, true);

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

#ifdef _MONITOR
	// Set Monitor Thread
	_monitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, nullptr);
	if (_monitorThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
#endif

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

}

void CLoginServer::OnInitialize()
{

}

void CLoginServer::OnTerminate()
{

}

void CLoginServer::OnThreadTerminate(wchar_t* threadName)
{

}

void CLoginServer::OnError(int errorCode, wchar_t* errorMsg)
{

}

void CLoginServer::OnDebug(int debugCode, wchar_t* debugMsg)
{

}

bool CLoginServer::OnConnectRequest()
{
	return false;
}

void CLoginServer::OnAcceptClient(unsigned __int64 sessionID)
{
	AcquireSRWLockExclusive(&_mapLock);

	CUser* user = _pUserPool->Alloc(sessionID);
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
	CUser* user;
	try
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

		user = mapIter->second;
		AcquireSRWLockExclusive(&user->_lock);
		ReleaseSRWLockShared(&_mapLock);

		user->_lastRecvTime = timeGetTime();
		short msgType = -1;
		*packet >> msgType;

		switch (msgType)
		{
		case en_PACKET_CS_LOGIN_REQ_LOGIN:
			HandleCSPacket_REQ_LOGIN(packet, user);
			break;

		default:
			Disconnect(sessionID);
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
			::wprintf(L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
			break;
		}

		ReleaseSRWLockExclusive(&user->_lock);
		InterlockedIncrement(&_handlePacketTPS);
	}
	catch (int packetError)
	{
		ReleaseSRWLockExclusive(&user->_lock);
		if (packetError == ERR_PACKET)
		{
			Disconnect(sessionID);
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
			::wprintf(L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
		}
	}
}

void CLoginServer::OnSend(unsigned __int64 sessionID, int sendSize)
{
	
}

unsigned int __stdcall CLoginServer::MonitorThread(void* arg)
{
	CLoginServer* pServer = (CLoginServer*)arg;
	CreateDirectory(L"MonitorLog", NULL);

	while (pServer->_serverAlive)
	{
		pServer->UpdateMonitorData();

		SYSTEMTIME stTime;
		GetLocalTime(&stTime);
		WCHAR text[dfMONITOR_TEXT_LEN];

		swprintf_s(text, dfMONITOR_TEXT_LEN, L"[%s %02d:%02d:%02d]\n\nConnected Session: %d\nPacket Pool: %d/%d\nUser Count: %d\n\nTotal Accept: %d\nTotal Disconnect: %d\nRecv/1sec: %d\nSend/1sec: %d\nAccept/1sec: %d\nDisconnect/1sec: %d\n\n",
			_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond,
			pServer->GetSessionCount(), CPacket::GetPoolSize(), CPacket::GetNodeCount(), pServer->_usersMap.size(),
			pServer->GetAcceptTotal(), pServer->GetDisconnectTotal(), pServer->GetRecvMsgTPS(), pServer->GetSendMsgTPS(), pServer->GetAcceptTPS(), pServer->GetDisconnectTPS());

		::wprintf(L"%s", text);

		pServer->_handlePacketTPS = 0;

		FILE* file;
		errno_t openRet = _wfopen_s(&file, L"MonitorLog/MonitorLog.txt", L"a+");
		if (openRet != 0)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Fail to open %s : %d\n", _T(__FUNCTION__), __LINE__, L"MonitorLog/MonitorLog.txt", openRet);
			::wprintf(L"%s[%d]: Fail to open %s : %d\n", _T(__FUNCTION__), __LINE__, L"MonitorLog/MonitorLog.txt", openRet);
		}
		if (file != nullptr)
		{
			fwprintf(file, text);
			fclose(file);
		}
		else
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Fileptr is nullptr %s\n", _T(__FUNCTION__), __LINE__, L"MonitorLog/MonitorLog.txt");
			::wprintf(L"%s[%d]: Fileptr is nullptr %s\n", _T(__FUNCTION__), __LINE__, L"MonitorLog/MonitorLog.txt");
		}

		Sleep(1000);
	}

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Monitor Thread (%d) Terminate\n", GetCurrentThreadId());
	::wprintf(L"Monitor Thread (%d) Terminate\n", GetCurrentThreadId());
	return 0;
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

void CLoginServer::GetIPforClient(WCHAR*& gameIP, WCHAR*& chatIP)
{
	// TO-DO
	wcscpy_s(gameIP, dfIP_LEN, L"127.0.0.1");
	wcscpy_s(chatIP, dfIP_LEN, L"127.0.0.1");
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
	GetIPforClient(gameIP, chatIP);

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


