#include "CChattingServer.h"
#include "CSystemLog.h"
#include <tchar.h>

#define _MONITOR
// #define _TIMEOUT

bool CChattingServer::Initialize()
{
	// Set Job
	_pJobPool = new CLockFreePool<CJob>(dfJOB_DEF, true);

	// Set Player
	int idx = 0;
	for (int i = 0; i < dfTHREAD_NUM; i++)
	{
		for (int j = 0; j < dfPLAYER_PER_THREAD; j++)
		{
			CPlayer* pPlayer = new CPlayer(idx);
			_players[i][j] = pPlayer;
			_usablePlayerIdx.Push(idx);
			idx++;
		}
	}
	InitializeSRWLock(&_playersLock);

	// Set Sector
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
	for (int i = 0; i < dfTHREAD_NUM; i++)
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

	for (int i = 0; i < dfTHREAD_NUM; i++)
	{
		InterlockedExchange(&_signal[i], 1);
		WakeByAddressSingle(&_signal[i]);
	}
	WaitForMultipleObjects(dfTHREAD_NUM, _updateThread, true, INFINITE);

#ifdef _MONITOR
	WaitForSingleObject(_monitorThread, INFINITE);
#endif

#ifdef _TIMEOUT
	WaitForSingleObject(_timeoutThread, INFINITE);
#endif

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
	CPlayer* pPlayer = _players[idx / dfPLAYER_PER_THREAD][idx % dfPLAYER_PER_THREAD];

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
	int threadIdx = pPlayer->_idx._threadIdx;
	ReleaseSRWLockShared(&_playersLock);

	InterlockedIncrement(&_signal[threadIdx]);
	WakeByAddressSingle(&_signal[threadIdx]);
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
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
		::wprintf(L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
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
		WaitOnAddress(&pServer->_signal[idx], &undesired, sizeof(long), INFINITE);

		for (int i = 0; i < dfPLAYER_PER_THREAD; i++)
		{
			CPlayer* pPlayer = pServer->_players[idx][i];
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
							LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
							::wprintf(L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
							pServer->Disconnect(job->_sessionID);
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
		swprintf_s(text, dfMONITOR_TEXT_LEN, L"[%s %02d:%02d:%02d]\n\nTotal Accept: %d\nTotal Disconnect: %d\nConnected Session: %d\nRecv/1sec: %d\nSend/1sec: %d\nAccept/1sec: %d\nDisconnect/1sec: %d\nPacket Pool: %d/%d\n\n",
			_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond, pServer->GetAcceptTotal(), pServer->GetDisconnectTotal(), pServer->GetSessionCount(), pServer->GetRecvMsgTPS(), pServer->GetSendMsgTPS(), pServer->GetAcceptTPS(), pServer->GetDisconnectTPS(), CPacket::GetPoolSize(), CPacket::GetNodeCount());
		::wprintf(L"%s", text);

		pServer->ResetMonitorData();

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
		for (int i = 0; i < dfTHREAD_NUM; i++)
		{
			for (int j = 0; j < dfPLAYER_PER_THREAD; j++)
			{
				CPlayer* pPlayer = pServer->_players[i][j];
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
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		::wprintf(L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		return;
	}

	if (sectorX > dfSECTOR_CNT_X && sectorY > dfSECTOR_CNT_Y)
	{
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

	CSector* inSector = &_sectors[player->_sectorY][player->_sectorX];
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
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		::wprintf(L"%s[%d] Account No is Different: %lld != %lld\n", _T(__FUNCTION__), __LINE__, player->_accountNo, accountNo);
		return;
	}

	CPacket* SCpacket = CPacket::Alloc();
	SCpacket->Clear();
	SetSCPacket_RES_MESSAGE(SCpacket, player->_accountNo, player->_ID, player->_nickname, messageLen, message);

	if (player->_sectorX != -1 && player->_sectorY != -1)
	{
		ReqSendAroundSector(SCpacket, &_sectors[player->_sectorY][player->_sectorX]);
	}

	delete[] message; // new[] - delete[]�� �ٸ� �Լ����� �ϴ� �� ����� ���ڴ�...
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