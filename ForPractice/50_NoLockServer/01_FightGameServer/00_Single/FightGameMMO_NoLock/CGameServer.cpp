#include "CGameServer.h"
#include "CSystemLog.h"
#include <tchar.h>

#define _MONITOR

bool CGameServer::Initialize()
{
	_pJobQueue = new CLockFreeQueue<CJob*>;
	_pJobPool = new CLockFreePool<CJob>(0, false);
	_playerPool = new CObjectPool<CPlayer>(0, true);

	int sectorIdx = 0;
	for (int y = 0; y < dfSECTOR_CNT_Y; y++)
	{
		for (int x = 0; x < dfSECTOR_CNT_X; x++)
		{
			_sectors[y][x].InitializeSector(sectorIdx++, x, y);
		}
	}
	SetSectorsAroundInfo();
	_oldTick = timeGetTime();
	srand(0);

	_updateThread = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, this, 0, nullptr);
	if (_updateThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Update Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Update Thread Error\n", _T(__FUNCTION__), __LINE__);
		return false;
	}

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int threadCnt = (int)si.dwNumberOfProcessors / 2;

	if (!NetworkInitialize(dfSERVER_IP, dfSERVER_PORT, threadCnt, false))
		Terminate();

#ifdef _MONITOR
	_monitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, nullptr);
	if (_monitorThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
#endif

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"GameServer Initialize\n");
	::wprintf(L"GameServer Initialize\n\n");
}

void CGameServer::Terminate()
{
	NetworkTerminate();
	_serverAlive = false;
	WaitForSingleObject(_updateThread, INFINITE);

#ifdef _MONITOR
	WaitForSingleObject(_monitorThread, INFINITE);
#endif

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"GameServer Terminate.\n");
	::wprintf(L"GameServer Terminate.\n");
}

void CGameServer::OnInitialize()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Initialize\n");
	::wprintf(L"Network Initialize\n");
}

void CGameServer::OnTerminate()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Terminate\n");
	::wprintf(L"Network Terminate\n");
}

void CGameServer::OnThreadTerminate(wchar_t* threadName)
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s Terminate\n", threadName);
	::wprintf(L"%s Terminate\n", threadName);
}

void CGameServer::OnError(int errorCode, wchar_t* errorMsg)
{
	LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s (%d)\n", errorMsg, errorCode);
	::wprintf(L"%s (%d)\n", errorMsg, errorCode);
}


bool CGameServer::OnConnectRequest()
{
	return true;
}

void CGameServer::OnAcceptClient(__int64 sessionID)
{
	CJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::ACCEPT, sessionID, nullptr);
	_pJobQueue->Enqueue(job);
}

void CGameServer::OnReleaseClient(__int64 sessionID)
{
	CJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::RELEASE, sessionID, nullptr);
	_pJobQueue->Enqueue(job);
}

void CGameServer::OnRecv(__int64 sessionID, CPacket* packet)
{
	packet->AddUsageCount(1);

	CJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::CONTENT, SYS_TYPE::NONE, sessionID, packet);
	_pJobQueue->Enqueue(job);
}

void CGameServer::OnSend(__int64 sessionID, int sendSize)
{
	
}

unsigned int __stdcall CGameServer::UpdateThread(void* arg)
{
	CGameServer* pServer = (CGameServer*)arg;

	while (pServer->_serverAlive)
	{
		pServer->HandleNetwork();
		pServer->LogicUpdate();
	}

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Update Thread Terminate.\n");
	::wprintf(L"Update Thread Terminate.\n");

	return 0;
}

unsigned int __stdcall CGameServer::MonitorThread(void* arg)
{
	CGameServer* pServer = (CGameServer*)arg;
	CreateDirectory(L"MonitorLog", NULL);
	long totalSyncCnt = 0;
	Sleep(1000);

	while (pServer->_serverAlive)
	{
		pServer->UpdateMonitorData();
		totalSyncCnt += pServer->_syncCnt;

		SYSTEMTIME stTime;
		GetLocalTime(&stTime);

		WCHAR text[dfMONITOR_TEXT_LEN];
		swprintf_s(text, dfMONITOR_TEXT_LEN,
			L"[%s %02d:%02d:%02d]\n\nLogic FPS: %d\nTotal Accept: %d\nTotal Release: %d\nConnected Session: %d\nTotal Sync: %d\nRelease/1sec: %d\nTimeout/1sec: %d\nDisconnect/1sec: %d\n\n",
			_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond, pServer->_logicFPS, pServer->GetAcceptTotal(), pServer->GetReleaseTotal(),
			pServer->GetSessionCount(), totalSyncCnt, pServer->GetReleaseTPS(), pServer->_timeoutCnt, pServer->GetDisconnectTPS());
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

		InterlockedExchange(&pServer->_logicFPS, 0);
		InterlockedExchange(&pServer->_syncCnt, 0);
		InterlockedExchange(&pServer->_timeoutCnt, 0);

		Sleep(1000);
	}

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Monitor Thread Terminate.\n");
	::wprintf(L"Monitor Thread Terminate.\n");

	return 0;
}

inline void CGameServer::HandleAccept(__int64 sessionID)
{
	if (_playersMap.size() > dfPLAYER_MAX)
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] player max\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d] player max\n", _T(__FUNCTION__), __LINE__);
		Disconnect(sessionID);
		return;
	}

	__int64 playerID = _playerIDGenerator++;
	CPlayer* pPlayer = _playerPool->Alloc(sessionID, playerID);
	_playersMap.insert(make_pair(sessionID, pPlayer));

	int sectorX = (pPlayer->_x / dfSECTOR_SIZE_X) + 2;
	int sectorY = (pPlayer->_y / dfSECTOR_SIZE_Y) + 2;
	pPlayer->_pSector = &_sectors[sectorY][sectorX];
	_sectors[sectorY][sectorX]._players.push_back(pPlayer);

	//===========================================================================

	CSector* pSector = pPlayer->_pSector;

	CPacket* createMePacket = CPacket::Alloc();
	createMePacket->Clear();
	int createMeRet = SetSCPacket_CREATE_MY_CHAR(createMePacket, playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	ReqSendUnicast(createMePacket, sessionID);

	CPacket* createMeToOtherPacket = CPacket::Alloc();
	createMeToOtherPacket->Clear();
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createMeToOtherPacket, playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	ReqSendAroundSector(createMeToOtherPacket, pSector, pPlayer);

	for (int i = 0; i < dfMOVE_DIR_MAX; i++)
	{
		vector<CPlayer*>::iterator iter = pSector->_around[i]->_players.begin();
		for (; iter < pSector->_around[i]->_players.end(); iter++)
		{
			CPacket* createOtherPacket = CPacket::Alloc();
			createOtherPacket->Clear();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(
				createOtherPacket, (*iter)->_playerID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			ReqSendUnicast(createOtherPacket, sessionID);
		}
	}

	vector<CPlayer*>::iterator iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
	for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
	{
		if ((*iter) != pPlayer)
		{
			CPacket* createOtherPacket = CPacket::Alloc();
			createOtherPacket->Clear();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(
				createOtherPacket, (*iter)->_playerID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			ReqSendUnicast(createOtherPacket, sessionID);
		}
	}
}

inline void CGameServer::HandleRelease(__int64 sessionID)
{
	unordered_map<__int64, CPlayer*>::iterator iter = _playersMap.find(sessionID);
	if (iter == _playersMap.end())
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: No Session\n", _T(__FUNCTION__), __LINE__);
		return;
	}
	CPlayer* pPlayer = iter->second;
	_playersMap.erase(iter);

	CSector* pSector = pPlayer->_pSector;
	vector<CPlayer*>::iterator vectorIter = pSector->_players.begin();
	for (; vectorIter < pSector->_players.end(); vectorIter++)
	{
		if (*vectorIter == pPlayer)
		{
			pSector->_players.erase(vectorIter);
			break;
		}
	}

	CPacket* deletePacket = CPacket::Alloc();
	deletePacket->Clear();
	int deleteRet = SetSCPacket_DELETE_CHAR(deletePacket, pPlayer->_playerID);
	ReqSendAroundSector(deletePacket, pSector);

	_playerPool->Free(pPlayer);
}

inline void CGameServer::HandleRecv(__int64 sessionID, CPacket* packet)
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
		case dfPACKET_CS_MOVE_START:
			HandleCSPacket_MOVE_START(packet, pPlayer);
			break;

		case dfPACKET_CS_MOVE_STOP:
			HandleCSPacket_MOVE_STOP(packet, pPlayer);
			break;

		case dfPACKET_CS_ATTACK1:
			HandleCSPacket_ATTACK1(packet, pPlayer);
			break;

		case dfPACKET_CS_ATTACK2:
			HandleCSPacket_ATTACK2(packet, pPlayer);
			break;

		case dfPACKET_CS_ATTACK3:
			HandleCSPacket_ATTACK3(packet, pPlayer);
			break;

		case dfPACKET_CS_ECHO:
			HandleCSPacket_ECHO(packet, pPlayer);
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
		{
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
			::wprintf(L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
			Disconnect(sessionID);
		}
	}
}

bool CGameServer::SkipForFixedFrame(DWORD time)
{
	if ((time - _oldTick) < _timeGap) return true;
	_oldTick += _timeGap;
	return false;
}

void CGameServer::LogicUpdate()
{
	DWORD time = timeGetTime();
	if (SkipForFixedFrame(time)) return;
	InterlockedIncrement(&_logicFPS);

	unordered_map<__int64, CPlayer*>::iterator iter = _playersMap.begin();
	for (; iter != _playersMap.end(); iter++)
	{
		CPlayer* pPlayer = iter->second;
		if (!pPlayer->_alive) continue;

		if ((time - pPlayer->_lastRecvTime) > dfNETWORK_PACKET_RECV_TIMEOUT)
		{
			SetPlayerDead(pPlayer);
			InterlockedIncrement(&_timeoutCnt);
			continue;
		}

		if (pPlayer->_move)
		{
			UpdatePlayerMove(pPlayer);
		}
	}
}

void CGameServer::HandleNetwork()
{
	if(_pJobQueue->GetUseSize() > 0)
	{
		CJob* job = _pJobQueue->Dequeue();

		while (job != nullptr)
		{
			if (job->_type == JOB_TYPE::SYSTEM)
			{
				if (job->_sysType == SYS_TYPE::ACCEPT)
				{
					HandleAccept(job->_sessionID);
				}
				else if (job->_sysType == SYS_TYPE::RELEASE)
				{
					HandleRelease(job->_sessionID);
				}
			}
			else if (job->_type == JOB_TYPE::CONTENT)
			{
				HandleRecv(job->_sessionID, job->_packet);
				CPacket::Free(job->_packet);
			}

			_pJobPool->Free(job);
			job = _pJobQueue->Dequeue();
		}
	}
}

void CGameServer::ReqSendUnicast(CPacket* packet, __int64 sessionID)
{
	packet->AddUsageCount(1);
	if (!SendPacket(sessionID, packet))
	{
		CPacket::Free(packet);
	}
}

void CGameServer::ReqSendSectors(CPacket* packet, CSector** sector, int sectorCnt, CPlayer* pExpPlayer)
{
	vector<__int64> sendID;

	if (pExpPlayer == nullptr)
	{
		for (int i = 0; i < sectorCnt; i++)
		{
			vector<CPlayer*>::iterator playerIter = sector[i]->_players.begin();
			for (; playerIter < sector[i]->_players.end(); playerIter++)
			{		
				sendID.push_back((*playerIter)->_sessionID);
			}
		}
	}
	else
	{
		for (int i = 0; i < sectorCnt; i++)
		{
			vector<CPlayer*>::iterator playerIter = sector[i]->_players.begin();
			for (; playerIter < sector[i]->_players.end(); playerIter++)
			{
				if (*playerIter != pExpPlayer)
				{
					sendID.push_back((*playerIter)->_sessionID);
				}
			}
		}
	}

	packet->AddUsageCount(sendID.size());
	vector<__int64>::iterator idIter = sendID.begin();
	for (; idIter != sendID.end(); idIter++)
	{
		if (!SendPacket(*idIter, packet))
		{
			CPacket::Free(packet);
		}
	}
}

void CGameServer::ReqSendAroundSector(CPacket* packet, CSector* centerSector, CPlayer* pExpPlayer)
{
	vector<__int64> sendID;

	if (pExpPlayer == nullptr)
	{
		for (int i = 0; i < dfAROUND_SECTOR_NUM; i++)
		{
			vector<CPlayer*>::iterator playerIter = centerSector->_around[i]->_players.begin();
			for (; playerIter < centerSector->_around[i]->_players.end(); playerIter++)
			{
				sendID.push_back((*playerIter)->_sessionID);
			}
		}
	}
	else
	{
		for (int i = 0; i < dfMOVE_DIR_MAX; i++)
		{
			vector<CPlayer*>::iterator playerIter = centerSector->_around[i]->_players.begin();
			for (; playerIter < centerSector->_around[i]->_players.end(); playerIter++)
			{
				sendID.push_back((*playerIter)->_sessionID);
			}
		}

		vector<CPlayer*>::iterator playerIter = centerSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; playerIter < centerSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); playerIter++)
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
			CPacket::Free(packet);
		}
	}
}

bool CGameServer::CheckMovable(short x, short y)
{
	if (x < dfRANGE_MOVE_LEFT || x > dfRANGE_MOVE_RIGHT ||
		y < dfRANGE_MOVE_TOP || y > dfRANGE_MOVE_BOTTOM)
		return false;
	return true;
}

void CGameServer::UpdatePlayerMove(CPlayer* pPlayer)
{
	CSector* pSector = pPlayer->_pSector;

	switch (pPlayer->_moveDirection)
	{
	case dfMOVE_DIR_LL:

		if (CheckMovable(pPlayer->_x - dfSPEED_PLAYER_X, pPlayer->_y))
		{
			pPlayer->_x -= dfSPEED_PLAYER_X;
			if (pPlayer->_x < pSector->_xPosMin)
				UpdateSector(pPlayer, dfMOVE_DIR_LL);
		}
		break;

	case dfMOVE_DIR_LU:

		if (CheckMovable(pPlayer->_x - dfSPEED_PLAYER_X, pPlayer->_y - dfSPEED_PLAYER_Y))
		{
			pPlayer->_x -= dfSPEED_PLAYER_X;
			pPlayer->_y -= dfSPEED_PLAYER_Y;

			if (pPlayer->_x < pSector->_xPosMin && pPlayer->_y < pSector->_yPosMin)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_LU);
			}
			else if (pPlayer->_x < pSector->_xPosMin)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_LL);
			}
			else if (pPlayer->_y < pSector->_yPosMin)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_UU);
			}
		}
		break;

	case dfMOVE_DIR_UU:

		if (CheckMovable(pPlayer->_x, pPlayer->_y - dfSPEED_PLAYER_Y))
		{
			pPlayer->_y -= dfSPEED_PLAYER_Y;
			if (pPlayer->_y < pSector->_yPosMin)
				UpdateSector(pPlayer, dfMOVE_DIR_UU);
		}
		break;

	case dfMOVE_DIR_RU:

		if (CheckMovable(pPlayer->_x + dfSPEED_PLAYER_X, pPlayer->_y - dfSPEED_PLAYER_Y))
		{
			pPlayer->_x += dfSPEED_PLAYER_X;
			pPlayer->_y -= dfSPEED_PLAYER_Y;

			if (pPlayer->_x > pSector->_xPosMax && pPlayer->_y < pSector->_yPosMin)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_RU);
			}
			else if (pPlayer->_x > pSector->_xPosMax)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_RR);
			}
			else if (pPlayer->_y < pSector->_yPosMin)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_UU);
			}
		}
		break;

	case dfMOVE_DIR_RR:

		if (CheckMovable(pPlayer->_x + dfSPEED_PLAYER_X, pPlayer->_y))
		{
			pPlayer->_x += dfSPEED_PLAYER_X;
			if (pPlayer->_x > pSector->_xPosMax)
				UpdateSector(pPlayer, dfMOVE_DIR_RR);
		}
		break;

	case dfMOVE_DIR_RD:

		if (CheckMovable(pPlayer->_x + dfSPEED_PLAYER_X, pPlayer->_y + dfSPEED_PLAYER_Y))
		{
			pPlayer->_x += dfSPEED_PLAYER_X;
			pPlayer->_y += dfSPEED_PLAYER_Y;

			if (pPlayer->_x > pSector->_xPosMax && pPlayer->_y > pSector->_yPosMax)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_RD);
			}
			else if (pPlayer->_x > pSector->_xPosMax)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_RR);
			}
			else if (pPlayer->_y > pSector->_yPosMax)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_DD);
			}
		}
		break;

	case dfMOVE_DIR_DD:

		if (CheckMovable(pPlayer->_x, pPlayer->_y + dfSPEED_PLAYER_Y))
		{
			pPlayer->_y += dfSPEED_PLAYER_Y;
			if (pPlayer->_y > pSector->_yPosMax)
				UpdateSector(pPlayer, dfMOVE_DIR_DD);
		}
		break;

	case dfMOVE_DIR_LD:

		if (CheckMovable(pPlayer->_x - dfSPEED_PLAYER_X, pPlayer->_y + dfSPEED_PLAYER_Y))
		{
			pPlayer->_x -= dfSPEED_PLAYER_X;
			pPlayer->_y += dfSPEED_PLAYER_Y;

			if (pPlayer->_x < pSector->_xPosMin && pPlayer->_y > pSector->_yPosMax)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_LD);
			}
			else if (pPlayer->_x < pSector->_xPosMin)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_LL);
			}
			else if (pPlayer->_y > pSector->_yPosMax)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_DD);
			}
		}
		break;
	}
}

void CGameServer::SetPlayerDead(CPlayer* pPlayer)
{
	pPlayer->_alive = false;
	Disconnect(pPlayer->_sessionID);
}


void CGameServer::UpdateSector(CPlayer* pPlayer, short direction)
{
	// Get Around Sector Data =======================================

	int sectorCnt = _sectorCnt[direction];

	CSector* oriSector = pPlayer->_pSector;
	CSector** inSector = oriSector->_new[direction];
	CSector** outSector = oriSector->_old[direction];
	CSector* newSector = oriSector->_around[direction];

	// Send Data About My Player ==============================================

	CPacket* createMeToOtherPacket = CPacket::Alloc();
	createMeToOtherPacket->Clear();
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createMeToOtherPacket, pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	ReqSendSectors(createMeToOtherPacket, inSector, sectorCnt, pPlayer);

	CPacket* moveMeToOtherPacket = CPacket::Alloc();
	moveMeToOtherPacket->Clear();
	int moveMeToOtherRet = SetSCPacket_MOVE_START(moveMeToOtherPacket, pPlayer->_playerID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	ReqSendSectors(moveMeToOtherPacket, inSector, sectorCnt, pPlayer);

	CPacket* deleteMeToOtherPacket = CPacket::Alloc();
	deleteMeToOtherPacket->Clear();
	int deleteMeToOtherRet = SetSCPacket_DELETE_CHAR(deleteMeToOtherPacket, pPlayer->_playerID);
	ReqSendSectors(deleteMeToOtherPacket, outSector, sectorCnt, pPlayer);

	// Send Data About Other Player ==============================================

	for (int i = 0; i < sectorCnt; i++)
	{
		vector<CPlayer*>::iterator iter = inSector[i]->_players.begin();
		for (; iter < inSector[i]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				CPacket* createOtherPacket = CPacket::Alloc();
				createOtherPacket->Clear();
				int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createOtherPacket, (*iter)->_playerID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
				ReqSendUnicast(createOtherPacket, pPlayer->_sessionID);

				if ((*iter)->_move)
				{
					CPacket* moveOtherPacket = CPacket::Alloc();
					moveOtherPacket->Clear();
					int moveOtherRet = SetSCPacket_MOVE_START(moveOtherPacket, (*iter)->_playerID, (*iter)->_direction, (*iter)->_x, (*iter)->_y);
					ReqSendUnicast(moveOtherPacket, pPlayer->_sessionID);
				}
			}
		}

		iter = outSector[i]->_players.begin();
		for (; iter < outSector[i]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				CPacket* deleteOtherPacket = CPacket::Alloc();
				deleteOtherPacket->Clear();
				int deleteOtherRet = SetSCPacket_DELETE_CHAR(deleteOtherPacket, (*iter)->_playerID);
				ReqSendUnicast(deleteOtherPacket, pPlayer->_sessionID);
			}
		}
	}

	vector<CPlayer*>::iterator iter = oriSector->_players.begin();
	for (; iter != oriSector->_players.end(); iter++)
	{
		if (pPlayer == (*iter))
		{
			oriSector->_players.erase(iter);
			break;
		}
	}

	newSector->_players.push_back(pPlayer);
	pPlayer->_pSector = newSector;

}

void CGameServer::SetSectorsAroundInfo()
{
	for (int y = 2; y < dfSECTOR_CNT_Y - 2; y++)
	{
		for (int x = 2; x < dfSECTOR_CNT_X - 2; x++)
		{
			CSector* pSector = &_sectors[y][x];

			pSector->_around[dfMOVE_DIR_LL] = &_sectors[y][x - 1];
			pSector->_around[dfMOVE_DIR_LU] = &_sectors[y - 1][x - 1];
			pSector->_around[dfMOVE_DIR_UU] = &_sectors[y - 1][x];
			pSector->_around[dfMOVE_DIR_RU] = &_sectors[y - 1][x + 1];
			pSector->_around[dfMOVE_DIR_RR] = &_sectors[y][x + 1];
			pSector->_around[dfMOVE_DIR_RD] = &_sectors[y + 1][x + 1];
			pSector->_around[dfMOVE_DIR_DD] = &_sectors[y + 1][x];
			pSector->_around[dfMOVE_DIR_LD] = &_sectors[y + 1][x - 1];
			pSector->_around[dfMOVE_DIR_INPLACE] = &_sectors[y][x];

			// dfMOVE_DIR_LL

			pSector->_llNew[0] = &_sectors[y - 1][x - 2];
			pSector->_llNew[1] = &_sectors[y][x - 2];
			pSector->_llNew[2] = &_sectors[y + 1][x - 2];

			pSector->_llOld[0] = &_sectors[y - 1][x + 1];
			pSector->_llOld[1] = &_sectors[y][x + 1];
			pSector->_llOld[2] = &_sectors[y + 1][x + 1];

			// dfMOVE_DIR_LU

			pSector->_luNew[0] = &_sectors[y - 2][x];
			pSector->_luNew[1] = &_sectors[y - 2][x - 1];
			pSector->_luNew[2] = &_sectors[y - 2][x - 2];
			pSector->_luNew[3] = &_sectors[y - 1][x - 2];
			pSector->_luNew[4] = &_sectors[y][x - 2];

			pSector->_luOld[0] = &_sectors[y + 1][x];
			pSector->_luOld[1] = &_sectors[y + 1][x - 1];
			pSector->_luOld[2] = &_sectors[y + 1][x + 1];
			pSector->_luOld[3] = &_sectors[y - 1][x + 1];
			pSector->_luOld[4] = &_sectors[y][x + 1];

			// dfMOVE_DIR_UU

			pSector->_uuNew[0] = &_sectors[y - 2][x - 1];
			pSector->_uuNew[1] = &_sectors[y - 2][x];
			pSector->_uuNew[2] = &_sectors[y - 2][x + 1];

			pSector->_uuOld[0] = &_sectors[y + 1][x - 1];
			pSector->_uuOld[1] = &_sectors[y + 1][x];
			pSector->_uuOld[2] = &_sectors[y + 1][x + 1];

			// dfMOVE_DIR_RU

			pSector->_ruNew[0] = &_sectors[y - 2][x];
			pSector->_ruNew[1] = &_sectors[y - 2][x + 1];
			pSector->_ruNew[2] = &_sectors[y - 2][x + 2];
			pSector->_ruNew[3] = &_sectors[y - 1][x + 2];
			pSector->_ruNew[4] = &_sectors[y][x + 2];

			pSector->_ruOld[0] = &_sectors[y][x - 1];
			pSector->_ruOld[1] = &_sectors[y - 1][x - 1];
			pSector->_ruOld[2] = &_sectors[y + 1][x - 1];
			pSector->_ruOld[3] = &_sectors[y + 1][x + 1];
			pSector->_ruOld[4] = &_sectors[y + 1][x];

			// dfMOVE_DIR_RR

			pSector->_rrNew[0] = &_sectors[y - 1][x + 2];
			pSector->_rrNew[1] = &_sectors[y][x + 2];
			pSector->_rrNew[2] = &_sectors[y + 1][x + 2];

			pSector->_rrOld[0] = &_sectors[y - 1][x - 1];
			pSector->_rrOld[1] = &_sectors[y][x - 1];
			pSector->_rrOld[2] = &_sectors[y + 1][x - 1];

			// dfMOVE_DIR_RD

			pSector->_rdNew[0] = &_sectors[y + 2][x];
			pSector->_rdNew[1] = &_sectors[y + 2][x + 1];
			pSector->_rdNew[2] = &_sectors[y + 2][x + 2];
			pSector->_rdNew[3] = &_sectors[y + 1][x + 2];
			pSector->_rdNew[4] = &_sectors[y][x + 2];

			pSector->_rdOld[0] = &_sectors[y - 1][x];
			pSector->_rdOld[1] = &_sectors[y - 1][x + 1];
			pSector->_rdOld[2] = &_sectors[y - 1][x - 1];
			pSector->_rdOld[3] = &_sectors[y + 1][x - 1];
			pSector->_rdOld[4] = &_sectors[y][x - 1];

			// dfMOVE_DIR_DD

			pSector->_ddNew[0] = &_sectors[y + 2][x - 1];
			pSector->_ddNew[1] = &_sectors[y + 2][x];
			pSector->_ddNew[2] = &_sectors[y + 2][x + 1];

			pSector->_ddOld[0] = &_sectors[y - 1][x - 1];
			pSector->_ddOld[1] = &_sectors[y - 1][x];
			pSector->_ddOld[2] = &_sectors[y - 1][x + 1];

			// dfMOVE_DIR_LD

			pSector->_ldNew[0] = &_sectors[y + 2][x];
			pSector->_ldNew[1] = &_sectors[y + 2][x - 1];
			pSector->_ldNew[2] = &_sectors[y + 2][x - 2];
			pSector->_ldNew[3] = &_sectors[y + 1][x - 2];
			pSector->_ldNew[4] = &_sectors[y][x - 2];

			pSector->_ldOld[0] = &_sectors[y][x + 1];
			pSector->_ldOld[1] = &_sectors[y + 1][x + 1];
			pSector->_ldOld[2] = &_sectors[y - 1][x + 1];
			pSector->_ldOld[3] = &_sectors[y - 1][x - 1];
			pSector->_ldOld[4] = &_sectors[y - 1][x];
		}
	}
}

inline void CGameServer::HandleCSPacket_MOVE_START(CPacket* recvPacket, CPlayer* pPlayer)
{
	unsigned char moveDirection;
	short x;
	short y;
	GetCSPacket_MOVE_START(recvPacket, moveDirection, x, y);

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		CPacket* syncPacket = CPacket::Alloc();
		syncPacket->Clear();
		int setRet = SetSCPacket_SYNC(syncPacket, pPlayer->_playerID, pPlayer->_x, pPlayer->_y);
		ReqSendUnicast(syncPacket, pPlayer->_sessionID);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	if (!SetPlayerMoveStart(pPlayer, moveDirection, x, y)) return;

	CPacket* movePacket = CPacket::Alloc();
	movePacket->Clear();
	int setRet = SetSCPacket_MOVE_START(movePacket, pPlayer->_playerID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(movePacket, pPlayer->_pSector, pPlayer);
}

inline void CGameServer::HandleCSPacket_MOVE_STOP(CPacket* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_MOVE_STOP(recvPacket, direction, x, y);

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		CPacket* syncPacket = CPacket::Alloc();
		syncPacket->Clear();
		int setRet = SetSCPacket_SYNC(syncPacket, pPlayer->_playerID, pPlayer->_x, pPlayer->_y);
		ReqSendUnicast(syncPacket, pPlayer->_sessionID);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	if (!SetPlayerMoveStop(pPlayer, direction, x, y)) return;

	CPacket* movePacket = CPacket::Alloc();
	movePacket->Clear();
	int setRet = SetSCPacket_MOVE_STOP(movePacket, pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(movePacket, pPlayer->_pSector, pPlayer);
}

inline void CGameServer::HandleCSPacket_ATTACK1(CPacket* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_ATTACK1(recvPacket, direction, x, y);

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		CPacket* syncPacket = CPacket::Alloc();
		syncPacket->Clear();
		int setRet = SetSCPacket_SYNC(syncPacket, pPlayer->_playerID, pPlayer->_x, pPlayer->_y);
		ReqSendUnicast(syncPacket, pPlayer->_sessionID);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	CPlayer* damagedPlayer = nullptr;
	if (!SetPlayerAttack1(pPlayer, damagedPlayer, direction, x, y)) return;

	CPacket* attackPacket = CPacket::Alloc();
	attackPacket->Clear();
	int attackSetRet = SetSCPacket_ATTACK1(attackPacket, pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(attackPacket, pPlayer->_pSector);

	if (damagedPlayer != nullptr)
	{
		CPacket* damagePacket = CPacket::Alloc();
		damagePacket->Clear();
		int damageSetRet = SetSCPacket_DAMAGE(damagePacket, pPlayer->_playerID, damagedPlayer->_playerID, damagedPlayer->_hp);
		ReqSendAroundSector(damagePacket, damagedPlayer->_pSector);
	}
}

inline void CGameServer::HandleCSPacket_ATTACK2(CPacket* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_ATTACK2(recvPacket, direction, x, y);

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		CPacket* syncPacket = CPacket::Alloc();
		syncPacket->Clear();
		int setRet = SetSCPacket_SYNC(syncPacket, pPlayer->_playerID, pPlayer->_x, pPlayer->_y);
		ReqSendUnicast(syncPacket, pPlayer->_sessionID);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	CPlayer* damagedPlayer = nullptr;
	if (!SetPlayerAttack2(pPlayer, damagedPlayer, direction, x, y)) return;

	CPacket* attackPacket = CPacket::Alloc();
	attackPacket->Clear();
	int attackSetRet = SetSCPacket_ATTACK2(attackPacket, pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(attackPacket, pPlayer->_pSector);

	if (damagedPlayer != nullptr)
	{
		CPacket* damagePacket = CPacket::Alloc();
		damagePacket->Clear();
		int damageSetRet = SetSCPacket_DAMAGE(damagePacket, pPlayer->_playerID, damagedPlayer->_playerID, damagedPlayer->_hp);
		ReqSendAroundSector(damagePacket, damagedPlayer->_pSector);
	}
}

inline void CGameServer::HandleCSPacket_ATTACK3(CPacket* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_ATTACK3(recvPacket, direction, x, y);

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		CPacket* syncPacket = CPacket::Alloc();
		syncPacket->Clear();
		int setRet = SetSCPacket_SYNC(syncPacket, pPlayer->_playerID, pPlayer->_x, pPlayer->_y);
		ReqSendUnicast(syncPacket, pPlayer->_sessionID);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	CPlayer* damagedPlayer = nullptr;
	if (!SetPlayerAttack3(pPlayer, damagedPlayer, direction, x, y)) return;

	CPacket* attackPacket = CPacket::Alloc();
	attackPacket->Clear();
	int attackSetRet = SetSCPacket_ATTACK3(attackPacket, pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(attackPacket, pPlayer->_pSector);

	if (damagedPlayer != nullptr)
	{
		CPacket* damagePacket = CPacket::Alloc();
		damagePacket->Clear();
		int damageSetRet = SetSCPacket_DAMAGE(damagePacket, pPlayer->_playerID, damagedPlayer->_playerID, damagedPlayer->_hp);
		ReqSendAroundSector(damagePacket, damagedPlayer->_pSector);
	}
}

inline void CGameServer::HandleCSPacket_ECHO(CPacket* recvPacket, CPlayer* pPlayer)
{
	int time;
	GetCSPacket_ECHO(recvPacket, time);

	CPacket* echoPacket = CPacket::Alloc();
	echoPacket->Clear();
	int setRet = SetSCPacket_ECHO(echoPacket, time);
	ReqSendUnicast(echoPacket, pPlayer->_sessionID);
}

inline void CGameServer::GetCSPacket_MOVE_START(CPacket* pPacket, unsigned char& moveDirection, short& x, short& y)
{
	*pPacket >> moveDirection;
	*pPacket >> x;
	*pPacket >> y;
}

inline void CGameServer::GetCSPacket_MOVE_STOP(CPacket* pPacket, unsigned char& direction, short& x, short& y)
{
	*pPacket >> direction;
	*pPacket >> x;
	*pPacket >> y;
}

inline void CGameServer::GetCSPacket_ATTACK1(CPacket* pPacket, unsigned char& direction, short& x, short& y)
{
	*pPacket >> direction;
	*pPacket >> x;
	*pPacket >> y;
}

inline void CGameServer::GetCSPacket_ATTACK2(CPacket* pPacket, unsigned char& direction, short& x, short& y)
{
	*pPacket >> direction;
	*pPacket >> x;
	*pPacket >> y;
}

inline void CGameServer::GetCSPacket_ATTACK3(CPacket* pPacket, unsigned char& direction, short& x, short& y)
{
	*pPacket >> direction;
	*pPacket >> x;
	*pPacket >> y;
}

inline void CGameServer::GetCSPacket_ECHO(CPacket* pPacket, int& time)
{
	*pPacket >> time;
}

inline bool CGameServer::SetPlayerMoveStart(CPlayer* pPlayer, unsigned char& moveDirection, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_move = true;
	pPlayer->_moveDirection = moveDirection;

	switch (moveDirection)
	{
	case  dfMOVE_DIR_LL:
	case  dfMOVE_DIR_LU:
	case  dfMOVE_DIR_LD:
		pPlayer->_direction = dfMOVE_DIR_LL;
		break;

	case  dfMOVE_DIR_RU:
	case  dfMOVE_DIR_RR:
	case  dfMOVE_DIR_RD:
		pPlayer->_direction = dfMOVE_DIR_RR;
		break;
	}
	return true;
}

inline bool CGameServer::SetPlayerMoveStop(CPlayer* pPlayer, unsigned char& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_move = false;
	pPlayer->_direction = direction;
	return true;
}

inline bool CGameServer::SetPlayerAttack1(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;

	short attackerX = pPlayer->_x;
	short attackerY = pPlayer->_y;
	CSector* pSector = pPlayer->_pSector;

	if (direction == dfMOVE_DIR_LL)
	{
		vector<CPlayer*>::iterator iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();

		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				int dist = attackerX - targetX;

				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
				{
					(*iter)->_hp -= dfATTACK1_DAMAGE;
					pDamagedPlayer = (*iter);
					if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
					return true;
				}
			}
		}

		if (attackerX <= pSector->_xPosMin + dfATTACK1_RANGE_X)
		{
			iter = pSector->_around[dfMOVE_DIR_LL]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_LL]->_players.end(); iter++)
			{
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				int dist = attackerX - targetX;

				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
				{
					(*iter)->_hp -= dfATTACK1_DAMAGE;
					pDamagedPlayer = (*iter);
					if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
					return true;
				}
			}

			if (attackerY <= pSector->_yPosMin + dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_LD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LD]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = attackerX - targetX;

					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
			else if (pPlayer->_y >= pSector->_yPosMax - dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_LU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LU]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = attackerX - targetX;

					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
		}
		else
		{
			if (attackerY <= pSector->_yPosMin + dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = attackerX - targetX;

					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
			else if (attackerY >= pSector->_yPosMax - dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = attackerX - targetX;

					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
		}
	}
	else if (direction == dfMOVE_DIR_RR)
	{
		vector<CPlayer*>::iterator iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				int dist = targetX - attackerX;

				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
				{
					(*iter)->_hp -= dfATTACK1_DAMAGE;
					pDamagedPlayer = (*iter);
					if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
					return true;
				}
			}
		}

		if (attackerX >= pSector->_xPosMax - dfATTACK1_RANGE_X)
		{
			iter = pSector->_around[dfMOVE_DIR_RR]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_RR]->_players.end(); iter++)
			{
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				int dist = targetX - attackerX;

				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
				{
					(*iter)->_hp -= dfATTACK1_DAMAGE;
					pDamagedPlayer = (*iter);
					if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
					return true;
				}
			}

			if (attackerY >= pSector->_yPosMax - dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_RU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RU]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = targetX - attackerX;

					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
			else if (attackerY <= pSector->_yPosMin + dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_RD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RD]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = targetX - attackerX;

					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
		}
		else
		{
			if (attackerY >= pSector->_yPosMax - dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = targetX - attackerX;

					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
			else if (attackerY <= pSector->_yPosMin + dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = targetX - attackerX;

					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
		}
	}
}

inline bool CGameServer::SetPlayerAttack2(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;

	short attackerX = pPlayer->_x;
	short attackerY = pPlayer->_y;
	CSector* pSector = pPlayer->_pSector;

	if (direction == dfMOVE_DIR_LL)
	{
		vector<CPlayer*>::iterator iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();

		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				int dist = attackerX - targetX;

				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
				{
					(*iter)->_hp -= dfATTACK2_DAMAGE;
					pDamagedPlayer = (*iter);
					if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
					return true;
				}
			}
		}

		if (attackerX <= pSector->_xPosMin + dfATTACK2_RANGE_X)
		{
			iter = pSector->_around[dfMOVE_DIR_LL]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_LL]->_players.end(); iter++)
			{
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				int dist = attackerX - targetX;

				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
				{
					(*iter)->_hp -= dfATTACK2_DAMAGE;
					pDamagedPlayer = (*iter);
					if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
					return true;
				}
			}

			if (attackerY <= pSector->_yPosMin + dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_LD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LD]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = attackerX - targetX;

					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
			else if (pPlayer->_y >= pSector->_yPosMax - dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_LU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LU]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = attackerX - targetX;

					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
		}
		else
		{
			if (attackerY <= pSector->_yPosMin + dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = attackerX - targetX;

					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
			else if (attackerY >= pSector->_yPosMax - dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = attackerX - targetX;

					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
		}
	}
	else if (direction == dfMOVE_DIR_RR)
	{
		vector<CPlayer*>::iterator iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				int dist = targetX - attackerX;

				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
				{
					(*iter)->_hp -= dfATTACK2_DAMAGE;
					pDamagedPlayer = (*iter);
					if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
					return true;
				}
			}
		}

		if (attackerX >= pSector->_xPosMax - dfATTACK2_RANGE_X)
		{
			iter = pSector->_around[dfMOVE_DIR_RR]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_RR]->_players.end(); iter++)
			{
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				int dist = targetX - attackerX;

				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
				{
					(*iter)->_hp -= dfATTACK2_DAMAGE;
					pDamagedPlayer = (*iter);
					if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
					return true;
				}
			}

			if (attackerY >= pSector->_yPosMax - dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_RU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RU]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = targetX - attackerX;

					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
			else if (attackerY <= pSector->_yPosMin + dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_RD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RD]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = targetX - attackerX;

					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
		}
		else
		{
			if (attackerY >= pSector->_yPosMax - dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = targetX - attackerX;

					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
			else if (attackerY <= pSector->_yPosMin + dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = targetX - attackerX;

					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
		}
	}
}

inline bool CGameServer::SetPlayerAttack3(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;

	short attackerX = pPlayer->_x;
	short attackerY = pPlayer->_y;
	CSector* pSector = pPlayer->_pSector;

	if (direction == dfMOVE_DIR_LL)
	{
		vector<CPlayer*>::iterator iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();

		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				int dist = attackerX - targetX;

				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
				{
					(*iter)->_hp -= dfATTACK3_DAMAGE;
					pDamagedPlayer = (*iter);
					if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
					return true;
				}
			}
		}

		if (attackerX <= pSector->_xPosMin + dfATTACK3_RANGE_X)
		{
			iter = pSector->_around[dfMOVE_DIR_LL]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_LL]->_players.end(); iter++)
			{
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				int dist = attackerX - targetX;

				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
				{
					(*iter)->_hp -= dfATTACK3_DAMAGE;
					pDamagedPlayer = (*iter);
					if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
					return true;
				}
			}

			if (attackerY <= pSector->_yPosMin + dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_LD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LD]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = attackerX - targetX;

					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
			else if (pPlayer->_y >= pSector->_yPosMax - dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_LU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LU]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = attackerX - targetX;

					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
		}
		else
		{
			if (attackerY <= pSector->_yPosMin + dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = attackerX - targetX;

					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
			else if (attackerY >= pSector->_yPosMax - dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = attackerX - targetX;

					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
		}
	}
	else if (direction == dfMOVE_DIR_RR)
	{
		vector<CPlayer*>::iterator iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				int dist = targetX - attackerX;

				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
				{
					(*iter)->_hp -= dfATTACK3_DAMAGE;
					pDamagedPlayer = (*iter);
					if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
					return true;
				}
			}
		}

		if (attackerX >= pSector->_xPosMax - dfATTACK3_RANGE_X)
		{
			iter = pSector->_around[dfMOVE_DIR_RR]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_RR]->_players.end(); iter++)
			{
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				int dist = targetX - attackerX;

				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
				{
					(*iter)->_hp -= dfATTACK3_DAMAGE;
					pDamagedPlayer = (*iter);
					if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
					return true;
				}
			}

			if (attackerY >= pSector->_yPosMax - dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_RU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RU]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = targetX - attackerX;

					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
			else if (attackerY <= pSector->_yPosMin + dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_RD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RD]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = targetX - attackerX;

					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
		}
		else
		{
			if (attackerY >= pSector->_yPosMax - dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = targetX - attackerX;

					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
			else if (attackerY <= pSector->_yPosMin + dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					int dist = targetX - attackerX;

					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						pDamagedPlayer = (*iter);
						if ((*iter)->_hp <= 0) SetPlayerDead(pDamagedPlayer);
						return true;
					}
				}
			}
		}
	}
}

inline int CGameServer::SetSCPacket_CREATE_MY_CHAR(CPacket* pPacket, int ID, unsigned char direction, short x, short y, unsigned char hp)
{
	short type = dfPACKET_SC_CREATE_MY_CHARACTER;
	int size = sizeof(type) + sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y) + sizeof(hp);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;
	*pPacket << hp;

	return pPacket->GetPacketSize();
}

inline int CGameServer::SetSCPacket_CREATE_OTHER_CHAR(CPacket* pPacket, int ID, unsigned char direction, short x, short y, unsigned char hp)
{
	short type = dfPACKET_SC_CREATE_OTHER_CHARACTER;
	int size = sizeof(type) + sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y) + sizeof(hp);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;
	*pPacket << hp;

	return pPacket->GetPacketSize();
}

inline int CGameServer::SetSCPacket_DELETE_CHAR(CPacket* pPacket, int ID)
{
	short type = dfPACKET_SC_DELETE_CHARACTER;
	int size = sizeof(type) + sizeof(ID);

	*pPacket << type;
	*pPacket << ID;

	return pPacket->GetPacketSize();
}

inline int CGameServer::SetSCPacket_MOVE_START(CPacket* pPacket, int ID, unsigned char moveDirection, short x, short y)
{
	short type = dfPACKET_SC_MOVE_START;
	int size = sizeof(type) + sizeof(ID) + sizeof(moveDirection) + sizeof(x) + sizeof(y);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << moveDirection;
	*pPacket << x;
	*pPacket << y;

	return pPacket->GetPacketSize();
}

inline int CGameServer::SetSCPacket_MOVE_STOP(CPacket* pPacket, int ID, unsigned char direction, short x, short y)
{
	short type = dfPACKET_SC_MOVE_STOP;
	int size = sizeof(type) + sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;

	return pPacket->GetPacketSize();
}

inline int CGameServer::SetSCPacket_ATTACK1(CPacket* pPacket, int ID, unsigned char direction, short x, short y)
{
	short type = dfPACKET_SC_ATTACK1;
	int size = sizeof(type) + sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;

	return pPacket->GetPacketSize();
}

inline int CGameServer::SetSCPacket_ATTACK2(CPacket* pPacket, int ID, unsigned char direction, short x, short y)
{
	short type = dfPACKET_SC_ATTACK2;
	int size = sizeof(type) + sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;

	return pPacket->GetPacketSize();
}

inline int CGameServer::SetSCPacket_ATTACK3(CPacket* pPacket, int ID, unsigned char direction, short x, short y)
{
	short type = dfPACKET_SC_ATTACK3;
	int size = sizeof(type) + sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;

	return pPacket->GetPacketSize();
}

inline int CGameServer::SetSCPacket_DAMAGE(CPacket* pPacket, int attackID, int damageID, unsigned char damageHP)
{
	short type = dfPACKET_SC_DAMAGE;
	int size = sizeof(type) + sizeof(attackID) + sizeof(damageID) + sizeof(damageHP);

	*pPacket << type;
	*pPacket << attackID;
	*pPacket << damageID;
	*pPacket << damageHP;

	return pPacket->GetPacketSize();
}

inline int CGameServer::SetSCPacket_SYNC(CPacket* pPacket, int ID, short x, short y)
{
	InterlockedIncrement(&_syncCnt);

	short type = dfPACKET_SC_SYNC;
	int size = sizeof(type) + sizeof(ID) + sizeof(x) + sizeof(y);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << x;
	*pPacket << y;

	return pPacket->GetPacketSize();
}

inline int CGameServer::SetSCPacket_ECHO(CPacket* pPacket, int time)
{
	short type = dfPACKET_SC_ECHO;
	int size = sizeof(type) + sizeof(time);

	*pPacket << type;
	*pPacket << time;

	return pPacket->GetPacketSize();
}