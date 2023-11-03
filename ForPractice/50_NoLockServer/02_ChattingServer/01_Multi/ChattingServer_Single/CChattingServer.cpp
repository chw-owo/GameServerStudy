#include "CChattingServer.h"
#include "CSystemLog.h"
#include <tchar.h>

#define _MONITOR
// #define _TIMEOUT

bool CChattingServer::Initialize()
{
	// Set Config =======================================================================================

	int monitorThread = 0;
	int timeoutThread = 0;

#ifdef _MONITOR
	monitorThread = 1;
#endif

#ifdef _TIMEOUT
	timeoutThread = 1;
#endif

	int exceptThread = monitorThread + timeoutThread;

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	int cpuCount = (int)si.dwNumberOfProcessors;
	int networkThreadCnt = 0;

	::wprintf(L"CPU total: %d\n", cpuCount);
	::wprintf(L"Network Thread Count (Except Accept Thread) (recommend under %d): ", cpuCount - 1);
	::scanf_s("%d", &networkThreadCnt);
	::wprintf(L"Content Thread Count (Except Monitor, Timeout Thread) (recommend under %d, player max: %d): ", cpuCount - networkThreadCnt - exceptThread - 1, dfPLAYER_MAX);
	::scanf_s("%d", &_updateThreadCnt);

	_playerPerThread = dfPLAYER_MAX / _updateThreadCnt; 
	// TO-DO: 나누어 떨어지지 않는 경우에 대한 처리 필요

	// Set Job ===========================================================================================

	_pJobPool = new CLockFreePool<CJob>(dfJOB_DEF, true);

	// Set Player ========================================================================================

	int idx = 0;

	_players = new CPlayer*[_updateThreadCnt];
	for (int i = 0; i < _updateThreadCnt; i++)
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

	// Set Sector ========================================================================================

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
	
	
	// Set Threads ========================================================================================

	_signal = new long[_updateThreadCnt];
	_updateThread = new HANDLE[_updateThreadCnt];
	for (int i = 0; i < _updateThreadCnt; i++)
	{
		ThreadArg* arg = new ThreadArg(this, i);
		_updateThread[i] = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, arg, 0, nullptr);
		if (_updateThread[i] == NULL)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Logic Thread Error\n", _T(__FUNCTION__), __LINE__);
			::wprintf(L"%s[%d]: Begin Logic Thread Error\n", _T(__FUNCTION__), __LINE__);
			return false;
		}
	}

	if (!NetworkInitialize(dfSERVER_IP, dfSERVER_PORT, networkThreadCnt, false))
	{
		Terminate();
		return false;
	}

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"\n<Server Setting>\nCPU Count: %d\nAccpet Thread: 1\nNetwork Thread: %d\nUpdate Thread: %d\nMonitor Thread: %d\nTimeout Thread: %d\n\n", cpuCount, networkThreadCnt, _updateThreadCnt, monitorThread, timeoutThread);
	::wprintf(L"\n<Server Setting>\nCPU Count: %d\nAccpet Thread: 1\nNetwork Thread: %d\nUpdate Thread: %d\nMonitor Thread: %d\nTimeout Thread: %d\n\n", cpuCount, networkThreadCnt, _updateThreadCnt, monitorThread, timeoutThread);

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Server Initialize\n");
	::wprintf(L"Server Initialize\n\n");

#ifdef _MONITOR

	_monitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, nullptr);
	if (_monitorThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
#endif

#ifdef _TIMEOUT

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

	for (int i = 0; i < _updateThreadCnt; i++)
	{
		long ret = InterlockedIncrement(&_signal[i]);
		if (ret == 1) WakeByAddressSingle(&_signal[i]);
	}
	WaitForMultipleObjects(_updateThreadCnt, _updateThread, true, INFINITE);
	delete[] _signal;
	delete[] _updateThread;

#ifdef _MONITOR
	WaitForSingleObject(_monitorThread, INFINITE);
#endif

#ifdef _TIMEOUT
	WaitForSingleObject(_timeoutThread, INFINITE);
#endif
	
	for (int i = 0; i < _updateThreadCnt; i++)
		delete[] _players[i];	
	delete[] _players;

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Server Terminate.\n");
	::wprintf(L"Server Terminate.\n");
}

void CChattingServer::OnInitialize()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Library Initialize\n");
	::wprintf(L"Network Library Initialize\n");
}

void CChattingServer::OnTerminate()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Library Terminate\n");
	::wprintf(L"Network Library Terminate\n");
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

void CChattingServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
	// LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s (%d)\n", debugMsg, debugCode);
	// ::wprintf(L"%s (%d)\n", debugMsg, debugCode);
}

bool CChattingServer::OnConnectRequest()
{
	return false;
}

void CChattingServer::OnAcceptClient(__int64 sessionID)
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

void CChattingServer::OnReleaseClient(__int64 sessionID)
{
	AcquireSRWLockExclusive(&_playersLock);
	unordered_map<__int64, CPlayer*>::iterator mapIter = _playersMap.find(sessionID);
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
		CJob* job = pPlayer->_jobQ.Dequeue();
		_pJobPool->Free(job);
	}
	DWORD x = pPlayer->_sectorX;
	DWORD y = pPlayer->_sectorY;
	int idx = pPlayer->CleanUp();

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

void CChattingServer::OnRecv(__int64 sessionID, CPacket* packet)
{
	packet->AddUsageCount(1);
	CJob* job = _pJobPool->Alloc();
	job->Setting(sessionID, packet);

	AcquireSRWLockShared(&_playersLock);
	unordered_map<__int64, CPlayer*>::iterator mapIter = _playersMap.find(sessionID);
	if (mapIter == _playersMap.end())
	{
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d]: No Session %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		CPacket::Free(packet);
		ReleaseSRWLockExclusive(&_playersLock);
		return;
	}

	CPlayer* pPlayer = mapIter->second;
	pPlayer->_jobQ.Enqueue(job);
	InterlockedIncrement(&_jobQSize);
	int threadIdx = pPlayer->_idx._threadIdx;
	ReleaseSRWLockShared(&_playersLock);

	long ret = InterlockedIncrement(&_signal[threadIdx]);
	if (ret == 1) WakeByAddressSingle(&_signal[threadIdx]);
}

void CChattingServer::OnSend(__int64 sessionID, int sendSize)
{
}

void CChattingServer::HandleRecv(CPlayer* pPlayer, CPacket* packet)
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

	InterlockedIncrement(&_handlePacketTPS);
}

unsigned int __stdcall CChattingServer::UpdateThread(void* arg)
{
	ThreadArg* threadArg = (ThreadArg*)arg;
	CChattingServer* pServer = threadArg->_pServer;
	int idx = threadArg->_threadIdx;
	long undesired = 0;

	while (pServer->_serverAlive)
	{
		WaitOnAddress(&pServer->_signal[idx], &undesired, sizeof(long), INFINITE);
		InterlockedIncrement(&pServer->_updateThreadWakeTPS);

		for (int i = 0; i < pServer->_playerPerThread; i++)
		{
			CPlayer* pPlayer = &pServer->_players[idx][i];
			AcquireSRWLockExclusive(&pPlayer->_lock);
			
			while (pPlayer->_jobQ.GetUseSize() > 0)
			{
				CJob* job = pPlayer->_jobQ.Dequeue();
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
				}

				CPacket::Free(job->_packet);
				pServer->_pJobPool->Free(job);
				InterlockedDecrement(&pServer->_signal[idx]);
			}	

			ReleaseSRWLockExclusive(&pPlayer->_lock);
		}
	}
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Update Thread (%d) Terminate\n", GetCurrentThreadId());
	::wprintf(L"Update Thread (%d) Terminate\n", GetCurrentThreadId());

	return 0;
}

unsigned int __stdcall CChattingServer::MonitorThread(void* arg)
{
	CChattingServer* pServer = (CChattingServer*)arg;
	CreateDirectory(L"MonitorLog", NULL);

	while (pServer->_serverAlive)
	{		
		pServer->UpdateMonitorData();

		SYSTEMTIME stTime;
		GetLocalTime(&stTime);
		WCHAR text[dfMONITOR_TEXT_LEN];
		swprintf_s(text, dfMONITOR_TEXT_LEN, L"[%s %02d:%02d:%02d]\n\nConnected Session: %d\nPacket Pool: %d/%d\n\nUpdate TPS: %d\nRequested Packet: %d\nHandled Packet: %d\nJob Pool: %d/%d\n\nPlayer Count: %d\nPlayer Pool: None\n\nTotal Accept: %d\nTotal Disconnect: %d\nRecv/1sec: %d\nSend/1sec: %d\nAccept/1sec: %d\nDisconnect/1sec: %d\n\n",
			_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond,
			pServer->GetSessionCount(), CPacket::GetPoolSize(), CPacket::GetNodeCount(),
			pServer->_updateThreadWakeTPS, pServer->_jobQSize, pServer->_handlePacketTPS, pServer->_pJobPool->GetPoolSize(), pServer->_pJobPool->GetNodeCount(), pServer->_playersMap.size(),
			pServer->GetAcceptTotal(), pServer->GetDisconnectTotal(), pServer->GetRecvMsgTPS(), pServer->GetSendMsgTPS(), pServer->GetAcceptTPS(), pServer->GetDisconnectTPS());

		::wprintf(L"%s", text);

		pServer->ResetMonitorData();
		pServer->_updateThreadWakeTPS = 0;
		pServer->_handlePacketTPS = 0;
		pServer->_jobQSize = 0;

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

unsigned int __stdcall CChattingServer::TimeoutThread(void* arg)
{
	CChattingServer* pServer = (CChattingServer*)arg;

	while (pServer->_serverAlive)
	{
		Sleep(dfTIMEOUT);
		for (int i = 0; i < pServer->_updateThreadCnt; i++)
		{
			for (int j = 0; j < pServer->_playerPerThread; j++)
			{
				CPlayer* pPlayer = &pServer->_players[i][j];
				AcquireSRWLockShared(&pPlayer->_lock);
				DWORD lastRecvTime = pPlayer->_lastRecvTime;
				__int64 sessionID = pPlayer->_sessionID;
				ReleaseSRWLockShared(&pPlayer->_lock);

				if ((timeGetTime() - lastRecvTime) >= dfTIMEOUT)
					pServer->Disconnect(sessionID);
			}
		}
	}

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Timeout Thread (%d) Terminate\n", GetCurrentThreadId());
	::wprintf(L"Timeout Thread (%d) Terminate\n", GetCurrentThreadId());

	return 0;
}

void CChattingServer::ReqSendUnicast(CPacket* packet, __int64 sessionID)
{
	packet->AddUsageCount(1);
	if (!SendPacket(sessionID, packet))
	{
		CPacket::Free(packet);
	}
}

void CChattingServer::ReqSendAroundSector(CPacket* packet, CSector* centerSector, CPlayer* pExpPlayer)
{
	vector<__int64> sendID;

	if (pExpPlayer == nullptr)
	{
		vector<CSector*>::iterator iter = centerSector->_around.begin();
		for (; iter < centerSector->_around.end(); iter++)
		{
			AcquireSRWLockShared(&(*iter)->_lock);
			vector<CPlayer*>::iterator playerIter = (*iter)->_players.begin();
			for (; playerIter < (*iter)->_players.end(); playerIter++)
			{
				sendID.push_back((*playerIter)->_sessionID);
			}
			ReleaseSRWLockShared(&(*iter)->_lock);
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
			AcquireSRWLockShared(&(*iter)->_lock);
			vector<CPlayer*>::iterator playerIter = (*iter)->_players.begin();
			for (; playerIter < (*iter)->_players.end(); playerIter++)
			{
				sendID.push_back((*playerIter)->_sessionID);
			}
			ReleaseSRWLockShared(&(*iter)->_lock);
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
			CPacket::Free(packet);
		}
		// ::wprintf(L"%lld (%d): Send Packet!\n", ((*idIter) & _idMask), GetCurrentThreadId());
	}
}

inline void CChattingServer::HandleCSPacket_REQ_LOGIN(CPacket* CSpacket, CPlayer* player)
{
	__int64 accountNo;
	wchar_t* ID = new wchar_t[dfID_LEN];
	wchar_t* nickname = new wchar_t[dfNICKNAME_LEN];
	char* sessionKey = new char[dfSESSIONKEY_LEN];
	GetCSPacket_REQ_LOGIN(CSpacket, accountNo, ID, nickname, sessionKey);

	// ::printf("%lld (%d): Login (%lld)\n", (player->_sessionID & _idMask), GetCurrentThreadId(), accountNo);

	player->_accountNo = accountNo;
	memcpy_s(player->_ID, dfID_LEN * sizeof(wchar_t), ID, dfID_LEN * sizeof(wchar_t));
	memcpy_s(player->_nickname, dfNICKNAME_LEN * sizeof(wchar_t), nickname, dfNICKNAME_LEN * sizeof(wchar_t));
	memcpy_s(player->_sessionKey, dfSESSIONKEY_LEN, sessionKey, dfSESSIONKEY_LEN);
	delete[] ID;
	delete[] nickname;
	delete[] sessionKey;
	BYTE status = 1;

	CPacket* SCpacket = CPacket::Alloc();
	SCpacket->Clear();
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
		Disconnect(player->_sessionID);
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		::wprintf(L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		return;
	}

	if (sectorX >= dfSECTOR_CNT_X || sectorY >= dfSECTOR_CNT_Y)
	{
		Disconnect(player->_sessionID);
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Sector Val is Wrong\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d] Sector Val is Wrong\n", _T(__FUNCTION__), __LINE__);
		return;
	}

	// ::printf("%lld (%d): Sector Move (%lld, [%d][%d]->[%d][%d])\n", (player->_sessionID & _idMask), GetCurrentThreadId(), accountNo, player->_sectorY, player->_sectorX, sectorY, sectorX);

	if (player->_sectorX < dfSECTOR_CNT_X && player->_sectorY < dfSECTOR_CNT_Y)
	{
		CSector* outSector = &_sectors[player->_sectorY][player->_sectorX];
		AcquireSRWLockExclusive(&outSector->_lock);
		vector<CPlayer*>::iterator iter = outSector->_players.begin();
		for (; iter < outSector->_players.end(); iter++)
		{
			if ((*iter) == player)
			{
				outSector->_players.erase(iter);
				break;
			}
		}
		ReleaseSRWLockExclusive(&outSector->_lock);
	}

	player->_sectorX = sectorX;
	player->_sectorY = sectorY;

	CSector* inSector = &_sectors[sectorY][sectorX];
	AcquireSRWLockExclusive(&inSector->_lock);
	inSector->_players.push_back(player);
	ReleaseSRWLockExclusive(&inSector->_lock);

	CPacket* SCpacket = CPacket::Alloc();
	SCpacket->Clear();
	SetSCPacket_RES_SECTOR_MOVE(SCpacket, player->_accountNo, player->_sectorX, player->_sectorY);
	ReqSendUnicast(SCpacket, player->_sessionID);
}

inline void CChattingServer::HandleCSPacket_REQ_MESSAGE(CPacket* CSpacket, CPlayer* player)
{
	__int64 accountNo;
	WORD messageLen;
	wchar_t* message;
	GetCSPacket_REQ_MESSAGE(CSpacket, accountNo, messageLen, message);

	if (player->_accountNo != accountNo)
	{
		Disconnect(player->_sessionID);
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		::wprintf(L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		return;
	}

	CPacket* SCpacket = CPacket::Alloc();
	SCpacket->Clear();
	SetSCPacket_RES_MESSAGE(SCpacket, player->_accountNo, player->_ID, player->_nickname, messageLen, message);

	if (player->_sectorX < dfSECTOR_CNT_X && player->_sectorY < dfSECTOR_CNT_Y)
	{
		ReqSendAroundSector(SCpacket, &_sectors[player->_sectorY][player->_sectorX]);
	}

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
	message = new wchar_t[messageLen / 2];
	packet->GetPayloadData((char*)message, messageLen);
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
	packet->PutPayloadData((char*)message, messageLen);
}
