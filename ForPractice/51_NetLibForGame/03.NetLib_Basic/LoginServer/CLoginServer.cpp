#include "CLoginServer.h"
#include <tchar.h>
#include <wchar.h>
#include <string>
#include <iostream>

bool CLoginServer::Initialize()
{
	// Set Data
	InitializeSRWLock(&_mapLock);
	_pUserPool = new CTlsPool<CUser>(dfUSER_MAX, true);

	// Set Network Library
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	int cpuCount = (int)si.dwNumberOfProcessors;
	int networkThreadCnt = 0;

	::wprintf(L"CPU total: %d\n", cpuCount);
	::wprintf(L"Network Thread Count (Except Accept Thread) (recommend under %d): ", cpuCount - 1);
	::scanf_s("%d", &networkThreadCnt);

	if (!NetworkInitialize(dfSERVER_IP, dfECHO_PORT, networkThreadCnt, false))
	{
		Terminate();
		return false;
	}

	// Initialize Compelete
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Server Initialize\n");
	::wprintf(L"Server Initialize\n\n");

	return true;
}

void CLoginServer::Terminate()
{

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
	LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s (%d)\n", debugMsg, debugCode);
	::wprintf(L"%s (%d)\n", debugMsg, debugCode);
}

bool CLoginServer::OnConnectRequest(WCHAR addr[dfADDRESS_LEN])
{
	return true;
}

void CLoginServer::OnAcceptClient(unsigned __int64 sessionID, WCHAR addr[dfADDRESS_LEN])
{
	CUser* user = _pUserPool->Alloc(sessionID);
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
	ReleaseSRWLockExclusive(&user->_lock);

	_pUserPool->Free(user);
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
	WORD msgType;
	*packet >> msgType;

	try
	{
		switch (msgType)
		{
		case en_PACKET_CS_GAME_REQ_LOGIN:
			// ::printf("%llx (%d):: Login Packet\n", sessionID, GetCurrentThreadId());
			HandlePacket_LOGIN(packet, user);
			break;

		case en_PACKET_CS_GAME_REQ_ECHO:
			// ::printf("%llx (%d):: Echo Packet\n", sessionID, GetCurrentThreadId());
			HandlePacket_ECHO(packet, user);
			break;

		default:
			Disconnect(sessionID);
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
			::wprintf(L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
			break;
		}
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
	ReleaseSRWLockExclusive(&user->_lock);
}

void CLoginServer::OnSend(unsigned __int64 sessionID, int sendSize)
{
	
}

void CLoginServer::ReqSendUnicast(CPacket* packet, unsigned __int64 sessionID)
{
	packet->AddUsageCount(1);
	if (!SendPacket(sessionID, packet))
	{
		CPacket::Free(packet);
	}
}

BYTE CLoginServer::CheckAccountNoValid(__int64 accountNo)
{
	return 1;
}

inline void CLoginServer::HandlePacket_LOGIN(CPacket* packet, CUser* user)
{
	__int64 accountNo;
	char* sessionKey = new char[dfSESSIONKEY_LEN];
	int version;
	GetCSPacket_REQ_LOGIN(packet, accountNo, sessionKey, version);
	delete[] sessionKey;

	BYTE status = CheckAccountNoValid(user->_sessionID);

	CPacket* sendPacket = CPacket::Alloc();
	sendPacket->Clear();
	SetSCPacket_RES_LOGIN(sendPacket, status, accountNo);
	ReqSendUnicast(sendPacket, user->_sessionID);
}

inline void CLoginServer::GetCSPacket_REQ_LOGIN(CPacket* packet, __int64& accountNo, char*& sessionKey, int& version)
{
	*packet >> accountNo;
	packet->GetPayloadData((char*)sessionKey, dfSESSIONKEY_LEN);
	*packet >> version;
}

inline void CLoginServer::SetSCPacket_RES_LOGIN(CPacket* packet, BYTE status, __int64 accountNo)
{
	*packet << (WORD)en_PACKET_CS_GAME_RES_LOGIN;
	*packet << status;
	*packet << accountNo;
}

inline void CLoginServer::HandlePacket_ECHO(CPacket* packet, CUser* user)
{
	__int64 accountNo;
	LONGLONG sendTick;
	GetCSPacket_REQ_ECHO(packet, accountNo, sendTick);

	CPacket* sendPacket = CPacket::Alloc();
	sendPacket->Clear();
	SetSCPacket_RES_ECHO(sendPacket, accountNo, sendTick);
	ReqSendUnicast(sendPacket, user->_sessionID);
}

inline void CLoginServer::GetCSPacket_REQ_ECHO(CPacket* packet, __int64& accountNo, LONGLONG& sendTick)
{
	*packet >> accountNo;
	*packet >> sendTick;
}

inline void CLoginServer::SetSCPacket_RES_ECHO(CPacket* packet, __int64 accountNo, LONGLONG sendTick)
{
	*packet << (WORD)en_PACKET_CS_GAME_RES_ECHO;
	*packet << accountNo;
	*packet << sendTick;
}