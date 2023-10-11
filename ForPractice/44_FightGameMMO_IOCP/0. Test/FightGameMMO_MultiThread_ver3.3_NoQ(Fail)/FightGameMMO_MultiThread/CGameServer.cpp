#include "CGameServer.h"
#include "Main.h"
#include "CSystemLog.h"

#define _MONITOR

// 기각~~~ 플래그를 사용하면 순서 보장이 안된다 !!

CGameServer::CGameServer()
{
	srand(0);
	for (int y = 0; y < dfSECTOR_CNT_Y; y++)
		for (int x = 0; x < dfSECTOR_CNT_X; x++)
			_sectors[y][x].InitializeSector(x, y);
	SetSectorsAroundInfo();

	_pPlayerPool = new CObjectPool<CPlayer>(dfPLAYER_MAX, true);
	CGameServer* pServer = this;

#ifdef _MONITOR
	CreateDirectory(L"Profile", NULL);
	_monitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, pServer, 0, nullptr);
	if (_monitorThread == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
	}
#endif

	_updateThread = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, pServer, 0, nullptr);
	if (_updateThread == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
	}

	::printf("Content Setting Complete\n");

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int cpuCnt = (int)si.dwNumberOfProcessors;

	if (!NetworkStart(dfSERVER_IP, dfSERVER_PORT, cpuCnt, false, dfSESSION_MAX))
		g_bShutdown = true;
}

CGameServer::~CGameServer()
{
	NetworkTerminate();
}

CGameServer* CGameServer::GetInstance()
{
	static CGameServer _CGameServer;
	return  &_CGameServer;
}

bool CGameServer::OnConnectRequest()
{
	// TO-DO: Invalid Check
	return true;
}

void CGameServer::OnAcceptClient(__int64 sessionID)
{
	Handle_ACCEPT(sessionID);
}

void CGameServer::OnReleaseClient(__int64 sessionID)
{
	Handle_RELEASE(sessionID);
}

void CGameServer::OnRecv(__int64 sessionID, CPacket* packet)
{
	unordered_map<__int64, CPlayer*>::iterator iter = _playerSessionID.find(sessionID);
	if (iter == _playerSessionID.end()) return;
	CPlayer* pPlayer = iter->second;
	pPlayer->_lastRecvTime = GetTickCount64();

	short msgType = -1;
	*packet >> msgType;

	switch (msgType)
	{
	case dfPACKET_CS_MOVE_START:
		HandleCSPacket_MOVE_START(packet, pPlayer);
		_pPacketPool->Free(packet);
		break;

	case dfPACKET_CS_MOVE_STOP:
		HandleCSPacket_MOVE_STOP(packet, pPlayer);
		_pPacketPool->Free(packet);
		break;

	case dfPACKET_CS_ATTACK1:
		HandleCSPacket_ATTACK1(packet, pPlayer);
		_pPacketPool->Free(packet);
		break;

	case dfPACKET_CS_ATTACK2:
		HandleCSPacket_ATTACK2(packet, pPlayer);
		_pPacketPool->Free(packet);
		break;

	case dfPACKET_CS_ATTACK3:
		HandleCSPacket_ATTACK3(packet, pPlayer);
		_pPacketPool->Free(packet);
		break;

	case dfPACKET_CS_ECHO:
		HandleCSPacket_ECHO(packet, pPlayer);
		_pPacketPool->Free(packet);
		break;

	default:
		::printf(" ------- Undefined Message: %d -------- \n", msgType);
		break;
	}
}

void CGameServer::OnSend(__int64 sessionID, int sendSize)
{

}

void CGameServer::OnError(int errorCode, wchar_t* errorMsg)
{

}

unsigned int __stdcall CGameServer::UpdateThread(void* arg)
{
	PRO_SET(pSTLSProfiler, GetCurrentThreadId());
	CGameServer* pServer = (CGameServer*)arg;

	for (;;)
	{
		pServer->LogicUpdate();
	}

	return 0;
}

unsigned int __stdcall CGameServer::MonitorThread(void* arg)
{
	CGameServer* pServer = (CGameServer*)arg;
	int connected = 0;

	for (;;)
	{
		Sleep(1000);

		pServer->UpdateMonitorData();

		::wprintf(L"[%s %s]\n\n", _T(__DATE__), _T(__TIME__));
		::wprintf(L"Logic FPS: %d\n", pServer->_logicFPS);
		::wprintf(L"Connected Session: %d\n", pServer->GetSessionCount());
		::wprintf(L"Sync/1sec: %d\n", pServer->_syncCnt);
		::wprintf(L"Accept/1sec: %d\n", pServer->GetAcceptTPS());
		::wprintf(L"Disconnect/1sec: %d\n", pServer->GetDisconnectTPS());
		::wprintf(L" - Dead: %d\n", pServer->_deadCnt);
		::wprintf(L" - Timeout: %d\n", pServer->_timeoutCnt); 
		::wprintf(L" - Connect End: %d\n", pServer->_connectEndCnt);
		// pServer->_processCPUTime.PrintCpuData();
		// pServer->_processorCPUTime.PrintCpuData();

		PRO_PRINT_ADDUP();

		/*
		if (pServer->_checkPointsIdx < dfMONITOR_CHECKPOINT &&
			pServer->_checkPoints[pServer->_checkPointsIdx] < connected)
		{
			WCHAR titleBuf[32] = { L"\0", };
			wsprintf(titleBuf, L"Profile/%d", pServer->_checkPoints[pServer->_checkPointsIdx]);
			PRO_SAVE_ADDUP(titleBuf);
			pServer->_checkPointsIdx++;
		}
		*/

		PRO_RESET();

		pServer->_logicFPS = 0;
		pServer->_syncCnt = 0;
		pServer->_deadCnt = 0;
		pServer->_timeoutCnt = 0;
		pServer->_connectEndCnt = 0;
	}
	return 0;
}

void CGameServer::ReqSendUnicast(CPacket* packet, __int64 sessionID)
{
	SendPacket(sessionID, packet);
}

void CGameServer::ReqSendOneSector(CPacket* packet, CSector* sector, CPlayer* pExpPlayer)
{
	if (pExpPlayer == nullptr)
	{
		vector<CPlayer*>::iterator playerIter = sector->_players.begin();
		for (; playerIter < sector->_players.end(); playerIter++)
		{
			if((*playerIter)->GetStateAlive())
				SendPacket((*playerIter)->_sessionID, packet);
		}
	}
	else
	{
		vector<CPlayer*>::iterator playerIter = sector->_players.begin();
		for (; playerIter < sector->_players.end(); playerIter++)
		{
			if (*playerIter != pExpPlayer)
			{
				if ((*playerIter)->GetStateAlive())
					SendPacket((*playerIter)->_sessionID, packet);
			}
		}
	}
}

void CGameServer::ReqSendAroundSector(CPacket* packet, CSector* centerSector, CPlayer* pExpPlayer)
{
	if (pExpPlayer == nullptr)
	{
		for (int i = 0; i < dfAROUND_SECTOR_NUM; i++)
		{
			vector<CPlayer*>::iterator playerIter
				= centerSector->_around[i]->_players.begin();

			for (; playerIter < centerSector->_around[i]->_players.end(); playerIter++)
			{
				if ((*playerIter)->GetStateAlive())
					SendPacket((*playerIter)->_sessionID, packet);
			}
		}
	}
	else
	{
		for (int i = 0; i < dfMOVE_DIR_MAX; i++)
		{
			vector<CPlayer*>::iterator playerIter
				= centerSector->_around[i]->_players.begin();

			for (; playerIter < centerSector->_around[i]->_players.end(); playerIter++)
			{			
				if ((*playerIter)->GetStateAlive())
					SendPacket((*playerIter)->_sessionID, packet);
			}
		}

		vector<CPlayer*>::iterator playerIter
			= centerSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; playerIter < centerSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); playerIter++)
		{
			if (*playerIter != pExpPlayer)
			{
				if ((*playerIter)->GetStateAlive())
					SendPacket((*playerIter)->_sessionID, packet);
			}
		}
	}
}

bool CGameServer::SkipForFixedFrame()
{
	static DWORD oldTick = timeGetTime();
	if ((timeGetTime() - oldTick) < _timeGap)
		return true;

	oldTick += _timeGap;
	return false;
}

void CGameServer::LogicUpdate()
{
	bool skip = SkipForFixedFrame();

	PRO_BEGIN(L"GS: LogicUpdate");
	for (int i = 0; i < dfPLAYER_MAX; i++)
	{
		if (_players[i] == nullptr) continue;
		SendPlayerState(_players[i]);

		if (skip) continue;

		if (_players[i]->GetStateAlive() &&
			(GetTickCount64() - _players[i]->_lastRecvTime) > dfNETWORK_PACKET_RECV_TIMEOUT)
		{
			SetPlayerDead(_players[i]);
			_timeoutCnt++;
			continue;
		}

		if (_players[i]->GetStateAlive() && _players[i]->_move)
			UpdatePlayerMove(_players[i]);
	}

	PRO_END(L"GS: LogicUpdate");
}

void CGameServer::CreatePlayer(__int64 sessionID)
{
	// Session Num is more than SESSION_MAX
	if (_usableIDCnt == 0 && _playerID == dfPLAYER_MAX)
		return;

	int ID;
	if (_usableIDCnt == 0)
		ID = _playerID++;
	else
		ID = _usableIDs[--_usableIDCnt];

	CPlayer* pPlayer = _pPlayerPool->Alloc(sessionID, ID);
	_playerSessionID.insert(make_pair(sessionID, pPlayer));
	_players[ID] = pPlayer;
	SetSector(pPlayer);

	CPacket* createMePacket = _pPacketPool->Alloc();
	createMePacket->Clear();
	int createMeRet = SetSCPacket_CREATE_MY_CHAR(createMePacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	ReqSendUnicast(createMePacket, pPlayer->_sessionID);
	_pPacketPool->Free(createMePacket);

	CPacket* createMeToOtherPacket = _pPacketPool->Alloc();
	createMeToOtherPacket->Clear();
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createMeToOtherPacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	ReqSendAroundSector(createMeToOtherPacket, pPlayer->_pSector, pPlayer);
	_pPacketPool->Free(createMeToOtherPacket);

	for (int i = 0; i < dfMOVE_DIR_MAX; i++)
	{
		vector<CPlayer*>::iterator iter
			= pPlayer->_pSector->_around[i]->_players.begin();
		for (; iter < pPlayer->_pSector->_around[i]->_players.end(); iter++)
		{
			CPacket* createOtherPacket = _pPacketPool->Alloc();
			createOtherPacket->Clear();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createOtherPacket,
				(*iter)->_playerID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			ReqSendUnicast(createOtherPacket, pPlayer->_sessionID);
			_pPacketPool->Free(createOtherPacket);

		}
	}

	vector<CPlayer*>::iterator iter
		= pPlayer->_pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
	for (; iter < pPlayer->_pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
	{
		if ((*iter) != pPlayer)
		{
			CPacket* createOtherPacket = _pPacketPool->Alloc();
			createOtherPacket->Clear();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createOtherPacket,
				(*iter)->_playerID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			ReqSendUnicast(createOtherPacket, pPlayer->_sessionID);
			_pPacketPool->Free(createOtherPacket);

		}
	}
}

void CGameServer::SendPlayerState(CPlayer* pPlayer)
{

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
	switch (pPlayer->_moveDirection)
	{
	case dfMOVE_DIR_LL:

		if (CheckMovable(pPlayer->_x - dfSPEED_PLAYER_X, pPlayer->_y))
			pPlayer->_x -= dfSPEED_PLAYER_X;

		if (pPlayer->_x < pPlayer->_pSector->_xPosMin)
			UpdateSector(pPlayer, dfMOVE_DIR_LL);

		break;

	case dfMOVE_DIR_LU:

		if (CheckMovable(pPlayer->_x - dfSPEED_PLAYER_X, pPlayer->_y - dfSPEED_PLAYER_Y))
		{
			pPlayer->_x -= dfSPEED_PLAYER_X;
			pPlayer->_y -= dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_x < pPlayer->_pSector->_xPosMin &&
			pPlayer->_y < pPlayer->_pSector->_yPosMin)
			UpdateSector(pPlayer, dfMOVE_DIR_LU);

		else if (pPlayer->_x < pPlayer->_pSector->_xPosMin)
			UpdateSector(pPlayer, dfMOVE_DIR_LL);

		else if (pPlayer->_y < pPlayer->_pSector->_yPosMin)
			UpdateSector(pPlayer, dfMOVE_DIR_UU);

		break;

	case dfMOVE_DIR_UU:

		if (CheckMovable(pPlayer->_x, pPlayer->_y - dfSPEED_PLAYER_Y))
			pPlayer->_y -= dfSPEED_PLAYER_Y;

		if (pPlayer->_y < pPlayer->_pSector->_yPosMin)
			UpdateSector(pPlayer, dfMOVE_DIR_UU);

		break;

	case dfMOVE_DIR_RU:

		if (CheckMovable(pPlayer->_x + dfSPEED_PLAYER_X, pPlayer->_y - dfSPEED_PLAYER_Y))
		{
			pPlayer->_x += dfSPEED_PLAYER_X;
			pPlayer->_y -= dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_x > pPlayer->_pSector->_xPosMax &&
			pPlayer->_y < pPlayer->_pSector->_yPosMin)
			UpdateSector(pPlayer, dfMOVE_DIR_RU);

		else if (pPlayer->_x > pPlayer->_pSector->_xPosMax)
			UpdateSector(pPlayer, dfMOVE_DIR_RR);

		else if (pPlayer->_y < pPlayer->_pSector->_yPosMin)
			UpdateSector(pPlayer, dfMOVE_DIR_UU);

		break;

	case dfMOVE_DIR_RR:

		if (CheckMovable(pPlayer->_x + dfSPEED_PLAYER_X, pPlayer->_y))
			pPlayer->_x += dfSPEED_PLAYER_X;

		if (pPlayer->_x > pPlayer->_pSector->_xPosMax)
			UpdateSector(pPlayer, dfMOVE_DIR_RR);

		break;

	case dfMOVE_DIR_RD:

		if (CheckMovable(pPlayer->_x + dfSPEED_PLAYER_X, pPlayer->_y + dfSPEED_PLAYER_Y))
		{
			pPlayer->_x += dfSPEED_PLAYER_X;
			pPlayer->_y += dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_x > pPlayer->_pSector->_xPosMax &&
			pPlayer->_y > pPlayer->_pSector->_yPosMax)
			UpdateSector(pPlayer, dfMOVE_DIR_RD);

		else if (pPlayer->_x > pPlayer->_pSector->_xPosMax)
			UpdateSector(pPlayer, dfMOVE_DIR_RR);

		else if (pPlayer->_y > pPlayer->_pSector->_yPosMax)
			UpdateSector(pPlayer, dfMOVE_DIR_DD);

		break;

	case dfMOVE_DIR_DD:

		if (CheckMovable(pPlayer->_x, pPlayer->_y + dfSPEED_PLAYER_Y))
			pPlayer->_y += dfSPEED_PLAYER_Y;

		if (pPlayer->_y > pPlayer->_pSector->_yPosMax)
			UpdateSector(pPlayer, dfMOVE_DIR_DD);

		break;

	case dfMOVE_DIR_LD:

		if (CheckMovable(pPlayer->_x - dfSPEED_PLAYER_X, pPlayer->_y + dfSPEED_PLAYER_Y))
		{
			pPlayer->_x -= dfSPEED_PLAYER_X;
			pPlayer->_y += dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_x < pPlayer->_pSector->_xPosMin &&
			pPlayer->_y > pPlayer->_pSector->_yPosMax)
			UpdateSector(pPlayer, dfMOVE_DIR_LD);

		else if (pPlayer->_x < pPlayer->_pSector->_xPosMin)
			UpdateSector(pPlayer, dfMOVE_DIR_LL);

		else if (pPlayer->_y > pPlayer->_pSector->_yPosMax)
			UpdateSector(pPlayer, dfMOVE_DIR_DD);

		break;
	}
}

void CGameServer::SetSector(CPlayer* pPlayer)
{
	int x = (pPlayer->_x / dfSECTOR_SIZE_X) + 2;
	int y = (pPlayer->_y / dfSECTOR_SIZE_Y) + 2;
	_sectors[y][x]._players.push_back(pPlayer);
	pPlayer->_pSector = &_sectors[y][x];
}

void CGameServer::UpdateSector(CPlayer* pPlayer, short direction)
{
	vector<CPlayer*>::iterator iter = pPlayer->_pSector->_players.begin();
	for (; iter != pPlayer->_pSector->_players.end(); iter++)
	{
		if (pPlayer == (*iter))
		{
			pPlayer->_pSector->_players.erase(iter);
			break;
		}
	}

	// Get Around Sector Data =======================================

	int sectorCnt = _sectorCnt[direction];
	CSector** inSector = pPlayer->_pSector->_new[direction];
	CSector** outSector = pPlayer->_pSector->_old[direction];
	CSector* newSector = pPlayer->_pSector->_around[direction];

	// Send Data About My Player ==============================================

	CPacket* createMeToOtherPacket = _pPacketPool->Alloc();
	createMeToOtherPacket->Clear();
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createMeToOtherPacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	for (int i = 0; i < sectorCnt; i++)
		ReqSendOneSector(createMeToOtherPacket, inSector[i]);
	_pPacketPool->Free(createMeToOtherPacket);
	

	CPacket* moveMeToOtherPacket = _pPacketPool->Alloc();
	moveMeToOtherPacket->Clear();
	int moveMeToOtherRet = SetSCPacket_MOVE_START(moveMeToOtherPacket,
		pPlayer->_playerID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	for (int i = 0; i < sectorCnt; i++)
		ReqSendOneSector(moveMeToOtherPacket, inSector[i]);
	_pPacketPool->Free(moveMeToOtherPacket);
	

	CPacket* deleteMeToOtherPacket = _pPacketPool->Alloc();
	deleteMeToOtherPacket->Clear();
	int deleteMeToOtherRet = SetSCPacket_DELETE_CHAR(deleteMeToOtherPacket, pPlayer->_playerID);
	for (int i = 0; i < sectorCnt; i++)
		ReqSendOneSector(deleteMeToOtherPacket, outSector[i]);
	_pPacketPool->Free(deleteMeToOtherPacket);
	

	// Send Data About Other Player ==============================================

	for (int i = 0; i < sectorCnt; i++)
	{
		vector<CPlayer*>::iterator iter = inSector[i]->_players.begin();
		for (; iter < inSector[i]->_players.end(); iter++)
		{
			CPacket* createOtherPacket = _pPacketPool->Alloc();
			createOtherPacket->Clear();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createOtherPacket,
				(*iter)->_playerID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			ReqSendUnicast(createOtherPacket, pPlayer->_sessionID); 
			_pPacketPool->Free(createOtherPacket);
			

			if ((*iter)->_move)
			{
				CPacket* moveOtherPacket = _pPacketPool->Alloc();
				moveOtherPacket->Clear();
				int moveOtherRet = SetSCPacket_MOVE_START(moveOtherPacket,
					(*iter)->_playerID, (*iter)->_moveDirection, (*iter)->_x, (*iter)->_y);
				ReqSendUnicast(moveOtherPacket, pPlayer->_sessionID);
				_pPacketPool->Free(moveOtherPacket);
			}
		}
	}

	for (int i = 0; i < sectorCnt; i++)
	{
		vector<CPlayer*>::iterator iter = outSector[i]->_players.begin();
		for (; iter < outSector[i]->_players.end(); iter++)
		{
			CPacket* deleteOtherPacket = _pPacketPool->Alloc();
			deleteOtherPacket->Clear();
			int deleteOtherRet = SetSCPacket_DELETE_CHAR(deleteOtherPacket, (*iter)->_playerID);
			ReqSendUnicast(deleteOtherPacket, pPlayer->_sessionID);
			_pPacketPool->Free(deleteOtherPacket);
			
		}
	}

	pPlayer->_pSector = newSector;
	newSector->_players.push_back(pPlayer);
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

void CGameServer::SetPlayerDead(CPlayer* pPlayer)
{
	pPlayer->SetStateDead();
	DeleteDeadPlayer(pPlayer);
	Disconnect(pPlayer->_sessionID);
}

void CGameServer::DeleteDeadPlayer(CPlayer* pPlayer)
{
	// Remove from Sector
	vector<CPlayer*>::iterator vectorIter = pPlayer->_pSector->_players.begin();
	for (; vectorIter < pPlayer->_pSector->_players.end(); vectorIter++)
	{
		if ((*vectorIter) == pPlayer)
		{
			pPlayer->_pSector->_players.erase(vectorIter);
			break;
		}
	}

	CPacket* deletePacket = _pPacketPool->Alloc();
	deletePacket->Clear();
	int deleteRet = SetSCPacket_DELETE_CHAR(deletePacket, pPlayer->_playerID);
	ReqSendAroundSector(deletePacket, pPlayer->_pSector);
	_pPacketPool->Free(deletePacket);

	pPlayer->SetStateDeleted();
}

inline void CGameServer::Handle_ACCEPT(__int64 sessionID)
{
	CreatePlayer(sessionID);
}

inline void CGameServer::Handle_RELEASE(__int64 sessionID)
{
	unordered_map<__int64, CPlayer*>::iterator iter = _playerSessionID.find(sessionID);
	if (iter == _playerSessionID.end()) return;
	CPlayer* pPlayer = iter->second;
	_playerSessionID.erase(iter);

	if (!pPlayer->GetStateDeleted())
	{
		_connectEndCnt++;
		DeleteDeadPlayer(pPlayer);
	}
	_players[pPlayer->_playerID] = nullptr;
	_usableIDs[_usableIDCnt++] = pPlayer->_playerID;
	_pPlayerPool->Free(pPlayer);
}

inline void CGameServer::HandleCSPacket_MOVE_START(CPacket* recvPacket, CPlayer* pPlayer)
{
	unsigned char moveDirection;
	short x;
	short y;
	GetCSPacket_MOVE_START(recvPacket, moveDirection, x, y);

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		pPlayer->SetPacketSync();
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	SetPlayerMoveStart(pPlayer, moveDirection, x, y);
}

inline void CGameServer::HandleCSPacket_MOVE_STOP(CPacket* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_MOVE_STOP(recvPacket, direction, x, y);

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		pPlayer->SetPacketSync();
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	SetPlayerMoveStop(pPlayer, direction, x, y);
}

inline void CGameServer::HandleCSPacket_ATTACK1(CPacket* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_ATTACK1(recvPacket, direction, x, y);

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		pPlayer->SetPacketSync();
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	CPlayer* damagedPlayer = nullptr;
	SetPlayerAttack1(pPlayer, damagedPlayer, direction, x, y);

	if (damagedPlayer != nullptr)
	{
		SetPlayerDamaged(damagedPlayer, pPlayer->_playerID);
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
		pPlayer->SetPacketSync();
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	CPlayer* damagedPlayer = nullptr;
	SetPlayerAttack2(pPlayer, damagedPlayer, direction, x, y);

	if (damagedPlayer != nullptr)
	{
		SetPlayerDamaged(damagedPlayer, pPlayer->_playerID);
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
		pPlayer->SetPacketSync();
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	CPlayer* damagedPlayer = nullptr;
	SetPlayerAttack3(pPlayer, damagedPlayer, direction, x, y);	

	if (damagedPlayer != nullptr)
	{
		SetPlayerDamaged(damagedPlayer, pPlayer->_playerID);
	}
}

inline void CGameServer::HandleCSPacket_ECHO(CPacket* recvPacket, CPlayer* pPlayer)
{
	int time;
	GetCSPacket_ECHO(recvPacket, time);
	SetPlayerEcho(pPlayer, time);
}

inline void CGameServer::Handle_MOVE_START_STATE(CPlayer* pPlayer)
{
	CPacket* movePacket = _pPacketPool->Alloc();
	movePacket->Clear();
	int setRet = SetSCPacket_MOVE_START(movePacket,
		pPlayer->_playerID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(movePacket, pPlayer->_pSector, pPlayer);
	_pPacketPool->Free(movePacket);
}

inline void CGameServer::Handle_MOVE_STOP_STATE(CPlayer* pPlayer)
{
	CPacket* movePacket = _pPacketPool->Alloc();
	movePacket->Clear();
	int setRet = SetSCPacket_MOVE_STOP(movePacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(movePacket, pPlayer->_pSector, pPlayer);
	_pPacketPool->Free(movePacket);
}

inline void CGameServer::Handle_ATTACK1_STATE(CPlayer* pPlayer)
{
	CPacket* attackPacket = _pPacketPool->Alloc();
	attackPacket->Clear();
	int attackSetRet = SetSCPacket_ATTACK1(attackPacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(attackPacket, pPlayer->_pSector);
	_pPacketPool->Free(attackPacket);
}

inline void CGameServer::Handle_ATTACK2_STATE(CPlayer* pPlayer)
{
	CPacket* attackPacket = _pPacketPool->Alloc();
	attackPacket->Clear();
	int attackSetRet = SetSCPacket_ATTACK2(attackPacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(attackPacket, pPlayer->_pSector);
	_pPacketPool->Free(attackPacket);
}

inline void CGameServer::Handle_ATTACK3_STATE(CPlayer* pPlayer)
{
	CPacket* attackPacket = _pPacketPool->Alloc();
	attackPacket->Clear();
	int attackSetRet = SetSCPacket_ATTACK3(attackPacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(attackPacket, pPlayer->_pSector);
	_pPacketPool->Free(attackPacket);
}

inline void CGameServer::Handle_DAMAGED_STATE(CPlayer* pPlayer)
{
	CPacket* damagePacket = _pPacketPool->Alloc();
	damagePacket->Clear();
	int damageSetRet = SetSCPacket_DAMAGE(damagePacket,
		pPlayer->_attackerID, pPlayer->_playerID, pPlayer->_hp);
	ReqSendAroundSector(damagePacket, pPlayer->_pSector);
	_pPacketPool->Free(damagePacket);
}

inline void CGameServer::Handle_SYNC_STATE(CPlayer* pPlayer)
{
	CPacket* syncPacket = _pPacketPool->Alloc();
	syncPacket->Clear();
	int setRet = SetSCPacket_SYNC(syncPacket, pPlayer->_playerID, pPlayer->_x, pPlayer->_y);
	ReqSendUnicast(syncPacket, pPlayer->_sessionID);
	_pPacketPool->Free(syncPacket);
}

inline void CGameServer::Handle_ECHO_STATE(CPlayer* pPlayer)
{
	CPacket* echoPacket = _pPacketPool->Alloc();
	echoPacket->Clear();
	int setRet = SetSCPacket_ECHO(echoPacket, pPlayer->_time);
	ReqSendUnicast(echoPacket, pPlayer->_sessionID);
	_pPacketPool->Free(echoPacket);
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

inline void CGameServer::SetPlayerMoveStart(CPlayer* pPlayer, unsigned char& moveDirection, short& x, short& y)
{
	pPlayer->SetPacketMoveStart();
	pPlayer->SetStateMoveStart();

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
}

inline void CGameServer::SetPlayerMoveStop(CPlayer* pPlayer, unsigned char& direction, short& x, short& y)
{
	pPlayer->SetPacketMoveStop();
	pPlayer->SetStateMoveStop();

	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_move = false;
	pPlayer->_direction = direction;
}

inline void CGameServer::SetPlayerAttack1(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y)
{
	pPlayer->SetPacketAttack1();

	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;

	if (direction == dfMOVE_DIR_LL)
	{
		vector<CPlayer*>::iterator iter;
		CSector* pSector = pPlayer->_pSector;

		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y &&
					(*iter)->GetStateAlive())
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
					return;
				}
			}
		}

		if (pPlayer->_x <= pSector->_xPosMin + dfATTACK1_RANGE_X)
		{
			iter = pSector->_around[dfMOVE_DIR_LL]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_LL]->_players.end(); iter++)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y &&
					(*iter)->GetStateAlive())
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
					return;
				}
			}

			if (pPlayer->_y <= pSector->_yPosMin + dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_LD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LD]->_players.end(); iter++)
				{
					int dist = pPlayer->_x - (*iter)->_x;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
						return;
					}
				}
			}
			else if (pPlayer->_y >= pSector->_yPosMax - dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_LU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LU]->_players.end(); iter++)
				{
					int dist = pPlayer->_x - (*iter)->_x;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
						return;
					}
				}
			}
		}
		else
		{
			if (pPlayer->_y <= pSector->_yPosMin + dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					int dist = pPlayer->_x - (*iter)->_x;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
						return;
					}
				}
			}
			else if (pPlayer->_y >= pSector->_yPosMax - dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					int dist = pPlayer->_x - (*iter)->_x;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
						return;
					}
				}
			}
		}
	}
	else if (direction == dfMOVE_DIR_RR)
	{
		vector<CPlayer*>::iterator iter;
		CSector* pSector = pPlayer->_pSector;

		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y &&
					(*iter)->GetStateAlive())
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
					return;
				}
			}
		}

		if (pPlayer->_x >= pSector->_xPosMax - dfATTACK1_RANGE_X)
		{
			iter = pSector->_around[dfMOVE_DIR_RR]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_RR]->_players.end(); iter++)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y &&
					(*iter)->GetStateAlive())
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
					return;
				}
			}

			if (pPlayer->_y >= pSector->_yPosMax - dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_RU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RU]->_players.end(); iter++)
				{
					int dist = (*iter)->_x - pPlayer->_x;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
						return;
					}
				}
			}
			else if (pPlayer->_y <= pSector->_yPosMin + dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_RD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RD]->_players.end(); iter++)
				{
					int dist = (*iter)->_x - pPlayer->_x;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
						return;
					}
				}
			}
		}
		else
		{
			if (pPlayer->_y >= pSector->_yPosMax - dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					int dist = (*iter)->_x - pPlayer->_x;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
						return;
					}
				}
			}
			else if (pPlayer->_y <= pSector->_yPosMin + dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					int dist = (*iter)->_x - pPlayer->_x;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
						return;
					}
				}
			}
		}
	}
}

inline void CGameServer::SetPlayerAttack2(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y)
{
	pPlayer->SetPacketAttack2();

	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;

	if (direction == dfMOVE_DIR_LL)
	{
		vector<CPlayer*>::iterator iter;
		CSector* pSector = pPlayer->_pSector;

		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y &&
					(*iter)->GetStateAlive())
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
					return;
				}
			}
		}

		if (pPlayer->_x <= pSector->_xPosMin + dfATTACK2_RANGE_X)
		{
			iter = pSector->_around[dfMOVE_DIR_LL]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_LL]->_players.end(); iter++)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y &&
					(*iter)->GetStateAlive())
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
					return;
				}
			}

			if (pPlayer->_y <= pSector->_yPosMin + dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_LD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LD]->_players.end(); iter++)
				{
					int dist = pPlayer->_x - (*iter)->_x;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
						return;
					}
				}
			}
			else if (pPlayer->_y >= pSector->_yPosMax - dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_LU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LU]->_players.end(); iter++)
				{
					int dist = pPlayer->_x - (*iter)->_x;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
						return;
					}
				}
			}
		}
		else
		{
			if (pPlayer->_y <= pSector->_yPosMin + dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					int dist = pPlayer->_x - (*iter)->_x;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
						return;
					}
				}
			}
			else if (pPlayer->_y >= pSector->_yPosMax - dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					int dist = pPlayer->_x - (*iter)->_x;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
						return;
					}
				}
			}
		}
	}
	else if (direction == dfMOVE_DIR_RR)
	{
		vector<CPlayer*>::iterator iter;
		CSector* pSector = pPlayer->_pSector;

		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y &&
					(*iter)->GetStateAlive())
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
					return;
				}
			}
		}

		if (pPlayer->_x >= pSector->_xPosMax - dfATTACK2_RANGE_X)
		{
			iter = pSector->_around[dfMOVE_DIR_RR]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_RR]->_players.end(); iter++)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y &&
					(*iter)->GetStateAlive())
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
					return;
				}
			}

			if (pPlayer->_y >= pSector->_yPosMax - dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_RU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RU]->_players.end(); iter++)
				{
					int dist = (*iter)->_x - pPlayer->_x;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
						return;
					}
				}
			}
			else if (pPlayer->_y <= pSector->_yPosMin + dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_RD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RD]->_players.end(); iter++)
				{
					int dist = (*iter)->_x - pPlayer->_x;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
						return;
					}
				}
			}
		}
		else
		{
			if (pPlayer->_y >= pSector->_yPosMax - dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					int dist = (*iter)->_x - pPlayer->_x;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
						return;
					}
				}
			}
			else if (pPlayer->_y <= pSector->_yPosMin + dfATTACK2_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					int dist = (*iter)->_x - pPlayer->_x;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
						return;
					}
				}
			}
		}
	}
}

inline void CGameServer::SetPlayerAttack3(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y)
{
	pPlayer->SetPacketAttack3();

	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;

	if (direction == dfMOVE_DIR_LL)
	{
		vector<CPlayer*>::iterator iter;
		CSector* pSector = pPlayer->_pSector;

		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y &&
					(*iter)->GetStateAlive())
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
					return;
				}
			}
		}

		if (pPlayer->_x <= pSector->_xPosMin + dfATTACK3_RANGE_X)
		{
			iter = pSector->_around[dfMOVE_DIR_LL]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_LL]->_players.end(); iter++)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y &&
					(*iter)->GetStateAlive())
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
					return;
				}
			}

			if (pPlayer->_y <= pSector->_yPosMin + dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_LD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LD]->_players.end(); iter++)
				{
					int dist = pPlayer->_x - (*iter)->_x;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
						return;
					}
				}
			}
			else if (pPlayer->_y >= pSector->_yPosMax - dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_LU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LU]->_players.end(); iter++)
				{
					int dist = pPlayer->_x - (*iter)->_x;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
						return;
					}
				}
			}
		}
		else
		{
			if (pPlayer->_y <= pSector->_yPosMin + dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					int dist = pPlayer->_x - (*iter)->_x;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
						return;
					}
				}
			}
			else if (pPlayer->_y >= pSector->_yPosMax - dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					int dist = pPlayer->_x - (*iter)->_x;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
						return;
					}
				}
			}
		}
	}
	else if (direction == dfMOVE_DIR_RR)
	{
		vector<CPlayer*>::iterator iter;
		CSector* pSector = pPlayer->_pSector;

		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y &&
					(*iter)->GetStateAlive())
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
					return;
				}
			}
		}

		if (pPlayer->_x >= pSector->_xPosMax - dfATTACK3_RANGE_X)
		{
			iter = pSector->_around[dfMOVE_DIR_RR]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_RR]->_players.end(); iter++)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y &&
					(*iter)->GetStateAlive())
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
					return;
				}
			}

			if (pPlayer->_y >= pSector->_yPosMax - dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_RU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RU]->_players.end(); iter++)
				{
					int dist = (*iter)->_x - pPlayer->_x;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
						return;
					}
				}
			}
			else if (pPlayer->_y <= pSector->_yPosMin + dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_RD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RD]->_players.end(); iter++)
				{
					int dist = (*iter)->_x - pPlayer->_x;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
						return;
					}
				}
			}
		}
		else
		{
			if (pPlayer->_y >= pSector->_yPosMax - dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					int dist = (*iter)->_x - pPlayer->_x;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
						return;
					}
				}
			}
			else if (pPlayer->_y <= pSector->_yPosMin + dfATTACK3_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					int dist = (*iter)->_x - pPlayer->_x;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y &&
						(*iter)->GetStateAlive())
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
						return;
					}
				}
			}
		}
	}
}

inline void CGameServer::SetPlayerDamaged(CPlayer* pPlayer, int attackerID)
{
	pPlayer->SetPacketDamaged();

	pPlayer->_attackerID = attackerID;
	if (pPlayer->_hp <= 0)
	{
		_deadCnt++;
		SetPlayerDead(pPlayer);
	}
}

inline void CGameServer::SetPlayerEcho(CPlayer* pPlayer, int time)
{
	pPlayer->SetPacketEcho();
	pPlayer->_time = time;
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
	_syncCnt++;

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

