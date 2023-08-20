#include "Server.h"
#include "Main.h"
#include <stdio.h>

Server::Server()
{
	int err;
	int bindRet;
	int listenRet;
	int ioctRet;

	srand(0);
	CreateDirectory(L"ProfileBasic", NULL);
	
	_pPlayerPool = new CObjectPool<Player>(dfPLAYER_MAX, true);
	_pSessionPool = new CObjectPool<Session>(dfSESSION_MAX, true);
	memset(_SessionMap, 0, dfSESSION_MAX * sizeof(Session*));
	memset(_PlayerMap, 0, dfSESSION_MAX * sizeof(Session*));

	_emptySessionID.reserve(dfSESSION_MAX);
	for (int i = dfSESSION_MAX - 1; i >= 0 ; i--)
	{
		_emptySessionID.push_back(i);
	}

	_acceptedSessions.reserve(dfDEFAULT_ACCEPTED_SESSIONS_NUM);
	_disconnectedSessionIDs.reserve(dfDEFAULT_DISCONNECT_SESSIONS_NUM);

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

	_oldTick = timeGetTime();
	::printf("Content Setting Complete!\n");

}

Server::~Server()
{	
	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		if (_SessionMap[i] == nullptr) continue;
		closesocket(_SessionMap[i]->_socket);
		_pSessionPool->Free(_SessionMap[i]);
	}

	for (int i = 0; i < dfPLAYER_MAX; i++)
	{
		if (_PlayerMap[i] == nullptr) continue;
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

	FD_SET rset;
	FD_SET wset;

	int idx = 1;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(_listensock, &rset);

	_connected = dfSESSION_MAX - _emptySessionID.size();

	if (_connected > 0)
	{
		for (int i = 0; i < dfSESSION_MAX; i++)
		{
			if (_SessionMap[i] == nullptr) continue;

			FD_SET(_SessionMap[i]->_socket, &rset);
			if (_SessionMap[i]->_sendBuf.GetUseSize() > 0)
				FD_SET(_SessionMap[i]->_socket, &wset);

			_sessionArray[idx] = _SessionMap[i];
			idx++;

			if (idx == FD_SETSIZE)
			{
				SelectProc(rset, wset, FD_SETSIZE);

				idx = 1;
				FD_ZERO(&rset);
				FD_ZERO(&wset);
				FD_SET(_listensock, &rset);
			}
		}

		if (idx > 1)
		{
			SelectProc(rset, wset, idx);
		}
	}
	else
	{
		// Select Socket Set
		int selectRet = select(0, &rset, NULL, NULL, &_time);
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

		if (selectRet > 0 && FD_ISSET(_listensock, &rset))
		{
			AcceptProc();
		}
	}

	PRO_END(L"Network");

	SetAcceptedSession();

	PRO_BEGIN(L"Delayed Disconnect");
	DisconnectDeadSession();
	PRO_END(L"Delayed Disconnect");	
}

void Server::ContentUpdate()
{
	if (SkipForFixedFrame()) return;

	PRO_BEGIN(L"Content");
	
	for (int i = 0; i < dfPLAYER_MAX; i++)
	{
		if (_PlayerMap[i] == nullptr) continue;

		if (timeGetTime() - _PlayerMap[i]->_pSession->_lastRecvTime > dfNETWORK_PACKET_RECV_TIMEOUT)
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

void Server::SelectProc(FD_SET rset, FD_SET wset, int max)
{
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

		for (int i = 1; i < max; i++)
		{
			if (FD_ISSET(_sessionArray[i]->_socket, &wset))	
				SendProc(_sessionArray[i]);

			if (FD_ISSET(_sessionArray[i]->_socket, &rset))
				RecvProc(_sessionArray[i]);
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
	_acceptedSessions.push_back(pSession);
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

void Server::SetAcceptedSession()
{
	vector<Session*>::iterator sessionIter =  _acceptedSessions.begin();
	for (; sessionIter < _acceptedSessions.end(); sessionIter++)
	{
		_SessionMap[(*sessionIter)->_ID] = (*sessionIter);
		CreatePlayer((*sessionIter));
		_acceptCnt++;
	}
	_acceptedSessions.clear();
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

		Player* pPlayer = _PlayerMap[(*iter)];
		if (pPlayer == nullptr)
		{
			::printf("Error! Func %s Line %d (ID: %d)\n",
				__func__, __LINE__, (*iter));
			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: Session ID Find Error\n", _T(__FUNCTION__), __LINE__);
			g_bShutdown = true;
			return;
		}
		_PlayerMap[(*iter)] = nullptr;

		SerializePacket buffer;
		int deleteRet = SetSCPacket_DELETE_CHAR(&buffer, pPlayer->_ID);
		EnqueueAroundSector(buffer.GetReadPtr(), deleteRet, pPlayer->_pSector);

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
		_pPlayerPool->Free(pPlayer);

		Session* pSession = _SessionMap[(*iter)];	
		if (pSession == nullptr)
		{
			::printf("Error! Func %s Line %d (ID: %d)\n", 
				__func__, __LINE__, (*iter));
			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: Session ID Find Error\n", _T(__FUNCTION__), __LINE__);
			g_bShutdown = true;
			return;
		}
		
		closesocket(pSession->_socket);
		_SessionMap[(*iter)] = nullptr;
		_pSessionPool->Free(pSession);
		_emptySessionID.push_back((*iter));

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
		Sector sectors[dfAROUND_SECTOR_NUM];
		GetAroundSector(centerSector, sectors);

		for(int i = 0; i < dfAROUND_SECTOR_NUM; i++)
		{
			vector<Player*>::iterator playerIter = sectors[i]._players.begin();
			for (; playerIter < sectors[i]._players.end(); playerIter++)
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
		Sector sectors[dfAROUND_SECTOR_NUM];
		GetAroundSector(centerSector, sectors);

		vector<Player*>::iterator playerIter = sectors[0]._players.begin();
		for (; playerIter < sectors[0]._players.end(); playerIter++)
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

		
		for (int i = 1; i < dfAROUND_SECTOR_NUM; i++)
		{
			vector<Player*>::iterator playerIter = sectors[i]._players.begin();
			for (; playerIter < sectors[i]._players.end(); playerIter++)
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
		{
			pPlayer->_x -= dfSPEED_PLAYER_X;
		}

		if (pPlayer->_x < pPlayer->_pSector->_xPosMin)
		{
			Sector inSector[3] = 
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex - 2],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex - 2],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 2]
			};

			Sector outSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 1]
			};
			
			UpdateSector(pPlayer, inSector, outSector, 3, 3,
				&_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex - 1]);
		}

		break;

	case dfPACKET_MOVE_DIR_LU:
		if (CheckMovable(pPlayer->_x - dfSPEED_PLAYER_X, pPlayer->_y - dfSPEED_PLAYER_Y))
		{
			pPlayer->_x -= dfSPEED_PLAYER_X;
			pPlayer->_y -= dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_x < pPlayer->_pSector->_xPosMin &&
			pPlayer->_y < pPlayer->_pSector->_yPosMin)
		{
			Sector inSector[5] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex - 2],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex - 2],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex - 2],
				
			};

			Sector outSector[5] =
			{
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex + 1],
				
			};

			UpdateSector(pPlayer, inSector, outSector, 5, 5,
				&_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex - 1]);
		}
		else if (pPlayer->_x < pPlayer->_pSector->_xPosMin)
		{
			Sector inSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex - 2],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex - 2],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 2]
			};

			Sector outSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 1]
			};

			UpdateSector(pPlayer, inSector, outSector, 3, 3,
				&_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex - 1]);
		}
		else if(pPlayer->_y < pPlayer->_pSector->_yPosMin)
		{
			Sector inSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex + 1]
			};

			Sector outSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 1]
			};

			UpdateSector(pPlayer, inSector, outSector, 3, 3,
				&_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex]);
		}

		break;

	case dfPACKET_MOVE_DIR_UU:
		if (CheckMovable(pPlayer->_x, pPlayer->_y - dfSPEED_PLAYER_Y))
		{
			pPlayer->_y -= dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_y < pPlayer->_pSector->_yPosMin)
		{
			Sector inSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex + 1]
			};

			Sector outSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 1]
			};

			UpdateSector(pPlayer, inSector, outSector, 3, 3,
				&_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex]);
		}

		break;

	case dfPACKET_MOVE_DIR_RU:
		if (CheckMovable(pPlayer->_x + dfSPEED_PLAYER_X, pPlayer->_y - dfSPEED_PLAYER_Y))
		{
			pPlayer->_x += dfSPEED_PLAYER_X;
			pPlayer->_y -= dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_x > pPlayer->_pSector->_xPosMax &&
			pPlayer->_y < pPlayer->_pSector->_yPosMin)
		{
			Sector inSector[5] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex + 2],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 2],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex + 2]
			};

			Sector outSector[5] =
			{
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex]
			};

			UpdateSector(pPlayer, inSector, outSector, 5, 5,
				&_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 1]);
		}
		else if (pPlayer->_x > pPlayer->_pSector->_xPosMax)
		{
			Sector inSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 2],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex + 2],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 2]
			};

			Sector outSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 1]
			};

			UpdateSector(pPlayer, inSector, outSector, 3, 3,
				&_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex + 1]);
		}
		else if (pPlayer->_y < pPlayer->_pSector->_yPosMin)
		{
			Sector inSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex - 2][pPlayer->_pSector->_xIndex + 1]
			};

			Sector outSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 1]
			};

			UpdateSector(pPlayer, inSector, outSector, 3, 3,
				&_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex]);
		}
		break;

	case dfPACKET_MOVE_DIR_RR:
		if (CheckMovable(pPlayer->_x + dfSPEED_PLAYER_X, pPlayer->_y))
		{
			pPlayer->_x += dfSPEED_PLAYER_X;
		}

		if (pPlayer->_x > pPlayer->_pSector->_xPosMax)
		{
			Sector inSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 2],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex + 2],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 2]
			};

			Sector outSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 1]
			};

			UpdateSector(pPlayer, inSector, outSector, 3, 3,
				&_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex + 1]);
		}
		break;

	case dfPACKET_MOVE_DIR_RD:
		if (CheckMovable(pPlayer->_x + dfSPEED_PLAYER_X, pPlayer->_y + dfSPEED_PLAYER_Y))
		{
			pPlayer->_x += dfSPEED_PLAYER_X;
			pPlayer->_y += dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_x > pPlayer->_pSector->_xPosMax &&
			pPlayer->_y > pPlayer->_pSector->_yPosMax)
		{
			Sector inSector[5] =
			{
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex + 2],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 2],
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex + 2],
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex]
			};

			Sector outSector[5] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex - 1]
			};

			UpdateSector(pPlayer, inSector, outSector, 5, 5,
				&_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 1]);
		}
		else if (pPlayer->_x > pPlayer->_pSector->_xPosMax)
		{
			Sector inSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 2],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex + 2],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 2]
			};

			Sector outSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 1]
			};

			UpdateSector(pPlayer, inSector, outSector, 3, 3,
				&_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex + 1]);
		}
		else if (pPlayer->_y > pPlayer->_pSector->_yPosMax)
		{
			Sector inSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex + 1]
			};

			Sector outSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 1]
			};

			UpdateSector(pPlayer, inSector, outSector, 3, 3,
				&_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex]);
		}
		break;

	case dfPACKET_MOVE_DIR_DD:
		if (CheckMovable(pPlayer->_x, pPlayer->_y + dfSPEED_PLAYER_Y))
		{
			pPlayer->_y += dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_y > pPlayer->_pSector->_yPosMax)
		{
			Sector inSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex + 1]
			};

			Sector outSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 1]
			};

			UpdateSector(pPlayer, inSector, outSector, 3, 3,
				&_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex]);
		}
		break;

	case dfPACKET_MOVE_DIR_LD:
		if (CheckMovable(pPlayer->_x - dfSPEED_PLAYER_X, pPlayer->_y + dfSPEED_PLAYER_Y))
		{
			pPlayer->_x -= dfSPEED_PLAYER_X;
			pPlayer->_y += dfSPEED_PLAYER_Y;
		}

		if (pPlayer->_x < pPlayer->_pSector->_xPosMin &&
			pPlayer->_y > pPlayer->_pSector->_yPosMax)
		{
			Sector inSector[5] =
			{
				
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex - 2],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 2],
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex - 2],
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex]
			};

			Sector outSector[5] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex + 1],
				
			};

			UpdateSector(pPlayer, inSector, outSector, 5, 5,
				&_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 1]);
		}
		else if (pPlayer->_x < pPlayer->_pSector->_xPosMin)
		{
			Sector inSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex - 2],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex - 2],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex - 2]
			};

			Sector outSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex + 1],
				_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex + 1]
			};

			UpdateSector(pPlayer, inSector, outSector, 3, 3,
				&_sectors[pPlayer->_pSector->_yIndex][pPlayer->_pSector->_xIndex - 1]);
		}
		else if (pPlayer->_y > pPlayer->_pSector->_yPosMax)
		{
			Sector inSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex + 2][pPlayer->_pSector->_xIndex + 1]
			};

			Sector outSector[3] =
			{
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex - 1],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex],
				_sectors[pPlayer->_pSector->_yIndex - 1][pPlayer->_pSector->_xIndex + 1]
			};

			UpdateSector(pPlayer, inSector, outSector, 3, 3,
				&_sectors[pPlayer->_pSector->_yIndex + 1][pPlayer->_pSector->_xIndex]);
		}
		break;
	}

}

void Server::SetSector(Player* pPlayer)
{
	int x = pPlayer->_x / dfSECTOR_SIZE_X + 1;
	int y = pPlayer->_y / dfSECTOR_SIZE_Y + 1;
	_sectors[y][x]._players.push_back(pPlayer);
	pPlayer->_pSector = &_sectors[y][x];
}

void Server::GetAroundSector(Sector* centerSector, Sector* aroundSector)
{
	aroundSector[0] = _sectors[centerSector->_yIndex][centerSector->_xIndex];
	aroundSector[1] = _sectors[centerSector->_yIndex + 1][centerSector->_xIndex + 1];
	aroundSector[2] = _sectors[centerSector->_yIndex + 1][centerSector->_xIndex];
	aroundSector[3] = _sectors[centerSector->_yIndex + 1][centerSector->_xIndex - 1];
	aroundSector[4] = _sectors[centerSector->_yIndex][centerSector->_xIndex + 1];
	aroundSector[5] = _sectors[centerSector->_yIndex][centerSector->_xIndex - 1];
	aroundSector[6] = _sectors[centerSector->_yIndex - 1][centerSector->_xIndex + 1];
	aroundSector[7] = _sectors[centerSector->_yIndex - 1][centerSector->_xIndex];
	aroundSector[8] = _sectors[centerSector->_yIndex - 1][centerSector->_xIndex - 1];
}

void Server::UpdateSector(Player* pPlayer, Sector* inSector, Sector* outSector, 
	int inSectorNum, int outSectorNum, Sector* newSector)
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

	// About My Player ==============================================

	SerializePacket createMeToOther;
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(&createMeToOther,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	for (int i = 0; i < inSectorNum; i++)
		EnqueueOneSector(createMeToOther.GetReadPtr(), 
			createMeToOtherRet, &inSector[i], pPlayer->_pSession);

	SerializePacket MoveMeToOther;
	int MoveMeToOtherRet = SetSCPacket_MOVE_START(&MoveMeToOther,
		pPlayer->_ID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	for (int i = 0; i < inSectorNum; i++)
		EnqueueOneSector(MoveMeToOther.GetReadPtr(), 
			MoveMeToOtherRet, &inSector[i], pPlayer->_pSession);

	SerializePacket deleteMeToOther;
	int deleteMeToOtherRet = SetSCPacket_DELETE_CHAR(&deleteMeToOther, pPlayer->_ID);
	for (int i = 0; i < outSectorNum; i++)
		EnqueueOneSector(deleteMeToOther.GetReadPtr(), 
			deleteMeToOtherRet, &outSector[i], pPlayer->_pSession);

	// About Other Player ==============================================
	
	for (int i = 0; i < inSectorNum; i++)
	{	
		vector<Player*>::iterator iter = inSector[i]._players.begin();
		for(; iter < inSector[i]._players.end(); iter++)
		{
			SerializePacket createOther;
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(&createOther,
				(*iter)->_ID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			EnqueueUnicast(createOther.GetReadPtr(), createOtherRet, pPlayer->_pSession);

			if((*iter)->_move)
			{
				SerializePacket MoveOther;
				int MoveOtherRet = SetSCPacket_MOVE_START(&MoveOther,
					(*iter)->_ID, (*iter)->_moveDirection, (*iter)->_x, (*iter)->_y);
				EnqueueUnicast(MoveOther.GetReadPtr(), MoveOtherRet, pPlayer->_pSession);
			}
		}
	}

	
	for (int i = 0; i < outSectorNum; i++)
	{
		vector<Player*>::iterator iter = outSector[i]._players.begin();
		for (; iter < outSector[i]._players.end(); iter++)
		{
			SerializePacket deleteOther;
			int deleteOtherRet = SetSCPacket_DELETE_CHAR(&deleteOther, (*iter)->_ID);
			EnqueueUnicast(deleteOther.GetReadPtr(), deleteOtherRet, pPlayer->_pSession);
		}
	}

	newSector->_players.push_back(pPlayer);
	pPlayer->_pSector = newSector;

	PRO_END(L"Content: Update Sector");
}

void Server::CreatePlayer(Session* pSession)
{
	Player* pPlayer = _pPlayerPool->Alloc(pSession, _playerID++);
	SetSector(pPlayer);
	_PlayerMap[pSession->_ID] = pPlayer;

	SerializePacket createMe;
	int createMeRet = SetSCPacket_CREATE_MY_CHAR(&createMe,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	EnqueueUnicast(createMe.GetReadPtr(), createMeRet, pPlayer->_pSession);

	SerializePacket createMeToOther;
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(&createMeToOther,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	EnqueueAroundSector(createMeToOther.GetReadPtr(), 
		createMeToOtherRet, pPlayer->_pSector, pPlayer->_pSession);
	
	Sector sectors[9];
	GetAroundSector(pPlayer->_pSector, sectors);
	
	vector<Player*>::iterator iter = sectors[0]._players.begin();
	for (; iter < sectors[0]._players.end(); iter++)
	{
		if((*iter) != pPlayer)
		{
			SerializePacket createOther;
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(&createOther,
				(*iter)->_ID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			EnqueueUnicast(createOther.GetReadPtr(), createOtherRet, pPlayer->_pSession);
		}
	}
	
	for (int i = 1; i < 9; i++)
	{
		iter = sectors[i]._players.begin();
		for (; iter < sectors[i]._players.end(); iter++)
		{
			SerializePacket createOther;
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(&createOther,
				(*iter)->_ID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			EnqueueUnicast(createOther.GetReadPtr(), createOtherRet, pPlayer->_pSession);
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

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		SerializePacket buffer;
		int setRet = SetSCPacket_SYNC(&buffer, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(buffer.GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	SetPlayerMoveStart(pPlayer, moveDirection, x, y);

	SerializePacket buffer;
	int setRet = SetSCPacket_MOVE_START(&buffer, pPlayer->_ID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(buffer.GetReadPtr(), setRet, pPlayer->_pSector, pPlayer->_pSession);

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

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		SerializePacket buffer;
		int setRet = SetSCPacket_SYNC(&buffer, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(buffer.GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	SetPlayerMoveStop(pPlayer, direction, x, y);

	SerializePacket buffer;
	int setRet = SetSCPacket_MOVE_STOP(&buffer, pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(buffer.GetReadPtr(), setRet, pPlayer->_pSector, pPlayer->_pSession);

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

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		SerializePacket buffer;
		int setRet = SetSCPacket_SYNC(&buffer, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(buffer.GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	Player* damagedPlayer = nullptr;
	SetPlayerAttack1(pPlayer, damagedPlayer, direction, x, y);

	SerializePacket attackBuffer;
	int attackSetRet = SetSCPacket_ATTACK1(&attackBuffer, 
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(attackBuffer.GetReadPtr(), attackSetRet, pPlayer->_pSector);

	if (damagedPlayer != nullptr)
	{
		SerializePacket damageBuffer;
		int damageSetRet = SetSCPacket_DAMAGE(
			&damageBuffer, pPlayer->_ID, damagedPlayer->_ID, damagedPlayer->_hp);
		EnqueueAroundSector(damageBuffer.GetReadPtr(), damageSetRet, damagedPlayer->_pSector);
	}

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

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		SerializePacket buffer;
		int setRet = SetSCPacket_SYNC(&buffer, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(buffer.GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	Player* damagedPlayer = nullptr;
	SetPlayerAttack2(pPlayer, damagedPlayer, direction, x, y);

	SerializePacket attackBuffer;
	int attackSetRet = SetSCPacket_ATTACK2(&attackBuffer, 
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(attackBuffer.GetReadPtr(), attackSetRet, pPlayer->_pSector);


	if (damagedPlayer != nullptr)
	{
		SerializePacket damageBuffer;
		int damageSetRet = SetSCPacket_DAMAGE(
			&damageBuffer, pPlayer->_ID, damagedPlayer->_ID, damagedPlayer->_hp);
		EnqueueAroundSector(damageBuffer.GetReadPtr(), damageSetRet, damagedPlayer->_pSector);
	}

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

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		SerializePacket buffer;
		int setRet = SetSCPacket_SYNC(&buffer, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(buffer.GetReadPtr(), setRet, pPlayer->_pSession);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	Player* damagedPlayer = nullptr;
	SetPlayerAttack3(pPlayer, damagedPlayer, direction, x, y);

	SerializePacket attackBuffer;
	int attackSetRet = SetSCPacket_ATTACK3(&attackBuffer, 
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(attackBuffer.GetReadPtr(), attackSetRet, pPlayer->_pSector);

	if (damagedPlayer != nullptr)
	{
		SerializePacket damageBuffer;
		int damageSetRet = SetSCPacket_DAMAGE(
			&damageBuffer, pPlayer->_ID, damagedPlayer->_ID, damagedPlayer->_hp);
		EnqueueAroundSector(damageBuffer.GetReadPtr(), damageSetRet, damagedPlayer->_pSector);
	}
	
	return true;
}

bool Server::HandleCSPacket_ECHO(Player* pPlayer)
{
	int time;
	bool getRet = GetCSPacket_ECHO(&(pPlayer->_pSession->_recvBuf), time);
	if (!getRet) return false;

	SerializePacket buffer;
	int setRet = SetSCPacket_ECHO(&buffer, time);
	EnqueueUnicast(buffer.GetReadPtr(), setRet, pPlayer->_pSession);

	return true;
}

bool Server::GetCSPacket_MOVE_START(RingBuffer* recvBuffer, BYTE& moveDirection, short& x, short& y)
{
	SerializePacket buffer;

	int size = sizeof(moveDirection) + sizeof(x) + sizeof(y);
	int dequeueRet = recvBuffer->Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
	buffer.MoveWritePos(dequeueRet);

	buffer >> moveDirection;
	buffer >> x;
	buffer >> y;

	
	return true;
}

bool Server::GetCSPacket_MOVE_STOP(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y)
{
	SerializePacket buffer;

	int size = sizeof(direction) + sizeof(x) + sizeof(y);
	int dequeueRet = recvBuffer->Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
	buffer.MoveWritePos(dequeueRet);

	buffer >> direction;
	buffer >> x;
	buffer >> y;

	return true;
}

bool Server::GetCSPacket_ATTACK1(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y)
{
	SerializePacket buffer;

	int size = sizeof(direction) + sizeof(x) + sizeof(y);
	int dequeueRet = recvBuffer->Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
	buffer.MoveWritePos(dequeueRet);

	buffer >> direction;
	buffer >> x;
	buffer >> y;

	
	return true;
}

bool Server::GetCSPacket_ATTACK2(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y)
{
	SerializePacket buffer;

	int size = sizeof(direction) + sizeof(x) + sizeof(y);
	int dequeueRet = recvBuffer->Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
	buffer.MoveWritePos(dequeueRet);

	buffer >> direction;
	buffer >> x;
	buffer >> y;

	
	return true;
}

bool Server::GetCSPacket_ATTACK3(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y)
{
	SerializePacket buffer;

	int size = sizeof(direction) + sizeof(x) + sizeof(y);
	int dequeueRet = recvBuffer->Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
	buffer.MoveWritePos(dequeueRet);

	buffer >> direction;
	buffer >> x;
	buffer >> y;

	
	return true;
}

bool Server::GetCSPacket_ECHO(RingBuffer* recvBuffer, int& time)
{
	SerializePacket buffer;

	int size = sizeof(time);
	int dequeueRet = recvBuffer->Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		LOG(L"ERROR", SystemLog::ERROR_LEVEL,
			L"%s[%d]\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
	buffer.MoveWritePos(dequeueRet);

	buffer >> time;

	
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

	Sector sectors[9];
	GetAroundSector(pPlayer->_pSector, sectors);

	if (direction == dfPACKET_MOVE_DIR_LL)
	{
		
		vector<Player*>::iterator iter = sectors[0]._players.begin();
		for (; iter < sectors[0]._players.end(); iter++)
		{
			if((*iter) != pPlayer)
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

		
		for (int i = 1; i < 9; i++)
		{
			iter = sectors[i]._players.begin();
			for (; iter < sectors[i]._players.end(); iter++)
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
		
		vector<Player*>::iterator iter = sectors[0]._players.begin();
		for (; iter < sectors[0]._players.end(); iter++)
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

		
		for (int i = 1; i < 9; i++)
		{
			iter = sectors[i]._players.begin();
			for (; iter < sectors[i]._players.end(); iter++)
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

	Sector sectors[9];
	GetAroundSector(pPlayer->_pSector, sectors);

	if (direction == dfPACKET_MOVE_DIR_LL)
	{
		
		vector<Player*>::iterator iter = sectors[0]._players.begin();
		for (; iter < sectors[0]._players.end(); iter++)
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

		
		for (int i = 1; i < 9; i++)
		{
			iter = sectors[i]._players.begin();
			for (; iter < sectors[i]._players.end(); iter++)
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
		
		vector<Player*>::iterator iter = sectors[0]._players.begin();
		for (; iter < sectors[0]._players.end(); iter++)
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

		
		for (int i = 1; i < 9; i++)
		{
			iter = sectors[i]._players.begin();
			for (; iter < sectors[i]._players.end(); iter++)
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

	Sector sectors[9];
	GetAroundSector(pPlayer->_pSector, sectors);

	if (direction == dfPACKET_MOVE_DIR_LL)
	{
		
		vector<Player*>::iterator iter = sectors[0]._players.begin();
		for (; iter < sectors[0]._players.end(); iter++)
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
		
		for (int i = 1; i < 9; i++)
		{
			iter = sectors[i]._players.begin();
			for (; iter < sectors[i]._players.end(); iter++)
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
		
		vector<Player*>::iterator iter = sectors[0]._players.begin();
		for (; iter < sectors[0]._players.end(); iter++)
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
		
		for (int i = 1; i < 9; i++)
		{
			iter = sectors[i]._players.begin();
			for (; iter < sectors[i]._players.end(); iter++)
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
	_xPosMin = (xIndex - 1) * dfSECTOR_SIZE_X ;
	_yPosMin = (yIndex - 1) * dfSECTOR_SIZE_Y;
	_xPosMax = xIndex * dfSECTOR_SIZE_X;
	_yPosMax = yIndex * dfSECTOR_SIZE_Y;

	if (_xIndex == 0 || _xIndex == (dfSECTOR_CNT_X - 1)||
		_yIndex == 0 || _yIndex == (dfSECTOR_CNT_Y - 1))
		return;

	_players.reserve(dfDEFAULT_PLAYERS_PER_SECTOR);
}

