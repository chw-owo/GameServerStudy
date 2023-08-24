#include "Server.h"
#include "Main.h"
#include <stdio.h>

CServer::CServer()
{
	int err;
	int bindRet;
	int listenRet;
	int ioctRet;

	srand(0);
	CreateDirectory(L"ProfileBasic", NULL);
	
	_pSessionPool = new CObjectPool<Session>(dfSESSION_MAX, false);
	_pPlayerPool = new CObjectPool<Player>(dfSESSION_MAX, true);
	
	// Initialize Winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		g_Shutdown = true;
		return;
	}

	// Set Listen Socket
	_listensock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listensock == INVALID_SOCKET)
	{
		err = WSAGetLastError();

		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, 
			L"%s[%d]: listen sock is INVALIED, %d\n", 
			_T(__FUNCTION__), __LINE__, err);

		::wprintf(L"%s[%d]: listen sock is INVALIED, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		g_Shutdown = true;
		return;
	}

	// Bind Socket
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, dfNETWORK_IP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(dfNETWORK_PORT);

	bindRet = bind(_listensock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bindRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();

		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: bind Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		::wprintf(L"%s[%d]: bind Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		g_Shutdown = true;
		return;
	}

	// Listen
	listenRet = listen(_listensock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();

		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: listen Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		::wprintf(L"%s[%d]: listen Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		g_Shutdown = true;
		return;
	}

	// Set Non-Blocking Mode
	u_long on = 1;
	ioctRet = ioctlsocket(_listensock, FIONBIO, &on);
	if (ioctRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: ioct Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		::wprintf(L"%s[%d]: ioct Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		g_Shutdown = true;
		return;
	}

	_time.tv_sec = 0;
	_time.tv_usec = 0;
	_addrlen = sizeof(SOCKADDR_IN);

	::wprintf(L"Network Setting Complete!\n");

	for (int y = 0; y < dfSECTOR_CNT_Y; y++)
	{
		for (int x = 0; x < dfSECTOR_CNT_X; x++)
		{
			_Sectors[y][x].InitializeSector(x, y);
		}
	}
	SetSectorsAroundInfo();
	_oldTick = GetTickCount64();

	::wprintf(L"Content Setting Complete!\n");

}

CServer::~CServer()
{	
	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		if (_Sessions[i] != nullptr)
		{
			closesocket(_Sessions[i]->_socket);
			_pSessionPool->Free(_Sessions[i]);
		}

		if (_Players[i] != nullptr)
			_pPlayerPool->Free(_Players[i]);
	}

	closesocket(_listensock);
	WSACleanup();

	delete _pSessionPool;
	delete _pPlayerPool;
}


void CServer::NetworkUpdate()
{ 
	PRO_BEGIN(L"Network");

	int rStopIdx = 0;
	int wStopIdx = 0;
	int rStartIdx = 0;
	int wStartIdx = 0;

	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		if (_Sessions[i] == nullptr) continue;

		_rSessions[rStopIdx++] = _Sessions[i];
		if (_Sessions[i]->_sendRBuffer.GetUseSize() > 0)
			_wSessions[wStopIdx++] = _Sessions[i];
	}

	while ((rStopIdx - rStartIdx) >= (FD_SETSIZE - 1) &&
		(wStopIdx - wStartIdx) >= FD_SETSIZE)
	{
		SelectProc(rStartIdx, (FD_SETSIZE - 1), wStartIdx, FD_SETSIZE);
		rStartIdx += (FD_SETSIZE - 1);
		wStartIdx += FD_SETSIZE;
	}

	while((rStopIdx - rStartIdx) >= (FD_SETSIZE - 1) && 
		(wStopIdx - wStartIdx) < FD_SETSIZE)
	{
		SelectProc(rStartIdx, (FD_SETSIZE - 1), 0, 0);
		rStartIdx += (FD_SETSIZE - 1);
	}

	while ((rStopIdx - rStartIdx) < (FD_SETSIZE - 1) &&
		(wStopIdx - wStartIdx) >= FD_SETSIZE)
	{
		SelectProc(0, 0, wStartIdx, FD_SETSIZE);
		wStartIdx += FD_SETSIZE;
	}

	SelectProc(rStartIdx, rStopIdx - rStartIdx, wStartIdx, wStopIdx - wStartIdx);

	PRO_END(L"Network");

	PRO_BEGIN(L"Delayed Disconnect");
	DisconnectDeadSession();
	PRO_END(L"Delayed Disconnect");	
}

void CServer::ContentUpdate()
{
	if (SkipForFixedFrame()) return;

	PRO_BEGIN(L"Content");
	
	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		if (_Players[i] == nullptr) continue;

		if (GetTickCount64() -_Players[i]->_pSession->_lastRecvTime 
				> dfNETWORK_PACKET_RECV_TIMEOUT)
		{
			_timeoutCnt++;
			SetSessionDead(_Players[i]->_pSession);
			continue;
		}

		if (_Players[i]->_move)
		{
			UpdatePlayerMove(_Players[i]);
		}
	}

	PRO_END(L"Content");
}

// About Network ==================================================

void CServer::SelectProc(int rStartIdx, int rCount, int wStartIdx, int wCount)
{
	FD_ZERO(&_rset);
	FD_ZERO(&_wset);

	FD_SET(_listensock, &_rset);
	for (int i = 0; i < wCount; i++)
		FD_SET(_wSessions[wStartIdx + i]->_socket, &_wset);
	for (int i = 0; i < rCount; i++)
		FD_SET(_rSessions[rStartIdx + i]->_socket, &_rset);

	// Select Socket Set
	int selectRet = select(0, &_rset, &_wset, NULL, &_time);
	if (selectRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: select Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		::wprintf(L"%s[%d]: select Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		g_Shutdown = true;
		return;
	}

	// Handle Selected Socket
	else if (selectRet > 0)
	{
		if (FD_ISSET(_listensock, &_rset))
			AcceptProc();

		for (int i = 0; i < wCount; i++)
			if (FD_ISSET(_wSessions[wStartIdx + i]->_socket, &_wset))
				SendProc(_wSessions[wStartIdx + i]);

		for (int i = 0; i < rCount; i++)
			if (FD_ISSET(_rSessions[rStartIdx + i]->_socket, &_rset))
				RecvProc(_rSessions[rStartIdx + i]);
	}
}

void CServer::AcceptProc()
{
	// Session Num is more than SESSION_MAX
	if (_usableCnt == 0 && _sessionIDs == dfSESSION_MAX)
		return;

	int ID;
	if (_usableCnt == 0)
		ID = _sessionIDs++;
	else
		ID = _usableSessionID[--_usableCnt];

	Session* pSession = _pSessionPool->Alloc();
	pSession->Initialize(ID);

	if (pSession == nullptr)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: new Error, %d\n",
			_T(__FUNCTION__), __LINE__);

		::wprintf(L"%s[%d]: new Error, %d\n",
			_T(__FUNCTION__), __LINE__);

		g_Shutdown = true;	
		return;
	}

	pSession->_socket = accept(_listensock, (SOCKADDR*)&pSession->_addr, &_addrlen);
	if (pSession->_socket == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: accept Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		::wprintf(L"%s[%d]: accept Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		g_Shutdown = true;
		return;
	}

	pSession->_lastRecvTime = GetTickCount64();
	_Sessions[pSession->_ID] = pSession;
	CreatePlayer(pSession);

	_acceptCnt++;
}

void CServer::RecvProc(Session* pSession)
{
	pSession->_lastRecvTime = GetTickCount64();

	PRO_BEGIN(L"Network: Recv");
	int recvRet = recv(pSession->_socket,
		pSession->_recvRBuffer.GetWritePtr(),
		pSession->_recvRBuffer.DirectEnqueueSize(), 0);
	PRO_END(L"Network: Recv");

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err == WSAECONNRESET || err == WSAECONNABORTED)
		{
			SetSessionDead(pSession, true);
			return;
		}
		else if(err != WSAEWOULDBLOCK)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: recv Error, %d\n",
				_T(__FUNCTION__), __LINE__, err);

			::wprintf(L"%s[%d]: recv Error, %d\n",
				_T(__FUNCTION__), __LINE__, err);

			SetSessionDead(pSession);
			return;
		}
	}
	else if (recvRet == 0)
	{
		SetSessionDead(pSession, true);
		return;
	}

	int moveRet = pSession->_recvRBuffer.MoveWritePos(recvRet);
	if (recvRet != moveRet)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d] Session %d - recvRBuf moveWritePos Error (req - %d, ret - %d)\n", 
			_T(__FUNCTION__), __LINE__, pSession->_ID, recvRet, moveRet);

		::wprintf(L"%s[%d] Session %d - recvRBuf moveWritePos Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, pSession->_ID, recvRet, moveRet);

		g_Shutdown = true;
		return;
	}
	int useSize = pSession->_recvRBuffer.GetUseSize();

	Player* pPlayer =_Players[pSession->_ID];
	
	if (pPlayer == nullptr)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d] Session %d player is nullptr\n", 
			_T(__FUNCTION__), __LINE__, pSession->_ID);

		::wprintf(L"%s[%d] Session %d player is nullptr\n",
			_T(__FUNCTION__), __LINE__, pSession->_ID);

		g_Shutdown = true;
		return;
	}
	
	while (useSize > 0)
	{
		if (useSize <= dfHEADER_SIZE)
			break;

		stPacketHeader header;
		int peekRet = pSession->_recvRBuffer.Peek((char*)&header, dfHEADER_SIZE);
		if (peekRet != dfHEADER_SIZE)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]  Session %d - recvRBuf Peek Error (req - %d, ret - %d)\n", 
				_T(__FUNCTION__), __LINE__, pSession->_ID, dfHEADER_SIZE, peekRet);

			::wprintf(L"%s[%d]  Session %d - recvRBuf Peek Error (req - %d, ret - %d)\n",
				_T(__FUNCTION__), __LINE__, pSession->_ID, dfHEADER_SIZE, peekRet);

			g_Shutdown = true;
			return;
		}

		if ((char) header.code != (char)dfPACKET_CODE)
		{
			
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: Session %d Wrong Header Code Error, %x\n", 
				_T(__FUNCTION__), __LINE__, pSession->_ID, header.code);

			::wprintf(L"%s[%d]: Session %d Wrong Header Code Error, %x\n",
				_T(__FUNCTION__), __LINE__, pSession->_ID, header.code);

			SetSessionDead(pSession);
			return;
		}

		if (useSize < dfHEADER_SIZE + header.size)
			break;

		int moveReadRet = pSession->_recvRBuffer.MoveReadPos(dfHEADER_SIZE);
		if (moveReadRet != dfHEADER_SIZE)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d] Session %d - recvRBuf moveReadPos Error (req - %d, ret - %d)\n",
				_T(__FUNCTION__), __LINE__, pSession->_ID, dfHEADER_SIZE, moveReadRet);

			::wprintf(L"%s[%d] Session %d - recvRBuf moveReadPos Error (req - %d, ret - %d)\n",
				_T(__FUNCTION__), __LINE__, pSession->_ID, dfHEADER_SIZE, moveReadRet);

			g_Shutdown = true;
			return;
		}
		
		bool handlePacketRet = HandleCSPackets(pPlayer, header.type);
		if (!handlePacketRet)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: Session %d Handle CS Packet Error\n", 
				_T(__FUNCTION__), __LINE__, pSession->_ID);

			::wprintf(L"%s[%d]: Session %d Handle CS Packet Error\n",
				_T(__FUNCTION__), __LINE__, pSession->_ID);

			SetSessionDead(pSession);
			return;
		}

		useSize = pSession->_recvRBuffer.GetUseSize();
	}
}

void CServer::SendProc(Session* pSession)
{
	PRO_BEGIN(L"Network: Send");
	int sendRet = send(pSession->_socket,
		pSession->_sendRBuffer.GetReadPtr(),
		pSession->_sendRBuffer.DirectDequeueSize(), 0);
	PRO_END(L"Network: Send");

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err == WSAECONNRESET || err == WSAECONNABORTED)
		{
			SetSessionDead(pSession, true);
			return;
		}
		else if (err != WSAEWOULDBLOCK)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: Session %d Send Error, %d\n", 
				_T(__FUNCTION__), __LINE__, pSession->_ID, err);

			::wprintf(L"%s[%d]: Session %d Send Error, %d\n",
				_T(__FUNCTION__), __LINE__, pSession->_ID, err);

			SetSessionDead(pSession);
			return;
		}
	}

	int moveRet = pSession->_sendRBuffer.MoveReadPos(sendRet);
	if (sendRet != moveRet)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d] Session %d - sendRBuf moveReadPos Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, pSession->_ID, sendRet, moveRet);

		::wprintf(L"%s[%d] Session %d - sendRBuf moveReadPos Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, pSession->_ID, sendRet, moveRet);

		SetSessionDead(pSession);
		return;
	}
}

void CServer::SetSessionDead(Session* pSession, bool connectEnd)
{
	if(pSession->_alive)
	{
		pSession->_alive = false;
		_disconnectSessionIDs[_disconnectCnt] = pSession->_ID;
		_disconnectCnt++;	

		if (connectEnd)
			_connectEndCnt++;
		_disconnectMonitorCnt++;
	}
}

void CServer::DisconnectDeadSession()
{
	for (int i = 0; i < _disconnectCnt; i++)
	{
		int ID = _disconnectSessionIDs[i];

		Player* pPlayer =_Players[ID];
		if (pPlayer == nullptr)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d] Session %d player is nullptr\n",
				_T(__FUNCTION__), __LINE__, ID);

			::wprintf(L"%s[%d] Session %d player is nullptr\n",
				_T(__FUNCTION__), __LINE__, ID);

			g_Shutdown = true;
			return;
		}
		_Players[ID] = nullptr;

		// Remove from Sector
		vector<Player*>::iterator vectorIter = pPlayer->_pSector->_players.begin();
		for (; vectorIter < pPlayer->_pSector->_players.end(); vectorIter++)
		{
			if ((*vectorIter) == pPlayer)
			{
				pPlayer->_pSector->_players.erase(vectorIter);
				break;
			}
		}
		
		pPlayer->_pSession->_sendSPacket.Clear();
		int deleteRet = SetSCPacket_DELETE_CHAR(&pPlayer->_pSession->_sendSPacket, pPlayer->_ID);
		EnqueueAroundSector(pPlayer->_pSession->_sendSPacket.GetReadPtr(), deleteRet, pPlayer->_pSector);
		_pPlayerPool->Free(pPlayer);
		
		Session* pSession = _Sessions[ID];
		if (pSession == nullptr)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d] Session %d is nullptr\n",
				_T(__FUNCTION__), __LINE__, ID);

			::wprintf(L"%s[%d] Session %d is nullptr\n",
				_T(__FUNCTION__), __LINE__, ID);

			g_Shutdown = true;
			return;
		}
		_Sessions[ID] = nullptr;

		closesocket(pSession->_socket);	
		_pSessionPool->Free(pSession);
		_usableSessionID[_usableCnt++] = ID;
	}

	_disconnectCnt = 0;
}

void CServer::EnqueueUnicast(char* msg, int size, Session* pSession)
{
	if (pSession == nullptr)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d] Session is nullptr\n", _T(__FUNCTION__), __LINE__);

		::wprintf(L"%s[%d] Session is nullptr\n", _T(__FUNCTION__), __LINE__);

		g_Shutdown = true;
		return;
	}

	int enqueueRet = pSession->_sendRBuffer.Enqueue(msg, size);
	if (enqueueRet != size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d] Session %d - sendRBuf Enqueue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, pSession->_ID, size, enqueueRet);

		::wprintf(L"%s[%d] Session %d - sendRBuf Enqueue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, pSession->_ID, size, enqueueRet);

		SetSessionDead(pSession);
	}
}

void CServer::EnqueueOneSector(char* msg, int size, Sector* sector, Session* pExpSession)
{
	if (pExpSession == nullptr)
	{
		vector<Player*>::iterator playerIter = sector->_players.begin();
		for (; playerIter < sector->_players.end(); playerIter++)
		{
			if ((*playerIter) == nullptr)
			{
				LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
					L"%s[%d] Player in sector[%d][%d] is nullptr\n", 
					_T(__FUNCTION__), __LINE__, sector->_xIndex, sector->_yIndex);

				::wprintf(L"%s[%d] Player in sector[%d][%d] is nullptr\n",
					_T(__FUNCTION__), __LINE__, sector->_xIndex, sector->_yIndex);

				g_Shutdown = true;
				return;
			}

			int enqueueRet = (*playerIter)->_pSession->_sendRBuffer.Enqueue(msg, size);
			if (enqueueRet != size)
			{
				LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
					L"%s[%d] Session %d - sendRBuf Enqueue Error (req - %d, ret - %d)\n",
					_T(__FUNCTION__), __LINE__, (*playerIter)->_pSession->_ID, size, enqueueRet);

				::wprintf(L"%s[%d] Session %d - sendRBuf Enqueue Error (req - %d, ret - %d)\n",
					_T(__FUNCTION__), __LINE__, (*playerIter)->_pSession->_ID, size, enqueueRet);

				SetSessionDead((*playerIter)->_pSession);
			}
			
		}
	}
	else
	{
		vector<Player*>::iterator playerIter = sector->_players.begin();
		for (; playerIter < sector->_players.end(); playerIter++)
		{
			if ((*playerIter)->_pSession != pExpSession)
			{
				if ((*playerIter) == nullptr)
				{
					LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
						L"%s[%d] Player in sector[%d][%d] is nullptr\n",
						_T(__FUNCTION__), __LINE__, sector->_xIndex, sector->_yIndex);

					::wprintf(L"%s[%d] Player in sector[%d][%d] is nullptr\n",
						_T(__FUNCTION__), __LINE__, sector->_xIndex, sector->_yIndex);

					g_Shutdown = true;
					return;
				}

				int enqueueRet = (*playerIter)->_pSession->_sendRBuffer.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
						L"%s[%d] Session %d - sendRBuf Enqueue Error (req - %d, ret - %d)\n",
						_T(__FUNCTION__), __LINE__, (*playerIter)->_pSession->_ID, size, enqueueRet);

					::wprintf(L"%s[%d] Session %d - sendRBuf Enqueue Error (req - %d, ret - %d)\n",
						_T(__FUNCTION__), __LINE__, (*playerIter)->_pSession->_ID, size, enqueueRet);

					SetSessionDead((*playerIter)->_pSession);
				}
			}
		}
	}
}

void CServer::EnqueueAroundSector(char* msg, int size, Sector* centerSector, Session* pExpSession)
{
	if (pExpSession == nullptr)
	{
		for(int i = 0; i < dfAROUND_SECTOR_NUM; i++)
		{
			vector<Player*>::iterator playerIter 
				= centerSector->_around[i]->_players.begin();

			for (; playerIter < centerSector->_around[i]->_players.end(); playerIter++)
			{
				if ((*playerIter) == nullptr)
				{
					LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
						L"%s[%d] Player in sector[%d][%d] is nullptr\n",
						_T(__FUNCTION__), __LINE__, 
						centerSector->_around[i]->_xIndex, 
						centerSector->_around[i]->_yIndex);

					::wprintf(L"%s[%d] Player in sector[%d][%d] is nullptr\n",
						_T(__FUNCTION__), __LINE__, 
						centerSector->_around[i]->_xIndex, 
						centerSector->_around[i]->_yIndex);

					g_Shutdown = true;
					return;
				}

				int enqueueRet = (*playerIter)->_pSession->_sendRBuffer.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
						L"%s[%d] Session %d - sendRBuf Enqueue Error (req - %d, ret - %d)\n",
						_T(__FUNCTION__), __LINE__, (*playerIter)->_pSession->_ID, size, enqueueRet);

					::wprintf(L"%s[%d] Session %d - sendRBuf Enqueue Error (req - %d, ret - %d)\n",
						_T(__FUNCTION__), __LINE__, (*playerIter)->_pSession->_ID, size, enqueueRet);

					SetSessionDead((*playerIter)->_pSession);
				}
			}
		}
	}
	else
	{		
		for (int i = 0; i < dfMOVE_DIR_MAX; i++)
		{
			vector<Player*>::iterator playerIter 
				= centerSector->_around[i]->_players.begin();

			for (; playerIter < centerSector->_around[i]->_players.end(); playerIter++)
			{
				if ((*playerIter) == nullptr)
				{
					LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
						L"%s[%d] Player in sector[%d][%d] is nullptr\n",
						_T(__FUNCTION__), __LINE__,
						centerSector->_around[i]->_xIndex,
						centerSector->_around[i]->_yIndex);

					::wprintf(L"%s[%d] Player in sector[%d][%d] is nullptr\n",
						_T(__FUNCTION__), __LINE__,
						centerSector->_around[i]->_xIndex,
						centerSector->_around[i]->_yIndex);

					g_Shutdown = true;
					return;
				}

				int enqueueRet = (*playerIter)->_pSession->_sendRBuffer.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
						L"%s[%d] Session %d - sendRBuf Enqueue Error (req - %d, ret - %d)\n",
						_T(__FUNCTION__), __LINE__, (*playerIter)->_pSession->_ID, size, enqueueRet);

					::wprintf(L"%s[%d] Session %d - sendRBuf Enqueue Error (req - %d, ret - %d)\n",
						_T(__FUNCTION__), __LINE__, (*playerIter)->_pSession->_ID, size, enqueueRet);

					SetSessionDead((*playerIter)->_pSession);
				}
			}
		}

		vector<Player*>::iterator playerIter 
			= centerSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; playerIter < centerSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); playerIter++)
		{
			if ((*playerIter)->_pSession != pExpSession)
			{
				if ((*playerIter) == nullptr)
				{
					LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
						L"%s[%d] Player in sector[%d][%d] is nullptr\n",
						_T(__FUNCTION__), __LINE__,
						centerSector->_around[dfMOVE_DIR_INPLACE]->_xIndex,
						centerSector->_around[dfMOVE_DIR_INPLACE]->_yIndex);

					::wprintf(L"%s[%d] Player in sector[%d][%d] is nullptr\n",
						_T(__FUNCTION__), __LINE__,
						centerSector->_around[dfMOVE_DIR_INPLACE]->_xIndex,
						centerSector->_around[dfMOVE_DIR_INPLACE]->_yIndex);

					g_Shutdown = true;
					return;
				}

				int enqueueRet = (*playerIter)->_pSession->_sendRBuffer.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
						L"%s[%d] Session %d - sendRBuf Enqueue Error (req - %d, ret - %d)\n",
						_T(__FUNCTION__), __LINE__, (*playerIter)->_pSession->_ID, size, enqueueRet);

					::wprintf(L"%s[%d] Session %d - sendRBuf Enqueue Error (req - %d, ret - %d)\n",
						_T(__FUNCTION__), __LINE__, (*playerIter)->_pSession->_ID, size, enqueueRet);

					SetSessionDead((*playerIter)->_pSession);
				}
			}
		}
	}
}

// About Content ====================================================

bool CServer::SkipForFixedFrame()
{
	static DWORD oldTick = GetTickCount64();
	if ((GetTickCount64() - oldTick) < (1000 / dfFPS))
		return true;
	oldTick += (1000 / dfFPS);
	return false;
}

bool CServer::CheckMovable(short x, short y)
{
	if (x < dfRANGE_MOVE_LEFT || x > dfRANGE_MOVE_RIGHT ||
		y < dfRANGE_MOVE_TOP || y > dfRANGE_MOVE_BOTTOM)
		return false;

	return true;
}

void CServer::UpdatePlayerMove(Player* pPlayer)
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
		
		else if(pPlayer->_y < pPlayer->_pSector->_yPosMin)
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

void CServer::SetSector(Player* pPlayer)
{
	int x = (pPlayer->_x / dfSECTOR_SIZE_X) + 2;
	int y = (pPlayer->_y / dfSECTOR_SIZE_Y) + 2;
	_Sectors[y][x]._players.push_back(pPlayer);
	pPlayer->_pSector = &_Sectors[y][x];
}

void CServer::UpdateSector(Player* pPlayer, short direction)
{
	PRO_BEGIN(L"Content: Update Sector");

	vector<Player*>::iterator iter = pPlayer->_pSector->_players.begin();
	for (; iter < pPlayer->_pSector->_players.end(); iter++)
	{
		if (pPlayer == (*iter))
		{
			pPlayer->_pSector->_players.erase(iter);
			break;
		}
	}

	// Get Around Sector Data =======================================
	
	int sectorCnt = _sectorCnt[direction];
	Sector** inSector = pPlayer->_pSector->_new[direction];
	Sector** outSector = pPlayer->_pSector->_old[direction];
	Sector* newSector = pPlayer->_pSector->_around[direction];

	// Send Data About My Player ==============================================

	pPlayer->_pSession->_sendSPacket.Clear();
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(&pPlayer->_pSession->_sendSPacket,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	for (int i = 0; i < sectorCnt; i++)
		EnqueueOneSector(pPlayer->_pSession->_sendSPacket.GetReadPtr(), createMeToOtherRet, inSector[i]);

	pPlayer->_pSession->_sendSPacket.Clear();
	int MoveMeToOtherRet = SetSCPacket_MOVE_START(&pPlayer->_pSession->_sendSPacket,
		pPlayer->_ID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	for (int i = 0; i < sectorCnt; i++)
		EnqueueOneSector(pPlayer->_pSession->_sendSPacket.GetReadPtr(), MoveMeToOtherRet, inSector[i]);
	
	pPlayer->_pSession->_sendSPacket.Clear();
	int deleteMeToOtherRet = SetSCPacket_DELETE_CHAR(&pPlayer->_pSession->_sendSPacket, pPlayer->_ID);
	for (int i = 0; i < sectorCnt; i++)
		EnqueueOneSector(pPlayer->_pSession->_sendSPacket.GetReadPtr(), deleteMeToOtherRet, outSector[i]);

	// Send Data About Other Player ==============================================
	
	for (int i = 0; i < sectorCnt; i++)
	{	
		vector<Player*>::iterator iter = inSector[i]->_players.begin();
		for(; iter < inSector[i]->_players.end(); iter++)
		{
			pPlayer->_pSession->_sendSPacket.Clear();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(&pPlayer->_pSession->_sendSPacket,
				(*iter)->_ID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			EnqueueUnicast(pPlayer->_pSession->_sendSPacket.GetReadPtr(), createOtherRet, pPlayer->_pSession);

			if((*iter)->_move)
			{
				pPlayer->_pSession->_sendSPacket.Clear();
				int MoveOtherRet = SetSCPacket_MOVE_START(&pPlayer->_pSession->_sendSPacket,
					(*iter)->_ID, (*iter)->_moveDirection, (*iter)->_x, (*iter)->_y);
				EnqueueUnicast(pPlayer->_pSession->_sendSPacket.GetReadPtr(), MoveOtherRet, pPlayer->_pSession);
			}
		}
	}

	for (int i = 0; i < sectorCnt; i++)
	{
		vector<Player*>::iterator iter = outSector[i]->_players.begin();
		for (; iter < outSector[i]->_players.end(); iter++)
		{
			pPlayer->_pSession->_sendSPacket.Clear();
			int deleteOtherRet = SetSCPacket_DELETE_CHAR(&pPlayer->_pSession->_sendSPacket, (*iter)->_ID);
			EnqueueUnicast(pPlayer->_pSession->_sendSPacket.GetReadPtr(), deleteOtherRet, pPlayer->_pSession);
		}
	}

	pPlayer->_pSector = newSector;
	newSector->_players.push_back(pPlayer);

	PRO_END(L"Content: Update Sector");
}

void CServer::CreatePlayer(Session* pSession)
{
	Player* pPlayer = _pPlayerPool->Alloc(pSession, _playerID++);
	_Players[pSession->_ID] = pPlayer;
	SetSector(pPlayer);
	
	pPlayer->_pSession->_sendSPacket.Clear();
	int createMeRet = SetSCPacket_CREATE_MY_CHAR(&pPlayer->_pSession->_sendSPacket,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	EnqueueUnicast(pPlayer->_pSession->_sendSPacket.GetReadPtr(), createMeRet, pPlayer->_pSession);

	pPlayer->_pSession->_sendSPacket.Clear();
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(&pPlayer->_pSession->_sendSPacket,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	EnqueueAroundSector(pPlayer->_pSession->_sendSPacket.GetReadPtr(),
		createMeToOtherRet, pPlayer->_pSector, pPlayer->_pSession);

	for (int i = 0; i < dfMOVE_DIR_MAX; i++)
	{
		vector<Player*>::iterator iter 
			= pPlayer->_pSector->_around[i]->_players.begin();
		for (; iter < pPlayer->_pSector->_around[i]->_players.end(); iter++)
		{
			pPlayer->_pSession->_sendSPacket.Clear();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(&pPlayer->_pSession->_sendSPacket,
				(*iter)->_ID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			EnqueueUnicast(pPlayer->_pSession->_sendSPacket.GetReadPtr(), createOtherRet, pPlayer->_pSession);
		}
	}

	vector<Player*>::iterator iter
		= pPlayer->_pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
	for (; iter < pPlayer->_pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
	{
		if ((*iter) != pPlayer)
		{
			pPlayer->_pSession->_sendSPacket.Clear();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(&pPlayer->_pSession->_sendSPacket,
				(*iter)->_ID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			EnqueueUnicast(pPlayer->_pSession->_sendSPacket.GetReadPtr(), createOtherRet, pPlayer->_pSession);
		}
	}
}

// About Packet ====================================================

bool CServer::HandleCSPackets(Player* pPlayer, BYTE type)
{
	switch (type)
	{
	case dfPACKET_CS_MOVE_START:
		return HandleCSPacket_MOVE_START(pPlayer);	
		break;

	case dfPACKET_CS_MOVE_STOP:
		return HandleCSPacket_MOVE_STOP(pPlayer);
		break;

	case dfPACKET_CS_ATTACK1:
		return HandleCSPacket_ATTACK1(pPlayer);
		break;

	case dfPACKET_CS_ATTACK2:
		return HandleCSPacket_ATTACK2(pPlayer);
		break;

	case dfPACKET_CS_ATTACK3:
		return HandleCSPacket_ATTACK3(pPlayer);
		break;
	
	case dfPACKET_CS_ECHO:
		return HandleCSPacket_ECHO(pPlayer);
		break;
	}

	LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
		L"%s[%d] No Switch Case, %d\n", _T(__FUNCTION__), __LINE__, type);

	::wprintf(L"%s[%d] No Switch Case, %d\n", _T(__FUNCTION__), __LINE__, type);
	return false;
}

bool CServer::HandleCSPacket_MOVE_START(Player* pPlayer)
{
	BYTE moveDirection;
	short x;
	short y;

	pPlayer->_pSession->_recvSPacket.Clear();
	bool getRet = GetCSPacket_MOVE_START(&pPlayer->_pSession->_recvSPacket, 
		&pPlayer->_pSession->_recvRBuffer, moveDirection, x, y);
	if(!getRet) return false;
	
	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		pPlayer->_pSession->_sendSPacket.Clear();
		int setRet = SetSCPacket_SYNC(&pPlayer->_pSession->_sendSPacket, 
			pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(pPlayer->_pSession->_sendSPacket.GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	SetPlayerMoveStart(pPlayer, moveDirection, x, y);

	pPlayer->_pSession->_sendSPacket.Clear();
	int setRet = SetSCPacket_MOVE_START(&pPlayer->_pSession->_sendSPacket, 
		pPlayer->_ID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(pPlayer->_pSession->_sendSPacket.GetReadPtr(), setRet, pPlayer->_pSector, pPlayer->_pSession);
	
	return true;
}

bool CServer::HandleCSPacket_MOVE_STOP(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	pPlayer->_pSession->_recvSPacket.Clear();
	bool getRet = GetCSPacket_MOVE_STOP(&pPlayer->_pSession->_recvSPacket, 
		&pPlayer->_pSession->_recvRBuffer, direction, x, y);
	if (!getRet) return false;
	
	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		pPlayer->_pSession->_sendSPacket.Clear();
		int setRet = SetSCPacket_SYNC(&pPlayer->_pSession->_sendSPacket, 
			pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(pPlayer->_pSession->_sendSPacket.GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	SetPlayerMoveStop(pPlayer, direction, x, y);

	pPlayer->_pSession->_sendSPacket.Clear();
	int setRet = SetSCPacket_MOVE_STOP(&pPlayer->_pSession->_sendSPacket, 
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(pPlayer->_pSession->_sendSPacket.GetReadPtr(), setRet, pPlayer->_pSector, pPlayer->_pSession);
	
	return true;
}

bool CServer::HandleCSPacket_ATTACK1(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	pPlayer->_pSession->_recvSPacket.Clear();
	bool getRet = GetCSPacket_ATTACK1(&pPlayer->_pSession->_recvSPacket, 
		&pPlayer->_pSession->_recvRBuffer, direction, x, y);
	if (!getRet) return false;
	
	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		pPlayer->_pSession->_sendSPacket.Clear();
		int setRet = SetSCPacket_SYNC(&pPlayer->_pSession->_sendSPacket, 
			pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(pPlayer->_pSession->_sendSPacket.GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	Player* damagedPlayer = nullptr;
	SetPlayerAttack1(pPlayer, damagedPlayer, direction, x, y);

	pPlayer->_pSession->_sendSPacket.Clear();
	int attackSetRet = SetSCPacket_ATTACK1(&pPlayer->_pSession->_sendSPacket, 
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(pPlayer->_pSession->_sendSPacket.GetReadPtr(), attackSetRet, pPlayer->_pSector);

	if (damagedPlayer != nullptr)
	{
		pPlayer->_pSession->_sendSPacket.Clear();
		int damageSetRet = SetSCPacket_DAMAGE(&pPlayer->_pSession->_sendSPacket, 
			pPlayer->_ID, damagedPlayer->_ID, damagedPlayer->_hp);
		EnqueueAroundSector(pPlayer->_pSession->_sendSPacket.GetReadPtr(), damageSetRet, damagedPlayer->_pSector);
	}

	return true;
}

bool CServer::HandleCSPacket_ATTACK2(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	pPlayer->_pSession->_recvSPacket.Clear();
	bool getRet = GetCSPacket_ATTACK2(&pPlayer->_pSession->_recvSPacket, 
		&pPlayer->_pSession->_recvRBuffer, direction, x, y);
	if (!getRet) return false;

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		pPlayer->_pSession->_sendSPacket.Clear();
		int setRet = SetSCPacket_SYNC(&pPlayer->_pSession->_sendSPacket, 
			pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(pPlayer->_pSession->_sendSPacket.GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	Player* damagedPlayer = nullptr;
	SetPlayerAttack2(pPlayer, damagedPlayer, direction, x, y);

	pPlayer->_pSession->_sendSPacket.Clear();
	int attackSetRet = SetSCPacket_ATTACK2(&pPlayer->_pSession->_sendSPacket, 
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(pPlayer->_pSession->_sendSPacket.GetReadPtr(), attackSetRet, pPlayer->_pSector);

	if (damagedPlayer != nullptr)
	{
		pPlayer->_pSession->_sendSPacket.Clear();
		int damageSetRet = SetSCPacket_DAMAGE(&pPlayer->_pSession->_sendSPacket, 
			pPlayer->_ID, damagedPlayer->_ID, damagedPlayer->_hp);
		EnqueueAroundSector(pPlayer->_pSession->_sendSPacket.GetReadPtr(), damageSetRet, damagedPlayer->_pSector);
	}

	
	return true;
}

bool CServer::HandleCSPacket_ATTACK3(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	pPlayer->_pSession->_recvSPacket.Clear();
	bool getRet = GetCSPacket_ATTACK3(&pPlayer->_pSession->_recvSPacket, 
		&pPlayer->_pSession->_recvRBuffer, direction, x, y);
	if (!getRet) return false;

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		pPlayer->_pSession->_sendSPacket.Clear();
		int setRet = SetSCPacket_SYNC(&pPlayer->_pSession->_sendSPacket, 
			pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(pPlayer->_pSession->_sendSPacket.GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	Player* damagedPlayer = nullptr;
	SetPlayerAttack3(pPlayer, damagedPlayer, direction, x, y);

	pPlayer->_pSession->_sendSPacket.Clear();
	int attackSetRet = SetSCPacket_ATTACK3(&pPlayer->_pSession->_sendSPacket, 
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(pPlayer->_pSession->_sendSPacket.GetReadPtr(), attackSetRet, pPlayer->_pSector);

	if (damagedPlayer != nullptr)
	{
		pPlayer->_pSession->_sendSPacket.Clear();
		int damageSetRet = SetSCPacket_DAMAGE(&pPlayer->_pSession->_sendSPacket, 
			pPlayer->_ID, damagedPlayer->_ID, damagedPlayer->_hp);
		EnqueueAroundSector(pPlayer->_pSession->_sendSPacket.GetReadPtr(), damageSetRet, damagedPlayer->_pSector);
	}

	return true;
}

bool CServer::HandleCSPacket_ECHO(Player* pPlayer)
{
	int time;

	pPlayer->_pSession->_recvSPacket.Clear();
	bool getRet = GetCSPacket_ECHO(&pPlayer->_pSession->_recvSPacket, &pPlayer->_pSession->_recvRBuffer, time);
	if (!getRet) return false;
	
	pPlayer->_pSession->_sendSPacket.Clear();
	int setRet = SetSCPacket_ECHO(&pPlayer->_pSession->_sendSPacket, time);
	EnqueueUnicast(pPlayer->_pSession->_sendSPacket.GetReadPtr(), setRet, pPlayer->_pSession);
	
	return true;
}

bool CServer::GetCSPacket_MOVE_START(CSerializePacket* pPacket, CRingBuffer* recvRBuffer, BYTE& moveDirection, short& x, short& y)
{
	int size = sizeof(moveDirection) + sizeof(x) + sizeof(y);
	int dequeueRet = recvRBuffer->Dequeue(pPacket->GetWritePtr(), size);
	if (dequeueRet != size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d] recvRBuf Dequeue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, size, dequeueRet);

		::wprintf(L"%s[%d] recvRBuf Dequeue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, size, dequeueRet);

		return false;
	}
	pPacket->MoveWritePos(dequeueRet);

	*pPacket >> moveDirection;
	*pPacket >> x;
	*pPacket >> y;

	return true;
}

bool CServer::GetCSPacket_MOVE_STOP(CSerializePacket* pPacket, CRingBuffer* recvRBuffer, BYTE& direction, short& x, short& y)
{
	int size = sizeof(direction) + sizeof(x) + sizeof(y);
	int dequeueRet = recvRBuffer->Dequeue(pPacket->GetWritePtr(), size);
	if (dequeueRet != size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d] recvRBuf Dequeue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, size, dequeueRet);

		::wprintf(L"%s[%d] recvRBuf Dequeue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, size, dequeueRet);

		return false;
	}
	pPacket->MoveWritePos(dequeueRet);

	*pPacket >> direction;
	*pPacket >> x;
	*pPacket >> y;
	
	return true;
}

bool CServer::GetCSPacket_ATTACK1(CSerializePacket* pPacket, CRingBuffer* recvRBuffer, BYTE& direction, short& x, short& y)
{
	int size = sizeof(direction) + sizeof(x) + sizeof(y);
	int dequeueRet = recvRBuffer->Dequeue(pPacket->GetWritePtr(), size);
	if (dequeueRet != size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d] recvRBuf Dequeue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, size, dequeueRet);

		::wprintf(L"%s[%d] recvRBuf Dequeue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, size, dequeueRet);

		return false;
	}
	pPacket->MoveWritePos(dequeueRet);

	*pPacket >> direction;
	*pPacket >> x;
	*pPacket >> y;

	return true;
}

bool CServer::GetCSPacket_ATTACK2(CSerializePacket* pPacket, CRingBuffer* recvRBuffer, BYTE& direction, short& x, short& y)
{
	int size = sizeof(direction) + sizeof(x) + sizeof(y);
	int dequeueRet = recvRBuffer->Dequeue(pPacket->GetWritePtr(), size);
	if (dequeueRet != size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d] recvRBuf Dequeue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, size, dequeueRet);

		::wprintf(L"%s[%d] recvRBuf Dequeue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, size, dequeueRet);

		return false;
	}
	pPacket->MoveWritePos(dequeueRet);

	*pPacket >> direction;
	*pPacket >> x;
	*pPacket >> y;
	
	return true;
}

bool CServer::GetCSPacket_ATTACK3(CSerializePacket* pPacket, CRingBuffer* recvRBuffer, BYTE& direction, short& x, short& y)
{
	int size = sizeof(direction) + sizeof(x) + sizeof(y);
	int dequeueRet = recvRBuffer->Dequeue(pPacket->GetWritePtr(), size);
	if (dequeueRet != size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d] recvRBuf Dequeue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, size, dequeueRet);

		::wprintf(L"%s[%d] recvRBuf Dequeue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, size, dequeueRet);

		return false;
	}
	pPacket->MoveWritePos(dequeueRet);

	*pPacket >> direction;
	*pPacket >> x;
	*pPacket >> y;

	return true;
}

bool CServer::GetCSPacket_ECHO(CSerializePacket* pPacket, CRingBuffer* recvRBuffer, int& time)
{
	int size = sizeof(time);
	int dequeueRet = recvRBuffer->Dequeue(pPacket->GetWritePtr(), size);
	if (dequeueRet != size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d] recvRBuf Dequeue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, size, dequeueRet);

		::wprintf(L"%s[%d] recvRBuf Dequeue Error (req - %d, ret - %d)\n",
			_T(__FUNCTION__), __LINE__, size, dequeueRet);

		return false;
	}
	pPacket->MoveWritePos(dequeueRet);
	*pPacket >> time;

	return true;
}

void CServer::SetPlayerMoveStart(Player* pPlayer, BYTE& moveDirection, short& x, short& y)
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

void CServer::SetPlayerMoveStop(Player* pPlayer, BYTE& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_move = false;
	pPlayer->_direction = direction;	
}

void CServer::SetPlayerAttack1(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;

	if (direction == dfMOVE_DIR_LL)
	{
		vector<Player*>::iterator iter;
		Sector* pSector = pPlayer->_pSector;

		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetSessionDead(pDamagedPlayer->_pSession);
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
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
						}
						return;
					}
				}
			}
		}
	}
	else if (direction == dfMOVE_DIR_RR)
	{
		vector<Player*>::iterator iter;
		Sector* pSector = pPlayer->_pSector;
		
		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetSessionDead(pDamagedPlayer->_pSession);
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
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
						}
						return;
					}
				}
			}
			else if(pPlayer->_y <= pSector->_yPosMin + dfATTACK1_RANGE_Y)
			{
				iter = pSector->_around[dfMOVE_DIR_RD]->_players.begin();
				for (; iter < pSector->_around[dfMOVE_DIR_RD]->_players.end(); iter++)
				{
					int dist = (*iter)->_x - pPlayer->_x;
					if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
						}
						return;
					}
				}
			}
		}
	}
}

void CServer::SetPlayerAttack2(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;

	if (direction == dfMOVE_DIR_LL)
	{
		vector<Player*>::iterator iter;
		Sector* pSector = pPlayer->_pSector;

		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetSessionDead(pDamagedPlayer->_pSession);
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
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
						}
						return;
					}
				}
			}
		}
	}
	else if (direction == dfMOVE_DIR_RR)
	{
		vector<Player*>::iterator iter;
		Sector* pSector = pPlayer->_pSector;

		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetSessionDead(pDamagedPlayer->_pSession);
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
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
						}
						return;
					}
				}
			}
		}
	}
}
void CServer::SetPlayerAttack3(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;

	if (direction == dfMOVE_DIR_LL)
	{
		vector<Player*>::iterator iter;
		Sector* pSector = pPlayer->_pSector;

		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetSessionDead(pDamagedPlayer->_pSession);
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
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
						}
						return;
					}
				}
			}
		}
	}
	else if (direction == dfMOVE_DIR_RR)
	{
		vector<Player*>::iterator iter;
		Sector* pSector = pPlayer->_pSector;

		iter = pSector->_around[dfMOVE_DIR_INPLACE]->_players.begin();
		for (; iter < pSector->_around[dfMOVE_DIR_INPLACE]->_players.end(); iter++)
		{
			if ((*iter) != pPlayer)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetSessionDead(pDamagedPlayer->_pSession);
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
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

					if (pDamagedPlayer->_hp <= 0)
					{
						_deadCnt++;
						SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
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
						abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
					{
						pDamagedPlayer = (*iter);
						pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;

						if (pDamagedPlayer->_hp <= 0)
						{
							_deadCnt++;
							SetSessionDead(pDamagedPlayer->_pSession);
						}
						return;
					}
				}
			}
		}
	}
}

void CServer::SetSCPacket_HEADER(CSerializePacket* pPacket, BYTE size, BYTE type)
{
	*pPacket << (BYTE)dfPACKET_CODE;
	*pPacket << size;
	*pPacket << type;
}

int CServer::SetSCPacket_CREATE_MY_CHAR(CSerializePacket* pPacket, int ID, BYTE direction, short x, short y, BYTE hp)
{
	int size = sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y) + sizeof(hp);
	SetSCPacket_HEADER(pPacket, size, dfPACKET_SC_CREATE_MY_CHARACTER);
	
	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;
	*pPacket << hp;

	if (pPacket->GetDataSize() != dfHEADER_SIZE + size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);

		::wprintf(L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);
	}

	return pPacket->GetDataSize();
}

int CServer::SetSCPacket_CREATE_OTHER_CHAR(CSerializePacket* pPacket, int ID, BYTE direction, short x, short y, BYTE hp)
{
	int size = sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y) + sizeof(hp);
	SetSCPacket_HEADER(pPacket, size, dfPACKET_SC_CREATE_OTHER_CHARACTER);

	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;
	*pPacket << hp;

	if (pPacket->GetDataSize() != dfHEADER_SIZE + size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);

		::wprintf(L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);
	}

	return pPacket->GetDataSize();
}

int CServer::SetSCPacket_DELETE_CHAR(CSerializePacket* pPacket, int ID)
{
	int size = sizeof(ID);
	SetSCPacket_HEADER(pPacket, size, dfPACKET_SC_DELETE_CHARACTER);

	*pPacket << ID;

	if (pPacket->GetDataSize() != dfHEADER_SIZE + size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);

		::wprintf(L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);
	}
	
	return pPacket->GetDataSize();
}

int CServer::SetSCPacket_MOVE_START(CSerializePacket* pPacket, int ID, BYTE moveDirection, short x, short y)
{
	int size = sizeof(ID) + sizeof(moveDirection) + sizeof(x) + sizeof(y);
	SetSCPacket_HEADER(pPacket, size, dfPACKET_SC_MOVE_START);

	*pPacket << ID;
	*pPacket << moveDirection;
	*pPacket << x;
	*pPacket << y;

	if (pPacket->GetDataSize() != dfHEADER_SIZE + size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);

		::wprintf(L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);
	}

	return pPacket->GetDataSize();
}

int CServer::SetSCPacket_MOVE_STOP(CSerializePacket* pPacket, int ID, BYTE direction, short x, short y)
{
	int size = sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);
	SetSCPacket_HEADER(pPacket, size, dfPACKET_SC_MOVE_STOP);

	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;

	if (pPacket->GetDataSize() != dfHEADER_SIZE + size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);

		::wprintf(L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);
	}

	return pPacket->GetDataSize();
}

int CServer::SetSCPacket_ATTACK1(CSerializePacket* pPacket, int ID, BYTE direction, short x, short y)
{
	int size = sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);
	SetSCPacket_HEADER(pPacket, size, dfPACKET_SC_ATTACK1);

	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;

	if (pPacket->GetDataSize() != dfHEADER_SIZE + size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);

		::wprintf(L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);
	}

	return pPacket->GetDataSize();
}

int CServer::SetSCPacket_ATTACK2(CSerializePacket* pPacket, int ID, BYTE direction, short x, short y)
{
	int size = sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);
	SetSCPacket_HEADER(pPacket, size, dfPACKET_SC_ATTACK2);

	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;

	if (pPacket->GetDataSize() != dfHEADER_SIZE + size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);

		::wprintf(L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);
	}

	return pPacket->GetDataSize();
}

int CServer::SetSCPacket_ATTACK3(CSerializePacket* pPacket, int ID, BYTE direction, short x, short y)
{
	int size = sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);
	SetSCPacket_HEADER(pPacket, size, dfPACKET_SC_ATTACK3);

	*pPacket << ID;
	*pPacket << direction;
	*pPacket << x;
	*pPacket << y;

	if (pPacket->GetDataSize() != dfHEADER_SIZE + size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);

		::wprintf(L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);
	}

	return pPacket->GetDataSize();
}

int CServer::SetSCPacket_DAMAGE(CSerializePacket* pPacket, int attackID, int damageID, BYTE damageHP)
{
	int size = sizeof(attackID) + sizeof(damageID) + sizeof(damageHP);
	SetSCPacket_HEADER(pPacket, size, dfPACKET_SC_DAMAGE);

	*pPacket << attackID;
	*pPacket << damageID;
	*pPacket << damageHP;

	if (pPacket->GetDataSize() != dfHEADER_SIZE + size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);

		::wprintf(L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);
	}

	
	return pPacket->GetDataSize();
}

int CServer::SetSCPacket_SYNC(CSerializePacket* pPacket, int ID, short x, short y)
{
	int size = sizeof(ID) + sizeof(x) + sizeof(y);
	SetSCPacket_HEADER(pPacket, size, dfPACKET_SC_SYNC);

	*pPacket << ID;
	*pPacket << x;
	*pPacket << y;

	if (pPacket->GetDataSize() != dfHEADER_SIZE + size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);

		::wprintf(L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);
	}

	_syncCnt++;
	return pPacket->GetDataSize();
}

int CServer::SetSCPacket_ECHO(CSerializePacket* pPacket, int time)
{
	int size = sizeof(time);
	SetSCPacket_HEADER(pPacket, size, dfPACKET_SC_ECHO);

	*pPacket << time;

	if (pPacket->GetDataSize() != dfHEADER_SIZE + size)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);

		::wprintf(L"%s[%d]: Create Packet Error, %d != %llu\n",
			_T(__FUNCTION__), __LINE__, pPacket->GetDataSize(), dfHEADER_SIZE + size);
	}
	
	return pPacket->GetDataSize();
}

void CServer::Sector::InitializeSector(short xIndex, short yIndex)
{
	_xIndex = xIndex;
	_yIndex = yIndex;

	if (_xIndex < 2 || _xIndex >= (dfSECTOR_CNT_X - 2) ||
		_yIndex < 2 || _yIndex >= (dfSECTOR_CNT_Y - 2))
		return;

	_xPosMin = (xIndex - 2) * dfSECTOR_SIZE_X ;
	_yPosMin = (yIndex - 2) * dfSECTOR_SIZE_Y;
	_xPosMax = (xIndex - 1) * dfSECTOR_SIZE_X;
	_yPosMax = (yIndex - 1) * dfSECTOR_SIZE_Y;
	_players.reserve(dfDEFAULT_PLAYERS_PER_SECTOR);
}

void CServer::SetSectorsAroundInfo()
{
	for (int y = 2; y < dfSECTOR_CNT_Y - 2; y++)
	{
		for (int x = 2; x < dfSECTOR_CNT_X - 2; x++)
		{
			Sector* pSector = &_Sectors[y][x];

			pSector->_around[dfMOVE_DIR_LL] = &_Sectors[y][x - 1];
			pSector->_around[dfMOVE_DIR_LU] = &_Sectors[y - 1][x - 1];
			pSector->_around[dfMOVE_DIR_UU] = &_Sectors[y - 1][x];
			pSector->_around[dfMOVE_DIR_RU] = &_Sectors[y - 1][x + 1];
			pSector->_around[dfMOVE_DIR_RR] = &_Sectors[y][x + 1];
			pSector->_around[dfMOVE_DIR_RD] = &_Sectors[y + 1][x + 1];
			pSector->_around[dfMOVE_DIR_DD] = &_Sectors[y + 1][x];
			pSector->_around[dfMOVE_DIR_LD] = &_Sectors[y + 1][x - 1];
			pSector->_around[dfMOVE_DIR_INPLACE] = &_Sectors[y][x];
			
			// dfMOVE_DIR_LL

			pSector->_llNew[0] = &_Sectors[y - 1][x - 2];
			pSector->_llNew[1] = &_Sectors[y][x - 2];
			pSector->_llNew[2] = &_Sectors[y + 1][x - 2];

			pSector->_llOld[0] = &_Sectors[y - 1][x + 1];
			pSector->_llOld[1] = &_Sectors[y][x + 1];
			pSector->_llOld[2] = &_Sectors[y + 1][x + 1];

			// dfMOVE_DIR_LU

			pSector->_luNew[0] = &_Sectors[y - 2][x];
			pSector->_luNew[1] = &_Sectors[y - 2][x - 1];
			pSector->_luNew[2] = &_Sectors[y - 2][x - 2];
			pSector->_luNew[3] = &_Sectors[y - 1][x - 2];
			pSector->_luNew[4] = &_Sectors[y][x - 2];

			pSector->_luOld[0] = &_Sectors[y + 1][x];
			pSector->_luOld[1] = &_Sectors[y + 1][x - 1];
			pSector->_luOld[2] = &_Sectors[y + 1][x + 1];
			pSector->_luOld[3] = &_Sectors[y - 1][x + 1];
			pSector->_luOld[4] = &_Sectors[y][x + 1];

			// dfMOVE_DIR_UU

			pSector->_uuNew[0] = &_Sectors[y - 2][x - 1];
			pSector->_uuNew[1] = &_Sectors[y - 2][x];
			pSector->_uuNew[2] = &_Sectors[y - 2][x + 1];

			pSector->_uuOld[0] = &_Sectors[y + 1][x - 1];
			pSector->_uuOld[1] = &_Sectors[y + 1][x];
			pSector->_uuOld[2] = &_Sectors[y + 1][x + 1];

			// dfMOVE_DIR_RU

			pSector->_ruNew[0] = &_Sectors[y - 2][x];
			pSector->_ruNew[1] = &_Sectors[y - 2][x + 1];
			pSector->_ruNew[2] = &_Sectors[y - 2][x + 2];
			pSector->_ruNew[3] = &_Sectors[y - 1][x + 2];
			pSector->_ruNew[4] = &_Sectors[y][x + 2];

			pSector->_ruOld[0] = &_Sectors[y][x - 1];
			pSector->_ruOld[1] = &_Sectors[y - 1][x - 1];
			pSector->_ruOld[2] = &_Sectors[y + 1][x - 1];
			pSector->_ruOld[3] = &_Sectors[y + 1][x + 1];
			pSector->_ruOld[4] = &_Sectors[y + 1][x];

			// dfMOVE_DIR_RR

			pSector->_rrNew[0] = &_Sectors[y - 1][x + 2];
			pSector->_rrNew[1] = &_Sectors[y][x + 2];
			pSector->_rrNew[2] = &_Sectors[y + 1][x + 2];

			pSector->_rrOld[0] = &_Sectors[y - 1][x - 1];
			pSector->_rrOld[1] = &_Sectors[y][x - 1];
			pSector->_rrOld[2] = &_Sectors[y + 1][x - 1];

			// dfMOVE_DIR_RD

			pSector->_rdNew[0] = &_Sectors[y + 2][x];
			pSector->_rdNew[1] = &_Sectors[y + 2][x + 1];
			pSector->_rdNew[2] = &_Sectors[y + 2][x + 2];
			pSector->_rdNew[3] = &_Sectors[y + 1][x + 2];
			pSector->_rdNew[4] = &_Sectors[y][x + 2];

			pSector->_rdOld[0] = &_Sectors[y - 1][x];
			pSector->_rdOld[1] = &_Sectors[y - 1][x + 1];
			pSector->_rdOld[2] = &_Sectors[y - 1][x - 1];
			pSector->_rdOld[3] = &_Sectors[y + 1][x - 1];
			pSector->_rdOld[4] = &_Sectors[y][x - 1];

			// dfMOVE_DIR_DD

			pSector->_ddNew[0] = &_Sectors[y + 2][x - 1];
			pSector->_ddNew[1] = &_Sectors[y + 2][x];
			pSector->_ddNew[2] = &_Sectors[y + 2][x + 1];

			pSector->_ddOld[0] = &_Sectors[y - 1][x - 1];
			pSector->_ddOld[1] = &_Sectors[y - 1][x];
			pSector->_ddOld[2] = &_Sectors[y - 1][x + 1];

			// dfMOVE_DIR_LD

			pSector->_ldNew[0] = &_Sectors[y + 2][x];
			pSector->_ldNew[1] = &_Sectors[y + 2][x - 1];
			pSector->_ldNew[2] = &_Sectors[y + 2][x - 2];
			pSector->_ldNew[3] = &_Sectors[y + 1][x - 2];
			pSector->_ldNew[4] = &_Sectors[y][x - 2];

			pSector->_ldOld[0] = &_Sectors[y][x + 1];
			pSector->_ldOld[1] = &_Sectors[y + 1][x + 1];
			pSector->_ldOld[2] = &_Sectors[y - 1][x + 1];
			pSector->_ldOld[3] = &_Sectors[y - 1][x - 1];
			pSector->_ldOld[4] = &_Sectors[y - 1][x];
		}
	}
}

inline void CServer::Session::Initialize(int ID)
{
	_ID = ID;
	_alive = true;
	_recvRBuffer.ClearBuffer();
	_sendRBuffer.ClearBuffer();
}

inline CServer::Player::Player(Session* pSession, int ID)
	: _pSession(pSession), _pSector(nullptr), _ID(ID), _hp(dfMAX_HP),
	_move(false), _direction(dfMOVE_DIR_LL), _moveDirection(dfMOVE_DIR_LL)
{
	_x = 50; // rand() % dfRANGE_MOVE_RIGHT;
	_y = 50; // rand() % dfRANGE_MOVE_BOTTOM;
}