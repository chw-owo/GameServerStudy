#include "Server.h"
#include "Main.h"
#include <stdio.h>
#include <wchar.h>

Server::Server()
{
	int err;
	int bindRet;
	int listenRet;
	int ioctRet;
	_allSessions.reserve(dfDEFAULT_SESSIONS_NUM);
	_allPlayers.reserve(dfDEFAULT_PLAYERS_NUM);

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
		g_bShutdown = true;
		return;
	}

	// Listen
	listenRet = listen(_listensock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
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
	::printf("Content Setting Complete!\n");

}

Server::~Server()
{
	vector<Session*>::iterator sessionIter = _allSessions.begin();
	for (; sessionIter < _allSessions.end(); sessionIter++)
	{
		Session* pSession = *sessionIter;
		sessionIter = _allSessions.erase(sessionIter);
		closesocket(pSession->_socket);
		delete(pSession);
	}

	closesocket(_listensock);
	WSACleanup();
}

Server* Server::GetInstance()
{
	static Server _server;
	return &_server;
}

void Server::NetworkUpdate()
{ 
	DisconnectDeadSession();

	FD_SET rset;
	FD_SET wset;

	int idx = 1;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(_listensock, &rset);
	memset(_sessionArray, 0, sizeof(_sessionArray));

	vector<Session*>::iterator i = _allSessions.begin();
	if (i == _allSessions.end())
	{
		// Select Socket Set
		int selectRet = select(0, &rset, NULL, NULL, &_time);
		if (selectRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			g_bShutdown = true;
			return;
		}
		
		if (selectRet > 0 && FD_ISSET(_listensock, &rset))
		{
			AcceptProc();
		}
	}
	else
	{
		for (; i < _allSessions.end(); i++)
		{
			FD_SET((*i)->_socket, &rset);
			if ((*i)->_sendBuf.GetUseSize() > 0)
				FD_SET((*i)->_socket, &wset);
			_sessionArray[idx] = (*i);
			idx++;

			if (idx == FD_SETSIZE)
			{
				SelectProc(rset, wset, FD_SETSIZE);

				idx = 1;
				FD_ZERO(&rset);
				FD_ZERO(&wset);
				FD_SET(_listensock, &rset);
				memset(_sessionArray, 0, sizeof(_sessionArray));
			}
		}

		if (idx > 1)
		{
			SelectProc(rset, wset, idx);
		}
	}
}

void Server::ContentUpdate()
{
	if (SkipForFixedFrame()) return;

	vector<Player*>::iterator playerIter = _allPlayers.begin();
	for (; playerIter < _allPlayers.end(); playerIter++)
	{
		Player* pPlayer = (*playerIter);

		if (!pPlayer->GetPlayerAlive())
		{
			continue;
		}

		if (pPlayer->_hp <= 0)
		{
			pPlayer->SetPlayerDead();
			continue;
		}

		if (timeGetTime() - pPlayer->_pSession->_lastRecvTime 
		> dfNETWORK_PACKET_RECV_TIMEOUT)
		{
			pPlayer->SetPlayerDead();
			continue;
		}
		
		if (pPlayer->GetPlayerMove())
		{
			UpdatePlayerMove(pPlayer);
		}
	}

	DestroyDeadPlayers();
}

void Server::Control()
{
}

void Server::Monitor()
{
}

// About Network ==================================================

void Server::SelectProc(FD_SET rset, FD_SET wset, int max)
{
	// Select Socket Set
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	int selectRet = select(0, &rset, &wset, NULL, &_time);
	if (selectRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
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
	//printf(" - %s\n", __func__);

	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	Session* pSession = new Session(_sessionID++);

	if (pSession == nullptr)
	{
		::printf("Error! Func %s Line %d: fail new\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}

	pSession->_socket = accept(_listensock, (SOCKADDR*)&clientaddr, &addrlen);
	if (pSession->_socket == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	pSession->SetSessionAlive();
	pSession->_addr = clientaddr;
	pSession->_lastRecvTime = timeGetTime();
	_allSessions.push_back(pSession);
	CreatePlayer(pSession);

	unordered_map<Session*, Player*>::iterator mapIter = _SessionPlayerMap.find(pSession);
	Player* pPlayer = mapIter->second;
	printf("Player %d: Complete to Accept\n", pPlayer->_ID);

}

void Server::RecvProc(Session* pSession)
{
	//printf(" - %s\n", __func__);

	pSession->_lastRecvTime = timeGetTime();

	int recvRet = recv(pSession->_socket,
		pSession->_recvBuf.GetWriteBufferPtr(),
		pSession->_recvBuf.DirectEnqueueSize(), 0);

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err == WSAECONNRESET)
		{
			unordered_map<Session*, Player*>::iterator mapIter = _SessionPlayerMap.find(pSession);
			Player* pPlayer = mapIter->second;
			pPlayer->SetPlayerDead();
			return;
		}
		else if(err != WSAEWOULDBLOCK)
		{
			::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			unordered_map<Session*, Player*>::iterator mapIter = _SessionPlayerMap.find(pSession);
			Player* pPlayer = mapIter->second;
			pPlayer->SetPlayerDead();
			return;
		}

	}
	else if (recvRet == 0)
	{
		unordered_map<Session*, Player*>::iterator mapIter = _SessionPlayerMap.find(pSession);
		Player* pPlayer = mapIter->second;
		pPlayer->SetPlayerDead();
		return;
	}

	int moveRet = pSession->_recvBuf.MoveWritePos(recvRet);
	if (recvRet != moveRet)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}

	int useSize = pSession->_recvBuf.GetUseSize();

	unordered_map<Session*, Player*>::iterator mapIter = _SessionPlayerMap.find(pSession);
	Player* pPlayer = mapIter->second;
	printf("Player %d: Complete to Recv %d bytes\n", pPlayer->_ID, useSize);

	while (useSize > 0)
	{
		if (useSize <= dfHEADER_SIZE)
			break;

		st_PACKET_HEADER header;
		int peekRet = pSession->_recvBuf.Peek((char*)&header, dfHEADER_SIZE);
		if (peekRet != dfHEADER_SIZE)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			g_bShutdown = true;
			return;
		}

		if ((char) header.byCode != (char)dfPACKET_CODE)
		{
			::printf("Error! Wrong Header Code! %x - Func %s Line %d\n", 
				header.byCode, __func__, __LINE__);
			pPlayer->SetPlayerDead();
			return;
		}

		if (useSize < dfHEADER_SIZE + header.bySize)
			break;

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_SIZE);
		if (moveReadRet != dfHEADER_SIZE)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			g_bShutdown = true;
			return;
		}

		bool handlePacketRet = HandleCSPackets(pPlayer, header.byType);
		if (!handlePacketRet)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			pPlayer->SetPlayerDead();
			return;
		}

		useSize = pSession->_recvBuf.GetUseSize();
	}
}

void Server::SendProc(Session* pSession)
{
	//printf(" - %s\n", __func__);

	if (pSession->_sendBuf.GetUseSize() <= 0)
		return;

	int sendRet = send(pSession->_socket,
		pSession->_sendBuf.GetReadBufferPtr(),
		pSession->_sendBuf.DirectDequeueSize(), 0);

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			unordered_map<Session*, Player*>::iterator mapIter = _SessionPlayerMap.find(pSession);
			Player* pPlayer = mapIter->second;
			pPlayer->SetPlayerDead();
			return;
		}
	}

	int moveRet = pSession->_sendBuf.MoveReadPos(sendRet);
	if (sendRet != moveRet)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		unordered_map<Session*, Player*>::iterator mapIter = _SessionPlayerMap.find(pSession);
		Player* pPlayer = mapIter->second;
		pPlayer->SetPlayerDead();
		return;
	}

	unordered_map<Session*, Player*>::iterator mapIter = _SessionPlayerMap.find(pSession);
	Player* pPlayer = mapIter->second;
	printf("Player %d: Complete to Send %d bytes\n", pPlayer->_ID, sendRet);

}


void Server::DisconnectDeadSession()
{
	vector<Session*>::iterator i = _allSessions.begin();
	for (; i < _allSessions.end();)
	{
		if (!(*i)->GetSessionAlive())
		{
			Session* pSession = *i;
			i = _allSessions.erase(i);
			closesocket(pSession->_socket);
			delete(pSession);
		}
		else
		{
			i++;
		}
	}
}


void Server::EnqueueUnicast(char* msg, int size, Player* pPlayer)
{
	//printf(" - %s\n", __func__);

	int enqueueRet = pPlayer->_pSession->_sendBuf.Enqueue(msg, size);
	if (enqueueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		pPlayer->SetPlayerDead();
	}
}

void Server::EnqueueOneSector(char* msg, int size, Sector* sector, Player* pExpPlayer)
{
	//printf(" - %s\n", __func__);

	int enqueueRet;
	if (pExpPlayer == nullptr)
	{
		vector<Player*>::iterator playerIter = sector->_players.begin();
		for (; playerIter < sector->_players.end(); playerIter++)
		{
			if ((*playerIter)->GetPlayerAlive())
			{
				printf("EnqueueAroundSector Player %d\n", (*playerIter)->_ID);
				enqueueRet = (*playerIter)->_pSession->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					(*playerIter)->SetPlayerDead();
				}
			}
		}
	}
	else
	{
		vector<Player*>::iterator playerIter = sector->_players.begin();
		for (; playerIter < sector->_players.end(); playerIter++)
		{
			if ((*playerIter) != pExpPlayer && (*playerIter)->GetPlayerAlive())
			{
				enqueueRet = (*playerIter)->_pSession->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					(*playerIter)->SetPlayerDead();
				}
			}
		}
	}
}

void Server::EnqueueAroundSector(char* msg, int size, Sector* centerSector, Player* pExpPlayer)
{
	// TO-DO: 몇천명 단위일 때 어떻게 다음 프레임으로 미룰 것인지
	//printf(" - %s\n", __func__);

	int enqueueRet;

	if (pExpPlayer == nullptr)
	{
		Sector sectors[dfAROUND_SECTOR_NUM];
		GetAroundSector(centerSector, sectors);

		for(int i = 0; i < dfAROUND_SECTOR_NUM; i++)
		{
			vector<Player*>::iterator playerIter = sectors[i]._players.begin();
			for (; playerIter < sectors[i]._players.end(); playerIter++)
			{
				if ((*playerIter)->GetPlayerAlive())
				{
					enqueueRet = (*playerIter)->_pSession->_sendBuf.Enqueue(msg, size);
					if (enqueueRet != size)
					{
						::printf("Error! Func %s Line %d\n", __func__, __LINE__);
						(*playerIter)->SetPlayerDead();
					}
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
			if ((*playerIter) != pExpPlayer && (*playerIter)->GetPlayerAlive())
			{
				enqueueRet = (*playerIter)->_pSession->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					(*playerIter)->SetPlayerDead();
				}
			}
		}

		for (int i = 1; i < dfAROUND_SECTOR_NUM; i++)
		{
			vector<Player*>::iterator playerIter = sectors[i]._players.begin();
			for (; playerIter < sectors[i]._players.end(); playerIter++)
			{
				if ((*playerIter)->GetPlayerAlive())
				{
					enqueueRet = (*playerIter)->_pSession->_sendBuf.Enqueue(msg, size);
					if (enqueueRet != size)
					{
						::printf("Error! Func %s Line %d\n", __func__, __LINE__);
						(*playerIter)->SetPlayerDead();
					}
				}
			}
		}
	}
}

// About Content ====================================================

bool Server::SkipForFixedFrame()
{
	timeBeginPeriod(1);
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
	//printf(" - %s\n", __func__);
	// printf("\n<Player %d>\n", pPlayer->_ID);
	// printf("Pos: (%d, %d)\n", pPlayer->_x, pPlayer->_y);
	// printf("Sector: (%d, %d)\n", pPlayer->_pSector->_xIndex, pPlayer->_pSector->_yIndex);
	// printf("MoveDir: %d\n", pPlayer->_moveDirection);
	// printf("Dir: %d\n", pPlayer->_direction);
	
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
	//printf(" - %s\n", __func__);

	int x = pPlayer->_x / dfSECTOR_SIZE_X + 1;
	int y = pPlayer->_y / dfSECTOR_SIZE_Y + 1;
	_sectors[y][x]._players.push_back(pPlayer);
	pPlayer->_pSector = &_sectors[y][x];

}

void Server::GetAroundSector(Sector* centerSector, Sector* aroundSector)
{
	//printf(" - %s\n", __func__);

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
	//printf(" - %s\n", __func__);
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
		EnqueueOneSector(createMeToOther.GetReadPtr(), createMeToOtherRet, &inSector[i], pPlayer);

	SerializePacket MoveMeToOther;
	int MoveMeToOtherRet = SetSCPacket_MOVE_START(&MoveMeToOther,
		pPlayer->_ID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	for (int i = 0; i < inSectorNum; i++)
		EnqueueOneSector(MoveMeToOther.GetReadPtr(), MoveMeToOtherRet, &inSector[i], pPlayer);

	SerializePacket deleteMeToOther;
	int deleteMeToOtherRet = SetSCPacket_DELETE_CHAR(&deleteMeToOther, pPlayer->_ID);
	for (int i = 0; i < outSectorNum; i++)
		EnqueueOneSector(deleteMeToOther.GetReadPtr(), deleteMeToOtherRet, &outSector[i], pPlayer);


	// About Other Player ==============================================

	for (int i = 0; i < inSectorNum; i++)
	{
		vector<Player*>::iterator iter = inSector[i]._players.begin();
		for(; iter < inSector[i]._players.end(); iter++)
		{
			SerializePacket createOther;
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(&createOther,
				(*iter)->_ID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			EnqueueUnicast(createOther.GetReadPtr(), createOtherRet, pPlayer);

			if((*iter)->_move)
			{
				SerializePacket MoveOther;
				int MoveOtherRet = SetSCPacket_MOVE_START(&MoveOther,
					(*iter)->_ID, (*iter)->_moveDirection, (*iter)->_x, (*iter)->_y);
				EnqueueUnicast(MoveOther.GetReadPtr(), MoveOtherRet, pPlayer);
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
			EnqueueUnicast(deleteOther.GetReadPtr(), deleteOtherRet, pPlayer);
		}
	}

	newSector->_players.push_back(pPlayer);
	pPlayer->_pSector = newSector;
}

void Server::CreatePlayer(Session* pSession)
{
	//printf(" - %s\n", __func__);

	Player* pPlayer = new Player(pSession, _playerID++);
	SetSector(pPlayer);
	_SessionPlayerMap.insert(make_pair(pSession, pPlayer));
	_allPlayers.push_back(pPlayer);

	SerializePacket createMe;
	int createMeRet = SetSCPacket_CREATE_MY_CHAR(&createMe,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	EnqueueUnicast(createMe.GetReadPtr(), createMeRet, pPlayer);

	SerializePacket createMeToOther;
	int createMeToOtherRet = SetSCPacket_CREATE_OTHER_CHAR(&createMeToOther,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	EnqueueAroundSector(createMeToOther.GetReadPtr(), createMeToOtherRet, pPlayer->_pSector, pPlayer);

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
			EnqueueUnicast(createOther.GetReadPtr(), createOtherRet, pPlayer);
		}
	}

	for (int i = 1; i < 9; i++)
	{
		vector<Player*>::iterator iter = sectors[i]._players.begin();
		for (; iter < sectors[i]._players.end(); iter++)
		{
			SerializePacket createOther;
			int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(&createOther,
				(*iter)->_ID, (*iter)->_direction, (*iter)->_x, (*iter)->_y, (*iter)->_hp);
			EnqueueUnicast(createOther.GetReadPtr(), createOtherRet, pPlayer);
		}
	}
}

void Server::DeletePlayer(Player* pPlayer)
{
	//printf(" - %s\n", __func__);

	pPlayer->SetPlayerDead();

	SerializePacket buffer;
	int deleteRet = SetSCPacket_DELETE_CHAR(&buffer, pPlayer->_ID);
	EnqueueAroundSector(buffer.GetReadPtr(), deleteRet, pPlayer->_pSector);
}

void Server::DestroyDeadPlayers()
{
	vector<Player*>::iterator iter = _allPlayers.begin();

	for (;iter < _allPlayers.end();)
	{
		// Destroy Dead Player
		if (!(*iter)->GetPlayerAlive())
		{
			//printf(" - %s\n", __func__);
			Player* pPlayer = (*iter);

			// Set Session Dead
			unordered_map<Session*, Player*>::iterator mapIter 
				= _SessionPlayerMap.find(pPlayer->_pSession);
			_SessionPlayerMap.erase(mapIter);
			
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

			iter = _allPlayers.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

// About Packet ====================================================

bool Server::HandleCSPackets(Player* pPlayer, BYTE type)
{
	//printf(" - %s\n", __func__);

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

	::printf("Error! Func %s Line %d Case %d\n", __func__, __LINE__, type);
	return false;
}

bool Server::HandleCSPacket_MOVE_START(Player* pPlayer)
{
	BYTE moveDirection;
	short x;
	short y;
	bool getRet = GetCSPacket_MOVE_START(&(pPlayer->_pSession->_recvBuf), moveDirection, x, y);
	if(!getRet) return false;

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		SerializePacket buffer;
		int setRet = SetSCPacket_SYNC(&buffer, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(buffer.GetReadPtr(), setRet, pPlayer);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	SetPlayerMoveStart(pPlayer, moveDirection, x, y);

	SerializePacket buffer;
	int setRet = SetSCPacket_MOVE_START(&buffer, pPlayer->_ID, pPlayer->_moveDirection, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(buffer.GetReadPtr(), setRet, pPlayer->_pSector, pPlayer);

	//printf(" - Complete %s\n", __func__);
	return true;
}

bool Server::HandleCSPacket_MOVE_STOP(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	bool getRet = GetCSPacket_MOVE_STOP(&(pPlayer->_pSession->_recvBuf), direction, x, y);
	if (!getRet) return false;

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		SerializePacket buffer;
		int setRet = SetSCPacket_SYNC(&buffer, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(buffer.GetReadPtr(), setRet, pPlayer);
		x = pPlayer->_x;
		y = pPlayer->_y;
	}

	SetPlayerMoveStop(pPlayer, direction, x, y);

	SerializePacket buffer;
	int setRet = SetSCPacket_MOVE_STOP(&buffer, pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y);
	EnqueueAroundSector(buffer.GetReadPtr(), setRet, pPlayer->_pSector, pPlayer);

	//printf(" - Complete %s\n", __func__);
	return true;
}

bool Server::HandleCSPacket_ATTACK1(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	bool getRet = GetCSPacket_ATTACK1(&(pPlayer->_pSession->_recvBuf), direction, x, y);
	if (!getRet) return false;

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		SerializePacket buffer;
		int setRet = SetSCPacket_SYNC(&buffer, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(buffer.GetReadPtr(), setRet, pPlayer);
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

	printf(" - Complete %s\n", __func__);
	return true;
}

bool Server::HandleCSPacket_ATTACK2(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	bool getRet = GetCSPacket_ATTACK2(&(pPlayer->_pSession->_recvBuf), direction, x, y);
	if (!getRet) return false;

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		SerializePacket buffer;
		int setRet = SetSCPacket_SYNC(&buffer, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(buffer.GetReadPtr(), setRet, pPlayer);
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
	
	//printf(" - Complete %s\n", __func__);
	return true;
}

bool Server::HandleCSPacket_ATTACK3(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	bool getRet = GetCSPacket_ATTACK3(&(pPlayer->_pSession->_recvBuf), direction, x, y);
	if (!getRet) return false;

	if (abs(pPlayer->_x - x) > dfERROR_RANGE || abs(pPlayer->_y - y) > dfERROR_RANGE)
	{
		SerializePacket buffer;
		int setRet = SetSCPacket_SYNC(&buffer, pPlayer->_ID, pPlayer->_x, pPlayer->_y);
		EnqueueUnicast(buffer.GetReadPtr(), setRet, pPlayer);
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
	
	//printf(" - Complete %s\n", __func__);
	return true;
}

bool Server::HandleCSPacket_ECHO(Player* pPlayer)
{
	int time;
	bool getRet = GetCSPacket_ECHO(&(pPlayer->_pSession->_recvBuf), time);
	if (!getRet) return false;

	SerializePacket buffer;
	int setRet = SetSCPacket_ECHO(&buffer, time);
	EnqueueUnicast(buffer.GetReadPtr(), setRet, pPlayer);

	//printf(" - Complete %s\n", __func__);
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
		return false;
	}
	buffer.MoveWritePos(dequeueRet);

	buffer >> moveDirection;
	buffer >> x;
	buffer >> y;

	//printf(" - Complete %s\n", __func__);
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
		return false;
	}
	buffer.MoveWritePos(dequeueRet);

	buffer >> direction;
	buffer >> x;
	buffer >> y;

	//printf(" - Complete %s\n", __func__);
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
		return false;
	}
	buffer.MoveWritePos(dequeueRet);

	buffer >> direction;
	buffer >> x;
	buffer >> y;

	printf(" - Complete %s\n", __func__);
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
		return false;
	}
	buffer.MoveWritePos(dequeueRet);

	buffer >> direction;
	buffer >> x;
	buffer >> y;

	//printf(" - Complete %s\n", __func__);
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
		return false;
	}
	buffer.MoveWritePos(dequeueRet);

	buffer >> direction;
	buffer >> x;
	buffer >> y;

	//printf(" - Complete %s\n", __func__);
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
		return false;
	}
	buffer.MoveWritePos(dequeueRet);

	buffer >> time;

	//printf(" - Complete %s\n", __func__);
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
	//printf(" - Complete %s\n", __func__);
}

void Server::SetPlayerMoveStop(Player* pPlayer, BYTE& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_move = false;
	pPlayer->_direction = direction;
	//printf(" - Complete %s\n", __func__);
}

void Server::SetPlayerAttack1(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y)
{
	pPlayer->_x = x;
	pPlayer->_y = y;
	pPlayer->_direction = direction;
	printf(" - Attacker: %d, %d\n", pPlayer->_x, pPlayer->_y);

	// TO-DO: 아래 코드가 실행이 안됨~ 어딘가에서 멈춤~

	Sector sectors[9];
	GetAroundSector(pPlayer->_pSector, sectors);

	if (direction == dfPACKET_MOVE_DIR_LL)
	{
		vector<Player*>::iterator iter = sectors[0]._players.begin();
		for (; iter < sectors[0]._players.end(); iter)
		{
			if((*iter) != pPlayer)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				printf(" - (LL) Other: %d, %d\n", (*iter)->_x, (*iter)->_y);
				printf(" - (LL) Dist: %d\n", dist);

				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
					return;
				}
			}
		}

		for (int i = 1; i < 9; i++)
		{
			iter = sectors[i]._players.begin();
			for (; iter < sectors[i]._players.end(); iter)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				printf(" - (LL) Other: %d, %d\n", (*iter)->_x, (*iter)->_y);
				printf(" - (LL) Dist: %d\n", dist);

				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
					return;
				}
			}
		}
	}
	else if (direction == dfPACKET_MOVE_DIR_RR)
	{
		vector<Player*>::iterator iter = sectors[0]._players.begin();
		for (; iter < sectors[0]._players.end(); iter)
		{
			if((*iter) != pPlayer)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				printf(" - (RR) Other: %d, %d\n", (*iter)->_x, (*iter)->_y);
				printf(" - (RR) Dist: %d\n", dist);

				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
					return;
				}
			}
		}

		for (int i = 1; i < 9; i++)
		{
			iter = sectors[i]._players.begin();
			for (; iter < sectors[i]._players.end(); iter)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				printf(" - (RR) Other: %d, %d\n", (*iter)->_x, (*iter)->_y);
				printf(" - (RR) Dist: %d\n", dist);

				if (dist >= 0 && dist <= dfATTACK1_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK1_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK1_DAMAGE;
					return;
				}
			}
		}
	}
	
	printf(" - Complete %s\n", __func__);
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
		for (; iter < sectors[0]._players.end(); iter)
		{
			if((*iter) != pPlayer)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
					return;
				}
			}
		}

		for (int i = 1; i < 9; i++)
		{
			iter = sectors[i]._players.begin();
			for (; iter < sectors[i]._players.end(); iter)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
					return;
				}
			}
		}
	}
	else if (direction == dfPACKET_MOVE_DIR_RR)
	{
		vector<Player*>::iterator iter = sectors[0]._players.begin();
		for (; iter < sectors[0]._players.end(); iter)
		{
			if((*iter) != pPlayer)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
					return;
				}
			}
		}

		for (int i = 1; i < 9; i++)
		{
			iter = sectors[i]._players.begin();
			for (; iter < sectors[i]._players.end(); iter)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				if (dist >= 0 && dist <= dfATTACK2_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK2_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK2_DAMAGE;
					return;
				}
			}
		}
	}
	//printf(" - Complete %s\n", __func__);
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
		for (; iter < sectors[0]._players.end(); iter)
		{
			if((*iter) != pPlayer)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
					return;
				}
			}
		}

		for (int i = 1; i < 9; i++)
		{
			iter = sectors[i]._players.begin();
			for (; iter < sectors[i]._players.end(); iter)
			{
				int dist = pPlayer->_x - (*iter)->_x;
				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
					return;
				}
			}
		}
	}
	else if (direction == dfPACKET_MOVE_DIR_RR)
	{
		vector<Player*>::iterator iter = sectors[0]._players.begin();
		for (; iter < sectors[0]._players.end(); iter)
		{
			if((*iter)!= pPlayer)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
					return;
				}
			}
		}

		for (int i = 1; i < 9; i++)
		{
			iter = sectors[i]._players.begin();
			for (; iter < sectors[i]._players.end(); iter)
			{
				int dist = (*iter)->_x - pPlayer->_x;
				if (dist >= 0 && dist <= dfATTACK3_RANGE_X &&
					abs((*iter)->_y - pPlayer->_y) <= dfATTACK3_RANGE_Y)
				{
					pDamagedPlayer = (*iter);
					pDamagedPlayer->_hp -= dfATTACK3_DAMAGE;
					return;
				}
			}
		}
	}
	//printf(" - Complete %s\n", __func__);
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
	}

	//printf(" - Complete %s\n", __func__);
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
	}

	//printf(" - Complete %s\n", __func__);
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
	}

	//printf(" - Complete %s\n", __func__);
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
	}

	//printf(" - Complete %s\n", __func__);
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
	}

	//printf(" - Complete %s\n", __func__);
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
	}

	printf(" - Complete %s\n", __func__);
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
	}

	//printf(" - Complete %s\n", __func__);
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
	}

	//printf(" - Complete %s\n", __func__);
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
	}

	//printf(" - Complete %s\n", __func__);
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
	}

	//printf(" - Complete %s\n", __func__);
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
	}

	//printf(" - Complete %s\n", __func__);
	return buffer->GetDataSize();
}

void Server::Sector::InitializeSector(short xIndex, short yIndex)
{
	_xIndex = xIndex;
	_yIndex = yIndex;
	_xPosMin = (xIndex - 1) * dfSECTOR_SIZE_X ;
	_yPosMin = (yIndex - 1) * dfSECTOR_SIZE_Y;
	_xPosMax = (xIndex * dfSECTOR_SIZE_X) - 1;
	_yPosMax = (yIndex * dfSECTOR_SIZE_Y) - 1;

	if (_xIndex == 0 || _xIndex == (dfSECTOR_CNT_X - 1)||
		_yIndex == 0 || _yIndex == (dfSECTOR_CNT_Y - 1))
		return;

	_players.reserve(dfDEFAULT_PLAYERS_PER_SECTOR);
}

void Server::Player::SetPlayerDead()
{
	_alive = false;
	_pSession->SetSessionDead();
}
