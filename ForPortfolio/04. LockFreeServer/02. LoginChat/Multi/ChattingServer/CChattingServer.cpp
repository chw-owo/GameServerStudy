#include "CChattingServer.h"
#include "CSystemLog.h"
#include <tchar.h>
#include <wchar.h>

// #define _TIMEOUT

bool CChattingServer::Initialize()
{
	// Set Resource

	_pJobPool = new CTlsPool<CNetJob>(dfJOB_DEF, true);

	for (int i = 0; i < dfSECTOR_CNT_Y; i++)
	{
		for (int j = 0; j < dfSECTOR_CNT_X; j++)
		{
			_sectors[i][j]._x = j;
			_sectors[i][j]._y = i;

			if (i > 0)												_sectors[i][j]._around.push_back(&_sectors[i - 1][j]);
			if (i > 0 && j > 0)										_sectors[i][j]._around.push_back(&_sectors[i - 1][j - 1]);
			if (i > 0 && j < dfSECTOR_CNT_Y - 1)					_sectors[i][j]._around.push_back(&_sectors[i - 1][j + 1]);

			if (i < dfSECTOR_CNT_X - 1)								_sectors[i][j]._around.push_back(&_sectors[i + 1][j]);
			if (i < dfSECTOR_CNT_X - 1 && j > 0)					_sectors[i][j]._around.push_back(&_sectors[i + 1][j - 1]);
			if (i < dfSECTOR_CNT_X - 1 && j < dfSECTOR_CNT_Y - 1)	_sectors[i][j]._around.push_back(&_sectors[i + 1][j + 1]);

			if (j > 0)												_sectors[i][j]._around.push_back(&_sectors[i][j - 1]);
			if (j < dfSECTOR_CNT_Y - 1)								_sectors[i][j]._around.push_back(&_sectors[i][j + 1]);
		}
	}

	// Set Logic Thread

	/*
	int totalThreadCnt = 0;
	int runningThreadCnt = 0;
	::wprintf(L"<Chatting Server>\n");
	::wprintf(L"Total Logic Thread Count: ");
	::scanf_s("%d", &totalThreadCnt);
	::wprintf(L"Running Logic Thread Count: ");
	::scanf_s("%d", &runningThreadCnt);
	*/

	_playerPerThread = dfPLAYER_MAX / dfUPDATE_THREAD_CNT;
	if (dfPLAYER_MAX % dfUPDATE_THREAD_CNT != 0)
		_playerPerThread++;

	int idx = 0;
	_players = new CPlayer * [dfUPDATE_THREAD_CNT];
	for (int i = 0; i < dfUPDATE_THREAD_CNT; i++)
	{
		_players[i] = new CPlayer[_playerPerThread];

		for (int j = 0; j < _playerPerThread; j++)
		{
			_players[i][j].Setting(idx, _playerPerThread);
			_usablePlayerIdx.Push(idx);
			idx++;
		}
	}
	InitializeSRWLock(&_playersLock);

	_signals = new long[dfUPDATE_THREAD_CNT];
	_updateThreads = new HANDLE[dfUPDATE_THREAD_CNT];
	for (int i = 0; i < dfUPDATE_THREAD_CNT; i++)
	{
		ThreadArg* arg = new ThreadArg(this, i);
		_updateThreads[i] = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, arg, 0, nullptr);
		if (_updateThreads[i] == NULL)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Logic Thread Error\n", _T(__FUNCTION__), __LINE__);
			::wprintf(L"%s[%d]: Begin Logic Thread Error\n", _T(__FUNCTION__), __LINE__);
			return false;
		}
	}

	// Set Network Library
	int totalThreadCnt = 0;
	int runningThreadCnt = 0;
	::wprintf(L"<Chatting Server>\n");
	::wprintf(L"Total Network Thread Count: ");
	::scanf_s("%d", &totalThreadCnt);
	::wprintf(L"Running Network Thread Count: ");
	::scanf_s("%d", &runningThreadCnt);

	if (!NetworkInitialize(dfSERVER_IP, dfCHAT_PORT, dfSEND_TIME, totalThreadCnt, runningThreadCnt, false, true))
	{
		Terminate();
		return false;
	}

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

void CChattingServer::Terminate()
{
	/*
	NetworkTerminate();
	_serverAlive = false;

	CNetJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::TERMINATE, 0, nullptr);
	EnqueueJob(0, job); // TO-DO

	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
	// WaitForSingleObject(_updateThread, INFINITE);

#ifdef _TIMEOUT
	WaitForSingleObject(_timeoutThread, INFINITE);
#endif

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Chat: Server Terminate.\n");
	::wprintf(L"Chat: Server Terminate.\n");
	*/
}

void CChattingServer::OnInitialize()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Chat: Network Library Initialize\n");
	::wprintf(L"Chat: Network Library Initialize\n");
}

void CChattingServer::OnTerminate()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Chat: Network Library Terminate\n");
	::wprintf(L"Chat: Network Library Terminate\n");
}

void CChattingServer::OnThreadTerminate(wchar_t* threadName)
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Chat: %s Terminate\n", threadName);
	::wprintf(L"Chat: %s Terminate\n", threadName);
}

void CChattingServer::OnError(int errorCode, wchar_t* errorMsg)
{
	LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"Chat: %s (%d)\n", errorMsg, errorCode);
	::wprintf(L"Chat: %s (%d)\n", errorMsg, errorCode);
}

void CChattingServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
	LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"Chat: %s (%d)\n", debugMsg, debugCode);
	::wprintf(L"Chat: %s (%d)\n", debugMsg, debugCode);
}

bool CChattingServer::OnConnectRequest(WCHAR* addr)
{
	return false;
}

void CChattingServer::OnAcceptClient(unsigned __int64 sessionID, WCHAR* addr)
{
	AcquireSRWLockExclusive(&_playersLock);

	if (_usablePlayerIdx.GetUseSize() == 0)
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] player max\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d] player max\n", _T(__FUNCTION__), __LINE__);
		Disconnect(sessionID);
		ReleaseSRWLockExclusive(&_playersLock);
		return;
	}
	int idx = _usablePlayerIdx.Pop();
	if (idx == 0)
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] player max\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d] player max\n", _T(__FUNCTION__), __LINE__);
		Disconnect(sessionID);
		ReleaseSRWLockExclusive(&_playersLock);
		return;
	}
	CPlayer* pPlayer = &_players[idx / _playerPerThread][idx % _playerPerThread];

	AcquireSRWLockExclusive(&pPlayer->_lock);
	pPlayer->Initialize(sessionID, _playerIDGenerator++);
	ReleaseSRWLockExclusive(&pPlayer->_lock);

	_playersMap.insert(make_pair(sessionID, pPlayer));

	ReleaseSRWLockExclusive(&_playersLock);
}

void CChattingServer::OnReleaseClient(unsigned __int64 sessionID)
{
	AcquireSRWLockExclusive(&_playersLock);
	unordered_map<unsigned __int64, CPlayer*>::iterator mapIter = _playersMap.find(sessionID);
	if (mapIter == _playersMap.end())
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		ReleaseSRWLockExclusive(&_playersLock);
		return;
	}
	CPlayer* pPlayer = mapIter->second;

	AcquireSRWLockExclusive(&pPlayer->_lock);

	while (pPlayer->_jobQ.GetUseSize() > 0)
	{
		CNetJob* job = pPlayer->_jobQ.Dequeue();
		_pJobPool->Free(job);
	}
	DWORD x = pPlayer->_sectorX;
	DWORD y = pPlayer->_sectorY;
	int idx = pPlayer->Terminate();

	ReleaseSRWLockExclusive(&pPlayer->_lock);

	_playersMap.erase(mapIter);
	_usablePlayerIdx.Push(idx);
	ReleaseSRWLockExclusive(&_playersLock);

	// Delete From Sector
	if (x < dfSECTOR_CNT_X && y < dfSECTOR_CNT_Y)
	{
		CSector* sector = &_sectors[y][x];
		AcquireSRWLockExclusive(&sector->_lock);
		vector<CPlayer*>::iterator vectorIter = sector->_players.begin();
		for (; vectorIter < sector->_players.end(); vectorIter++)
		{
			if ((*vectorIter) == pPlayer)
			{
				sector->_players.erase(vectorIter);
				break;
			}
		}
		ReleaseSRWLockExclusive(&sector->_lock);
	}
}

void CChattingServer::OnRecv(unsigned __int64 sessionID, CNetMsg* packet)
{
	CNetJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::CONTENT, SYS_TYPE::NONE, sessionID, packet);

	AcquireSRWLockShared(&_playersLock);
	unordered_map<unsigned __int64, CPlayer*>::iterator mapIter = _playersMap.find(sessionID);
	if (mapIter == _playersMap.end())
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		CNetMsg::Free(packet);
		ReleaseSRWLockShared(&_playersLock);
		return;
	}

	CPlayer* pPlayer = mapIter->second;
	pPlayer->_jobQ.Enqueue(job);
	InterlockedIncrement(&_jobQSize);
	int threadIdx = pPlayer->_idx._threadIdx;
	ReleaseSRWLockShared(&_playersLock);

	long ret = InterlockedIncrement(&_signals[threadIdx]);
	if (ret == 1) WakeByAddressSingle(&_signals[threadIdx]);
}

void CChattingServer::OnSend(unsigned __int64 sessionID, int sendSize)
{
}

void CChattingServer::HandleTimeout()
{
	AcquireSRWLockShared(&_playersLock);
	unordered_map<unsigned __int64, CPlayer*>::iterator iter = _playersMap.begin();
	for (; iter != _playersMap.end();)
	{
		CPlayer* player = (*iter).second;

		if ((timeGetTime() - player->_lastRecvTime) >= dfTIMEOUT)
		{
			Disconnect(player->_sessionID);

			ReleaseSRWLockShared(&_playersLock);
			AcquireSRWLockExclusive(&_playersLock);
			iter = _playersMap.erase(iter);
			ReleaseSRWLockExclusive(&_playersLock);
			AcquireSRWLockShared(&_playersLock);
		}
		else
		{
			iter++;
		}
	}
	ReleaseSRWLockShared(&_playersLock);
}

void CChattingServer::HandleRecv(CPlayer* pPlayer, CNetMsg* packet)
{
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
		// LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
		// ::wprintf(L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
		Disconnect(pPlayer->_sessionID);
		break;
	}
}

unsigned int __stdcall CChattingServer::UpdateThread(void* arg)
{
	ThreadArg* threadArg = (ThreadArg*)arg;
	CChattingServer* pServer = threadArg->_pServer;
	int idx = threadArg->_threadIdx;
	long undesired = 0;

	while (pServer->_serverAlive)
	{
		WaitOnAddress(&pServer->_signals[idx], &undesired, sizeof(long), INFINITE);
		InterlockedIncrement(&pServer->_updateCnt);

		for (int i = 0; i < pServer->_playerPerThread; i++)
		{
			CPlayer* pPlayer = &pServer->_players[idx][i];
			AcquireSRWLockExclusive(&pPlayer->_lock);

			while (pPlayer->_jobQ.GetUseSize() > 0)
			{
				CNetJob* job = pPlayer->_jobQ.Dequeue();
				if (job == nullptr) break;

				if (job->_sessionID == pPlayer->_sessionID)
				{
					try
					{
						pServer->HandleRecv(pPlayer, job->_packet);
					}
					catch (int packetError)
					{
						if (packetError == ERR_PACKET)
						{
							pServer->Disconnect(job->_sessionID);
							// LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
							// ::wprintf(L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
						}
					}
					CNetMsg::Free(job->_packet);
				}

				pServer->_pJobPool->Free(job);
				InterlockedDecrement(&pServer->_signals[idx]);
			}

			ReleaseSRWLockExclusive(&pPlayer->_lock);
		}
	}
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Update Thread (%d) Terminate\n", GetCurrentThreadId());
	::wprintf(L"Update Thread (%d) Terminate\n", GetCurrentThreadId());

	return 0;
}

unsigned int __stdcall CChattingServer::TimeoutThread(void* arg)
{
	CChattingServer* pServer = (CChattingServer*)arg;

	while (pServer->_serverAlive)
	{
		// CNetJob* job = pServer->_pJobPool->Alloc();
		// job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::TIMEOUT, 0, nullptr);	
		// pServer->EnqueueJob(0, job); // TO-DO
		// InterlockedIncrement(&pServer->_signal);
		// WakeByAddressSingle(&pServer->_signal);

		Sleep(dfTIMEOUT);
	}

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Timeout Thread (%d) Terminate\n", GetCurrentThreadId());
	::wprintf(L"Timeout Thread (%d) Terminate\n", GetCurrentThreadId());

	return 0;
}

void CChattingServer::ReqSendUnicast(CNetSendPacket* packet, __int64 sessionID)
{
	packet->AddUsageCount(1);
	if (!SendPacket(sessionID, packet))
	{
		CNetSendPacket::Free(packet);
	}
}


void CChattingServer::ReqSendAroundSector(CNetSendPacket* packet, CSector* centerSector, CPlayer* pExpPlayer)
{
	vector<__int64> sendID;

	if (pExpPlayer == nullptr)
	{
		vector<CSector*>::iterator iter = centerSector->_around.begin();
		for (; iter < centerSector->_around.end(); iter++)
		{
			CSector* sector = (*iter);
			AcquireSRWLockShared(&sector->_lock);
			vector<CPlayer*>::iterator playerIter = sector->_players.begin();
			for (; playerIter < sector->_players.end(); playerIter++)
			{
				sendID.push_back((*playerIter)->_sessionID);
			}
			ReleaseSRWLockShared(&sector->_lock);
		}

		AcquireSRWLockShared(&centerSector->_lock);
		vector<CPlayer*>::iterator playerIter = centerSector->_players.begin();
		for (; playerIter < centerSector->_players.end(); playerIter++)
		{
			sendID.push_back((*playerIter)->_sessionID);
		}
		ReleaseSRWLockShared(&centerSector->_lock);
	}
	else
	{
		vector<CSector*>::iterator iter = centerSector->_around.begin();
		for (; iter < centerSector->_around.end(); iter++)
		{
			CSector* sector = (*iter);
			AcquireSRWLockShared(&sector->_lock);
			vector<CPlayer*>::iterator playerIter = sector->_players.begin();
			for (; playerIter < sector->_players.end(); playerIter++)
			{
				sendID.push_back((*playerIter)->_sessionID);
			}
			ReleaseSRWLockShared(&sector->_lock);
		}

		AcquireSRWLockShared(&centerSector->_lock);
		vector<CPlayer*>::iterator playerIter = centerSector->_players.begin();
		for (; playerIter < centerSector->_players.end(); playerIter++)
		{
			if (*playerIter != pExpPlayer)
			{
				sendID.push_back((*playerIter)->_sessionID);
			}
		}
		ReleaseSRWLockShared(&centerSector->_lock);
	}

	packet->AddUsageCount(sendID.size());
	vector<__int64>::iterator idIter = sendID.begin();
	for (; idIter != sendID.end(); idIter++)
	{
		if (!SendPacket(*idIter, packet))
		{
			CNetSendPacket::Free(packet);
		}
	}
}

inline void CChattingServer::HandleCSPacket_REQ_LOGIN(CNetMsg* CSpacket, CPlayer* player)
{
	__int64 accountNo;
	ID* id = g_IDPool.Alloc();
	Nickname* nickname = g_NicknamePool.Alloc();
	SessionKey* key = g_SessionKey.Alloc();
	GetCSPacket_REQ_LOGIN(CSpacket, accountNo, id->_id, nickname->_nickname, key->_key);

	if (accountNo < 0)
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Account No is Wrong: %lld\n", _T(__FUNCTION__), __LINE__, accountNo);
		::wprintf(L"%s[%d] Account No is Wrong: %lld\n", _T(__FUNCTION__), __LINE__, accountNo);
		Disconnect(player->_sessionID);
		return;
	}

	// ::printf("%d: Login (%016llx)\n", GetCurrentThreadId(), player->_sessionID);

	BYTE status = 1;
	player->_accountNo = accountNo;

	g_IDPool.Free(player->_id);
	g_NicknamePool.Free(player->_nickname);
	g_SessionKey.Free(player->_key);

	player->_id = id;
	player->_nickname = nickname;
	player->_key = key;

	CNetSendPacket* SCpacket = CNetSendPacket::Alloc();
	SetSCNetPacket_RES_LOGIN(SCpacket, status, player->_accountNo);
	ReqSendUnicast(SCpacket, player->_sessionID);
}

inline void CChattingServer::HandleCSPacket_REQ_SECTOR_MOVE(CNetMsg* CSpacket, CPlayer* player)
{
	__int64 accountNo;
	WORD sectorX;
	WORD sectorY;
	GetCSPacket_REQ_SECTOR_MOVE(CSpacket, accountNo, sectorX, sectorY);

	if (player->_accountNo != accountNo)
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		::wprintf(L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		Disconnect(player->_sessionID);
		return;
	}

	if (sectorX >= dfSECTOR_CNT_X || sectorY >= dfSECTOR_CNT_Y)
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Sector Val is Wrong\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d] Sector Val is Wrong\n", _T(__FUNCTION__), __LINE__);
		Disconnect(player->_sessionID);
		return;
	}

	// ::printf("%d: Sector Move [%d][%d]->[%d][%d] (%016llx)\n", GetCurrentThreadId(), player->_sectorY, player->_sectorX, sectorY, sectorX, player->_sessionID);

	if (player->_sectorX < dfSECTOR_CNT_X && player->_sectorY < dfSECTOR_CNT_Y)
	{
		CSector* sector = &_sectors[player->_sectorY][player->_sectorX];
		AcquireSRWLockExclusive(&sector->_lock);
		vector<CPlayer*>::iterator iter = sector->_players.begin();
		for (; iter < sector->_players.end(); iter++)
		{
			if ((*iter) == player)
			{
				sector->_players.erase(iter);
				break;
			}
		}
		ReleaseSRWLockExclusive(&sector->_lock);
	}

	player->_sectorX = sectorX;
	player->_sectorY = sectorY;

	AcquireSRWLockExclusive(&_sectors[player->_sectorY][player->_sectorX]._lock);
	_sectors[player->_sectorY][player->_sectorX]._players.push_back(player);
	ReleaseSRWLockExclusive(&_sectors[player->_sectorY][player->_sectorX]._lock);

	CNetSendPacket* SCpacket = CNetSendPacket::Alloc();
	SetSCNetPacket_RES_SECTOR_MOVE(SCpacket, player->_accountNo, player->_sectorX, player->_sectorY);
	ReqSendUnicast(SCpacket, player->_sessionID);
}

inline void CChattingServer::HandleCSPacket_REQ_MESSAGE(CNetMsg* CSpacket, CPlayer* player)
{
	__int64 accountNo;
	WORD messageLen;
	wchar_t message[dfMSG_MAX];
	GetCSPacket_REQ_MESSAGE(CSpacket, accountNo, messageLen, message);

	if (player->_accountNo != accountNo)
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		::wprintf(L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		Disconnect(player->_sessionID);
		return;
	}

	// ::printf("%d: Message (%016llx)\n", GetCurrentThreadId(), player->_sessionID);

	CNetSendPacket* SCpacket = CNetSendPacket::Alloc();
	SetSCNetPacket_RES_MESSAGE(SCpacket, player->_accountNo, player->_id->_id, player->_nickname->_nickname, messageLen, message);

	if (player->_sectorX < dfSECTOR_CNT_X && player->_sectorY < dfSECTOR_CNT_Y)
	{
		ReqSendAroundSector(SCpacket, &_sectors[player->_sectorY][player->_sectorX]);
	}
}

inline void CChattingServer::HandleCSPacket_REQ_HEARTBEAT(CPlayer* player)
{
	player->_lastRecvTime = timeGetTime();
}

inline void CChattingServer::GetCSPacket_REQ_LOGIN(CNetMsg* packet, __int64& accountNo, wchar_t ID[dfID_LEN], wchar_t nickname[dfNICKNAME_LEN], char sessionKey[dfSESSIONKEY_LEN])
{
	*packet >> accountNo;
	packet->GetPayloadData((char*)ID, dfID_LEN * sizeof(wchar_t));
	packet->GetPayloadData((char*)nickname, dfNICKNAME_LEN * sizeof(wchar_t));
	packet->GetPayloadData((char*)sessionKey, dfSESSIONKEY_LEN);
}

inline void CChattingServer::GetCSPacket_REQ_SECTOR_MOVE(CNetMsg* packet, __int64& accountNo, WORD& sectorX, WORD& sectorY)
{
	*packet >> accountNo;
	*packet >> sectorX;
	*packet >> sectorY;
}

inline void CChattingServer::GetCSPacket_REQ_MESSAGE(CNetMsg* packet, __int64& accountNo, WORD& messageLen, wchar_t message[dfMSG_MAX])
{
	*packet >> accountNo;
	*packet >> messageLen;
	packet->GetPayloadData((char*)message, messageLen);
}

inline void CChattingServer::SetSCNetPacket_RES_LOGIN(CNetSendPacket* packet, BYTE status, __int64 accountNo)
{
	WORD type = en_PACKET_CS_CHAT_RES_LOGIN;
	*packet << type;
	*packet << status;
	*packet << accountNo;
}

inline void CChattingServer::SetSCNetPacket_RES_SECTOR_MOVE(CNetSendPacket* packet, __int64 accountNo, WORD sectorX, WORD sectorY)
{
	WORD type = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
	*packet << type;
	*packet << accountNo;
	*packet << sectorX;
	*packet << sectorY;
}

inline void CChattingServer::SetSCNetPacket_RES_MESSAGE(CNetSendPacket* packet, __int64 accountNo, wchar_t* ID, wchar_t* nickname, WORD messageLen, wchar_t* message)
{
	WORD type = en_PACKET_CS_CHAT_RES_MESSAGE;
	*packet << type;
	*packet << accountNo;
	packet->PutPayloadData((char*)ID, dfID_LEN * sizeof(wchar_t));
	packet->PutPayloadData((char*)nickname, dfNICKNAME_LEN * sizeof(wchar_t));
	*packet << messageLen;
	packet->PutPayloadData((char*)message, messageLen);
}
