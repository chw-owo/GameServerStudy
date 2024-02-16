#include "CGameServer.h"
#include "CSystemLog.h"

#define _MONITOR

CGameServer::CGameServer()
{

}

void CGameServer::Initialize()
{
	srand(0);
	for (int y = 0; y < dfSECTOR_CNT_Y; y++)
		for (int x = 0; x < dfSECTOR_CNT_X; x++)
			_sectors[y][x].InitializeSector(x, y);
	SetSectorsAroundInfo();

	_pPlayerPool = new CObjectPool<CPlayer>(dfPLAYER_MAX, true);
	_pJobPool = new CTlsPool<CLanJob>(dfJOB_DEF, true);
	_jobQueue = new CLockFreeQueue<CLanJob*>;

	CGameServer* pServer = this;

#ifdef _MONITOR
	CreateDirectory(L"MonitorLog", NULL);
	_monitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, pServer, 0, nullptr);
	if (_monitorThread == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		__debugbreak();
	}
#endif

	_updateThread = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, pServer, 0, nullptr);
	if (_updateThread == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		__debugbreak();
	}

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int cpuCnt = (int)si.dwNumberOfProcessors;
	
	if (!NetworkInitialize(dfSERVER_IP, dfSERVER_PORT, dfSEND_TIME, dfTOTAL_THREAD, dfRUNNING_THREAD, false, true))
	{
		__debugbreak();
	}

	::printf("Setting Complete\n");

}

CGameServer::~CGameServer()
{
	NetworkTerminate();
}

void CGameServer::OnInitialize()
{
}

void CGameServer::OnTerminate()
{
}

void CGameServer::OnThreadTerminate(wchar_t* threadName)
{
}

bool CGameServer::OnConnectRequest(WCHAR* addr)
{
	// TO-DO: Invalid Check
	return true;
}

void CGameServer::OnAcceptClient(unsigned __int64 sessionID, WCHAR* addr)
{
	CLanJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::ACCEPT, sessionID, nullptr);
	_jobQueue->Enqueue(job);
}

void CGameServer::OnReleaseClient(unsigned __int64 sessionID)
{
	CLanJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::RELEASE, sessionID, nullptr);
	_jobQueue->Enqueue(job);
}

void CGameServer::OnRecv(unsigned __int64 sessionID, CLanMsg* packet)
{
	CLanJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::CONTENT, (SYS_TYPE)0, sessionID, packet);
	_jobQueue->Enqueue(job);
}

void CGameServer::OnSend(unsigned __int64 sessionID, int sendSize)
{

}

void CGameServer::OnError(int errorCode, wchar_t* errorMsg)
{
	::wprintf(L"OnError: %s", errorMsg);
	__debugbreak();
}

void CGameServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
	::wprintf(L"OnDebug: %s", debugMsg);
	__debugbreak();
}

unsigned int __stdcall CGameServer::UpdateThread(void* arg)
{
	CGameServer* pServer = (CGameServer*)arg;

	for (;;)
	{
		pServer->GetDataFromPacket();
		pServer->LogicUpdate();
	}

	return 0;
}

#define dfMONITOR_MAX 1024
unsigned int __stdcall CGameServer::MonitorThread(void* arg)
{
	CGameServer* pServer = (CGameServer*)arg;
	int connected = 0;
	SYSTEMTIME stTime;

	for (;;)
	{
		Sleep(1000);
		pServer->UpdateMonitorData();

		SYSTEMTIME stTime;
		GetLocalTime(&stTime);

		WCHAR text[dfMONITOR_MAX];
		swprintf_s(text, dfMONITOR_MAX,
			L"\n[%s %02d:%02d:%02d]\n\n"
			L"Connected Session: %d\n"
			L"LogicUpdate/1sec: %d\n"
			L"Sync/1sec: %d\n"
			L"Recv/1sec: %d\n"
			L"Send/1sec: %d\n"
			L"Accept/1sec: %d\n"
			L"Disconnect/1sec: %d\n",
			_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond,
			pServer->GetSessionCount(), pServer->_logicFPS, pServer->_syncCnt,
			pServer->GetRecvTPS(), pServer->GetSendTPS(), pServer->GetAcceptTPS(), pServer->GetDisconnectTPS());
		::wprintf(L"%s", text);

		FILE* file;
		errno_t openRet = _wfopen_s(&file, L"MonitorLog/MonitorLog.txt", L"a+");
		if (openRet != 0)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: Fail to open %s : %d\n",
				_T(__FUNCTION__), __LINE__, L"MonitorLog/MonitorLog.txt", openRet);

			::wprintf(L"%s[%d]: Fail to open %s : %d\n",
				_T(__FUNCTION__), __LINE__, L"MonitorLog/MonitorLog.txt", openRet);
		}

		if (file != nullptr)
		{
			fwprintf(file, text);
			fclose(file);
		}
		else
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: Fileptr is nullptr %s\n",
				_T(__FUNCTION__), __LINE__, L"MonitorLog/MonitorLog.txt");

			::wprintf(L"%s[%d]: Fileptr is nullptr %s\n",
				_T(__FUNCTION__), __LINE__, L"MonitorLog/MonitorLog.txt");
		}

		pServer->_logicFPS = 0;
		pServer->_syncCnt = 0;
	}
	return 0;
}

void CGameServer::ReqSendUnicast(CLanSendPacket* packet, __int64 sessionID)
{
	packet->AddUsageCount(1);
	if (!SendPacket(sessionID, packet))
	{
		CLanSendPacket::Free(packet);
	}
}

void CGameServer::ReqSendOneSector(CLanSendPacket* packet, CSector* sector, CPlayer* pExpPlayer)
{
	vector<unsigned __int64> sendID;

	if (pExpPlayer == nullptr)
	{
		vector<CPlayer*>::iterator playerIter = sector->_players.begin();
		for (; playerIter < sector->_players.end(); playerIter++)
		{
			if((*playerIter)->_state == ALIVE)
				sendID.push_back((*playerIter)->_sessionID);
		}
	}
	else
	{
		vector<CPlayer*>::iterator playerIter = sector->_players.begin();
		for (; playerIter < sector->_players.end(); playerIter++)
		{
			if (*playerIter != pExpPlayer)
			{
				if ((*playerIter)->_state == ALIVE)
				{
					sendID.push_back((*playerIter)->_sessionID);
				}
			}
		}
	}

	packet->AddUsageCount(sendID.size());
	vector<unsigned __int64>::iterator idIter = sendID.begin();
	for (; idIter != sendID.end(); idIter++)
	{
		if (!SendPacket(*idIter, packet))
		{
			CLanSendPacket::Free(packet);
		}
	}
}

void CGameServer::ReqSendAroundSector(CLanSendPacket* packet, CSector* centerSector, CPlayer* pExpPlayer)
{
	vector<unsigned __int64> sendID;

	if (pExpPlayer == nullptr)
	{
		for (int i = 0; i < dfAROUND_SECTOR_NUM; i++)
		{
			vector<CPlayer*>::iterator playerIter
				= centerSector->_around[i]->_players.begin();

			for (; playerIter < centerSector->_around[i]->_players.end(); playerIter++)
			{
				if ((*playerIter)->_state == ALIVE)
					sendID.push_back((*playerIter)->_sessionID);
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
				if ((*playerIter)->_state == ALIVE)
					sendID.push_back((*playerIter)->_sessionID);
			}
		}

		vector<CPlayer*>::iterator playerIter
			= centerSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; playerIter < centerSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); playerIter++)
		{
			if (*playerIter != pExpPlayer)
			{
				if ((*playerIter)->_state == ALIVE)
					sendID.push_back((*playerIter)->_sessionID);
			}
		}
	}

	packet->AddUsageCount(sendID.size());
	vector<unsigned __int64>::iterator idIter = sendID.begin();
	for (; idIter != sendID.end(); idIter++)
	{
		if (!SendPacket(*idIter, packet))
		{
			CLanSendPacket::Free(packet);
		}
	}
}

void CGameServer::GetDataFromPacket()
{
	while (_jobQueue->GetUseSize() > 0)
	{
		CLanJob* job = _jobQueue->Dequeue();

		if (job->_type == JOB_TYPE::SYSTEM)
		{
			switch (job->_sysType)
			{
			case SYS_TYPE::ACCEPT:
				Handle_ACCEPT(job->_sessionID);
				break;

			case SYS_TYPE::RELEASE:
				Handle_RELEASE(job->_sessionID);
				break;
			}
		}
		else if (job->_type == JOB_TYPE::CONTENT)
		{
			CLanMsg* pPacket = job->_packet;

			unordered_map<__int64, CPlayer*>::iterator iter = _playerSessionID.find(job->_sessionID);
			if(iter == _playerSessionID.end()) continue;
			CPlayer* pPlayer = iter->second;
			pPlayer->_lastRecvTime = timeGetTime();

			short msgType = -1;
			*pPacket >> msgType;

			switch (msgType)
			{
			case dfPACKET_CS_MOVE_START:
				HandleCSPacket_MOVE_START(pPacket, pPlayer);
				break;

			case dfPACKET_CS_MOVE_STOP:
				HandleCSPacket_MOVE_STOP(pPacket, pPlayer);
				break;

			case dfPACKET_CS_ATTACK1:
				HandleCSPacket_ATTACK1(pPacket, pPlayer);
				break;

			case dfPACKET_CS_ATTACK2:
				HandleCSPacket_ATTACK2(pPacket, pPlayer);				
				break;

			case dfPACKET_CS_ATTACK3:
				HandleCSPacket_ATTACK3(pPacket, pPlayer);
				break;

			case dfPACKET_CS_ECHO:
				HandleCSPacket_ECHO(pPacket, pPlayer);
				break;

			default:
				::printf(" ------- Undefined Message: %d -------- \n", msgType);
				break;
			}
			CLanMsg::Free(pPacket);
		}

		_pJobPool->Free(job);
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
	if (SkipForFixedFrame()) return;

	_logicFPS++;
	for (int i = 0; i < dfPLAYER_MAX; i++)
	{
		if (_players[i] == nullptr) continue;

		if (_players[i]->_state == ALIVE &&
			(timeGetTime() - _players[i]->_lastRecvTime) > dfNETWORK_PACKET_RECV_TIMEOUT)
		{
			SetPlayerDead(_players[i]);
			_timeoutCnt++;
			continue;
		}

		if (_players[i]->_state == ALIVE && _players[i]->_move)
			UpdatePlayerMove(_players[i]);
	}
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

	CLanSendPacket* createMePacket = CLanSendPacket::Alloc();
	int createMeRet = SetSCPacket_CREATE_MY_CHAR(createMePacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	ReqSendUnicast(createMePacket, pPlayer->_sessionID);

	CLanSendPacket* createMeToOtherPacket = CLanSendPacket::Alloc();
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createMeToOtherPacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	ReqSendAroundSector(createMeToOtherPacket, pPlayer->_pSector, pPlayer);

	for (int i = 0; i < dfMOVE_DIR_MAX; i++)
	{
		vector<CPlayer*>::iterator iter
			= pPlayer->_pSector->_around[i]->_players.begin();
		for (; iter < pPlayer->_pSector->_around[i]->_players.end(); iter++)
		{
			CLanSendPacket* createOtherPacket = CLanSendPacket::Alloc();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createOtherPacket,
				(*iter)->_playerID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			ReqSendUnicast(createOtherPacket, pPlayer->_sessionID);
		}
	}

	vector<CPlayer*>::iterator iter
		= pPlayer->_pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
	for (; iter < pPlayer->_pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
	{
		if ((*iter) != pPlayer)
		{
			CLanSendPacket* createOtherPacket = CLanSendPacket::Alloc();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createOtherPacket,
				(*iter)->_playerID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			ReqSendUnicast(createOtherPacket, pPlayer->_sessionID);
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

	CLanSendPacket* createMeToOtherPacket = CLanSendPacket::Alloc();
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createMeToOtherPacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	for (int i = 0; i < sectorCnt; i++)
		ReqSendOneSector(createMeToOtherPacket, inSector[i]);
	
	CLanSendPacket* moveMeToOtherPacket = CLanSendPacket::Alloc();
	int moveMeToOtherRet = SetSCPacket_MOVE_START(moveMeToOtherPacket,
		pPlayer->_playerID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	for (int i = 0; i < sectorCnt; i++)
		ReqSendOneSector(moveMeToOtherPacket, inSector[i]);
	
	CLanSendPacket* deleteMeToOtherPacket = CLanSendPacket::Alloc();
	int deleteMeToOtherRet = SetSCPacket_DELETE_CHAR(deleteMeToOtherPacket, pPlayer->_playerID);
	for (int i = 0; i < sectorCnt; i++)
		ReqSendOneSector(deleteMeToOtherPacket, outSector[i]);

	// Send Data About Other Player ==============================================

	for (int i = 0; i < sectorCnt; i++)
	{
		vector<CPlayer*>::iterator iter = inSector[i]->_players.begin();
		for (; iter < inSector[i]->_players.end(); iter++)
		{
			CLanSendPacket* createOtherPacket = CLanSendPacket::Alloc();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createOtherPacket,
				(*iter)->_playerID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			ReqSendUnicast(createOtherPacket, pPlayer->_sessionID); 

			if ((*iter)->_move)
			{
				CLanSendPacket* moveOtherPacket = CLanSendPacket::Alloc();
				int moveOtherRet = SetSCPacket_MOVE_START(moveOtherPacket,
					(*iter)->_playerID, (*iter)->_moveDirection, (*iter)->_x, (*iter)->_y);
				ReqSendUnicast(moveOtherPacket, pPlayer->_sessionID);
			}
		}
	}

	for (int i = 0; i < sectorCnt; i++)
	{
		vector<CPlayer*>::iterator iter = outSector[i]->_players.begin();
		for (; iter < outSector[i]->_players.end(); iter++)
		{
			CLanSendPacket* deleteOtherPacket = CLanSendPacket::Alloc();
			int deleteOtherRet = SetSCPacket_DELETE_CHAR(deleteOtherPacket, (*iter)->_playerID);
			ReqSendUnicast(deleteOtherPacket, pPlayer->_sessionID);
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

inline void CGameServer::Handle_ACCEPT(__int64 sessionID)
{
	CreatePlayer(sessionID);
}

void CGameServer::SetPlayerDead(CPlayer* pPlayer)
{
	pPlayer->_state = DEAD;
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

	CLanSendPacket* deletePacket = CLanSendPacket::Alloc();
	int deleteRet = SetSCPacket_DELETE_CHAR(deletePacket, pPlayer->_playerID);
	ReqSendAroundSector(deletePacket, pPlayer->_pSector);

	pPlayer->_state = DELETED;
}

inline void CGameServer::Handle_RELEASE(__int64 sessionID)
{
	unordered_map<__int64, CPlayer*>::iterator iter = _playerSessionID.find(sessionID);
	if (iter == _playerSessionID.end()) return;
	CPlayer* pPlayer = iter->second;
	_playerSessionID.erase(iter);

	if (pPlayer->_state != DELETED) 
	{
		_connectEndCnt++;
		DeleteDeadPlayer(pPlayer);
	}
	_players[pPlayer->_playerID] = nullptr;
	_usableIDs[_usableIDCnt++] = pPlayer->_playerID;
	_pPlayerPool->Free(pPlayer);
}

inline bool CGameServer::HandleCSPacket_MOVE_START(CLanMsg* recvPacket, CPlayer* pPlayer)
{
	unsigned char moveDirection;
	short x;
	short y;
	GetCSPacket_MOVE_START(recvPacket, moveDirection, x, y);

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		CLanSendPacket* synCLanSendPacket = CLanSendPacket::Alloc();
		int setRet = SetSCPacket_SYNC(synCLanSendPacket, pPlayer->_playerID, pPlayer->_x, pPlayer->_y);
		ReqSendUnicast(synCLanSendPacket, pPlayer->_sessionID);
		
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	SetPlayerMoveStart(pPlayer, moveDirection, x, y);

	CLanSendPacket* movePacket = CLanSendPacket::Alloc();
	int setRet = SetSCPacket_MOVE_START(movePacket,
		pPlayer->_playerID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(movePacket, pPlayer->_pSector, pPlayer);
	
	return true;
}

inline bool CGameServer::HandleCSPacket_MOVE_STOP(CLanMsg* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_MOVE_STOP(recvPacket, direction, x, y);

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		CLanSendPacket* synCLanSendPacket = CLanSendPacket::Alloc();
		int setRet = SetSCPacket_SYNC(synCLanSendPacket, pPlayer->_playerID, pPlayer->_x, pPlayer->_y);
		ReqSendUnicast(synCLanSendPacket, pPlayer->_sessionID);		
		
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	SetPlayerMoveStop(pPlayer, direction, x, y);

	CLanSendPacket* movePacket = CLanSendPacket::Alloc();
	int setRet = SetSCPacket_MOVE_STOP(movePacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(movePacket, pPlayer->_pSector, pPlayer);

	return true;
}

inline bool CGameServer::HandleCSPacket_ATTACK1(CLanMsg* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_ATTACK1(recvPacket, direction, x, y);

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		CLanSendPacket* synCLanSendPacket = CLanSendPacket::Alloc();
		int setRet = SetSCPacket_SYNC(synCLanSendPacket,
			pPlayer->_playerID, pPlayer->_x, pPlayer->_y);
		ReqSendUnicast(synCLanSendPacket, pPlayer->_sessionID);
		
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	CPlayer* damagedPlayer = nullptr;
	SetPlayerAttack1(pPlayer, damagedPlayer, direction, x, y);

	CLanSendPacket* attackPacket = CLanSendPacket::Alloc();
	int attackSetRet = SetSCPacket_ATTACK1(attackPacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(attackPacket, pPlayer->_pSector);

	if (damagedPlayer != nullptr)
	{
		CLanSendPacket* damagePacket = CLanSendPacket::Alloc();
		int damageSetRet = SetSCPacket_DAMAGE(damagePacket,
			pPlayer->_playerID, damagedPlayer->_playerID, damagedPlayer->_hp);
		ReqSendAroundSector(damagePacket, damagedPlayer->_pSector);
	}

	return true;
}

inline bool CGameServer::HandleCSPacket_ATTACK2(CLanMsg* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_ATTACK2(recvPacket, direction, x, y);

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		CLanSendPacket* synCLanSendPacket = CLanSendPacket::Alloc();
		int setRet = SetSCPacket_SYNC(synCLanSendPacket,
			pPlayer->_playerID, pPlayer->_x, pPlayer->_y);
		ReqSendUnicast(synCLanSendPacket, pPlayer->_sessionID);		
		
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	CPlayer* damagedPlayer = nullptr;
	SetPlayerAttack2(pPlayer, damagedPlayer, direction, x, y);

	CLanSendPacket* attackPacket = CLanSendPacket::Alloc();
	int attackSetRet = SetSCPacket_ATTACK2(attackPacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(attackPacket, pPlayer->_pSector);	

	if (damagedPlayer != nullptr)
	{
		CLanSendPacket* damagePacket = CLanSendPacket::Alloc();
		int damageSetRet = SetSCPacket_DAMAGE(damagePacket,
			pPlayer->_playerID, damagedPlayer->_playerID, damagedPlayer->_hp);
		ReqSendAroundSector(damagePacket, damagedPlayer->_pSector);		
	}

	return true;
}

inline bool CGameServer::HandleCSPacket_ATTACK3(CLanMsg* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_ATTACK3(recvPacket, direction, x, y);

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		CLanSendPacket* synCLanSendPacket = CLanSendPacket::Alloc();
		int setRet = SetSCPacket_SYNC(synCLanSendPacket,
			pPlayer->_playerID, pPlayer->_x, pPlayer->_y);
		ReqSendUnicast(synCLanSendPacket, pPlayer->_sessionID);	 
		
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	CPlayer* damagedPlayer = nullptr;
	SetPlayerAttack3(pPlayer, damagedPlayer, direction, x, y);

	CLanSendPacket* attackPacket = CLanSendPacket::Alloc();
	int attackSetRet = SetSCPacket_ATTACK3(attackPacket,
		pPlayer->_playerID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	ReqSendAroundSector(attackPacket, pPlayer->_pSector);
	
	if (damagedPlayer != nullptr)
	{
		CLanSendPacket* damagePacket = CLanSendPacket::Alloc();
		int damageSetRet = SetSCPacket_DAMAGE(damagePacket,
			pPlayer->_playerID, damagedPlayer->_playerID, damagedPlayer->_hp);
		ReqSendAroundSector(damagePacket, damagedPlayer->_pSector);
	}

	return true;
}

inline bool CGameServer::HandleCSPacket_ECHO(CLanMsg* recvPacket, CPlayer* pPlayer)
{
	int time;
	GetCSPacket_ECHO(recvPacket, time);

	CLanSendPacket* echoPacket = CLanSendPacket::Alloc();
	int setRet = SetSCPacket_ECHO(echoPacket, time);
	ReqSendUnicast(echoPacket, pPlayer->_sessionID);	

	return true;
}

inline void CGameServer::GetCSPacket_MOVE_START(CLanMsg* pPacket, unsigned char& moveDirection, short& x, short& y)
{
	*pPacket >> moveDirection;
	*pPacket >> x;
	*pPacket >> y;
}

inline void CGameServer::GetCSPacket_MOVE_STOP(CLanMsg* pPacket, unsigned char& direction, short& x, short& y)
{
	*pPacket >> direction;
	*pPacket >> x;
	*pPacket >> y;
}

inline void CGameServer::GetCSPacket_ATTACK1(CLanMsg* pPacket, unsigned char& direction, short& x, short& y)
{
	*pPacket >> direction;
	*pPacket >> x;
	*pPacket >> y;
}

inline void CGameServer::GetCSPacket_ATTACK2(CLanMsg* pPacket, unsigned char& direction, short& x, short& y)
{
	*pPacket >> direction;
	*pPacket >> x;
	*pPacket >> y;
}

inline void CGameServer::GetCSPacket_ATTACK3(CLanMsg* pPacket, unsigned char& direction, short& x, short& y)
{
	*pPacket >> direction;
	*pPacket >> x;
	*pPacket >> y;
}

inline void CGameServer::GetCSPacket_ECHO(CLanMsg* pPacket, int& time)
{
	*pPacket >> time;
}

inline void CGameServer::SetPlayerMoveStart(CPlayer* pPlayer, unsigned char& moveDirection, short& x, short& y)
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
}

inline void CGameServer::SetPlayerMoveStop(CPlayer* pPlayer, unsigned char& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_move = false;
	pPlayer->_direction = direction;
}

inline void CGameServer::SetPlayerAttack1(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y)
{
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
					(*iter)->_state == ALIVE)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetPlayerDead(pDamagedPlayer);
					}
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
					(*iter)->_state == ALIVE)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetPlayerDead(pDamagedPlayer);
					}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
					(*iter)->_state == ALIVE)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetPlayerDead(pDamagedPlayer);
						
					}
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
					(*iter)->_state == ALIVE)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetPlayerDead(pDamagedPlayer);
						
					}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
						return;
					}
				}
			}
		}
	}
}

inline void CGameServer::SetPlayerAttack2(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y)
{
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
					(*iter)->_state == ALIVE)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetPlayerDead(pDamagedPlayer);
						
					}
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
					(*iter)->_state == ALIVE)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetPlayerDead(pDamagedPlayer);
						
					}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
					(*iter)->_state == ALIVE)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetPlayerDead(pDamagedPlayer);
						
					}
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
					(*iter)->_state == ALIVE)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetPlayerDead(pDamagedPlayer);
						
					}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
						return;
					}
				}
			}
		}
	}
}

inline void CGameServer::SetPlayerAttack3(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y)
{
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
					(*iter)->_state == ALIVE)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetPlayerDead(pDamagedPlayer);
						
					}
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
					(*iter)->_state == ALIVE)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetPlayerDead(pDamagedPlayer);
						
					}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
					(*iter)->_state == ALIVE)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetPlayerDead(pDamagedPlayer);
						
					}
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
					(*iter)->_state == ALIVE)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetPlayerDead(pDamagedPlayer);
						
					}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
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
						(*iter)->_state == ALIVE)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetPlayerDead(pDamagedPlayer);
							
						}
						return;
					}
				}
			}
		}
	}
}

inline int CGameServer::SetSCPacket_CREATE_MY_CHAR(CLanSendPacket* pPacket, int ID, unsigned char direction, short x, short y, unsigned char hp)
{
	short type = dfPACKET_SC_CREATE_MY_CHARACTER;
	int size = sizeof(type) + sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y) + sizeof(hp);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;
	*pPacket << hp;

	return pPacket->GetPayloadSize();
}

inline int CGameServer::SetSCPacket_CREATE_OTHER_CHAR(CLanSendPacket* pPacket, int ID, unsigned char direction, short x, short y, unsigned char hp)
{
	short type = dfPACKET_SC_CREATE_OTHER_CHARACTER;
	int size = sizeof(type) + sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y) + sizeof(hp);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;
	*pPacket << hp;

	return pPacket->GetPayloadSize();
}

inline int CGameServer::SetSCPacket_DELETE_CHAR(CLanSendPacket* pPacket, int ID)
{
	short type = dfPACKET_SC_DELETE_CHARACTER;
	int size = sizeof(type) + sizeof(ID);

	*pPacket << type;
	*pPacket << ID;

	return pPacket->GetPayloadSize();
}

inline int CGameServer::SetSCPacket_MOVE_START(CLanSendPacket* pPacket, int ID, unsigned char moveDirection, short x, short y)
{
	short type = dfPACKET_SC_MOVE_START;
	int size = sizeof(type) + sizeof(ID) + sizeof(moveDirection) + sizeof(x) + sizeof(y);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << moveDirection;
	*pPacket << x;
	*pPacket << y;

	return pPacket->GetPayloadSize();
}

inline int CGameServer::SetSCPacket_MOVE_STOP(CLanSendPacket* pPacket, int ID, unsigned char direction, short x, short y)
{
	short type = dfPACKET_SC_MOVE_STOP;
	int size = sizeof(type) + sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;

	return pPacket->GetPayloadSize();
}

inline int CGameServer::SetSCPacket_ATTACK1(CLanSendPacket* pPacket, int ID, unsigned char direction, short x, short y)
{
	short type = dfPACKET_SC_ATTACK1;
	int size = sizeof(type) + sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;

	return pPacket->GetPayloadSize();
}

inline int CGameServer::SetSCPacket_ATTACK2(CLanSendPacket* pPacket, int ID, unsigned char direction, short x, short y)
{
	short type = dfPACKET_SC_ATTACK2;
	int size = sizeof(type) + sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;

	return pPacket->GetPayloadSize();
}

inline int CGameServer::SetSCPacket_ATTACK3(CLanSendPacket* pPacket, int ID, unsigned char direction, short x, short y)
{
	short type = dfPACKET_SC_ATTACK3;
	int size = sizeof(type) + sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;

	return pPacket->GetPayloadSize();
}

inline int CGameServer::SetSCPacket_DAMAGE(CLanSendPacket* pPacket, int attackID, int damageID, unsigned char damageHP)
{
	short type = dfPACKET_SC_DAMAGE;
	int size = sizeof(type) + sizeof(attackID) + sizeof(damageID) + sizeof(damageHP);

	*pPacket << type;
	*pPacket << attackID;
	*pPacket << damageID;
	*pPacket << damageHP;

	return pPacket->GetPayloadSize();
}

inline int CGameServer::SetSCPacket_SYNC(CLanSendPacket* pPacket, int ID, short x, short y)
{
	_syncCnt++;

	short type = dfPACKET_SC_SYNC;
	int size = sizeof(type) + sizeof(ID) + sizeof(x) + sizeof(y);

	*pPacket << type;
	*pPacket << ID;
	*pPacket << x;
	*pPacket << y;

	return pPacket->GetPayloadSize();
}

inline int CGameServer::SetSCPacket_ECHO(CLanSendPacket* pPacket, int time)
{
	short type = dfPACKET_SC_ECHO;
	int size = sizeof(type) + sizeof(time);

	*pPacket << type;
	*pPacket << time;

	return pPacket->GetPayloadSize();
}

