#include "Server.h"
#include "Main.h"
#include <stdio.h>

 // Test: 4800부터 Sync 발생

Server::Server()
{
	int err;
	int bindRet;
	int listenRet;
	int ioctRet;

	srand(0);
	CreateDirectory(L"ProfileBasic", NULL);
	
	_pSessionPool = new CObjectPool<Session>(dfSESSION_MAX, true);
	_pPlayerPool = new CObjectPool<Player>(dfSESSION_MAX, true);
	_pSPacketPool = new CObjectPool<SerializePacket>(dfSPACKET_MAX, false);

	memset(_SessionMap, 0, dfSESSION_MAX * sizeof(Session*));
	memset(_PlayerMap, 0, dfSESSION_MAX * sizeof(Player*));

	_disconnectedSessionIDs.reserve(dfDEFAULT_DISCONNECT_NUM);
	_emptySessionID.reserve(dfSESSION_MAX);

	for (int i = dfSESSION_MAX - 1; i >= 0; i--)
	{
		_emptySessionID.push_back(i);
	}

	// Initialize Winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		g_bShutdown = true;
		return;
	}

	// Set Listen Socket
	_listensock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listensock == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL, 
			L"%s[%d]: listen sock is INVALIED, %d\n", 
			_T(__FUNCTION__), __LINE__, err);
		g_bShutdown = true;
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
		::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);

		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: bind Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		g_bShutdown = true;
		return;
	}

	// Listen
	listenRet = listen(_listensock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);

		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: listen Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		g_bShutdown = true;
		return;
	}

	// Set Non-Blocking Mode
	u_long on = 1;
	ioctRet = ioctlsocket(_listensock, FIONBIO, &on);
	if (ioctRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		::printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: ioct Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);
		g_bShutdown = true;
		return;
	}

	_time.tv_sec = 0;
	_time.tv_usec = 0;

	::printf("Network Setting Complete!\n");

	for (int y = 0; y < dfSECTOR_CNT_Y; y++)
	{
		for (int x = 0; x < dfSECTOR_CNT_X; x++)
		{
			_sectors[y][x].InitializeSector(x, y);
		}
	}

	SetSectorsAroundInfo();

	_oldTick = timeGetTime();
	::printf("Content Setting Complete!\n");

}

Server::~Server()
{	
	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		if (_SessionMap[i] != nullptr)
		{
			closesocket(_SessionMap[i]->_socket);
			_pSessionPool->Free(_SessionMap[i]);
		}

		if (_PlayerMap[i] != nullptr)
			_pPlayerPool->Free(_PlayerMap[i]);
	}

	closesocket(_listensock);
	WSACleanup();

	delete _pSessionPool;
	delete _pPlayerPool;
}

Server* Server::GetInstance()
{
	static Server _server;
	return &_server;
}

void Server::NetworkUpdate()
{ 
	PRO_BEGIN(L"Network");

	int rStopIdx = 0;
	int wStopIdx = 0;
	int rStartIdx = 0;
	int wStartIdx = 0;

	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		if (_SessionMap[i] == nullptr) continue;

		_rSessions[rStopIdx++] = _SessionMap[i];
		if (_SessionMap[i]->_sendBuf.GetUseSize() > 0)
			_wSessions[wStopIdx++] = _SessionMap[i];
	}

	while ((rStopIdx - rStartIdx) >= (FD_SETSIZE - 1) || 
		(wStopIdx - wStartIdx) >= FD_SETSIZE)
	{
		if ((wStopIdx - wStartIdx) < FD_SETSIZE)
		{
			SelectProc(rStartIdx, (FD_SETSIZE - 1), 0, 0);
			rStartIdx += (FD_SETSIZE - 1);
		}
		else if ((rStopIdx - rStartIdx) < (FD_SETSIZE - 1))
		{
			SelectProc(0, 0, wStartIdx, FD_SETSIZE);
			wStartIdx += FD_SETSIZE;
		}
		else
		{
			SelectProc(rStartIdx, (FD_SETSIZE - 1), wStartIdx, FD_SETSIZE);
			rStartIdx += (FD_SETSIZE - 1);
			wStartIdx += FD_SETSIZE;
		}	
	}
	
	SelectProc(rStartIdx, rStopIdx - rStartIdx, wStartIdx, wStopIdx - wStartIdx);

	PRO_END(L"Network");

	PRO_BEGIN(L"Delayed Disconnect");
	DisconnectDeadSession();
	PRO_END(L"Delayed Disconnect");	
}

void Server::ContentUpdate()
{
	if (SkipForFixedFrame()) return;

	PRO_BEGIN(L"Content");
	
	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		if (_PlayerMap[i] == nullptr) continue;

		if (timeGetTime() - _PlayerMap[i]->_pSession->_lastRecvTime 
				> dfNETWORK_PACKET_RECV_TIMEOUT)
		{
			_timeoutCnt++;
			SetSessionDead(_PlayerMap[i]->_pSession);
			continue;
		}

		if (_PlayerMap[i]->_move)
		{
			UpdatePlayerMove(_PlayerMap[i]);
		}
	}

	PRO_END(L"Content");
}

// About Network ==================================================

void Server::SelectProc(int rStartIdx, int rCount, int wStartIdx, int wCount)
{
	FD_SET rset;
	FD_SET wset;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(_listensock, &rset);

	for (int i = 0; i < wCount; i++)
		FD_SET(_wSessions[wStartIdx + i]->_socket, &wset);

	for (int i = 0; i < rCount; i++)
		FD_SET(_rSessions[rStartIdx + i]->_socket, &rset);

	// Select Socket Set
	int selectRet = select(0, &rset, &wset, NULL, &_time);
	if (selectRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: select Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);
		g_bShutdown = true;
		return;
	}

	// Handle Selected Socket
	else if (selectRet > 0)
	{
		if (FD_ISSET(_listensock, &rset))
			AcceptProc();

		for (int i = 0; i < wCount; i++)
		{
			if (FD_ISSET(_wSessions[wStartIdx + i]->_socket, &wset))
				SendProc(_wSessions[wStartIdx + i]);
		}

		for (int i = 0; i < rCount; i++)
		{
			if (FD_ISSET(_rSessions[rStartIdx + i]->_socket, &rset))
				RecvProc(_rSessions[rStartIdx + i]);
		}

	}
}

void Server::AcceptProc()
{
	if(_emptySessionID.empty())
	{
		::printf("Can't Accept More!\n");
		return;
	}

	int ID = _emptySessionID.back();
	Session* pSession = _pSessionPool->Alloc(ID);
	_emptySessionID.pop_back();

	if (pSession == nullptr)
	{
		::printf("Error! Func %s Line %d: fail new\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: new Error, %d\n",
			_T(__FUNCTION__), __LINE__);
		g_bShutdown = true;	
		return;
	}

	int addrlen = sizeof(pSession->_addr);
	pSession->_socket = accept(_listensock, (SOCKADDR*)&pSession->_addr, &addrlen);
	if (pSession->_socket == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: accept Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);
		g_bShutdown = true;
		return;
	}

	pSession->_lastRecvTime = timeGetTime();
	_SessionMap[pSession->_ID] = pSession;
	CreatePlayer(pSession);

	_acceptCnt++;
}

void Server::RecvProc(Session* pSession)
{
	pSession->_lastRecvTime = timeGetTime();

	PRO_BEGIN(L"Network: Recv");
	int recvRet = recv(pSession->_socket,
		pSession->_recvBuf.GetWriteBufferPtr(),
		pSession->_recvBuf.DirectEnqueueSize(), 0);
	PRO_END(L"Network: Recv");

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err == WSAECONNRESET)
		{
			_ConnResetCnt++;
			SetSessionDead(pSession);
			return;
		}
		else if (err == WSAECONNABORTED)
		{
			SetSessionDead(pSession);
			return;
		}
		else if(err != WSAEWOULDBLOCK)
		{
			::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: recv Error, %d\n",
				_T(__FUNCTION__), __LINE__, err);
			SetSessionDead(pSession);
			return;
		}

	}
	else if (recvRet == 0)
	{
		SetSessionDead(pSession);
		return;
	}

	int moveRet = pSession->_recvBuf.MoveWritePos(recvRet);
	if (recvRet != moveRet)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		g_bShutdown = true;
		return;
	}

	int useSize = pSession->_recvBuf.GetUseSize();

	Player* pPlayer = _PlayerMap[pSession->_ID];
	
	if (pPlayer == nullptr)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		g_bShutdown = true;
		return;
	}

	
	while (useSize > 0)
	{
		if (useSize <= dfHEADER_SIZE)
			break;

		st_PACKET_HEADER header;
		int peekRet = pSession->_recvBuf.Peek((char*)&header, dfHEADER_SIZE);
		if (peekRet != dfHEADER_SIZE)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
			g_bShutdown = true;
			return;
		}

		if ((char) header.byCode != (char)dfPACKET_CODE)
		{
			::printf("Error! Wrong Header Code! %x - Func %s Line %d\n", 
				header.byCode, __func__, __LINE__);
			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: Wrong Header Code, %x\n", 
				_T(__FUNCTION__), __LINE__, header.byCode);
			SetSessionDead(pSession);
			return;
		}

		if (useSize < dfHEADER_SIZE + header.bySize)
			break;

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_SIZE);
		if (moveReadRet != dfHEADER_SIZE)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
			g_bShutdown = true;
			return;
		}
		
		PRO_BEGIN(L"Network: Handle Packet");
		bool handlePacketRet = HandleCSPackets(pPlayer, header.byType);
		PRO_END(L"Network: Handle Packet");

		if (!handlePacketRet)
		{
			::printf("Error! Func %s Line %d: PlayerID %d, Session ID %d\n", 
				__func__, __LINE__, pPlayer->_ID, pSession->_ID);
			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: Handle CS Packet Error\n", _T(__FUNCTION__), __LINE__);
			SetSessionDead(pSession);
			return;
		}

		useSize = pSession->_recvBuf.GetUseSize();
	}
}

void Server::SendProc(Session* pSession)
{
	PRO_BEGIN(L"Network: Send");
	int sendRet = send(pSession->_socket,
		pSession->_sendBuf.GetReadBufferPtr(),
		pSession->_sendBuf.DirectDequeueSize(), 0);
	PRO_END(L"Network: Send");

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err == WSAECONNRESET)
		{
			SetSessionDead(pSession);
			return;
		}
		else if (err == WSAECONNABORTED)
		{
			SetSessionDead(pSession);
			return;
		}
		else if (err != WSAEWOULDBLOCK)
		{
			::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: send Error, %d\n", 
				_T(__FUNCTION__), __LINE__, err);
			SetSessionDead(pSession);
			return;
		}
	}

	int moveRet = pSession->_sendBuf.MoveReadPos(sendRet);
	if (sendRet != moveRet)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		SetSessionDead(pSession);
		return;
	}
}

void Server::SetSessionDead(Session* pSession)
{
	if(pSession->_alive)
	{
		pSession->_alive = false;
		_disconnectedSessionIDs.push_back(pSession->_ID);
	}
}

void Server::DisconnectDeadSession()
{
	vector<int>::iterator iter = _disconnectedSessionIDs.begin();

	for (; iter != _disconnectedSessionIDs.end(); iter++)
	{
		int ID = *iter;

		Player* pPlayer = _PlayerMap[ID];
		if (pPlayer == nullptr)
		{
			::printf("Error! Func %s Line %d (ID: %d)\n", __func__, __LINE__, ID);
			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: Session ID Find Error\n", _T(__FUNCTION__), __LINE__);
			g_bShutdown = true;
			return;
		}
		_PlayerMap[ID] = nullptr;

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

		SerializePacket* packet = _pSPacketPool->Alloc();

		packet->Clear();
		int deleteRet = SetSCPacket_DELETE_CHAR(packet, pPlayer->_ID);
		EnqueueAroundSector(packet->GetReadPtr(), deleteRet, pPlayer->_pSector);
		_pPlayerPool->Free(pPlayer);
		_pSPacketPool->Free(packet);

		Session* pSession = _SessionMap[ID];
		if (pSession == nullptr)
		{
			::printf("Error! Func %s Line %d (ID: %d)\n", __func__, __LINE__, ID);
			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: Session ID Find Error\n", _T(__FUNCTION__), __LINE__);
			g_bShutdown = true;
			return;
		}
		_SessionMap[ID] = nullptr;

		closesocket(pSession->_socket);	
		_pSessionPool->Free(pSession);
		_emptySessionID.push_back(ID);
		_disconnectCnt++;
	}

	_disconnectedSessionIDs.clear();
}

void Server::EnqueueUnicast(char* msg, int size, Session* pSession)
{
	if (pSession == nullptr)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		g_bShutdown = true;
		return;
	}

	int enqueueRet = pSession->_sendBuf.Enqueue(msg, size);
	if (enqueueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		SetSessionDead(pSession);
	}
}

void Server::EnqueueOneSector(char* msg, int size, Sector* sector, Session* pExpSession)
{
	int enqueueRet;
	if (pExpSession == nullptr)
	{
		vector<Player*>::iterator playerIter = sector->_players.begin();
		for (; playerIter < sector->_players.end(); playerIter++)
		{
			if ((*playerIter) == nullptr)
			{
				::printf("Error! Func %s Line %d\n", __func__, __LINE__);
				LOG(L"ERROR", SystemLog::ERROR_LEVEL,
					L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
				g_bShutdown = true;
				return;
			}

			enqueueRet = (*playerIter)->_pSession->_sendBuf.Enqueue(msg, size);
			if (enqueueRet != size)
			{
				::printf("Error! Func %s Line %d\n", __func__, __LINE__);
				LOG(L"ERROR", SystemLog::ERROR_LEVEL,
					L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
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
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					LOG(L"ERROR", SystemLog::ERROR_LEVEL,
						L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
					g_bShutdown = true;
					return;
				}

				enqueueRet = (*playerIter)->_pSession->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					LOG(L"ERROR", SystemLog::ERROR_LEVEL,
						L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
					SetSessionDead((*playerIter)->_pSession);
				}
			}
		}
	}
}

void Server::EnqueueAroundSector(char* msg, int size, Sector* centerSector, Session* pExpSession)
{
	int enqueueRet;
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
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					LOG(L"ERROR", SystemLog::ERROR_LEVEL,
						L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
					g_bShutdown = true;
					return;
				}

				enqueueRet = (*playerIter)->_pSession->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					LOG(L"ERROR", SystemLog::ERROR_LEVEL,
						L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
					SetSessionDead((*playerIter)->_pSession);
				}
			}
		}
	}
	else
	{		
		for (int i = 0; i < 8; i++)
		{
			vector<Player*>::iterator playerIter 
				= centerSector->_around[i]->_players.begin();

			for (; playerIter < centerSector->_around[i]->_players.end(); playerIter++)
			{
				if ((*playerIter) == nullptr)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					LOG(L"ERROR", SystemLog::ERROR_LEVEL,
						L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
					g_bShutdown = true;
					return;
				}

				enqueueRet = (*playerIter)->_pSession->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					LOG(L"ERROR", SystemLog::ERROR_LEVEL,
						L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
					SetSessionDead((*playerIter)->_pSession);
				}
			}
		}

		vector<Player*>::iterator playerIter 
			= centerSector->_around[8]->_players.begin();
		for (; playerIter < centerSector->_around[8]->_players.end(); playerIter++)
		{
			if ((*playerIter)->_pSession != pExpSession)
			{
				if ((*playerIter) == nullptr)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					LOG(L"ERROR", SystemLog::ERROR_LEVEL,
						L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
					g_bShutdown = true;
					return;
				}

				enqueueRet = (*playerIter)->_pSession->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					LOG(L"ERROR", SystemLog::ERROR_LEVEL,
						L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
					SetSessionDead((*playerIter)->_pSession);
				}
			}
		}
	}
}

// About Content ====================================================

bool Server::SkipForFixedFrame()
{
	static DWORD oldTick = timeGetTime();
	if ((timeGetTime() - oldTick) < (1000 / dfFPS))
		return true;
	oldTick += (1000 / dfFPS);
	return false;
}

bool Server::CheckMovable(short x, short y)
{
	if (x < dfRANGE_MOVE_LEFT || x > dfRANGE_MOVE_RIGHT ||
		y < dfRANGE_MOVE_TOP || y > dfRANGE_MOVE_BOTTOM)
		return false;

	return true;
}

void Server::UpdatePlayerMove(Player* pPlayer)
{
	switch (pPlayer->_moveDirection)
	{
	case dfPACKET_MOVE_DIR_LL:

		if (CheckMovable(pPlayer->_x - dfSPEED_PLAYER_X, pPlayer->_y))
			pPlayer->_x -= dfSPEED_PLAYER_X;

		if (pPlayer->_x < pPlayer->_pSector->_xPosMin)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_LL);

		break;

	case dfPACKET_MOVE_DIR_LU:
		
		if (CheckMovable(pPlayer->_x - dfSPEED_PLAYER_X, pPlayer->_y - dfSPEED_PLAYER_Y))
		{
			pPlayer->_x -= dfSPEED_PLAYER_X;
			pPlayer->_y -= dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_x < pPlayer->_pSector->_xPosMin &&
				pPlayer->_y < pPlayer->_pSector->_yPosMin)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_LU);

		else if (pPlayer->_x < pPlayer->_pSector->_xPosMin)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_LL);
		
		else if(pPlayer->_y < pPlayer->_pSector->_yPosMin)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_UU);

		break;

	case dfPACKET_MOVE_DIR_UU:
		
		if (CheckMovable(pPlayer->_x, pPlayer->_y - dfSPEED_PLAYER_Y))
			pPlayer->_y -= dfSPEED_PLAYER_Y;

		if (pPlayer->_y < pPlayer->_pSector->_yPosMin)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_UU);

		break;

	case dfPACKET_MOVE_DIR_RU:
		
		if (CheckMovable(pPlayer->_x + dfSPEED_PLAYER_X, pPlayer->_y - dfSPEED_PLAYER_Y))
		{
			pPlayer->_x += dfSPEED_PLAYER_X;
			pPlayer->_y -= dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_x > pPlayer->_pSector->_xPosMax &&
				pPlayer->_y < pPlayer->_pSector->_yPosMin)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_RU);

		else if (pPlayer->_x > pPlayer->_pSector->_xPosMax)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_RR);

		else if (pPlayer->_y < pPlayer->_pSector->_yPosMin)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_UU);

		break;

	case dfPACKET_MOVE_DIR_RR:

		if (CheckMovable(pPlayer->_x + dfSPEED_PLAYER_X, pPlayer->_y))
			pPlayer->_x += dfSPEED_PLAYER_X;

		if (pPlayer->_x > pPlayer->_pSector->_xPosMax)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_RR);

		break;

	case dfPACKET_MOVE_DIR_RD:
		
		if (CheckMovable(pPlayer->_x + dfSPEED_PLAYER_X, pPlayer->_y + dfSPEED_PLAYER_Y))
		{
			pPlayer->_x += dfSPEED_PLAYER_X;
			pPlayer->_y += dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_x > pPlayer->_pSector->_xPosMax &&
				pPlayer->_y > pPlayer->_pSector->_yPosMax)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_RD);

		else if (pPlayer->_x > pPlayer->_pSector->_xPosMax)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_RR);
		
		else if (pPlayer->_y > pPlayer->_pSector->_yPosMax)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_DD);

		break;

	case dfPACKET_MOVE_DIR_DD:
		
		if (CheckMovable(pPlayer->_x, pPlayer->_y + dfSPEED_PLAYER_Y))
			pPlayer->_y += dfSPEED_PLAYER_Y;

		if (pPlayer->_y > pPlayer->_pSector->_yPosMax)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_DD);
		
		break;

	case dfPACKET_MOVE_DIR_LD:
		
		if (CheckMovable(pPlayer->_x - dfSPEED_PLAYER_X, pPlayer->_y + dfSPEED_PLAYER_Y))
		{
			pPlayer->_x -= dfSPEED_PLAYER_X;
			pPlayer->_y += dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_x < pPlayer->_pSector->_xPosMin &&
				pPlayer->_y > pPlayer->_pSector->_yPosMax)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_LD);

		else if (pPlayer->_x < pPlayer->_pSector->_xPosMin)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_LL);

		else if (pPlayer->_y > pPlayer->_pSector->_yPosMax)
			UpdateSector(pPlayer, dfPACKET_MOVE_DIR_DD);

		break;
	}
}

void Server::SetSector(Player* pPlayer)
{
	int x = (pPlayer->_x / dfSECTOR_SIZE_X) + 2;
	int y = (pPlayer->_y / dfSECTOR_SIZE_Y) + 2;
	_sectors[y][x]._players.push_back(pPlayer);
	pPlayer->_pSector = &_sectors[y][x];
}

void Server::UpdateSector(Player* pPlayer, short direction)
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

	SerializePacket* packet = _pSPacketPool->Alloc();

	packet->Clear();
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(packet,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	for (int i = 0; i < sectorCnt; i++)
		EnqueueOneSector(packet->GetReadPtr(), createMeToOtherRet, inSector[i]);
	
	packet->Clear();
	int MoveMeToOtherRet = SetSCPacket_MOVE_START(packet,
		pPlayer->_ID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	for (int i = 0; i < sectorCnt; i++)
		EnqueueOneSector(packet->GetReadPtr(), MoveMeToOtherRet, inSector[i]);

	packet->Clear();
	int deleteMeToOtherRet = SetSCPacket_DELETE_CHAR(packet, pPlayer->_ID);
	for (int i = 0; i < sectorCnt; i++)
		EnqueueOneSector(packet->GetReadPtr(), deleteMeToOtherRet, outSector[i]);

	// Send Data About Other Player ==============================================
	
	for (int i = 0; i < sectorCnt; i++)
	{	
		vector<Player*>::iterator iter = inSector[i]->_players.begin();
		for(; iter < inSector[i]->_players.end(); iter++)
		{
			packet->Clear();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(packet,
				(*iter)->_ID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			EnqueueUnicast(packet->GetReadPtr(), createOtherRet, pPlayer->_pSession);

			if((*iter)->_move)
			{
				packet->Clear();
				int MoveOtherRet = SetSCPacket_MOVE_START(packet,
					(*iter)->_ID, (*iter)->_moveDirection, (*iter)->_x, (*iter)->_y);
				EnqueueUnicast(packet->GetReadPtr(), MoveOtherRet, pPlayer->_pSession);
			}
		}
	}

	for (int i = 0; i < sectorCnt; i++)
	{
		vector<Player*>::iterator iter = outSector[i]->_players.begin();
		for (; iter < outSector[i]->_players.end(); iter++)
		{
			packet->Clear();
			int deleteOtherRet = SetSCPacket_DELETE_CHAR(packet, (*iter)->_ID);
			EnqueueUnicast(packet->GetReadPtr(), deleteOtherRet, pPlayer->_pSession);
		}
	}

	pPlayer->_pSector = newSector;
	newSector->_players.push_back(pPlayer);
	_pSPacketPool->Free(packet);

	PRO_END(L"Content: Update Sector");
}

void Server::CreatePlayer(Session* pSession)
{
	Player* pPlayer = _pPlayerPool->Alloc(pSession, _playerID++);
	_PlayerMap[pSession->_ID] = pPlayer;
	SetSector(pPlayer);
	
	SerializePacket* packet = _pSPacketPool->Alloc();

	packet->Clear();
	int createMeRet = SetSCPacket_CREATE_MY_CHAR(packet,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	EnqueueUnicast(packet->GetReadPtr(), createMeRet, pPlayer->_pSession);

	packet->Clear();
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(packet,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	EnqueueAroundSector(packet->GetReadPtr(),
		createMeToOtherRet, pPlayer->_pSector, pPlayer->_pSession);
	
	for (int i = 0; i < 8; i++)
	{
		vector<Player*>::iterator iter 
			= pPlayer->_pSector->_around[i]->_players.begin();
		for (; iter < pPlayer->_pSector->_around[i]->_players.end(); iter++)
		{
			packet->Clear();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(packet,
				(*iter)->_ID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			EnqueueUnicast(packet->GetReadPtr(), createOtherRet, pPlayer->_pSession);
		}
	}

	vector<Player*>::iterator iter
		= pPlayer->_pSector->_around[8]->_players.begin();
	for (; iter < pPlayer->_pSector->_around[8]->_players.end(); iter++)
	{
		if ((*iter) != pPlayer)
		{
			packet->Clear();
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(packet,
				(*iter)->_ID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			EnqueueUnicast(packet->GetReadPtr(), createOtherRet, pPlayer->_pSession);
		}
	}
}

// About Packet ====================================================

bool Server::HandleCSPackets(Player* pPlayer, BYTE type)
{
	switch (type)
	{
	case dfPACKET_CS_MOVE_START:
	{
		return HandleCSPacket_MOVE_START(pPlayer);		
	}
		break;

	case dfPACKET_CS_MOVE_STOP:
	{
		return HandleCSPacket_MOVE_STOP(pPlayer);	
	}
		break;

	case dfPACKET_CS_ATTACK1:
	{
		return HandleCSPacket_ATTACK1(pPlayer);	
	}
		break;

	case dfPACKET_CS_ATTACK2:
	{
		return HandleCSPacket_ATTACK2(pPlayer);	
	}
		break;

	case dfPACKET_CS_ATTACK3:
	{
		return HandleCSPacket_ATTACK3(pPlayer);	
	}
		break;
	
	case dfPACKET_CS_ECHO:
	{
		return HandleCSPacket_ECHO(pPlayer);	
	}
		break;
	}

	::printf("Error! Func %s Line %d Case %d\n", __func__, __LINE__, type);
	LOG(L"ERROR", SystemLog::ERROR_LEVEL,
		L"%s[%d] No Switch Case, %d\n", _T(__FUNCTION__), __LINE__, type);
	return false;
}

bool Server::HandleCSPacket_MOVE_START(Player* pPlayer)
{
	BYTE moveDirection;
	short x;
	short y;
	bool getRet = GetCSPacket_MOVE_START(&(pPlayer->_pSession->_recvBuf), moveDirection, x, y);
	if(!getRet) 
	{
		return false;
	}

	SerializePacket* packet = _pSPacketPool->Alloc();

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		packet->Clear();
		int setRet = SetSCPacket_SYNC(packet, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(packet->GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	SetPlayerMoveStart(pPlayer, moveDirection, x, y);

	packet->Clear();
	int setRet = SetSCPacket_MOVE_START(packet, pPlayer->_ID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(packet->GetReadPtr(), setRet, pPlayer->_pSector, pPlayer->_pSession);

	_pSPacketPool->Free(packet);
	return true;
}

bool Server::HandleCSPacket_MOVE_STOP(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	bool getRet = GetCSPacket_MOVE_STOP(&(pPlayer->_pSession->_recvBuf), direction, x, y);
	if (!getRet) 
	{
		return false;
	}

	SerializePacket* packet = _pSPacketPool->Alloc();

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		packet->Clear();
		int setRet = SetSCPacket_SYNC(packet, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(packet->GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	SetPlayerMoveStop(pPlayer, direction, x, y);

	packet->Clear();
	int setRet = SetSCPacket_MOVE_STOP(packet, pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(packet->GetReadPtr(), setRet, pPlayer->_pSector, pPlayer->_pSession);

	return true;
}

bool Server::HandleCSPacket_ATTACK1(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	bool getRet = GetCSPacket_ATTACK1(&(pPlayer->_pSession->_recvBuf), direction, x, y);
	if (!getRet) 
	{
		return false;
	}

	SerializePacket* packet = _pSPacketPool->Alloc();

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		packet->Clear();
		int setRet = SetSCPacket_SYNC(packet, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(packet->GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	Player* damagedPlayer = nullptr;
	SetPlayerAttack1(pPlayer, damagedPlayer, direction, x, y);

	packet->Clear();
	int attackSetRet = SetSCPacket_ATTACK1(packet, 
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(packet->GetReadPtr(), attackSetRet, pPlayer->_pSector);

	if (damagedPlayer != nullptr)
	{
		packet->Clear();
		int damageSetRet = SetSCPacket_DAMAGE(
			packet, pPlayer->_ID, damagedPlayer->_ID, damagedPlayer->_hp);
		EnqueueAroundSector(packet->GetReadPtr(), damageSetRet, damagedPlayer->_pSector);
	}

	_pSPacketPool->Free(packet);
	return true;
}

bool Server::HandleCSPacket_ATTACK2(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	bool getRet = GetCSPacket_ATTACK2(&(pPlayer->_pSession->_recvBuf), direction, x, y);
	if (!getRet) 
	{
		return false;
	}

	SerializePacket* packet = _pSPacketPool->Alloc();

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		packet->Clear();
		int setRet = SetSCPacket_SYNC(packet, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(packet->GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	Player* damagedPlayer = nullptr;
	SetPlayerAttack2(pPlayer, damagedPlayer, direction, x, y);

	packet->Clear();
	int attackSetRet = SetSCPacket_ATTACK2(packet,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(packet->GetReadPtr(), attackSetRet, pPlayer->_pSector);

	if (damagedPlayer != nullptr)
	{
		packet->Clear();
		int damageSetRet = SetSCPacket_DAMAGE(
			packet, pPlayer->_ID, damagedPlayer->_ID, damagedPlayer->_hp);
		EnqueueAroundSector(packet->GetReadPtr(), damageSetRet, damagedPlayer->_pSector);
	}

	_pSPacketPool->Free(packet);
	return true;
}

bool Server::HandleCSPacket_ATTACK3(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	bool getRet = GetCSPacket_ATTACK3(&(pPlayer->_pSession->_recvBuf), direction, x, y);
	if (!getRet) 
	{
		return false;
	}

	SerializePacket* packet = _pSPacketPool->Alloc();

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		packet->Clear();
		int setRet = SetSCPacket_SYNC(packet, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(packet->GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	Player* damagedPlayer = nullptr;
	SetPlayerAttack3(pPlayer, damagedPlayer, direction, x, y);

	packet->Clear();
	int attackSetRet = SetSCPacket_ATTACK3(packet, 
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(packet->GetReadPtr(), attackSetRet, pPlayer->_pSector);

	if (damagedPlayer != nullptr)
	{
		packet->Clear();
		int damageSetRet = SetSCPacket_DAMAGE(
			packet, pPlayer->_ID, damagedPlayer->_ID, damagedPlayer->_hp);
		EnqueueAroundSector(packet->GetReadPtr(), damageSetRet, damagedPlayer->_pSector);
	}
	
	_pSPacketPool->Free(packet);
	return true;
}

bool Server::HandleCSPacket_ECHO(Player* pPlayer)
{
	int time;
	bool getRet = GetCSPacket_ECHO(&(pPlayer->_pSession->_recvBuf), time);
	if (!getRet) return false;

	SerializePacket* packet = _pSPacketPool->Alloc();
	packet->Clear();
	int setRet = SetSCPacket_ECHO(packet, time);
	EnqueueUnicast(packet->GetReadPtr(), setRet, pPlayer->_pSession);

	_pSPacketPool->Free(packet);
	return true;
}

bool Server::GetCSPacket_MOVE_START(RingBuffer* recvBuffer, BYTE& moveDirection, short& x, short& y)
{
	SerializePacket* packet = _pSPacketPool->Alloc();
	packet->Clear();

	int size = sizeof(moveDirection) + sizeof(x) + sizeof(y);
	int dequeueRet = recvBuffer->Dequeue(packet->GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
	packet->MoveWritePos(dequeueRet);

	*packet >> moveDirection;
	*packet >> x;
	*packet >> y;

	_pSPacketPool->Free(packet);
	return true;
}

bool Server::GetCSPacket_MOVE_STOP(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y)
{
	SerializePacket* packet = _pSPacketPool->Alloc();
	packet->Clear();

	int size = sizeof(direction) + sizeof(x) + sizeof(y);
	int dequeueRet = recvBuffer->Dequeue(packet->GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
	packet->MoveWritePos(dequeueRet);

	*packet >> direction;
	*packet >> x;
	*packet >> y;

	_pSPacketPool->Free(packet);
	return true;
}

bool Server::GetCSPacket_ATTACK1(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y)
{
	SerializePacket* packet = _pSPacketPool->Alloc();
	packet->Clear();

	int size = sizeof(direction) + sizeof(x) + sizeof(y);
	int dequeueRet = recvBuffer->Dequeue(packet->GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
	packet->MoveWritePos(dequeueRet);

	*packet >> direction;
	*packet >> x;
	*packet >> y;

	_pSPacketPool->Free(packet);
	return true;
}

bool Server::GetCSPacket_ATTACK2(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y)
{
	SerializePacket* packet = _pSPacketPool->Alloc();
	packet->Clear();

	int size = sizeof(direction) + sizeof(x) + sizeof(y);
	int dequeueRet = recvBuffer->Dequeue(packet->GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
	packet->MoveWritePos(dequeueRet);

	*packet >> direction;
	*packet >> x;
	*packet >> y;

	_pSPacketPool->Free(packet);
	return true;
}

bool Server::GetCSPacket_ATTACK3(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y)
{
	SerializePacket* packet = _pSPacketPool->Alloc();
	packet->Clear();

	int size = sizeof(direction) + sizeof(x) + sizeof(y);
	int dequeueRet = recvBuffer->Dequeue(packet->GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
	packet->MoveWritePos(dequeueRet);

	*packet >> direction;
	*packet >> x;
	*packet >> y;

	_pSPacketPool->Free(packet);
	return true;
}

bool Server::GetCSPacket_ECHO(RingBuffer* recvBuffer, int& time)
{
	SerializePacket* packet = _pSPacketPool->Alloc();
	packet->Clear();

	int size = sizeof(time);
	int dequeueRet = recvBuffer->Dequeue(packet->GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
	packet->MoveWritePos(dequeueRet);

	*packet >> time;
	_pSPacketPool->Free(packet);
	return true;
}

void Server::SetPlayerMoveStart(Player* pPlayer, BYTE& moveDirection, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_move = true;
	pPlayer->_moveDirection = moveDirection;

	switch (moveDirection)
	{
	case  dfPACKET_MOVE_DIR_LL:
	case  dfPACKET_MOVE_DIR_LU:
	case  dfPACKET_MOVE_DIR_LD:
		pPlayer->_direction = dfPACKET_MOVE_DIR_LL;
		break;

	case  dfPACKET_MOVE_DIR_RU:
	case  dfPACKET_MOVE_DIR_RR:
	case  dfPACKET_MOVE_DIR_RD:
		pPlayer->_direction = dfPACKET_MOVE_DIR_RR;
		break;
	}
	
}

void Server::SetPlayerMoveStop(Player* pPlayer, BYTE& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_move = false;
	pPlayer->_direction = direction;	
}

void Server::SetPlayerAttack1(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;

	if (direction == dfPACKET_MOVE_DIR_LL)
	{
		vector<Player*>::iterator iter 
			= pPlayer->_pSector->_around[8]->_players.begin();

		for (; iter < pPlayer->_pSector->_around[8]->_players.end(); iter++)
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

		for (int i = 0; i < 8; i++)
		{
			vector<Player*>::iterator iter
				= pPlayer->_pSector->_around[i]->_players.begin();

			for (; iter < pPlayer->_pSector->_around[i]->_players.end(); iter++)
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
	else if (direction == dfPACKET_MOVE_DIR_RR)
	{
		vector<Player*>::iterator iter 
			= pPlayer->_pSector->_around[8]->_players.begin();
		for (; iter < pPlayer->_pSector->_around[8]->_players.end(); iter++)
		{
			if((*iter) != pPlayer)
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

		
		for (int i = 0; i < 8; i++)
		{
			vector<Player*>::iterator iter
				= pPlayer->_pSector->_around[i]->_players.begin();
			for (; iter < pPlayer->_pSector->_around[i]->_players.end(); iter++)
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

void Server::SetPlayerAttack2(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;

	if (direction == dfPACKET_MOVE_DIR_LL)
	{	
		vector<Player*>::iterator iter 
			= pPlayer->_pSector->_around[8]->_players.begin();

		for (; iter < pPlayer->_pSector->_around[8]->_players.end(); iter++)
		{
			if((*iter) != pPlayer)
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

		
		for (int i = 0; i < 8; i++)
		{
			vector<Player*>::iterator iter
				= pPlayer->_pSector->_around[i]->_players.begin();
			for (; iter < pPlayer->_pSector->_around[i]->_players.end(); iter++)
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
	else if (direction == dfPACKET_MOVE_DIR_RR)
	{
		
		vector<Player*>::iterator iter 
			= pPlayer->_pSector->_around[8]->_players.begin();

		for (; iter < pPlayer->_pSector->_around[8]->_players.end(); iter++)
		{
			if((*iter) != pPlayer)
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

		
		for (int i = 0; i < 8; i++)
		{
			vector<Player*>::iterator iter
				= pPlayer->_pSector->_around[i]->_players.begin();
			for (; iter < pPlayer->_pSector->_around[i]->_players.end(); iter++)
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

void Server::SetPlayerAttack3(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;

	if (direction == dfPACKET_MOVE_DIR_LL)
	{
		vector<Player*>::iterator iter 
			= pPlayer->_pSector->_around[8]->_players.begin();
		for (; iter < pPlayer->_pSector->_around[8]->_players.end(); iter++)
		{
			if((*iter) != pPlayer)
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
		
		for (int i = 0; i < 8; i++)
		{
			vector<Player*>::iterator iter 
				= pPlayer->_pSector->_around[i]->_players.begin();
			for (; iter < pPlayer->_pSector->_around[i]->_players.end(); iter++)
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
	else if (direction == dfPACKET_MOVE_DIR_RR)
	{
		vector<Player*>::iterator iter 
			= pPlayer->_pSector->_around[8]->_players.begin();
		for (; iter < pPlayer->_pSector->_around[8]->_players.end(); iter++)
		{
			if((*iter)!= pPlayer)
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
		
		for (int i = 0; i < 8; i++)
		{
			vector<Player*>::iterator iter 
				= pPlayer->_pSector->_around[i]->_players.begin();
			for (; iter < pPlayer->_pSector->_around[i]->_players.end(); iter++)
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

void Server::SetSCPacket_HEADER(SerializePacket* buffer, BYTE size, BYTE type)
{
	*buffer << (BYTE)dfPACKET_CODE;
	*buffer << size;
	*buffer << type;
}

int Server::SetSCPacket_CREATE_MY_CHAR(SerializePacket* buffer, int ID, BYTE direction, short x, short y, BYTE hp)
{
	int size = sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y) + sizeof(hp);
	SetSCPacket_HEADER(buffer, size, dfPACKET_SC_CREATE_MY_CHARACTER);
	
	*buffer << ID;
	*buffer << direction;
	*buffer << x;
	*buffer << y;
	*buffer << hp;

	if (buffer->GetDataSize() != dfHEADER_SIZE + size)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), dfHEADER_SIZE + size, __func__, __LINE__);

		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %d\n",
			_T(__FUNCTION__), __LINE__, buffer->GetDataSize(), dfHEADER_SIZE + size);
	}

	
	return buffer->GetDataSize();
}

int Server::SetSCPacket_CREATE_OTHER_CHAR(SerializePacket* buffer, int ID, BYTE direction, short x, short y, BYTE hp)
{
	int size = sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y) + sizeof(hp);
	SetSCPacket_HEADER(buffer, size, dfPACKET_SC_CREATE_OTHER_CHARACTER);

	*buffer << ID;
	*buffer << direction;
	*buffer << x;
	*buffer << y;
	*buffer << hp;

	if (buffer->GetDataSize() != dfHEADER_SIZE + size)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), dfHEADER_SIZE + size, __func__, __LINE__);

		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %d\n",
			_T(__FUNCTION__), __LINE__, buffer->GetDataSize(), dfHEADER_SIZE + size);
	}

	
	return buffer->GetDataSize();
}

int Server::SetSCPacket_DELETE_CHAR(SerializePacket* buffer, int ID)
{
	int size = sizeof(ID);
	SetSCPacket_HEADER(buffer, size, dfPACKET_SC_DELETE_CHARACTER);

	*buffer << ID;

	if (buffer->GetDataSize() != dfHEADER_SIZE + size)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), dfHEADER_SIZE + size, __func__, __LINE__);

		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %d\n",
			_T(__FUNCTION__), __LINE__, buffer->GetDataSize(), dfHEADER_SIZE + size);
	}

	
	return buffer->GetDataSize();
}

int Server::SetSCPacket_MOVE_START(SerializePacket* buffer, int ID, BYTE moveDirection, short x, short y)
{
	int size = sizeof(ID) + sizeof(moveDirection) + sizeof(x) + sizeof(y);
	SetSCPacket_HEADER(buffer, size, dfPACKET_SC_MOVE_START);

	*buffer << ID;
	*buffer << moveDirection;
	*buffer << x;
	*buffer << y;

	if (buffer->GetDataSize() != dfHEADER_SIZE + size)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), dfHEADER_SIZE + size, __func__, __LINE__);

		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %d\n",
			_T(__FUNCTION__), __LINE__, buffer->GetDataSize(), dfHEADER_SIZE + size);
	}

	
	return buffer->GetDataSize();
}

int Server::SetSCPacket_MOVE_STOP(SerializePacket* buffer, int ID, BYTE direction, short x, short y)
{
	int size = sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);
	SetSCPacket_HEADER(buffer, size, dfPACKET_SC_MOVE_STOP);

	*buffer << ID;
	*buffer << direction;
	*buffer << x;
	*buffer << y;

	if (buffer->GetDataSize() != dfHEADER_SIZE + size)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), dfHEADER_SIZE + size, __func__, __LINE__);

		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %d\n",
			_T(__FUNCTION__), __LINE__, buffer->GetDataSize(), dfHEADER_SIZE + size);
	}

	
	return buffer->GetDataSize();
}

int Server::SetSCPacket_ATTACK1(SerializePacket* buffer, int ID, BYTE direction, short x, short y)
{
	int size = sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);
	SetSCPacket_HEADER(buffer, size, dfPACKET_SC_ATTACK1);

	*buffer << ID;
	*buffer << direction;
	*buffer << x;
	*buffer << y;

	if (buffer->GetDataSize() != dfHEADER_SIZE + size)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), dfHEADER_SIZE + size, __func__, __LINE__);

		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %d\n",
			_T(__FUNCTION__), __LINE__, buffer->GetDataSize(), dfHEADER_SIZE + size);
	}

	
	return buffer->GetDataSize();
}

int Server::SetSCPacket_ATTACK2(SerializePacket* buffer, int ID, BYTE direction, short x, short y)
{
	int size = sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);
	SetSCPacket_HEADER(buffer, size, dfPACKET_SC_ATTACK2);

	*buffer << ID;
	*buffer << direction;
	*buffer << x;
	*buffer << y;

	if (buffer->GetDataSize() != dfHEADER_SIZE + size)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), dfHEADER_SIZE + size, __func__, __LINE__);

		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %d\n",
			_T(__FUNCTION__), __LINE__, buffer->GetDataSize(), dfHEADER_SIZE + size);
	}

	
	return buffer->GetDataSize();
}

int Server::SetSCPacket_ATTACK3(SerializePacket* buffer, int ID, BYTE direction, short x, short y)
{
	int size = sizeof(ID) + sizeof(direction) + sizeof(x) + sizeof(y);
	SetSCPacket_HEADER(buffer, size, dfPACKET_SC_ATTACK3);

	*buffer << ID;
	*buffer << direction;
	*buffer << x;
	*buffer << y;

	if (buffer->GetDataSize() != dfHEADER_SIZE + size)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), dfHEADER_SIZE + size, __func__, __LINE__);

		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %d\n",
			_T(__FUNCTION__), __LINE__, buffer->GetDataSize(), dfHEADER_SIZE + size);
	}

	
	return buffer->GetDataSize();
}

int Server::SetSCPacket_DAMAGE(SerializePacket* buffer, int attackID, int damageID, BYTE damageHP)
{
	int size = sizeof(attackID) + sizeof(damageID) + sizeof(damageHP);
	SetSCPacket_HEADER(buffer, size, dfPACKET_SC_DAMAGE);

	*buffer << attackID;
	*buffer << damageID;
	*buffer << damageHP;

	if (buffer->GetDataSize() != dfHEADER_SIZE + size)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), dfHEADER_SIZE + size, __func__, __LINE__);

		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %d\n",
			_T(__FUNCTION__), __LINE__, buffer->GetDataSize(), dfHEADER_SIZE + size);
	}

	
	return buffer->GetDataSize();
}

int Server::SetSCPacket_SYNC(SerializePacket* buffer, int ID, short x, short y)
{
	int size = sizeof(ID) + sizeof(x) + sizeof(y);
	SetSCPacket_HEADER(buffer, size, dfPACKET_SC_SYNC);

	*buffer << ID;
	*buffer << x;
	*buffer << y;

	if (buffer->GetDataSize() != dfHEADER_SIZE + size)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), dfHEADER_SIZE + size, __func__, __LINE__);

		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %d\n",
			_T(__FUNCTION__), __LINE__, buffer->GetDataSize(), dfHEADER_SIZE + size);
	}

	_syncCnt++;
	return buffer->GetDataSize();
}

int Server::SetSCPacket_ECHO(SerializePacket* buffer, int time)
{
	int size = sizeof(time);
	SetSCPacket_HEADER(buffer, size, dfPACKET_SC_ECHO);

	*buffer << time;

	if (buffer->GetDataSize() != dfHEADER_SIZE + size)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), dfHEADER_SIZE + size, __func__, __LINE__);

		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]: Create Packet Error, %d != %d\n",
			_T(__FUNCTION__), __LINE__, buffer->GetDataSize(), dfHEADER_SIZE + size);
	}
	
	return buffer->GetDataSize();
}

void Server::Sector::InitializeSector(short xIndex, short yIndex)
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

void Server::SetSectorsAroundInfo()
{
	for (int y = 2; y < dfSECTOR_CNT_Y - 2; y++)
	{
		for (int x = 2; x < dfSECTOR_CNT_X - 2; x++)
		{
			Sector* pSector = &_sectors[y][x];

			pSector->_around[dfPACKET_MOVE_DIR_LL] = &_sectors[y][x - 1];
			pSector->_around[dfPACKET_MOVE_DIR_LU] = &_sectors[y - 1][x - 1];
			pSector->_around[dfPACKET_MOVE_DIR_UU] = &_sectors[y - 1][x];
			pSector->_around[dfPACKET_MOVE_DIR_RU] = &_sectors[y - 1][x + 1];
			pSector->_around[dfPACKET_MOVE_DIR_RR] = &_sectors[y][x + 1];
			pSector->_around[dfPACKET_MOVE_DIR_RD] = &_sectors[y + 1][x + 1];
			pSector->_around[dfPACKET_MOVE_DIR_DD] = &_sectors[y + 1][x];
			pSector->_around[dfPACKET_MOVE_DIR_LD] = &_sectors[y + 1][x - 1];
			pSector->_around[8] = &_sectors[y][x];
			
			// dfPACKET_MOVE_DIR_LL

			pSector->_llNew[0] = &_sectors[y - 1][x - 2];
			pSector->_llNew[1] = &_sectors[y][x - 2];
			pSector->_llNew[2] = &_sectors[y + 1][x - 2];

			pSector->_llOld[0] = &_sectors[y - 1][x + 1];
			pSector->_llOld[1] = &_sectors[y][x + 1];
			pSector->_llOld[2] = &_sectors[y + 1][x + 1];

			// dfPACKET_MOVE_DIR_LU

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

			// dfPACKET_MOVE_DIR_UU

			pSector->_uuNew[0] = &_sectors[y - 2][x - 1];
			pSector->_uuNew[1] = &_sectors[y - 2][x];
			pSector->_uuNew[2] = &_sectors[y - 2][x + 1];

			pSector->_uuOld[0] = &_sectors[y + 1][x - 1];
			pSector->_uuOld[1] = &_sectors[y + 1][x];
			pSector->_uuOld[2] = &_sectors[y + 1][x + 1];

			// dfPACKET_MOVE_DIR_RU

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

			// dfPACKET_MOVE_DIR_RR

			pSector->_rrNew[0] = &_sectors[y - 1][x + 2];
			pSector->_rrNew[1] = &_sectors[y][x + 2];
			pSector->_rrNew[2] = &_sectors[y + 1][x + 2];

			pSector->_rrOld[0] = &_sectors[y - 1][x - 1];
			pSector->_rrOld[1] = &_sectors[y][x - 1];
			pSector->_rrOld[2] = &_sectors[y + 1][x - 1];

			// dfPACKET_MOVE_DIR_RD

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

			// dfPACKET_MOVE_DIR_DD

			pSector->_ddNew[0] = &_sectors[y + 2][x - 1];
			pSector->_ddNew[1] = &_sectors[y + 2][x];
			pSector->_ddNew[2] = &_sectors[y + 2][x + 1];

			pSector->_ddOld[0] = &_sectors[y - 1][x - 1];
			pSector->_ddOld[1] = &_sectors[y - 1][x];
			pSector->_ddOld[2] = &_sectors[y - 1][x + 1];

			// dfPACKET_MOVE_DIR_LD

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
