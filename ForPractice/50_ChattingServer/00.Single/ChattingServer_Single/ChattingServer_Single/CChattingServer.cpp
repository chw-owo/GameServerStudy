#include "CChattingServer.h"
#include "CSystemLog.h"
#include <tchar.h>

#define _MONITOR

bool CChattingServer::Initialize()
{
	// Set Queue & Pool
	_pJobQueue = new CLockFreeQueue<CJob*>;
	_pJobPool = new CLockFreePool<CJob>(0, false);
	_pPlayerPool = new CObjectPool<CPlayer>(dfPLAYER_MAX, true);

	// Set Logic Thread
	_updateThread = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, this, 0, nullptr);
	if (_updateThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Logic Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Logic Thread Error\n", _T(__FUNCTION__), __LINE__);
		return false;
	}

	// Set Network Library
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int threadCnt = (int)si.dwNumberOfProcessors / 2;
	if (!NetworkInitialize(dfSERVER_IP, dfSERVER_PORT, threadCnt, false))
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

	return true;
}

void CChattingServer::Terminate()
{
	NetworkTerminate();
	_serverAlive = false;
	WaitForSingleObject(_updateThread, INFINITE);

#ifdef _MONITOR
	WaitForSingleObject(_monitorThread, INFINITE);
#endif

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Server Terminate.\n");
	::wprintf(L"Server Terminate.\n");
}

void CChattingServer::OnInitialize()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Initialize\n");
	::wprintf(L"Network Initialize\n");
}

void CChattingServer::OnTerminate()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Terminate\n");
	::wprintf(L"Network Terminate\n");
}

void CChattingServer::OnThreadTerminate(wchar_t* threadName)
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s Terminate\n", threadName);
	::wprintf(L"%s Terminate\n", threadName);
}

void CChattingServer::OnError(int errorCode, wchar_t* errorMsg)
{
	LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s (%d)\n", errorMsg, errorCode);
	::wprintf(L"%s (%d)\n", errorMsg, errorCode);
}

bool CChattingServer::OnConnectRequest()
{
	return false;
}

void CChattingServer::OnAcceptClient(__int64 sessionID)
{
	CJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::ACCEPT, sessionID, nullptr);
	_pJobQueue->Enqueue(job);
}

void CChattingServer::OnReleaseClient(__int64 sessionID)
{
	CJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::RELEASE, sessionID, nullptr);
	_pJobQueue->Enqueue(job);
}

void CChattingServer::OnRecv(__int64 sessionID, CPacket* packet)
{
	CJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::CONTENT, SYS_TYPE::NONE, sessionID, packet);
	_pJobQueue->Enqueue(job);
}

void CChattingServer::OnSend(__int64 sessionID, int sendSize)
{
}

void CChattingServer::HandleAccept(__int64 sessionID)
{
	if (_players.size() >= dfPLAYER_MAX)
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] player max\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d] player max\n", _T(__FUNCTION__), __LINE__);
		Disconnect(sessionID);
		return;
	}

	CPlayer* pPlayer = _pPlayerPool->Alloc(sessionID, _playerIDGenerator++);
	_players.insert(pPlayer);
	_playersMap.insert(make_pair(sessionID, pPlayer));
	_sectors[pPlayer->_sectorY][pPlayer->_sectorX]._players.push_back(pPlayer);

	return;
}

void CChattingServer::HandleRelease(__int64 sessionID)
{
	// Delete From SessionID-Player Map
	unordered_map<__int64, CPlayer*>::iterator mapIter = _playersMap.find(sessionID);
	if (mapIter == _playersMap.end())
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		return;
	}
	CPlayer* pPlayer = mapIter->second;
	_playersMap.erase(mapIter);

	// Delete From Player Set
	unordered_set<CPlayer*>::iterator setIter = _players.find(pPlayer);
	if (setIter != _players.end())
	{
		_players.erase(setIter);
	}

	// Delete From Sector
	CSector sector = _sectors[pPlayer->_sectorY][pPlayer->_sectorX];
	vector<CPlayer*>::iterator vectorIter = sector._players.begin();
	for (; vectorIter < sector._players.end(); vectorIter++)
	{
		if ((*vectorIter) == pPlayer)
		{
			sector._players.erase(vectorIter);
			break;
		}
	}

	// Delete Player
	_pPlayerPool->Free(pPlayer);
}

void CChattingServer::HandleRecv(__int64 sessionID, CPacket* packet)
{
	try
	{
		unordered_map<__int64, CPlayer*>::iterator iter = _playersMap.find(sessionID);
		if (iter == _playersMap.end())
		{
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session\n", _T(__FUNCTION__), __LINE__);
			::wprintf(L"%s[%d]: No Session\n", _T(__FUNCTION__), __LINE__);
			return;
		}
		CPlayer* pPlayer = iter->second;
		pPlayer->_lastRecvTime = timeGetTime();
		short msgType = -1;
		*packet >> msgType;

		switch (msgType)
		{
		case en_PACKET_CS_CHAT_REQ_LOGIN:
			HandleCSPacket_REQ_LOGIN(packet, pPlayer);
			break;

		case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
			HandleCSPacket_REQ_SECTOR_MOVE(packet, pPlayer);
			break;

		case en_PACKET_CS_CHAT_REQ_MESSAGE:
			HandleCSPacket_REQ_MESSAGE(packet, pPlayer);
			break;

		case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
			HandleCSPacket_REQ_HEARTBEAT(pPlayer);
			break;

		default:
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
			::wprintf(L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
			break;
		}
	}
	catch (int packetError)
	{
		if (packetError == ERR_PACKET)
			Disconnect(sessionID);
	}
}

unsigned int __stdcall CChattingServer::UpdateThread(void* arg)
{
	CChattingServer* pServer = (CChattingServer*)arg;
	CLockFreeQueue<CJob*>* pJobQueue = pServer->_pJobQueue;
	DWORD oldTick = timeGetTime();

	while (pServer->_serverAlive)
	{
		if (pJobQueue->GetUseSize() == 0) continue;

		CJob* job = pJobQueue->Dequeue();
		if (job->_type == JOB_TYPE::SYSTEM)
		{
			if (job->_sysType == SYS_TYPE::ACCEPT)
			{
				pServer->HandleAccept(job->_sessionID);
			}
			else if (job->_sysType == SYS_TYPE::RELEASE)
			{
				pServer->HandleRelease(job->_sessionID);
			}
		}
		else if (job->_type == JOB_TYPE::CONTENT)
		{
			pServer->HandleRecv(job->_sessionID, job->_packet);
		}

		if ((timeGetTime() - oldTick) >= dfTIMEOUT)
		{
			unordered_set<CPlayer*>::iterator setIter = pServer->_players.begin();
			for(; setIter != pServer->_players.end();)
			{
				if ((timeGetTime() - (*setIter)->_lastRecvTime) >= dfTIMEOUT)
				{
					pServer->Disconnect((*setIter)->_sessionID);
					setIter = pServer->_players.erase(setIter);
				}
				else
				{
					setIter++;
				}
			}
			oldTick = timeGetTime();
		}
	}
	return 0;
}

unsigned int __stdcall CChattingServer::MonitorThread(void* arg)
{
	CChattingServer* pServer = (CChattingServer*)arg;
	CreateDirectory(L"MonitorLog", NULL);

	while (pServer->_serverAlive)
	{
		Sleep(1000);
		pServer->UpdateMonitorData();
		pServer->_totalAccept += pServer->GetAcceptTPS();
		pServer->_totalDisconnect += pServer->GetDisconnectTPS();

		SYSTEMTIME stTime;
		GetLocalTime(&stTime);
		WCHAR text[dfMONITOR_TEXT_LEN];
		swprintf_s(text, dfMONITOR_TEXT_LEN, L"[%s %02d:%02d:%02d]\n\nTotal Accept: %d\nTotal Disconnect: %d\nConnected Session: %d\nAccept/1sec: %d\nDisconnect/1sec: %d\n\n",
			_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond, pServer->_totalAccept, pServer->_totalDisconnect, pServer->GetSessionCount(), pServer->GetAcceptTPS(), pServer->GetDisconnectTPS());
		::wprintf(L"%s", text);

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
	}
	return 0;
}

void CChattingServer::ReqSendUnicast(CPacket* packet, __int64 sessionID)
{
	SendPacket(sessionID, packet);
}

void CChattingServer::ReqSendAroundSector(CPacket* packet, CSector* centerSector, CPlayer* pExpPlayer)
{
	if (pExpPlayer == nullptr)
	{
		for (int i = 0; i < dfAROUND_SECTOR_NUM; i++)
		{
			vector<CPlayer*>::iterator playerIter = centerSector->_around[i]->_players.begin();
			for (; playerIter < centerSector->_around[i]->_players.end(); playerIter++)
			{
				SendPacket((*playerIter)->_sessionID, packet);
			}
		}
	}
	else
	{
		for (int i = 0; i < dfAROUND_SECTOR_NUM; i++)
		{
			vector<CPlayer*>::iterator playerIter = centerSector->_around[i]->_players.begin();
			for (; playerIter < centerSector->_around[i]->_players.end(); playerIter++)
			{
				SendPacket((*playerIter)->_sessionID, packet);
			}
		}

		vector<CPlayer*>::iterator playerIter = centerSector->_players.begin();
		for (; playerIter < centerSector->_players.end(); playerIter++)
		{
			if (*playerIter != pExpPlayer)
			{
				SendPacket((*playerIter)->_sessionID, packet);
			}
		}
	}
}

inline void CChattingServer::HandleCSPacket_REQ_LOGIN(CPacket* CSpacket, CPlayer* player)
{
	__int64 accountNo;
	wchar_t* ID = new wchar_t[dfID_LEN];
	wchar_t* nickname = new wchar_t[dfNICKNAME_LEN];
	char* sessionKey = new char[dfSESSIONKEY_LEN];
	GetCSPacket_REQ_LOGIN(CSpacket, accountNo, ID, nickname, sessionKey);

	player->_accountNo = accountNo;
	memcpy_s(player->_ID, dfID_LEN * sizeof(wchar_t), ID, dfID_LEN * sizeof(wchar_t));
	memcpy_s(player->_nickname, dfNICKNAME_LEN * sizeof(wchar_t), nickname, dfNICKNAME_LEN * sizeof(wchar_t));
	memcpy_s(player->_sessionKey, dfSESSIONKEY_LEN, sessionKey, dfSESSIONKEY_LEN);
	delete[] ID;
	delete[] nickname;
	delete[] sessionKey;
	BYTE status = 1;

	CPacket* SCpacket = CPacket::Alloc();
	SetSCPacket_RES_LOGIN(SCpacket, status, player->_accountNo);
	ReqSendUnicast(SCpacket, player->_sessionID);
}

inline void CChattingServer::HandleCSPacket_REQ_SECTOR_MOVE(CPacket* CSpacket, CPlayer* player)
{
	__int64 accountNo;
	WORD sectorX;
	WORD sectorY;
	GetCSPacket_REQ_SECTOR_MOVE(CSpacket, accountNo, sectorX, sectorY);

	if (player->_accountNo != accountNo) 
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		::wprintf(L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		return;
	}
	player->_sectorX = sectorX;
	player->_sectorY = sectorY;

	CPacket* SCpacket = CPacket::Alloc();
	SetSCPacket_RES_SECTOR_MOVE(SCpacket, player->_accountNo, player->_sectorX, player->_sectorY);
	ReqSendAroundSector(SCpacket, &_sectors[player->_sectorY][player->_sectorX]);
}

inline void CChattingServer::HandleCSPacket_REQ_MESSAGE(CPacket* CSpacket, CPlayer* player)
{
	__int64 accountNo;
	WORD messageLen;
	wchar_t* message;
	GetCSPacket_REQ_MESSAGE(CSpacket, accountNo, messageLen, message);

	if (player->_accountNo != accountNo)
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		::wprintf(L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		return;
	}

	CPacket* SCpacket = CPacket::Alloc();
	SetSCPacket_RES_MESSAGE(SCpacket, player->_accountNo, player->_ID, player->_nickname, messageLen, message);
	ReqSendAroundSector(SCpacket, &_sectors[player->_sectorY][player->_sectorX]);
	
	delete[] message; // new[] - delete[]를 다른 함수에서 하는 게 기분이 나쁘다...
}

inline void CChattingServer::HandleCSPacket_REQ_HEARTBEAT(CPlayer* player)
{
	player->_lastRecvTime = timeGetTime();
}

inline void CChattingServer::GetCSPacket_REQ_LOGIN(CPacket* packet, __int64& accountNo, wchar_t*& ID, wchar_t*& nickname, char*& sessionKey)
{
	*packet >> accountNo;
	packet->GetPayloadData((char*)ID, dfID_LEN * sizeof(wchar_t));
	packet->GetPayloadData((char*)nickname, dfNICKNAME_LEN * sizeof(wchar_t));
	packet->GetPayloadData((char*)sessionKey, dfSESSIONKEY_LEN);
}

inline void CChattingServer::GetCSPacket_REQ_SECTOR_MOVE(CPacket* packet, __int64& accountNo, WORD& sectorX, WORD& sectorY)
{
	*packet >> accountNo;
	*packet >> sectorX;
	*packet >> sectorY;
}

inline void CChattingServer::GetCSPacket_REQ_MESSAGE(CPacket* packet, __int64& accountNo, WORD& messageLen, wchar_t*& message)
{
	*packet >> accountNo;
	*packet >> messageLen;
	message = new wchar_t[messageLen]; 
	packet->GetPayloadData((char*)message, messageLen * sizeof(wchar_t));
}

inline void CChattingServer::SetSCPacket_RES_LOGIN(CPacket* packet, BYTE status, __int64 accountNo)
{
	WORD type = en_PACKET_CS_CHAT_RES_LOGIN;
	*packet << type;
	*packet << status;
	*packet << accountNo;
}

inline void CChattingServer::SetSCPacket_RES_SECTOR_MOVE(CPacket* packet, __int64 accountNo, WORD sectorX, WORD sectorY)
{
	WORD type = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
	*packet << type;
	*packet << accountNo;
	*packet << sectorX;
	*packet << sectorY;
}

inline void CChattingServer::SetSCPacket_RES_MESSAGE(CPacket* packet, __int64 accountNo, wchar_t* ID, wchar_t* nickname, WORD messageLen, wchar_t* message)
{
	WORD type = en_PACKET_CS_CHAT_RES_MESSAGE;
	*packet << type;
	*packet << accountNo;
	packet->PutPayloadData((char*)ID, dfID_LEN * sizeof(wchar_t));
	packet->PutPayloadData((char*)nickname, dfNICKNAME_LEN * sizeof(wchar_t));
	*packet << messageLen;
	packet->PutPayloadData((char*)message, messageLen * sizeof(wchar_t));
}
