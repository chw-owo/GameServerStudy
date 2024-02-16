#include "CGameServer.h"
#include "CSystemLog.h"

#define _MONITOR

void CGameServer::Initialize()
{
	srand(0);

	int sectorIdx = 0;
	for (int y = 0; y < dfSECTOR_CNT_Y; y++)
	{
		for (int x = 0; x < dfSECTOR_CNT_X; x++)
		{
			_sectors[y][x].InitializeSector(sectorIdx++, x, y);
		}
	}
	SetSectorsAroundInfo();

	int idx = 0;
	for (int i = 0; i < dfLOGIC_THREAD_NUM; i++)
	{
		for (int j = 0; j < dfLOGIC_PLAYER_NUM; j++)
		{
			CPlayer* pPlayer = new CPlayer(idx);
			_playersArray[i][j] = pPlayer;
			_usablePlayerIdx.Push(idx);
			idx++;
		}
	}

	InitializeSRWLock(&_playersMapLock);

#ifdef _MONITOR
	// CreateDirectory(L"Profile", NULL);
	_monitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, nullptr);
	if (_monitorThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Begin Monitor Thread Error\n",
			_T(__FUNCTION__), __LINE__);

		::wprintf(L"%s[%d]: Begin Monitor Thread Error\n",
			_T(__FUNCTION__), __LINE__);

		__debugbreak();
		return;
	}
#endif

	_logicThreads = new HANDLE[dfLOGIC_THREAD_NUM];

	for (int i = 0; i < dfLOGIC_THREAD_NUM; i++)
	{
		ThreadArg* arg = new ThreadArg(this, i);
		_logicThreads[i] = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, arg, 0, nullptr);
		if (_logicThreads[i] == NULL)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: Begin Logic Thread Error\n",
				_T(__FUNCTION__), __LINE__);

			::wprintf(L"%s[%d]: Begin Logic Thread Error\n",
				_T(__FUNCTION__), __LINE__);

			__debugbreak();
			return;
		}
	}

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Content Setting Complete\n");
	::wprintf(L"Content Setting Complete\n");

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int threadCnt = (int)si.dwNumberOfProcessors / 2;

	if (!NetworkInitialize(dfSERVER_IP, dfSERVER_PORT, 0, 4, 2, false, false))
		__debugbreak();
}

CGameServer::CGameServer()
{
}

// TO-DO: 종료 처리
CGameServer::~CGameServer()
{
	NetworkTerminate();
	WaitForMultipleObjects(dfLOGIC_THREAD_NUM, _logicThreads, true, INFINITE);

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL,
		L"GameServer's All Thread Terminate. Content Terminate.\n");
	::wprintf(L"GameServer's All Thread Terminate. Content Terminate.\n");
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
	return true;
}

void CGameServer::OnAcceptClient(unsigned __int64 sessionID, WCHAR* addr)
{
	HandleAccept(sessionID);
}

void CGameServer::OnReleaseClient(unsigned __int64 sessionID)
{
	HandleRelease(sessionID);
}

void CGameServer::OnRecv(unsigned __int64 sessionID, CLanMsg* packet)
{
	HandleRecv(sessionID, packet);
}

void CGameServer::OnSend(unsigned __int64 sessionID, int sendSize)
{

}

void CGameServer::OnError(int errorCode, wchar_t* errorMsg)
{

}

void CGameServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
}

unsigned int __stdcall CGameServer::UpdateThread(void* arg)
{

	ThreadArg* threadArg = (ThreadArg*)arg;
	CGameServer* pServer = threadArg->_pServer;
	int num = threadArg->_num;

	DWORD oldTick = timeGetTime();

	for(;;)
	{
		pServer->SleepForFixedFrame(oldTick);
		pServer->LogicUpdate(num);
	}

	delete arg;
	return 0;
}

#define dfMONITOR_MAX 1024
unsigned int __stdcall CGameServer::MonitorThread(void* arg)
{
	CGameServer* pServer = (CGameServer*)arg;
	CreateDirectory(L"MonitorLog", NULL);

	for(;;)
	{
		Sleep(1000);
		SYSTEMTIME stTime;
		GetLocalTime(&stTime);

		pServer->UpdateMonitorData();
		long logicFPS = InterlockedExchange(&pServer->_logicFPS, 0);
		long syncCnt = InterlockedExchange(&pServer->_syncCnt, 0);

		WCHAR text[dfMONITOR_MAX];
		swprintf_s(text, dfMONITOR_MAX,
			L"\n[%s %02d:%02d:%02d]\n\n"
			L"Connected Session: %d\n"
			L"Logic Update/1sec: %d\n"
			L"Sync/1sec: %d\n"
			L"Recv/1sec: %d\n"
			L"Send/1sec: %d\n"
			L"Accept/1sec: %d\n"
			L"Disconnect/1sec: %d\n",
			_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond, 
			pServer->GetSessionCount(), logicFPS / dfLOGIC_THREAD_NUM, syncCnt,
			pServer->GetRecvTPS(), pServer->GetSendTPS(), 
			pServer->GetAcceptTPS(), pServer->GetDisconnectTPS());
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
	}
	return 0;
}


void CGameServer::ReqSendUnicast(CLanSendPacket* packet, unsigned __int64 sessionID)
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

void CGameServer::ReqSendAroundSector(CLanSendPacket* packet, CSector* centerSector, CPlayer* pExpPlayer)
{
	vector<unsigned __int64> sendID;

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
	vector<unsigned __int64>::iterator idIter = sendID.begin();
	for (; idIter != sendID.end(); idIter++)
	{
		if (!SendPacket(*idIter, packet))
		{
			CLanSendPacket::Free(packet);
		}
	}
}

void CGameServer::SleepForFixedFrame(DWORD& oldTick)
{
	if ((timeGetTime() - oldTick) < _timeGap)
		Sleep(_timeGap - (timeGetTime() - oldTick));
	oldTick += _timeGap;
}

void CGameServer::LogicUpdate(int threadNum)
{
	InterlockedIncrement(&_logicFPS);

	for (int i = 0; i < dfLOGIC_PLAYER_NUM; i++)
	{
		AcquireSRWLockShared(&_playersArray[threadNum][i]->_lock);
		if (_playersArray[threadNum][i]->_state != PLAYER_STATE::CONNECT)
		{
			ReleaseSRWLockShared(&_playersArray[threadNum][i]->_lock);
			continue;
		}
		CPlayer* pPlayer = _playersArray[threadNum][i];
		DWORD recvTime = pPlayer->_lastRecvTime;
		bool move = pPlayer->_move;
		ReleaseSRWLockShared(&_playersArray[threadNum][i]->_lock);

		if ((timeGetTime() - recvTime) > dfNETWORK_PACKET_RECV_TIMEOUT)
		{
			// pPlayer->PushStateForDebug(__LINE__);
			SetPlayerDead(pPlayer, true);
			InterlockedIncrement(&_timeoutCnt);
			continue;
		}

		if (move)
		{
			UpdatePlayerMove(pPlayer);
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
	AcquireSRWLockShared(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockExclusive(&pPlayer->_lock);
		return;
	}
	short x = pPlayer->_x;
	short y = pPlayer->_y;
	unsigned char moveDir = pPlayer->_moveDirection;
	CSector* pSector = pPlayer->_pSector;
	ReleaseSRWLockShared(&pPlayer->_lock);

	switch (moveDir)
	{
	case dfMOVE_DIR_LL:

		if (CheckMovable(x - dfSPEED_PLAYER_X, y))
		{
			AcquireSRWLockExclusive(&pPlayer->_lock);
			if (pPlayer->_state != PLAYER_STATE::CONNECT)
			{
				ReleaseSRWLockExclusive(&pPlayer->_lock);
				return;
			}
			pPlayer->_x -= dfSPEED_PLAYER_X;
			x = pPlayer->_x;
			ReleaseSRWLockExclusive(&pPlayer->_lock);

			if (x < pSector->_xPosMin)
				UpdateSector(pPlayer, dfMOVE_DIR_LL);
		}

		break;

	case dfMOVE_DIR_LU:

		if (CheckMovable(x - dfSPEED_PLAYER_X, y - dfSPEED_PLAYER_Y))
		{
			AcquireSRWLockExclusive(&pPlayer->_lock);
			if (pPlayer->_state != PLAYER_STATE::CONNECT)
			{
				ReleaseSRWLockExclusive(&pPlayer->_lock);
				return;
			}
			pPlayer->_x -= dfSPEED_PLAYER_X;
			pPlayer->_y -= dfSPEED_PLAYER_Y;
			x = pPlayer->_x;
			y = pPlayer->_y;
			ReleaseSRWLockExclusive(&pPlayer->_lock);

			if (x < pSector->_xPosMin && y < pSector->_yPosMin)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_LU);
			}
			else if (x < pSector->_xPosMin)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_LL);
			}
			else if (y < pSector->_yPosMin)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_UU);
			}
		}

		break;

	case dfMOVE_DIR_UU:

		if (CheckMovable(x, y - dfSPEED_PLAYER_Y))
		{
			AcquireSRWLockExclusive(&pPlayer->_lock);
			if (pPlayer->_state != PLAYER_STATE::CONNECT)
			{
				ReleaseSRWLockExclusive(&pPlayer->_lock);
				return;
			}
			pPlayer->_y -= dfSPEED_PLAYER_Y;
			y = pPlayer->_y;
			ReleaseSRWLockExclusive(&pPlayer->_lock);

			if (y < pSector->_yPosMin)
				UpdateSector(pPlayer, dfMOVE_DIR_UU);
		}

		break;

	case dfMOVE_DIR_RU:

		if (CheckMovable(x + dfSPEED_PLAYER_X, y - dfSPEED_PLAYER_Y))
		{
			AcquireSRWLockExclusive(&pPlayer->_lock);
			if (pPlayer->_state != PLAYER_STATE::CONNECT)
			{
				ReleaseSRWLockExclusive(&pPlayer->_lock);
				return;
			}
			pPlayer->_x += dfSPEED_PLAYER_X;
			pPlayer->_y -= dfSPEED_PLAYER_Y;
			x = pPlayer->_x;
			y = pPlayer->_y;
			ReleaseSRWLockExclusive(&pPlayer->_lock);

			if (x > pSector->_xPosMax && y < pSector->_yPosMin)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_RU);
			}
			else if (x > pSector->_xPosMax)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_RR);
			}
			else if (y < pSector->_yPosMin)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_UU);
			}
		}

		break;

	case dfMOVE_DIR_RR:

		if (CheckMovable(x + dfSPEED_PLAYER_X, y))
		{
			AcquireSRWLockExclusive(&pPlayer->_lock);
			if (pPlayer->_state != PLAYER_STATE::CONNECT)
			{
				ReleaseSRWLockExclusive(&pPlayer->_lock);
				return;
			}
			pPlayer->_x += dfSPEED_PLAYER_X;
			x = pPlayer->_x;
			ReleaseSRWLockExclusive(&pPlayer->_lock);

			if (x > pSector->_xPosMax)
				UpdateSector(pPlayer, dfMOVE_DIR_RR);
		}

		break;

	case dfMOVE_DIR_RD:

		if (CheckMovable(pPlayer->_x + dfSPEED_PLAYER_X, pPlayer->_y + dfSPEED_PLAYER_Y))
		{
			AcquireSRWLockExclusive(&pPlayer->_lock);
			if (pPlayer->_state != PLAYER_STATE::CONNECT)
			{
				ReleaseSRWLockExclusive(&pPlayer->_lock);
				return;
			}
			pPlayer->_x += dfSPEED_PLAYER_X;
			pPlayer->_y += dfSPEED_PLAYER_Y;
			x = pPlayer->_x;
			y = pPlayer->_y;
			ReleaseSRWLockExclusive(&pPlayer->_lock);

			if (x > pSector->_xPosMax && y > pSector->_yPosMax)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_RD);
			}
			else if (x > pSector->_xPosMax)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_RR);
			}
			else if (y > pSector->_yPosMax)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_DD);
			}
		}
		break;

	case dfMOVE_DIR_DD:

		if (CheckMovable(x, y + dfSPEED_PLAYER_Y))
		{
			AcquireSRWLockExclusive(&pPlayer->_lock);
			if (pPlayer->_state != PLAYER_STATE::CONNECT)
			{
				ReleaseSRWLockExclusive(&pPlayer->_lock);
				return;
			}
			pPlayer->_y += dfSPEED_PLAYER_Y;
			y = pPlayer->_y;
			ReleaseSRWLockExclusive(&pPlayer->_lock);

			if (y > pSector->_yPosMax)
				UpdateSector(pPlayer, dfMOVE_DIR_DD);
		}

		break;

	case dfMOVE_DIR_LD:

		if (CheckMovable(x - dfSPEED_PLAYER_X, y + dfSPEED_PLAYER_Y))
		{
			AcquireSRWLockExclusive(&pPlayer->_lock);
			if (pPlayer->_state != PLAYER_STATE::CONNECT)
			{
				ReleaseSRWLockExclusive(&pPlayer->_lock);
				return;
			}
			pPlayer->_x -= dfSPEED_PLAYER_X;
			pPlayer->_y += dfSPEED_PLAYER_Y;
			x = pPlayer->_x;
			y = pPlayer->_y;
			ReleaseSRWLockExclusive(&pPlayer->_lock);

			if (x < pSector->_xPosMin && y > pSector->_yPosMax)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_LD);
			}
			else if (x < pSector->_xPosMin)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_LL);
			}
			else if (y > pSector->_yPosMax)
			{
				UpdateSector(pPlayer, dfMOVE_DIR_DD);
			}
		}
		break;
	}
}

void CGameServer::UpdateSector(CPlayer* pPlayer, short direction)
{
	// Get Around Sector Data =======================================

	int sectorCnt = _sectorCnt[direction];

	AcquireSRWLockShared(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockShared(&pPlayer->_lock);
		return;
	}

	CSector* oriSector = pPlayer->_pSector;
	__int64 sessionID = pPlayer->_sessionID;
	__int64 playerID = pPlayer->_playerID;
	unsigned char dir = pPlayer->_direction;
	unsigned char moveDir = pPlayer->_moveDirection;
	short x = pPlayer->_x;
	short y = pPlayer->_y;
	char hp = pPlayer->_hp;

	ReleaseSRWLockShared(&pPlayer->_lock);

	CSector** inSector = oriSector->_new[direction];
	CSector** outSector = oriSector->_old[direction];
	CSector* newSector = oriSector->_around[direction];

	// Send Data About My Player ==============================================

	CLanSendPacket* createMeToOtherPacket = CLanSendPacket::Alloc();
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createMeToOtherPacket, playerID, dir, x, y, hp);
	for (int i = 0; i < sectorCnt; i++)
		ReqSendOneSector(createMeToOtherPacket, inSector[i], pPlayer);

	CLanSendPacket* moveMeToOtherPacket = CLanSendPacket::Alloc();
	int moveMeToOtherRet = SetSCPacket_MOVE_START(moveMeToOtherPacket, playerID, moveDir, x, y);
	for (int i = 0; i < sectorCnt; i++)
		ReqSendOneSector(moveMeToOtherPacket, inSector[i], pPlayer);

	CLanSendPacket* deleteMeToOtherPacket = CLanSendPacket::Alloc();
	int deleteMeToOtherRet = SetSCPacket_DELETE_CHAR(deleteMeToOtherPacket, playerID);
	for (int i = 0; i < sectorCnt; i++)
		ReqSendOneSector(deleteMeToOtherPacket, outSector[i], pPlayer);

	// Send Data About Other Player ==============================================

	for (int i = 0; i < sectorCnt; i++)
	{
		vector<CPlayer*>::iterator iter;

		AcquireSRWLockShared(&inSector[i]->_lock);
		iter = inSector[i]->_players.begin();
		for (; iter < inSector[i]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				__int64 iterID = (*iter)->_playerID;
				unsigned char iterDir = (*iter)->_direction;
				unsigned char iterMoveDir = (*iter)->_moveDirection;
				short iterX = (*iter)->_x;
				short iterY = (*iter)->_y;
				char iterHp = (*iter)->_hp;
				ReleaseSRWLockShared(&(*iter)->_lock);

				CLanSendPacket* createOtherPacket = CLanSendPacket::Alloc();
				int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createOtherPacket, iterID, iterDir, iterX, iterY, iterHp);
				ReqSendUnicast(createOtherPacket, playerID);

				if ((*iter)->_move)
				{
					CLanSendPacket* moveOtherPacket = CLanSendPacket::Alloc();
					int moveOtherRet = SetSCPacket_MOVE_START(moveOtherPacket, iterID, iterMoveDir, iterX, iterY);
					ReqSendUnicast(moveOtherPacket, sessionID);
				}
			}
		}
		ReleaseSRWLockShared(&inSector[i]->_lock);

		AcquireSRWLockShared(&outSector[i]->_lock);
		iter = outSector[i]->_players.begin();
		for (; iter < outSector[i]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				__int64 iterID = (*iter)->_playerID;
				ReleaseSRWLockShared(&(*iter)->_lock);

				CLanSendPacket* deleteOtherPacket = CLanSendPacket::Alloc();
				int deleteOtherRet = SetSCPacket_DELETE_CHAR(deleteOtherPacket, iterID);
				ReqSendUnicast(deleteOtherPacket, sessionID);
			}
		}
		ReleaseSRWLockShared(&outSector[i]->_lock);
	}

	if (oriSector->_idx < newSector->_idx)
	{
		AcquireSRWLockExclusive(&oriSector->_lock);
		vector<CPlayer*>::iterator iter = oriSector->_players.begin();
		for (; iter != oriSector->_players.end(); iter++)
		{
			if (pPlayer == (*iter))
			{
				oriSector->_players.erase(iter);
				break;
			}
		}
		ReleaseSRWLockExclusive(&oriSector->_lock);

		AcquireSRWLockExclusive(&newSector->_lock);
		newSector->_players.push_back(pPlayer);
		ReleaseSRWLockExclusive(&newSector->_lock);
	}
	else if (oriSector->_idx > newSector->_idx)
	{
		AcquireSRWLockExclusive(&newSector->_lock);
		newSector->_players.push_back(pPlayer);
		ReleaseSRWLockExclusive(&newSector->_lock);

		AcquireSRWLockExclusive(&oriSector->_lock);
		vector<CPlayer*>::iterator iter = oriSector->_players.begin();
		for (; iter != oriSector->_players.end(); iter++)
		{
			if (pPlayer == (*iter))
			{
				oriSector->_players.erase(iter);
				break;
			}
		}
		ReleaseSRWLockExclusive(&oriSector->_lock);
	}

	AcquireSRWLockExclusive(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockExclusive(&pPlayer->_lock);
		return;
	}
	pPlayer->_pSector = newSector;
	ReleaseSRWLockExclusive(&pPlayer->_lock);
}

inline void CGameServer::HandleRelease(unsigned __int64 sessionID)
{
	AcquireSRWLockExclusive(&_playersMapLock);
	unordered_map<unsigned __int64, CPlayer*>::iterator iter = _playersMap.find(sessionID);
	if (iter == _playersMap.end())
	{
		ReleaseSRWLockExclusive(&_playersMapLock);
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL,
			L"%s[%d]: No Session\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: No Session\n", _T(__FUNCTION__), __LINE__);
		return;
	}
	CPlayer* pPlayer = iter->second;

	AcquireSRWLockExclusive(&pPlayer->_lock);

	PLAYER_STATE state = pPlayer->_state;

	// pPlayer->PushStateForDebug(__LINE__);
	int num = pPlayer->_idx._num;
	if (state == PLAYER_STATE::DISCONNECT)
	{
		ReleaseSRWLockExclusive(&_playersMapLock);
		ReleaseSRWLockExclusive(&pPlayer->_lock);

		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL,
			L"%s[%d] Idx %d is Already Disconnect (%d == %d)\n",
			__func__, __LINE__, num, PLAYER_STATE::DISCONNECT, state);

		::wprintf(L"%s[%d] Idx %d is Already Disconnect (%d == %d)\n",
			__func__, __LINE__, num, PLAYER_STATE::DISCONNECT, state);

		return;
	}
	_playersMap.erase(iter);
	ReleaseSRWLockExclusive(&_playersMapLock);

	// pPlayer->PushStateForDebug(__LINE__);

	if (state != PLAYER_STATE::DEAD)
	{
		// pPlayer->PushStateForDebug(__LINE__);
		__int64 playerID = pPlayer->_playerID;
		CSector* pSector = pPlayer->_pSector;
		PLAYER_STATE state = pPlayer->_state;
		int num = pPlayer->_idx._num;

		if (state == PLAYER_STATE::DISCONNECT)
		{
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL,
				L"%s[%d] Idx %d is Already Disconnect (%d == %d)\n",
				__func__, __LINE__, num, PLAYER_STATE::DISCONNECT, state);

			::wprintf(L"%s[%d] Idx %d is Already Disconnect (%d == %d)\n",
				__func__, __LINE__, num, PLAYER_STATE::DISCONNECT, state);

			ReleaseSRWLockExclusive(&pPlayer->_lock);
			return;
		}
		pPlayer->CleanUp();
		// pPlayer->PushStateForDebug(__LINE__);
		ReleaseSRWLockExclusive(&pPlayer->_lock);

		AcquireSRWLockExclusive(&pSector->_lock);
		vector<CPlayer*>::iterator vectorIter = pSector->_players.begin();
		for (; vectorIter < pSector->_players.end(); vectorIter++)
		{
			if ((*vectorIter)->_playerID == playerID)
			{
				pSector->_players.erase(vectorIter);
				break;
			}
		}
		ReleaseSRWLockExclusive(&pSector->_lock);

		CLanSendPacket* deletePacket = CLanSendPacket::Alloc();
		int deleteRet = SetSCPacket_DELETE_CHAR(deletePacket, playerID);
		ReqSendAroundSector(deletePacket, pSector);
		InterlockedIncrement(&_connectEndCnt);
	}
	else
	{
		pPlayer->CleanUp();
		ReleaseSRWLockExclusive(&pPlayer->_lock);
	}

	_usablePlayerIdx.Push(num);
}

void CGameServer::SetPlayerDead(CPlayer* pPlayer, bool timeout)
{
	AcquireSRWLockExclusive(&pPlayer->_lock);

	__int64 sessionID = pPlayer->_sessionID;
	__int64 playerID = pPlayer->_playerID;
	CSector* pSector = pPlayer->_pSector;
	PLAYER_STATE state = pPlayer->_state;
	int num = pPlayer->_idx._num;

	// pPlayer->PushStateForDebug(__LINE__);
	if (state == PLAYER_STATE::DISCONNECT)
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL,
			L"%s[%d] Idx %d is Already Disconnect (%d == %d)\n",
			__func__, __LINE__, num, PLAYER_STATE::DISCONNECT, state);

		::wprintf(L"%s[%d] Idx %d is Already Disconnect (%d == %d)\n",
			__func__, __LINE__, num, PLAYER_STATE::DISCONNECT, state);

		ReleaseSRWLockExclusive(&pPlayer->_lock);
		return;
	}
	pPlayer->_state = PLAYER_STATE::DEAD;
	// pPlayer->PushStateForDebug(__LINE__);
	ReleaseSRWLockExclusive(&pPlayer->_lock);

	AcquireSRWLockExclusive(&pSector->_lock);
	vector<CPlayer*>::iterator vectorIter = pSector->_players.begin();
	for (; vectorIter < pSector->_players.end(); vectorIter++)
	{
		if ((*vectorIter)->_playerID == playerID)
		{
			pSector->_players.erase(vectorIter);
			break;
		}
	}
	ReleaseSRWLockExclusive(&pSector->_lock);

	CLanSendPacket* deletePacket = CLanSendPacket::Alloc();
	int deleteRet = SetSCPacket_DELETE_CHAR(deletePacket, playerID);
	ReqSendAroundSector(deletePacket, pSector);

	Disconnect(sessionID);
}

inline void CGameServer::HandleAccept(unsigned __int64 sessionID)
{
	int idx = _usablePlayerIdx.Pop();
	if (idx == 0)
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL,
			L"%s[%d] player max\n", __func__, __LINE__);
		::printf("%s[%d] player max\n", __func__, __LINE__);
		Disconnect(sessionID);
		return;
	}

	int threadNum = idx / dfLOGIC_PLAYER_NUM;
	int threadIdx = idx % dfLOGIC_PLAYER_NUM;
	CPlayer* pPlayer = _playersArray[threadNum][threadIdx];

	AcquireSRWLockExclusive(&_playersMapLock);
	_playersMap.insert(make_pair(sessionID, pPlayer));
	AcquireSRWLockExclusive(&pPlayer->_lock);
	ReleaseSRWLockExclusive(&_playersMapLock);

	PLAYER_STATE state = pPlayer->_state;
	if (state != PLAYER_STATE::DISCONNECT)
	{
		ReleaseSRWLockExclusive(&pPlayer->_lock);

		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL,
			L"%s[%d] Idx %d is Already Connect (%d != %d)\n",
			__func__, __LINE__, idx, PLAYER_STATE::DISCONNECT, state);

		::printf("%s[%d] Idx %d is Already Connect (%d != %d)\n",
			__func__, __LINE__, idx, PLAYER_STATE::DISCONNECT, state);

		return;
	}

	long long playerID = InterlockedIncrement64(&_playerIDGenerator);
	pPlayer->Initialize(sessionID, playerID);

	int sectorX = (pPlayer->_x / dfSECTOR_SIZE_X) + 2;
	int sectorY = (pPlayer->_y / dfSECTOR_SIZE_Y) + 2;
	pPlayer->_pSector = &_sectors[sectorY][sectorX];

	CSector* pSector = pPlayer->_pSector;
	unsigned char dir = pPlayer->_direction;
	short x = pPlayer->_x;
	short y = pPlayer->_y;
	char hp = pPlayer->_hp;

	ReleaseSRWLockExclusive(&pPlayer->_lock);

	AcquireSRWLockExclusive(&_sectors[sectorY][sectorX]._lock);
	_sectors[sectorY][sectorX]._players.push_back(pPlayer);
	ReleaseSRWLockExclusive(&_sectors[sectorY][sectorX]._lock);

	//===========================================================================

	CLanSendPacket* createMePacket = CLanSendPacket::Alloc();
	int createMeRet = SetSCPacket_CREATE_MY_CHAR(createMePacket, playerID, dir, x, y, hp);
	ReqSendUnicast(createMePacket, sessionID);

	CLanSendPacket* createMeToOtherPacket = CLanSendPacket::Alloc();
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createMeToOtherPacket, playerID, dir, x, y, hp);
	ReqSendAroundSector(createMeToOtherPacket, pSector, pPlayer);

	for (int i = 0; i < dfMOVE_DIR_MAX; i++)
	{
		AcquireSRWLockShared(&pSector->_around[i]->_lock);
		vector<CPlayer*>::iterator iter
			= pSector->_around[i]->_players.begin();
		for (; iter < pSector->_around[i]->_players.end(); iter++)
		{
			AcquireSRWLockShared(&(*iter)->_lock);
			if ((*iter)->_state != PLAYER_STATE::CONNECT)
			{
				ReleaseSRWLockShared(&(*iter)->_lock);
				continue;
			}
			__int64 iterID = (*iter)->_playerID;
			unsigned char iterDir = (*iter)->_direction;
			unsigned char iterMoveDir = (*iter)->_moveDirection;
			short iterX = (*iter)->_x;
			short iterY = (*iter)->_y;
			char iterHp = (*iter)->_hp;
			ReleaseSRWLockShared(&(*iter)->_lock);

			CLanSendPacket* createOtherPacket = CLanSendPacket::Alloc();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createOtherPacket, iterID, iterDir, iterX, iterY, iterHp);
			ReqSendUnicast(createOtherPacket, sessionID);

		}
		ReleaseSRWLockShared(&pSector->_around[i]->_lock);
	}

	AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);
	vector<CPlayer*>::iterator iter
		= pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
	for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
	{
		if ((*iter) != pPlayer)
		{
			AcquireSRWLockShared(&(*iter)->_lock);

			if ((*iter)->_state != PLAYER_STATE::CONNECT)
			{
				ReleaseSRWLockShared(&(*iter)->_lock);
				continue;
			}

			__int64 iterID = (*iter)->_playerID;
			unsigned char iterDir = (*iter)->_direction;
			unsigned char iterMoveDir = (*iter)->_moveDirection;
			short iterX = (*iter)->_x;
			short iterY = (*iter)->_y;
			char iterHp = (*iter)->_hp;
			ReleaseSRWLockShared(&(*iter)->_lock);

			CLanSendPacket* createOtherPacket = CLanSendPacket::Alloc();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(createOtherPacket, iterID, iterDir, iterX, iterY, iterHp);
			ReqSendUnicast(createOtherPacket, sessionID);
		}
	}
	ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);

}

inline void CGameServer::HandleRecv(unsigned __int64 sessionID, CLanMsg* packet)
{
	AcquireSRWLockShared(&_playersMapLock);
	unordered_map<unsigned __int64, CPlayer*>::iterator iter = _playersMap.find(sessionID);
	if (iter == _playersMap.end())
	{
		ReleaseSRWLockShared(&_playersMapLock);
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL,
			L"%s[%d]: No Session\n", _T(__FUNCTION__), __LINE__);

		::wprintf(L"%s[%d]: No Session\n", _T(__FUNCTION__), __LINE__);
		return;
	}
	CPlayer* pPlayer = iter->second;

	AcquireSRWLockExclusive(&pPlayer->_lock);
	ReleaseSRWLockShared(&_playersMapLock);

	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL,
			L"%s[%d] Idx %d is Invalid Session (%d)\n",
			__func__, __LINE__, pPlayer->_idx._num, pPlayer->_state);

		::wprintf(L"%s[%d] Idx %d is Invalid Session (%d)\n",
			__func__, __LINE__, pPlayer->_idx._num, pPlayer->_state);
		ReleaseSRWLockExclusive(&pPlayer->_lock);
		return;
	}

	pPlayer->_lastRecvTime = timeGetTime();
	ReleaseSRWLockExclusive(&pPlayer->_lock);

	short msgType = -1;
	*packet >> msgType;

	switch (msgType)
	{
	case dfPACKET_CS_MOVE_START:
		// pPlayer->PushStateForDebug(__LINE__);
		HandleCSPacket_MOVE_START(packet, pPlayer);
		break;

	case dfPACKET_CS_MOVE_STOP:
		// pPlayer->PushStateForDebug(__LINE__);
		HandleCSPacket_MOVE_STOP(packet, pPlayer);
		break;

	case dfPACKET_CS_ATTACK1:
		// pPlayer->PushStateForDebug(__LINE__);
		HandleCSPacket_ATTACK1(packet, pPlayer);
		break;

	case dfPACKET_CS_ATTACK2:
		// pPlayer->PushStateForDebug(__LINE__);
		HandleCSPacket_ATTACK2(packet, pPlayer);
		break;

	case dfPACKET_CS_ATTACK3:
		// pPlayer->PushStateForDebug(__LINE__);
		HandleCSPacket_ATTACK3(packet, pPlayer);
		break;

	case dfPACKET_CS_ECHO:
		// pPlayer->PushStateForDebug(__LINE__);
		HandleCSPacket_ECHO(packet, pPlayer);
		break;

	default:
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL,
			L"%s[%d] Undefined Message, %d\n", __func__, __LINE__, msgType);
		::wprintf(L"%s[%d] Undefined Message, %d\n", __func__, __LINE__, msgType);
		break;
	}

	CLanMsg::Free(packet);
}

inline void CGameServer::HandleCSPacket_MOVE_START(CLanMsg* recvPacket, CPlayer* pPlayer)
{
	unsigned char moveDirection;
	short x;
	short y;
	GetCSPacket_MOVE_START(recvPacket, moveDirection, x, y);

	AcquireSRWLockShared(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockShared(&pPlayer->_lock);
		return;
	}
	__int64 sessionID = pPlayer->_sessionID;
	__int64 playerID = pPlayer->_playerID;
	short playerX = pPlayer->_x;
	short playerY = pPlayer->_y;
	ReleaseSRWLockShared(&pPlayer->_lock);

	if (abs(playerX - x) > dfERROR_RANGE || abs(playerY - y) > dfERROR_RANGE)
	{
		// pPlayer->PushStateForDebug(__LINE__);
		CLanSendPacket* syncPacket = CLanSendPacket::Alloc();
		int setRet = SetSCPacket_SYNC(syncPacket, playerID, playerX, playerY);
		ReqSendUnicast(syncPacket, sessionID);

		x = playerX;
		y = playerY;
	}

	if (!SetPlayerMoveStart(pPlayer, moveDirection, x, y)) return;

	AcquireSRWLockShared(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockShared(&pPlayer->_lock);
		return;
	}

	CSector* pSector = pPlayer->_pSector;
	unsigned char moveDir = pPlayer->_moveDirection;
	playerX = pPlayer->_x;
	playerY = pPlayer->_y;
	ReleaseSRWLockShared(&pPlayer->_lock);

	CLanSendPacket* movePacket = CLanSendPacket::Alloc();
	int setRet = SetSCPacket_MOVE_START(movePacket, playerID, moveDir, playerX, playerY);
	ReqSendAroundSector(movePacket, pSector, pPlayer);
}

inline void CGameServer::HandleCSPacket_MOVE_STOP(CLanMsg* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_MOVE_STOP(recvPacket, direction, x, y);

	AcquireSRWLockShared(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockShared(&pPlayer->_lock);
		return;
	}
	__int64 sessionID = pPlayer->_sessionID;
	__int64 playerID = pPlayer->_playerID;
	short playerX = pPlayer->_x;
	short playerY = pPlayer->_y;
	ReleaseSRWLockShared(&pPlayer->_lock);

	if (abs(playerX - x) > dfERROR_RANGE || abs(playerY - y) > dfERROR_RANGE)
	{
		// pPlayer->PushStateForDebug(__LINE__);
		CLanSendPacket* syncPacket = CLanSendPacket::Alloc();
		int setRet = SetSCPacket_SYNC(syncPacket, playerID, playerX, playerY);
		ReqSendUnicast(syncPacket, sessionID);

		x = playerX;
		y = playerY;
	}

	if (!SetPlayerMoveStop(pPlayer, direction, x, y)) return;

	AcquireSRWLockShared(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockShared(&pPlayer->_lock);
		return;
	}
	CSector* pSector = pPlayer->_pSector;
	unsigned char playerDir = pPlayer->_direction;
	playerX = pPlayer->_x;
	playerY = pPlayer->_y;
	ReleaseSRWLockShared(&pPlayer->_lock);

	CLanSendPacket* movePacket = CLanSendPacket::Alloc();
	int setRet = SetSCPacket_MOVE_STOP(movePacket, playerID, playerDir, playerX, playerY);
	ReqSendAroundSector(movePacket, pSector, pPlayer);
}

inline void CGameServer::HandleCSPacket_ATTACK1(CLanMsg* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_ATTACK1(recvPacket, direction, x, y);

	AcquireSRWLockShared(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockShared(&pPlayer->_lock);
		return;
	}
	__int64 sessionID = pPlayer->_sessionID;
	__int64 attackPlayerID = pPlayer->_playerID;
	short playerX = pPlayer->_x;
	short playerY = pPlayer->_y;
	ReleaseSRWLockShared(&pPlayer->_lock);

	if (abs(playerX - x) > dfERROR_RANGE || abs(playerY - y) > dfERROR_RANGE)
	{
		// pPlayer->PushStateForDebug(__LINE__);
		CLanSendPacket* syncPacket = CLanSendPacket::Alloc();
		int setRet = SetSCPacket_SYNC(syncPacket, attackPlayerID, playerX, playerY);
		ReqSendUnicast(syncPacket, sessionID);

		x = playerX;
		y = playerY;
	}

	CPlayer* damagedPlayer = nullptr;
	if (!SetPlayerAttack1(pPlayer, damagedPlayer, direction, x, y)) return;

	AcquireSRWLockShared(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockShared(&pPlayer->_lock);
		return;
	}
	CSector* pAttackSector = pPlayer->_pSector;
	unsigned char playerDir = pPlayer->_direction;
	playerX = pPlayer->_x;
	playerY = pPlayer->_y;
	ReleaseSRWLockShared(&pPlayer->_lock);

	CLanSendPacket* attackPacket = CLanSendPacket::Alloc();
	int attackSetRet = SetSCPacket_ATTACK1(attackPacket, attackPlayerID, playerDir, playerX, playerY);
	ReqSendAroundSector(attackPacket, pAttackSector);

	if (damagedPlayer != nullptr)
	{
		AcquireSRWLockShared(&damagedPlayer->_lock);
		if (damagedPlayer->_state == PLAYER_STATE::DISCONNECT)
		{
			ReleaseSRWLockShared(&damagedPlayer->_lock);
			return;
		}
		CSector* pDamagedSector = damagedPlayer->_pSector;
		__int64 damagedID = damagedPlayer->_playerID;
		char damagedHp = damagedPlayer->_hp;
		ReleaseSRWLockShared(&damagedPlayer->_lock);

		CLanSendPacket* damagePacket = CLanSendPacket::Alloc();
		int damageSetRet = SetSCPacket_DAMAGE(damagePacket, attackPlayerID, damagedID, damagedHp);
		ReqSendAroundSector(damagePacket, pDamagedSector);
	}
}

inline void CGameServer::HandleCSPacket_ATTACK2(CLanMsg* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_ATTACK2(recvPacket, direction, x, y);

	AcquireSRWLockShared(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockShared(&pPlayer->_lock);
		return;
	}
	__int64 sessionID = pPlayer->_sessionID;
	__int64 attackPlayerID = pPlayer->_playerID;
	short playerX = pPlayer->_x;
	short playerY = pPlayer->_y;
	ReleaseSRWLockShared(&pPlayer->_lock);

	if (abs(playerX - x) > dfERROR_RANGE || abs(playerY - y) > dfERROR_RANGE)
	{
		// pPlayer->PushStateForDebug(__LINE__);
		CLanSendPacket* syncPacket = CLanSendPacket::Alloc();
		int setRet = SetSCPacket_SYNC(syncPacket, attackPlayerID, playerX, playerY);
		ReqSendUnicast(syncPacket, sessionID);

		x = playerX;
		y = playerY;
	}

	CPlayer* damagedPlayer = nullptr;
	if (!SetPlayerAttack2(pPlayer, damagedPlayer, direction, x, y)) return;

	AcquireSRWLockShared(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockShared(&pPlayer->_lock);
		return;
	}
	CSector* pAttackSector = pPlayer->_pSector;
	unsigned char playerDir = pPlayer->_direction;
	playerX = pPlayer->_x;
	playerY = pPlayer->_y;
	ReleaseSRWLockShared(&pPlayer->_lock);

	CLanSendPacket* attackPacket = CLanSendPacket::Alloc();
	int attackSetRet = SetSCPacket_ATTACK2(attackPacket, attackPlayerID, playerDir, playerX, playerY);
	ReqSendAroundSector(attackPacket, pAttackSector);

	if (damagedPlayer != nullptr)
	{
		AcquireSRWLockShared(&damagedPlayer->_lock);
		if (damagedPlayer->_state == PLAYER_STATE::DISCONNECT)
		{
			ReleaseSRWLockShared(&damagedPlayer->_lock);
			return;
		}
		CSector* pDamagedSector = damagedPlayer->_pSector;
		__int64 damagedID = damagedPlayer->_playerID;
		char damagedHp = damagedPlayer->_hp;
		ReleaseSRWLockShared(&damagedPlayer->_lock);

		CLanSendPacket* damagePacket = CLanSendPacket::Alloc();
		int damageSetRet = SetSCPacket_DAMAGE(damagePacket, attackPlayerID, damagedID, damagedHp);
		ReqSendAroundSector(damagePacket, pDamagedSector);
	}
}

inline void CGameServer::HandleCSPacket_ATTACK3(CLanMsg* recvPacket, CPlayer* pPlayer)
{
	BYTE direction;
	short x;
	short y;
	GetCSPacket_ATTACK3(recvPacket, direction, x, y);

	AcquireSRWLockShared(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockShared(&pPlayer->_lock);
		return;
	}
	__int64 sessionID = pPlayer->_sessionID;
	__int64 attackPlayerID = pPlayer->_playerID;
	short playerX = pPlayer->_x;
	short playerY = pPlayer->_y;
	ReleaseSRWLockShared(&pPlayer->_lock);

	if (abs(playerX - x) > dfERROR_RANGE || abs(playerY - y) > dfERROR_RANGE)
	{
		// pPlayer->PushStateForDebug(__LINE__);
		CLanSendPacket* syncPacket = CLanSendPacket::Alloc();
		int setRet = SetSCPacket_SYNC(syncPacket, attackPlayerID, playerX, playerY);
		ReqSendUnicast(syncPacket, sessionID);

		x = playerX;
		y = playerY;
	}

	CPlayer* damagedPlayer = nullptr;
	if (!SetPlayerAttack3(pPlayer, damagedPlayer, direction, x, y)) return;

	AcquireSRWLockShared(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockShared(&pPlayer->_lock);
		return;
	}
	CSector* pAttackSector = pPlayer->_pSector;
	unsigned char playerDir = pPlayer->_direction;
	playerX = pPlayer->_x;
	playerY = pPlayer->_y;
	ReleaseSRWLockShared(&pPlayer->_lock);

	CLanSendPacket* attackPacket = CLanSendPacket::Alloc();
	int attackSetRet = SetSCPacket_ATTACK3(attackPacket, attackPlayerID, playerDir, playerX, playerY);
	ReqSendAroundSector(attackPacket, pAttackSector);

	if (damagedPlayer != nullptr)
	{
		AcquireSRWLockShared(&damagedPlayer->_lock);
		if (damagedPlayer->_state == PLAYER_STATE::DISCONNECT)
		{
			ReleaseSRWLockShared(&damagedPlayer->_lock);
			return;
		}
		CSector* pDamagedSector = damagedPlayer->_pSector;
		__int64 damagedID = damagedPlayer->_playerID;
		char damagedHp = damagedPlayer->_hp;
		ReleaseSRWLockShared(&damagedPlayer->_lock);

		CLanSendPacket* damagePacket = CLanSendPacket::Alloc();
		int damageSetRet = SetSCPacket_DAMAGE(damagePacket, attackPlayerID, damagedID, damagedHp);
		ReqSendAroundSector(damagePacket, pDamagedSector);
	}
}

inline void CGameServer::HandleCSPacket_ECHO(CLanMsg* recvPacket, CPlayer* pPlayer)
{
	int time;
	GetCSPacket_ECHO(recvPacket, time);

	CLanSendPacket* echoPacket = CLanSendPacket::Alloc();
	int setRet = SetSCPacket_ECHO(echoPacket, time);

	AcquireSRWLockShared(&pPlayer->_lock);
	__int64 sessionID = pPlayer->_sessionID;
	ReleaseSRWLockShared(&pPlayer->_lock);
	ReqSendUnicast(echoPacket, sessionID);
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

inline bool CGameServer::SetPlayerMoveStart(CPlayer* pPlayer, unsigned char& moveDirection, short& x, short& y)
{
	AcquireSRWLockExclusive(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockExclusive(&pPlayer->_lock);
		return false;
	}

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

	ReleaseSRWLockExclusive(&pPlayer->_lock);

	return true;
}

inline bool CGameServer::SetPlayerMoveStop(CPlayer* pPlayer, unsigned char& direction, short& x, short& y)
{
	AcquireSRWLockExclusive(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockExclusive(&pPlayer->_lock);
		return false;
	}
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_move = false;
	pPlayer->_direction = direction;
	ReleaseSRWLockExclusive(&pPlayer->_lock);

	return true;
}

inline bool CGameServer::SetPlayerAttack1(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y)
{
	AcquireSRWLockExclusive(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockExclusive(&pPlayer->_lock);
		return false;
	}
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;
	short attackerX = pPlayer->_x;
	short attackerY = pPlayer->_y;
	CSector* pSector = pPlayer->_pSector;
	ReleaseSRWLockExclusive(&pPlayer->_lock);

	if (direction == dfMOVE_DIR_LL)
	{
		vector<CPlayer*>::iterator iter;
		AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);
		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();

		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				ReleaseSRWLockShared(&(*iter)->_lock);

				int dist = attackerX - targetX;
				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
				{
					AcquireSRWLockExclusive(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockExclusive(&(*iter)->_lock);
						continue;
					}
					(*iter)->_hp -= dfATTACK1_DAMAGE;
					char targetHp = (*iter)->_hp;
					pDamagedPlayer = (*iter);
					ReleaseSRWLockExclusive(&(*iter)->_lock);

					ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);

					if (targetHp <= 0)
					{
						InterlockedIncrement(&_deadCnt);
						SetPlayerDead(pDamagedPlayer, false);
					}

					return true;
				}
			}
		}
		ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);

		if (attackerX <= pSector->_xPosMin + dfATTACK1_RANGE_X)
		{
			AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_LL]->_lock);
			iter = pSector->_around[dfMOVE_DIR_LL]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_LL]->_players.end(); iter++)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				ReleaseSRWLockShared(&(*iter)->_lock);

				int dist = attackerX - targetX;

				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
				{
					AcquireSRWLockExclusive(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockExclusive(&(*iter)->_lock);
						continue;
					}
					(*iter)->_hp -= dfATTACK1_DAMAGE;
					char targetHp = (*iter)->_hp;
					pDamagedPlayer = (*iter);
					ReleaseSRWLockExclusive(&(*iter)->_lock);

					ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LL]->_lock);

					if (targetHp <= 0)
					{
						InterlockedIncrement(&_deadCnt);
						SetPlayerDead(pDamagedPlayer, false);
					}

					return true;
				}
			}
			ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LL]->_lock);

			if (attackerY <= pSector->_yPosMin + dfATTACK1_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_LD]->_lock);
				iter = pSector->_around[dfMOVE_DIR_LD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LD]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = attackerX - targetX;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LD]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LD]->_lock);
			}
			else if (pPlayer->_y >= pSector->_yPosMax - dfATTACK1_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_LU]->_lock);
				iter = pSector->_around[dfMOVE_DIR_LU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LU]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = attackerX - targetX;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LU]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LU]->_lock);
			}
		}
		else
		{
			if (attackerY <= pSector->_yPosMin + dfATTACK1_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = attackerX - targetX;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);
			}
			else if (attackerY >= pSector->_yPosMax - dfATTACK1_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = attackerX - targetX;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);
			}
		}
	}
	else if (direction == dfMOVE_DIR_RR)
	{
		vector<CPlayer*>::iterator iter;
		AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);
		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				ReleaseSRWLockShared(&(*iter)->_lock);

				int dist = targetX - attackerX;
				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
				{
					AcquireSRWLockExclusive(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockExclusive(&(*iter)->_lock);
						continue;
					}
					(*iter)->_hp -= dfATTACK1_DAMAGE;
					char targetHp = (*iter)->_hp;
					pDamagedPlayer = (*iter);
					ReleaseSRWLockExclusive(&(*iter)->_lock);

					ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);

					if (targetHp <= 0)
					{
						InterlockedIncrement(&_deadCnt);
						SetPlayerDead(pDamagedPlayer, false);
					}

					return true;
				}
			}
		}
		ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);

		if (attackerX >= pSector->_xPosMax - dfATTACK1_RANGE_X)
		{
			AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_RR]->_lock);
			iter = pSector->_around[dfMOVE_DIR_RR]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_RR]->_players.end(); iter++)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				ReleaseSRWLockShared(&(*iter)->_lock);

				int dist = targetX - attackerX;
				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
				{
					AcquireSRWLockExclusive(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockExclusive(&(*iter)->_lock);
						continue;
					}
					(*iter)->_hp -= dfATTACK1_DAMAGE;
					char targetHp = (*iter)->_hp;
					pDamagedPlayer = (*iter);
					ReleaseSRWLockExclusive(&(*iter)->_lock);

					ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RR]->_lock);

					if (targetHp <= 0)
					{
						InterlockedIncrement(&_deadCnt);
						SetPlayerDead(pDamagedPlayer, false);
					}

					return true;
				}
			}
			ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RR]->_lock);

			if (attackerY >= pSector->_yPosMax - dfATTACK1_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_RU]->_lock);
				iter = pSector->_around[dfMOVE_DIR_RU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RU]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = targetX - attackerX;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RU]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RU]->_lock);
			}
			else if (attackerY <= pSector->_yPosMin + dfATTACK1_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_RD]->_lock);
				iter = pSector->_around[dfMOVE_DIR_RD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RD]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = targetX - attackerX;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RD]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RD]->_lock);
			}
		}
		else
		{
			if (attackerY >= pSector->_yPosMax - dfATTACK1_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = targetX - attackerX;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);
			}
			else if (attackerY <= pSector->_yPosMin + dfATTACK1_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = targetX - attackerX;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK1_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK1_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);
			}
		}
	}
}

inline bool CGameServer::SetPlayerAttack2(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y)
{
	AcquireSRWLockExclusive(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockExclusive(&pPlayer->_lock);
		return false;
	}
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;
	short attackerX = pPlayer->_x;
	short attackerY = pPlayer->_y;
	CSector* pSector = pPlayer->_pSector;
	ReleaseSRWLockExclusive(&pPlayer->_lock);

	if (direction == dfMOVE_DIR_LL)
	{
		vector<CPlayer*>::iterator iter;
		AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);
		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();

		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				ReleaseSRWLockShared(&(*iter)->_lock);

				int dist = attackerX - targetX;
				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
				{
					AcquireSRWLockExclusive(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockExclusive(&(*iter)->_lock);
						continue;
					}
					(*iter)->_hp -= dfATTACK2_DAMAGE;
					char targetHp = (*iter)->_hp;
					pDamagedPlayer = (*iter);
					ReleaseSRWLockExclusive(&(*iter)->_lock);

					ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);

					if (targetHp <= 0)
					{
						InterlockedIncrement(&_deadCnt);
						SetPlayerDead(pDamagedPlayer, false);
					}

					return true;
				}
			}
		}
		ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);

		if (attackerX <= pSector->_xPosMin + dfATTACK2_RANGE_X)
		{
			AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_LL]->_lock);
			iter = pSector->_around[dfMOVE_DIR_LL]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_LL]->_players.end(); iter++)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				ReleaseSRWLockShared(&(*iter)->_lock);

				int dist = attackerX - targetX;

				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
				{
					AcquireSRWLockExclusive(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockExclusive(&(*iter)->_lock);
						continue;
					}
					(*iter)->_hp -= dfATTACK2_DAMAGE;
					char targetHp = (*iter)->_hp;
					pDamagedPlayer = (*iter);
					ReleaseSRWLockExclusive(&(*iter)->_lock);

					ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LL]->_lock);

					if (targetHp <= 0)
					{
						InterlockedIncrement(&_deadCnt);
						SetPlayerDead(pDamagedPlayer, false);
					}

					return true;
				}
			}
			ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LL]->_lock);

			if (attackerY <= pSector->_yPosMin + dfATTACK2_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_LD]->_lock);
				iter = pSector->_around[dfMOVE_DIR_LD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LD]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = attackerX - targetX;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LD]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LD]->_lock);
			}
			else if (pPlayer->_y >= pSector->_yPosMax - dfATTACK2_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_LU]->_lock);
				iter = pSector->_around[dfMOVE_DIR_LU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LU]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = attackerX - targetX;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LU]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LU]->_lock);
			}
		}
		else
		{
			if (attackerY <= pSector->_yPosMin + dfATTACK2_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = attackerX - targetX;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);
			}
			else if (attackerY >= pSector->_yPosMax - dfATTACK2_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = attackerX - targetX;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);
			}
		}
	}
	else if (direction == dfMOVE_DIR_RR)
	{
		vector<CPlayer*>::iterator iter;
		AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);
		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				ReleaseSRWLockShared(&(*iter)->_lock);

				int dist = targetX - attackerX;
				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
				{
					AcquireSRWLockExclusive(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockExclusive(&(*iter)->_lock);
						continue;
					}
					(*iter)->_hp -= dfATTACK2_DAMAGE;
					char targetHp = (*iter)->_hp;
					pDamagedPlayer = (*iter);
					ReleaseSRWLockExclusive(&(*iter)->_lock);

					ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);

					if (targetHp <= 0)
					{
						InterlockedIncrement(&_deadCnt);
						SetPlayerDead(pDamagedPlayer, false);
					}

					return true;
				}
			}
		}
		ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);

		if (attackerX >= pSector->_xPosMax - dfATTACK2_RANGE_X)
		{
			AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_RR]->_lock);
			iter = pSector->_around[dfMOVE_DIR_RR]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_RR]->_players.end(); iter++)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				ReleaseSRWLockShared(&(*iter)->_lock);

				int dist = targetX - attackerX;
				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
				{
					AcquireSRWLockExclusive(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockExclusive(&(*iter)->_lock);
						continue;
					}
					(*iter)->_hp -= dfATTACK2_DAMAGE;
					char targetHp = (*iter)->_hp;
					pDamagedPlayer = (*iter);
					ReleaseSRWLockExclusive(&(*iter)->_lock);

					ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RR]->_lock);

					if (targetHp <= 0)
					{
						InterlockedIncrement(&_deadCnt);
						SetPlayerDead(pDamagedPlayer, false);
					}

					return true;
				}
			}
			ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RR]->_lock);

			if (attackerY >= pSector->_yPosMax - dfATTACK2_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_RU]->_lock);
				iter = pSector->_around[dfMOVE_DIR_RU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RU]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = targetX - attackerX;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RU]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RU]->_lock);
			}
			else if (attackerY <= pSector->_yPosMin + dfATTACK2_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_RD]->_lock);
				iter = pSector->_around[dfMOVE_DIR_RD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RD]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = targetX - attackerX;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RD]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RD]->_lock);
			}
		}
		else
		{
			if (attackerY >= pSector->_yPosMax - dfATTACK2_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = targetX - attackerX;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);
			}
			else if (attackerY <= pSector->_yPosMin + dfATTACK2_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = targetX - attackerX;
					if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK2_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK2_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);
			}
		}
	}
}

inline bool CGameServer::SetPlayerAttack3(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y)
{
	AcquireSRWLockExclusive(&pPlayer->_lock);
	if (pPlayer->_state != PLAYER_STATE::CONNECT)
	{
		ReleaseSRWLockExclusive(&pPlayer->_lock);
		return false;
	}
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;
	short attackerX = pPlayer->_x;
	short attackerY = pPlayer->_y;
	CSector* pSector = pPlayer->_pSector;
	ReleaseSRWLockExclusive(&pPlayer->_lock);

	if (direction == dfMOVE_DIR_LL)
	{
		vector<CPlayer*>::iterator iter;
		AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);
		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();

		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				ReleaseSRWLockShared(&(*iter)->_lock);

				int dist = attackerX - targetX;
				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
				{
					AcquireSRWLockExclusive(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockExclusive(&(*iter)->_lock);
						continue;
					}
					(*iter)->_hp -= dfATTACK3_DAMAGE;
					char targetHp = (*iter)->_hp;
					pDamagedPlayer = (*iter);
					ReleaseSRWLockExclusive(&(*iter)->_lock);

					ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);

					if (targetHp <= 0)
					{
						InterlockedIncrement(&_deadCnt);
						SetPlayerDead(pDamagedPlayer, false);
					}

					return true;
				}
			}
		}
		ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);

		if (attackerX <= pSector->_xPosMin + dfATTACK3_RANGE_X)
		{
			AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_LL]->_lock);
			iter = pSector->_around[dfMOVE_DIR_LL]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_LL]->_players.end(); iter++)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				ReleaseSRWLockShared(&(*iter)->_lock);

				int dist = attackerX - targetX;

				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
				{
					AcquireSRWLockExclusive(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockExclusive(&(*iter)->_lock);
						continue;
					}
					(*iter)->_hp -= dfATTACK3_DAMAGE;
					char targetHp = (*iter)->_hp;
					pDamagedPlayer = (*iter);
					ReleaseSRWLockExclusive(&(*iter)->_lock);

					ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LL]->_lock);

					if (targetHp <= 0)
					{
						InterlockedIncrement(&_deadCnt);
						SetPlayerDead(pDamagedPlayer, false);
					}

					return true;
				}
			}
			ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LL]->_lock);

			if (attackerY <= pSector->_yPosMin + dfATTACK3_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_LD]->_lock);
				iter = pSector->_around[dfMOVE_DIR_LD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LD]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = attackerX - targetX;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LD]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LD]->_lock);
			}
			else if (pPlayer->_y >= pSector->_yPosMax - dfATTACK3_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_LU]->_lock);
				iter = pSector->_around[dfMOVE_DIR_LU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_LU]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = attackerX - targetX;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LU]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_LU]->_lock);
			}
		}
		else
		{
			if (attackerY <= pSector->_yPosMin + dfATTACK3_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = attackerX - targetX;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);
			}
			else if (attackerY >= pSector->_yPosMax - dfATTACK3_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = attackerX - targetX;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);
			}
		}
	}
	else if (direction == dfMOVE_DIR_RR)
	{
		vector<CPlayer*>::iterator iter;
		AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);
		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				ReleaseSRWLockShared(&(*iter)->_lock);

				int dist = targetX - attackerX;
				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
				{
					AcquireSRWLockExclusive(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockExclusive(&(*iter)->_lock);
						continue;
					}
					(*iter)->_hp -= dfATTACK3_DAMAGE;
					char targetHp = (*iter)->_hp;
					pDamagedPlayer = (*iter);
					ReleaseSRWLockExclusive(&(*iter)->_lock);

					ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);

					if (targetHp <= 0)
					{
						InterlockedIncrement(&_deadCnt);
						SetPlayerDead(pDamagedPlayer, false);
					}

					return true;
				}
			}
		}
		ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_INPLACE]->_lock);

		if (attackerX >= pSector->_xPosMax - dfATTACK3_RANGE_X)
		{
			AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_RR]->_lock);
			iter = pSector->_around[dfMOVE_DIR_RR]->_players.begin();
			for (; iter < pSector->_around[dfMOVE_DIR_RR]->_players.end(); iter++)
			{
				AcquireSRWLockShared(&(*iter)->_lock);
				if ((*iter)->_state != PLAYER_STATE::CONNECT)
				{
					ReleaseSRWLockShared(&(*iter)->_lock);
					continue;
				}
				short targetX = (*iter)->_x;
				short targetY = (*iter)->_y;
				ReleaseSRWLockShared(&(*iter)->_lock);

				int dist = targetX - attackerX;
				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
				{
					AcquireSRWLockExclusive(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockExclusive(&(*iter)->_lock);
						continue;
					}
					(*iter)->_hp -= dfATTACK3_DAMAGE;
					char targetHp = (*iter)->_hp;
					pDamagedPlayer = (*iter);
					ReleaseSRWLockExclusive(&(*iter)->_lock);

					ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RR]->_lock);

					if (targetHp <= 0)
					{
						InterlockedIncrement(&_deadCnt);
						SetPlayerDead(pDamagedPlayer, false);
					}

					return true;
				}
			}
			ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RR]->_lock);

			if (attackerY >= pSector->_yPosMax - dfATTACK3_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_RU]->_lock);
				iter = pSector->_around[dfMOVE_DIR_RU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RU]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = targetX - attackerX;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RU]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RU]->_lock);
			}
			else if (attackerY <= pSector->_yPosMin + dfATTACK3_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_RD]->_lock);
				iter = pSector->_around[dfMOVE_DIR_RD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RD]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = targetX - attackerX;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RD]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_RD]->_lock);
			}
		}
		else
		{
			if (attackerY >= pSector->_yPosMax - dfATTACK3_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);
				iter = pSector->_around[dfMOVE_DIR_UU]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_UU]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = targetX - attackerX;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_UU]->_lock);
			}
			else if (attackerY <= pSector->_yPosMin + dfATTACK3_RANGE_Y)
			{
				AcquireSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);
				iter = pSector->_around[dfMOVE_DIR_DD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_DD]->_players.end(); iter++)
				{
					AcquireSRWLockShared(&(*iter)->_lock);
					if ((*iter)->_state != PLAYER_STATE::CONNECT)
					{
						ReleaseSRWLockShared(&(*iter)->_lock);
						continue;
					}
					short targetX = (*iter)->_x;
					short targetY = (*iter)->_y;
					ReleaseSRWLockShared(&(*iter)->_lock);

					int dist = targetX - attackerX;
					if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
						abs(targetY - attackerY) <= dfATTACK3_RANGE_Y)
					{
						AcquireSRWLockExclusive(&(*iter)->_lock);
						if ((*iter)->_state != PLAYER_STATE::CONNECT)
						{
							ReleaseSRWLockExclusive(&(*iter)->_lock);
							continue;
						}
						(*iter)->_hp -= dfATTACK3_DAMAGE;
						char targetHp = (*iter)->_hp;
						pDamagedPlayer = (*iter);
						ReleaseSRWLockExclusive(&(*iter)->_lock);

						ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);

						if (targetHp <= 0)
						{
							InterlockedIncrement(&_deadCnt);
							SetPlayerDead(pDamagedPlayer, false);
						}

						return true;
					}
				}
				ReleaseSRWLockShared(&pSector->_around[dfMOVE_DIR_DD]->_lock);
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
	InterlockedIncrement(&_syncCnt);

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
