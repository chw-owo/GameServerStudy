#include "NetworkManager.h"
#include "ContentManager.h"
#include "Protocol.h"
#include "Session.h"
#include "Main.h"
#include <stdio.h>

#define IP L"0.0.0.0"
#define dfDEFAULT_SESSIONS_MAX 1000

NetworkManager::NetworkManager()
{
	int err;
	int bindRet;
	int listenRet;
	int ioctRet;
	_allSessions.resize(dfDEFAULT_SESSIONS_MAX);

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
		printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	// Bind Socket
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, IP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(dfNETWORK_PORT);

	bindRet = bind(_listensock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bindRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	// Listen
	listenRet = listen(_listensock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	// Set Non-Blocking Mode
	u_long on = 1;
	ioctRet = ioctlsocket(_listensock, FIONBIO, &on);
	if (ioctRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	printf("Network Setting Complete!\n");
}

NetworkManager::~NetworkManager()
{
	// Disconnect All Connected Session 
	vector<Session*>::iterator i = _allSessions.begin();
	for (; i != _allSessions.end();)
	{
		Session* pSession = *i;
		i = _allSessions.erase(i);
		closesocket(pSession->_socket);
		delete(pSession);
	}

	closesocket(_listensock);
	WSACleanup();
}

NetworkManager* NetworkManager::GetInstance()
{
	static NetworkManager _networkManager;
	return &_networkManager;
}

void NetworkManager::Update()
{
	FD_SET rset;
	FD_SET wset;

	int idx = 1;
	FD_ZERO(&rset);
	FD_ZERO(&wset);
	FD_SET(_listensock, &rset);
	memset(_sessionArray, 0, sizeof(_sessionArray));
	
	vector<Session*>::iterator i = _allSessions.begin();
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

	DisconnectSession();
}

void NetworkManager::SelectProc(FD_SET rset, FD_SET wset, int max)
{
	// Select Socket Set
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	int selectRet = select(0, &rset, &wset, NULL, &time);
	if (selectRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
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

void NetworkManager::AcceptProc()
{
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	Session* pSession = new Session(_sessionID++);

	if (pSession == nullptr)
	{
		printf("Error! Func %s Line %d: fail new\n", __func__, __LINE__);
		return;
	}

	pSession->_socket = accept(_listensock, (SOCKADDR*)&clientaddr, &addrlen);
	if (pSession->_socket == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		delete pSession;
		return;
	}

	pSession->SetSessionAlive();
	pSession->_addr = clientaddr;
	_allSessions.push_back(pSession);

	if (_pContentManager == nullptr)
		_pContentManager = ContentManager::GetInstance();

	_pContentManager->CreateUser(pSession);
}

void NetworkManager::RecvProc(Session* pSession)
{
	int recvRet = recv(pSession->_socket,
		pSession->_recvBuf.GetWriteBufferPtr(),
		pSession->_recvBuf.DirectEnqueueSize(), 0);

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			pSession->SetSessionDead();
			return;
		}
	}
	else if(recvRet == 0)
	{
		pSession->SetSessionDead();
		return;
	}

	int moveRet = pSession->_recvBuf.MoveWritePos(recvRet);
	if (recvRet != moveRet)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		pSession->SetSessionDead();
		return;
	}
}

void NetworkManager::SendProc(Session* pSession)
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
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			pSession->SetSessionDead();
			return;
		}
	}

	int moveRet = pSession->_sendBuf.MoveReadPos(sendRet);
	if (sendRet != moveRet)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		pSession->SetSessionDead();
		return;
	}
}


void NetworkManager::DisconnectSession()
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


void NetworkManager::EnqueueUnicast(char* msg, int size, Session* pSession)
{
	int enqueueRet = pSession->_sendBuf.Enqueue(msg, size);
	if (enqueueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		pSession->SetSessionDead();
	}
}

void NetworkManager::EnqueueBroadcast(char* msg, int size, Session* pExpSession)
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
					printf("Error! Func %s Line %d\n", __func__, __LINE__);
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
					printf("Error! Func %s Line %d\n", __func__, __LINE__);
					(*i)->SetSessionDead();
				}
			}
		}
	}
}
