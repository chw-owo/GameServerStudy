#include "CChattingServer.h"
#include "CSystemLog.h"

#include <tchar.h>
#include <wchar.h>

// #define _TIMEOUT

bool CChattingServer::Initialize()
{
	// Set Resource

	InitializeSRWLock(&_JobLock);
	_pJobPool = new CObjectPool<CNetJob>(dfJOB_DEF, true);
	_pPlayerPool = new CObjectPool<CPlayer>(dfPLAYER_MAX, false);

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
	_updateThread = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, this, 0, nullptr);
	if (_updateThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Logic Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Logic Thread Error\n", _T(__FUNCTION__), __LINE__);
		return false;
	}

	// Set Network Library
	int totalThreadCnt = 0;
	int runningThreadCnt = 0;
	::wprintf(L"<Chatting Server>\n");
	::wprintf(L"Total Network Thread Count: ");
	::scanf_s("%d", &totalThreadCnt);
	::wprintf(L"Running Network Thread Count: ");
	::scanf_s("%d", &runningThreadCnt);

	if (!NetworkInitialize(dfSERVER_IP, dfSERVER_PORT, dfSEND_TIME, totalThreadCnt, runningThreadCnt, false, true))
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
	NetworkTerminate();
	_serverAlive = false;

	CNetJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::TERMINATE, 0, nullptr);
	// EnqueueJob(0, job); // TO-DO

	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
	WaitForSingleObject(_updateThread, INFINITE);

#ifdef _TIMEOUT
	WaitForSingleObject(_timeoutThread, INFINITE);
#endif

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Chat: Server Terminate.\n");
	::wprintf(L"Chat: Server Terminate.\n");
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
	CNetJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::ACCEPT, sessionID, nullptr);

	PRO_BEGIN("Job EnQ");
	AcquireSRWLockExclusive(&_JobLock);
	_JobQueue.push(job);
	ReleaseSRWLockExclusive(&_JobLock);
	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
	PRO_END("Job EnQ");
}

void CChattingServer::OnReleaseClient(unsigned __int64 sessionID)
{
	CNetJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::RELEASE, sessionID, nullptr);

	PRO_BEGIN("Job EnQ");
	AcquireSRWLockExclusive(&_JobLock);
	_JobQueue.push(job);
	ReleaseSRWLockExclusive(&_JobLock);
	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
	PRO_END("Job EnQ");
}

void CChattingServer::OnRecv(unsigned __int64 sessionID, CNetMsg* packet)
{
	CNetJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::CONTENT, SYS_TYPE::NONE, sessionID, packet);

	PRO_BEGIN("Job EnQ");
	AcquireSRWLockExclusive(&_JobLock);
	_JobQueue.push(job);
	ReleaseSRWLockExclusive(&_JobLock);
	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
	PRO_END("Job EnQ");
}

void CChattingServer::OnSend(unsigned __int64 sessionID, int sendSize)
{
}

void CChattingServer::HandleTimeout()
{
	unordered_map<unsigned __int64, CPlayer*>::iterator iter = _playersMap.begin();
	for (; iter != _playersMap.end();)
	{
		CPlayer* player = (*iter).second;

		if ((timeGetTime() - player->_lastRecvTime) >= dfTIMEOUT)
		{
			Disconnect(player->_sessionID);
			iter = _playersMap.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

void CChattingServer::HandleAccept(unsigned __int64 sessionID)
{
	if (_playersMap.size() >= dfPLAYER_MAX)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d] Player Max\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d] player max\n", _T(__FUNCTION__), __LINE__);
		Disconnect(sessionID);
		return;
	}

	CPlayer* pPlayer = _pPlayerPool->Alloc();
	pPlayer->Initailize(sessionID, _playerIDGenerator++);
	auto ret = _playersMap.insert(make_pair(sessionID, pPlayer));
	if (ret.second == false)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d] Already Exist SessionID: %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d] Already Exist SessionID: %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
	}

	// ::printf("%lld (%d): Handle Accept\n", (_sessionID & _idMask), GetCurrentThreadId());

	return;
}

void CChattingServer::HandleRelease(unsigned __int64 sessionID)
{
	// Delete From SessionID-Player Map
	unordered_map<unsigned __int64, CPlayer*>::iterator mapIter = _playersMap.find(sessionID);
	if (mapIter == _playersMap.end())
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		return;
	}
	CPlayer* pPlayer = mapIter->second;
	_playersMap.erase(mapIter);

	// Delete From Sector
	if (pPlayer->_sectorX < dfSECTOR_CNT_X && pPlayer->_sectorY < dfSECTOR_CNT_Y)
	{
		CSector* sector = &_sectors[pPlayer->_sectorY][pPlayer->_sectorX];
		vector<CPlayer*>::iterator vectorIter = sector->_players.begin();
		for (; vectorIter < sector->_players.end(); vectorIter++)
		{
			if ((*vectorIter) == pPlayer)
			{
				sector->_players.erase(vectorIter);
				break;
			}
		}
	}

	// Delete Player
	pPlayer->Terminate();
	_pPlayerPool->Free(pPlayer);

	// ::printf("%lld (%d): Handle Release\n", (_sessionID & _idMask), GetCurrentThreadId());
}


void CChattingServer::HandleRecv(unsigned __int64 sessionID, CNetMsg* packet)
{
	unordered_map<unsigned __int64, CPlayer*>::iterator iter = _playersMap.find(sessionID);

	if (iter == _playersMap.end())
	{
		Disconnect(sessionID);
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: No Session\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: No Session\n", _T(__FUNCTION__), __LINE__);
	}
	else
	{
		CPlayer* pPlayer = iter->second;
		try
		{
			pPlayer->_lastRecvTime = timeGetTime();
			short msgType = -1;
			*packet >> msgType;

			// ::printf("read %d, write %d, use %d\n", packet->_readPos, packet->_writePos, packet->_packet->_usageCount);

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
				Disconnect(pPlayer->_sessionID);
				LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
				::wprintf(L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
				break;
			}
		}
		catch (int packetError)
		{
			if (packetError == ERR_PACKET)
			{
				Disconnect(pPlayer->_sessionID);
				LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
				::wprintf(L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
			}
		}
	}

	CNetMsg::Free(packet);
}

unsigned int __stdcall CChattingServer::UpdateThread(void* arg)
{
	CChattingServer* pServer = (CChattingServer*)arg;
	queue<CNetJob*>* pJobQueue = &pServer->_JobQueue;
	long undesired = 0;

	while (pServer->_serverAlive)
	{
		WaitOnAddress(&pServer->_signal, &undesired, sizeof(long), INFINITE);

		while (pJobQueue->size() > 0)
		{
			PRO_BEGIN("Job DeQ");
			AcquireSRWLockExclusive(&pServer->_JobLock);
			CNetJob* job = pJobQueue->front();
			pJobQueue->pop();
			ReleaseSRWLockExclusive(&pServer->_JobLock);
			PRO_END("Job DeQ");
			
			pServer->_updateCnt++;

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
				else if (job->_sysType == SYS_TYPE::TIMEOUT)
				{
					pServer->HandleTimeout();
				}
				else if (job->_sysType == SYS_TYPE::TERMINATE)
				{
					break;
				}
			}
			else if (job->_type == JOB_TYPE::CONTENT)
			{
				pServer->HandleRecv(job->_sessionID, job->_packet);
			}

			pServer->_pJobPool->Free(job);
			InterlockedDecrement(&pServer->_signal);
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
		CNetJob* job = pServer->_pJobPool->Alloc();
		job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::TIMEOUT, 0, nullptr);
		// pServer->EnqueueJob(0, job); // TO-DO

		InterlockedIncrement(&pServer->_signal);
		WakeByAddressSingle(&pServer->_signal);

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
			vector<CPlayer*>::iterator playerIter = (*iter)->_players.begin();
			for (; playerIter < (*iter)->_players.end(); playerIter++)
			{
				sendID.push_back((*playerIter)->_sessionID);
			}
		}

		vector<CPlayer*>::iterator playerIter = centerSector->_players.begin();
		for (; playerIter < centerSector->_players.end(); playerIter++)
		{
			sendID.push_back((*playerIter)->_sessionID);
		}
	}
	else
	{
		vector<CSector*>::iterator iter = centerSector->_around.begin();
		for (; iter < centerSector->_around.end(); iter++)
		{
			vector<CPlayer*>::iterator playerIter = (*iter)->_players.begin();
			for (; playerIter < (*iter)->_players.end(); playerIter++)
			{
				sendID.push_back((*playerIter)->_sessionID);
			}
		}

		vector<CPlayer*>::iterator playerIter = centerSector->_players.begin();
		for (; playerIter < centerSector->_players.end(); playerIter++)
		{
			if (*playerIter != pExpPlayer)
			{
				sendID.push_back((*playerIter)->_sessionID);
			}
		}
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
		vector<CPlayer*>::iterator iter = sector->_players.begin();
		for (; iter < sector->_players.end(); iter++)
		{
			if ((*iter) == player)
			{
				sector->_players.erase(iter);
				break;
			}
		}
	}

	player->_sectorX = sectorX;
	player->_sectorY = sectorY;
	_sectors[player->_sectorY][player->_sectorX]._players.push_back(player);

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
