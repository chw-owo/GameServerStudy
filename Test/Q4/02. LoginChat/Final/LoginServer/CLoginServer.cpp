#include "CLoginServer.h"
#include <tchar.h>
#include <wchar.h>
#include <string>
#include <iostream>

// #define _TIMEOUT

bool CLoginServer::Initialize()
{
	// Set Data
	_tlsKey = TlsAlloc();
	InitializeSRWLock(&_mapLock);
	InitializeSRWLock(&_setLock);
	InitializeSRWLock(&_connLock);
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
	int threadCnt = 0;
	int runningThreadCnt = 0;

	::wprintf(L"CPU total: %d\n\n", cpuCount);

	::wprintf(L"<Login Server>\n");
	::wprintf(L"Total Network Thread Count: ");
	::scanf_s("%d", &threadCnt);
	::wprintf(L"Running Network Thread Count: ");
	::scanf_s("%d", &runningThreadCnt);

	if (!NetworkInitialize(dfSERVER_IP, dfLOGIN_PORT, dfSEND_TIME, threadCnt, runningThreadCnt, false))
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

	_redis = CRedis::GetInstance()->_redis;
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

bool CLoginServer::OnConnectRequest(WCHAR* addr)
{
	return true;
}

void CLoginServer::OnAcceptClient(unsigned __int64 sessionID, WCHAR* addr)
{
	WCHAR* IP = nullptr;
	WCHAR* next = nullptr;
	IP = wcstok_s(addr, L":", &next);
	CUser* user = _pUserPool->Alloc(sessionID, IP);

	AcquireSRWLockExclusive(&_mapLock);
	auto ret = _usersMap.insert(make_pair(sessionID, user));
	if (ret.second == false)
	{
		_pUserPool->Free(user);
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

	AcquireSRWLockExclusive(&user->_lock);
	ReleaseSRWLockExclusive(&_mapLock);
	__int64 accountNo = user->_accountNo;
	ReleaseSRWLockExclusive(&user->_lock);

	_pUserPool->Free(user);

	AcquireSRWLockExclusive(&_setLock);
	unordered_set<__int64>::iterator setIter = _accountNos.find(accountNo);
	if (setIter != _accountNos.end())
	{
		_accountNos.erase(setIter);
	}
	ReleaseSRWLockExclusive(&_setLock);
}

void CLoginServer::OnRecv(unsigned __int64 sessionID, CRecvNetPacket* packet)
{
	AcquireSRWLockShared(&_mapLock);
	unordered_map<unsigned __int64, CUser*>::iterator mapIter = _usersMap.find(sessionID);
	if (mapIter == _usersMap.end())
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		CRecvNetPacket::Free(packet);
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

		InterlockedIncrement(&_authTPS);
	}
	catch (int packetError)
	{
		if (packetError == ERR_PACKET)
		{
			Disconnect(sessionID);
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
			::wprintf(L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
		}
	}

	CRecvNetPacket::Free(packet);
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

void CLoginServer::ReqSendUnicast(CNetPacket* packet, unsigned __int64 sessionID)
{
	packet->AddUsageCount(1);
	if (!SendPacket(sessionID, packet))
	{
		CNetPacket::Free(packet);
	}
}

bool CLoginServer::InitDBConnect(long& idx)
{
	AcquireSRWLockExclusive(&_connLock);

	idx = _connCnt++;
	bool ret = TlsSetValue(_tlsKey, (LPVOID)idx);
	if (ret == 0)
	{	
		int err = GetLastError();
		LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Can not Set Tls: %d\n", err);
		::wprintf(L"Can not Set Tls: %d\n", err);
		ReleaseSRWLockExclusive(&_connLock);
		return false;
	}

	mysql_init(&_conn[idx]);
	_connection[idx] = mysql_real_connect(&_conn[idx],
		"127.0.0.1", "root", "password", "accountdb", 3306, (char*)NULL, 0);
	if (_connection == NULL)
	{
		LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Mysql connection error : %s", mysql_error(&_conn[idx]));
		::printf("Mysql connection error : %s", mysql_error(&_conn[idx]));	
		ReleaseSRWLockExclusive(&_connLock);
		return false;
	}

	ReleaseSRWLockExclusive(&_connLock);
	return true;
}

bool CLoginServer::GetDataFromMysql(CUser* user)
{
	long idx = (long)TlsGetValue(_tlsKey);
	if (idx == 0) 
	{
		if (!InitDBConnect(idx)) 
			return false;
	}

	MYSQL conn = _conn[idx];
	MYSQL* connection = _connection[idx];
	
	int query_stat;
	MYSQL_ROW sql_row;
	MYSQL_RES* sql_result;

	char query[dfQUERY_MAX] = { 0 };
	int ret = sprintf_s(query, dfQUERY_MAX,
		"SELECT * FROM account WHERE accountno = %d", user->_accountNo);

	query_stat = mysql_query(connection, query);
	if (query_stat != 0)
	{
		LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Mysql query error : %s (%d)\n", mysql_error(&conn), ret);
		::printf("Mysql query error : %s (%d)\n", mysql_error(&conn), ret);
		return false;
	}

	sql_result = mysql_store_result(connection);
	while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
	{
		// Get User Data from DB
	}

	SetUserData(user); // Now, Not Use DB Data
	mysql_free_result(sql_result);
	return true;
}

void CLoginServer::SetDataToRedis(CUser* user)
{
	__int64 accountNo = user->_accountNo;
	string key = to_string(user->_accountNo);
	_redis->set(key, user->_sessionKey);
	_redis->sync_commit();
}

void CLoginServer::GetIPforClient(CUser* user, WCHAR* gameIP, WCHAR* chatIP)
{
	map<WCHAR*, WCHAR*, cmp>::iterator it = _LanIPs.find(user->_IP);
	if (it != _LanIPs.end())
	{
		wcscpy_s(gameIP, dfIP_LEN, it->second);
		wcscpy_s(chatIP, dfIP_LEN, it->second);
	}
	else
	{
		wcscpy_s(gameIP, dfIP_LEN, _WanIPs);
		wcscpy_s(chatIP, dfIP_LEN, _WanIPs);
	}	
}

void CLoginServer::SetUserData(CUser* user)
{
	int a = (int)user->_accountNo / 10000;
	int b = (int)user->_accountNo % 10000;

	if (a > 0)
	{
		if (b == 0)
		{
			swprintf_s(user->_ID, dfID_LEN, L"ID_%d", a);
			swprintf_s(user->_nickname, dfNICKNAME_LEN, L"NICK_%d", a);
		}
		else
		{
			swprintf_s(user->_ID, dfID_LEN, L"ID_%d%d", a, b + 1);
			swprintf_s(user->_nickname, dfNICKNAME_LEN, L"NICK_%d%d", a, b + 1);
		}
	}
	else if (a == 0)
	{
		if (b == 1)
		{
			swprintf_s(user->_ID, dfID_LEN, L"ID_%d", a);
			swprintf_s(user->_nickname, dfNICKNAME_LEN, L"NICK_%d", a);
		}
		else
		{
			swprintf_s(user->_ID, dfID_LEN, L"ID_%d%d", a, b);
			swprintf_s(user->_nickname, dfNICKNAME_LEN, L"NICK_%d%d", a, b);
		}
	}
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

inline void CLoginServer::HandleCSPacket_REQ_LOGIN(CRecvNetPacket* CSpacket, CUser* user)
{
	__int64 accountNo;
	char* sessionKey = new char[dfSESSIONKEY_LEN];
	GetCSPacket_REQ_LOGIN(CSpacket, accountNo, sessionKey);

	BYTE status = CheckAccountNoValid(accountNo);
	user->_accountNo = accountNo;
	memcpy_s(user->_sessionKey, dfSESSIONKEY_LEN, sessionKey, dfSESSIONKEY_LEN);
	delete[] sessionKey;
	
	if(!GetDataFromMysql(user)) 
	{
		Disconnect(user->_sessionID);
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d] Fail to Get Data from DB: %lld\n", _T(__FUNCTION__), __LINE__,accountNo);
		::wprintf(L"%s[%d] Fail to Get Data from DB: %lld\n", _T(__FUNCTION__), __LINE__, accountNo);
		return;
	}

	SetDataToRedis(user);
	WCHAR* gameIP = new WCHAR[dfIP_LEN];
	WCHAR* chatIP = new WCHAR[dfIP_LEN];
	GetIPforClient(user, gameIP, chatIP);

	CNetPacket* sendPacket = CNetPacket::Alloc();
	sendPacket->Clear();
	SetSCPacket_RES_LOGIN(sendPacket, user->_accountNo,
		status, user->_ID, user->_nickname, gameIP, dfGAME_PORT, chatIP, dfCHAT_PORT);
	ReqSendUnicast(sendPacket, user->_sessionID);

	delete[] gameIP;
	delete[] chatIP;
}

inline void CLoginServer::GetCSPacket_REQ_LOGIN(CRecvNetPacket* packet, __int64& accountNo, char*& sessionKey)
{
	*packet >> accountNo;
	packet->GetPayloadData((char*)sessionKey, dfSESSIONKEY_LEN);
}

inline void CLoginServer::SetSCPacket_RES_LOGIN(CNetPacket* packet, __int64 accountNo, BYTE status, WCHAR*& ID, WCHAR*& nickname, WCHAR*& gameIP, unsigned short gamePort, WCHAR*& chatIP, unsigned short chatPort)
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

