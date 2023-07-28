#include "Server.h"
#include "Main.h"
#include <wchar.h>
#include <stdio.h>

#define dfNETWORK_IP L"0.0.0.0"
#define dfDEFAULT_SESSIONS_MAX 1000

// TO-DO: 섹터
// TO-DO: 프레임
// TO-DO: 타임 아웃
// TO-DO: 이동, 공격

Server::Server()
{
	int err;
	int bindRet;
	int listenRet;
	int ioctRet;
	_allSessions.reserve(dfDEFAULT_SESSIONS_MAX);

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
}

Server::~Server()
{
	vector<Session*>::iterator sessionIter = _allSessions.begin();
	for (; sessionIter != _allSessions.end(); sessionIter++)
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

void Server::Update()
{
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
		for (; i != _allSessions.end(); i++)
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

	DisconnectDeadSession();
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
	_allSessions.push_back(pSession);

	CreatePlayer(pSession);
}

void Server::RecvProc(Session* pSession)
{
	pSession->_lastRecvTime = GetTickCount64();

	int recvRet = recv(pSession->_socket,
		pSession->_recvBuf.GetWriteBufferPtr(),
		pSession->_recvBuf.DirectEnqueueSize(), 0);

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err == WSAECONNRESET)
		{
			pSession->SetSessionDead();
			return;
		}
		else if(err != WSAEWOULDBLOCK)
		{
			::printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			pSession->SetSessionDead();
			return;
		}

	}
	else if (recvRet == 0)
	{
		pSession->SetSessionDead();
		return;
	}

	int moveRet = pSession->_recvBuf.MoveWritePos(recvRet);
	if (recvRet != moveRet)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}

	unordered_map<Session*, Player*>::iterator iter = _SessionPlayerMap.find(pSession);
	Player* pPlayer = iter->second;

	int useSize = pSession->_recvBuf.GetUseSize();

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

		if ((char)header.byCode != (char)dfPACKET_CODE)
		{
			::printf("Error! Wrong Header Code! %x - Func %s Line %d\n", 
				header.byCode, __func__, __LINE__);
			pSession->SetSessionDead();
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
			pSession->SetSessionDead();
			return;
		}

		useSize = pSession->_recvBuf.GetUseSize();
	}
}

void Server::SendProc(Session* pSession)
{
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
			pSession->SetSessionDead();
			return;
		}
	}

	int moveRet = pSession->_sendBuf.MoveReadPos(sendRet);
	if (sendRet != moveRet)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		pSession->SetSessionDead();
		return;
	}
}


void Server::DisconnectDeadSession()
{
	vector<Session*>::iterator i = _allSessions.begin();
	for (; i != _allSessions.end();)
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


void Server::EnqueueUnicast(char* msg, int size, Session* pSession)
{
	int enqueueRet = pSession->_sendBuf.Enqueue(msg, size);
	if (enqueueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		pSession->SetSessionDead();
	}
}

void Server::EnqueueBroadcastInSector(char* msg, int size, Session* pExpSession)
{
	// TO-DO: Sector 제작 후 수정
	// TO-DO: 몇천명 단위일 때 어떻게 다음 프레임으로 미룰 것인지

	int enqueueRet;

	if (pExpSession == nullptr)
	{
		vector<Session*>::iterator i = _allSessions.begin();
		for (; i != _allSessions.end(); i++)
		{
			if ((*i)->GetSessionAlive())
			{
				enqueueRet = (*i)->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					(*i)->SetSessionDead();
				}
			}
		}
	}
	else
	{
		vector<Session*>::iterator i = _allSessions.begin();
		for (; i != _allSessions.end(); i++)
		{
			if ((*i)->GetSessionAlive() && pExpSession->_socket != (*i)->_socket)
			{
				enqueueRet = (*i)->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					(*i)->SetSessionDead();
				}
			}
		}
	}
}

void Server::EnqueueBroadcast(char* msg, int size, Session* pExpSession)
{
	// TO-DO: 몇천명 단위일 때 어떻게 다음 프레임으로 미룰 것인지

	int enqueueRet;

	if (pExpSession == nullptr)
	{
		vector<Session*>::iterator i = _allSessions.begin();
		for (; i != _allSessions.end(); i++)
		{
			if ((*i)->GetSessionAlive())
			{
				enqueueRet = (*i)->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					(*i)->SetSessionDead();
				}
			}
		}
	}
	else
	{
		vector<Session*>::iterator i = _allSessions.begin();
		for (; i != _allSessions.end(); i++)
		{
			if ((*i)->GetSessionAlive() && pExpSession->_socket != (*i)->_socket)
			{
				enqueueRet = (*i)->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					::printf("Error! Func %s Line %d\n", __func__, __LINE__);
					(*i)->SetSessionDead();
				}
			}
		}
	}
}

// About Content ====================================================

void Server::CreatePlayer(Session* pSession)
{
	// Code For Create Player
	Player* pPlayer = new Player(pSession, _assignID++);
	_SessionPlayerMap.insert(make_pair(pSession, pPlayer));
	
	SerializePacket* bufferForMyChar;
	int createMyRet = SetSCPacket_CREATE_MY_CHAR(bufferForMyChar,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	EnqueueBroadcastInSector(bufferForMyChar->GetReadPtr(), createMyRet, pSession);

	SerializePacket* bufferForOtherChar;
	int createOtherRet = SetSCPacket_CREATE_OTHER_CHAR(bufferForOtherChar,
		pPlayer->_ID, pPlayer->_direction, pPlayer->_x, pPlayer->_y, pPlayer->_hp);
	EnqueueBroadcastInSector(bufferForOtherChar->GetReadPtr(), createOtherRet, pSession);

}

void Server::DeletePlayer(Player* pPlayer)
{
	SerializePacket* buffer;
	int deleteRet = SetSCPacket_DELETE_CHAR(buffer, pPlayer->_ID);
	EnqueueBroadcastInSector(buffer->GetReadPtr(), deleteRet);

	unordered_map<Session*, Player*>::iterator iter = _SessionPlayerMap.find(pPlayer->_pSession);
	_SessionPlayerMap.erase(iter);
	pPlayer->_pSession->SetSessionDead();
	delete pPlayer;
}

// About Packet ====================================================

bool Server::HandleCSPackets(Player* pPlayer, WORD type)
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

	::printf("Error! Func %s Line %d Case %d\n", __func__, __LINE__, type);
	return false;
}

bool Server::HandleCSPacket_MOVE_START(Player* pPlayer)
{
	BYTE moveDirection;
	short x;
	short y;

	// Get Data from CSPacket
	bool getRet = GetCSPacket_MOVE_START(&(pPlayer->_pSession->_recvBuf), moveDirection, x, y);
	if(!getRet) return false;

	// Set Data on SCPacket
	SerializePacket* buffer;
	int setRet = SetSCPacket_MOVE_START(buffer, pPlayer->_ID, moveDirection, x, y);

	// Enqueue SCPacket
	EnqueueBroadcastInSector(buffer->GetReadPtr(), setRet, pPlayer->_pSession);

	return true;
}

bool Server::HandleCSPacket_MOVE_STOP(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	// Get Data from CSPacket
	bool getRet = GetCSPacket_MOVE_STOP(&(pPlayer->_pSession->_recvBuf), direction, x, y);
	if (!getRet) return false;

	// Set Data on SCPacket
	SerializePacket* buffer;
	int setRet = SetSCPacket_MOVE_STOP(buffer, pPlayer->_ID, direction, x, y);

	// Enqueue SCPacket
	EnqueueBroadcastInSector(buffer->GetReadPtr(), setRet, pPlayer->_pSession);

	return true;
}

bool Server::HandleCSPacket_ATTACK1(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	// Get Data from CSPacket
	bool getRet = GetCSPacket_ATTACK1(&(pPlayer->_pSession->_recvBuf), direction, x, y);
	if (!getRet) return false;

	// Set Data on SCPacket
	SerializePacket* attackBuffer;
	int attackSetRet = SetSCPacket_ATTACK1(attackBuffer, pPlayer->_ID, direction, x, y);

	// Enqueue SCPacket (Attack)
	EnqueueBroadcastInSector(attackBuffer->GetReadPtr(), attackSetRet);

	if (1)
	{
		Player* damagedPlayer = nullptr; // TO-DO

		SerializePacket* damageBuffer;
		int damageSetRet = SetSCPacket_DAMAGE(
			damageBuffer, pPlayer->_ID, damagedPlayer->_ID, damagedPlayer->_hp);

		// Enqueue SCPacket (Damage)
		EnqueueUnicast(damageBuffer->GetReadPtr(), damageSetRet, damagedPlayer->_pSession);
	}

	return false;
}

bool Server::HandleCSPacket_ATTACK2(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	// Get Data from CSPacket
	bool getRet = GetCSPacket_ATTACK2(&(pPlayer->_pSession->_recvBuf), direction, x, y);
	if (!getRet) return false;

	// Set Data on SCPacket
	SerializePacket* attackBuffer;
	int attackSetRet = SetSCPacket_ATTACK2(attackBuffer, pPlayer->_ID, direction, x, y);

	// Enqueue SCPacket (Attack)
	EnqueueBroadcastInSector(attackBuffer->GetReadPtr(), attackSetRet);

	if (1)
	{
		Player* damagedPlayer = nullptr; // TO-DO

		SerializePacket* damageBuffer;
		int damageSetRet = SetSCPacket_DAMAGE(
			damageBuffer, pPlayer->_ID, damagedPlayer->_ID, damagedPlayer->_hp);

		// Enqueue SCPacket (Damage)
		EnqueueUnicast(damageBuffer->GetReadPtr(), damageSetRet, damagedPlayer->_pSession);
	}


	return false;
}

bool Server::HandleCSPacket_ATTACK3(Player* pPlayer)
{
	BYTE direction;
	short x;
	short y;

	// Get Data from CSPacket
	bool getRet = GetCSPacket_ATTACK3(&(pPlayer->_pSession->_recvBuf), direction, x, y);
	if (!getRet) return false;

	// Set Data on SCPacket
	SerializePacket* attackBuffer;
	int attackSetRet = SetSCPacket_ATTACK3(attackBuffer, pPlayer->_ID, direction, x, y);

	// Enqueue SCPacket (Attack)
	EnqueueBroadcastInSector(attackBuffer->GetReadPtr(), attackSetRet);

	if (1)
	{
		Player* damagedPlayer = nullptr; // TO-DO

		SerializePacket* damageBuffer;
		int damageSetRet = SetSCPacket_DAMAGE(
			damageBuffer, pPlayer->_ID, damagedPlayer->_ID, damagedPlayer->_hp);

		// Enqueue SCPacket (Damage)
		EnqueueUnicast(damageBuffer->GetReadPtr(), damageSetRet, damagedPlayer->_pSession);
	}

	return false;
}

bool Server::HandleCSPacket_ECHO(Player* pPlayer)
{
	int time;

	// Get Data from CSPacket
	bool getRet = GetCSPacket_ECHO(&(pPlayer->_pSession->_recvBuf), time);
	if (!getRet) return false;

	// Set Data on SCPacket
	SerializePacket* buffer;
	int setRet = SetSCPacket_ECHO(buffer, time);

	// Enqueue SCPacket
	EnqueueUnicast(buffer->GetReadPtr(), setRet, pPlayer->_pSession);
	return false;
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

	return buffer->GetDataSize();
}

