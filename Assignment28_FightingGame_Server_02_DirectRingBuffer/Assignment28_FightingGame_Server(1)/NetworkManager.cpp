#include "NetworkManager.h"
#include "PlayerManager.h"
#include "Main.h"
#include <stdio.h>

#define IP L"0.0.0.0"
#define PORT 5000

NetworkManager::NetworkManager()
{
	
}
NetworkManager::~NetworkManager()
{

}

NetworkManager* NetworkManager::GetInstance()
{
	static NetworkManager _networkMgr;
	return &_networkMgr;
}

void NetworkManager::Initialize()
{
	int err;
	int bindRet;
	int listenRet;
	int ioctRet;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		g_bShutdown = true;
		return;
	}

	// Create Socket
	_listensock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listensock == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	// Bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, IP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(PORT);

	bindRet = bind(_listensock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bindRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	// Listen
	listenRet = listen(_listensock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
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

	printf("Setting Complete!\n");
}

void NetworkManager::Update()
{
	FD_ZERO(&_rset);
	FD_ZERO(&_wset);
	FD_SET(_listensock, &_rset);
	for (CList<Session*>::iterator i = _sessionList.begin(); i != _sessionList.end(); i++)
	{
		FD_SET((*i)->_sock, &_rset);
		if ((*i)->_sendBuf.GetUseSize() > 0)
			FD_SET((*i)->_sock, &_wset);
	}

	// Select
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	int selectRet = select(0, &_rset, &_wset, NULL, &time);
	if (selectRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}
	else if (selectRet > 0)
	{
		for (CList<Session*>::iterator i = _sessionList.begin(); i != _sessionList.end(); i++)
		{
			if (FD_ISSET((*i)->_sock, &_wset))
				SendProc((*i));
		}

		if (FD_ISSET(_listensock, &_rset))
			AcceptProc();

		for (CList<Session*>::iterator i = _sessionList.begin(); i != _sessionList.end(); i++)
		{
			if (FD_ISSET((*i)->_sock, &_rset))
				RecvProc((*i));
		}

		DisconnectDeadSessions();
	}
}

void NetworkManager::Terminate()
{
	// Disconnect All Connected Session 
	for (CList<Session*>::iterator i = _sessionList.begin(); i != _sessionList.end();)
	{
		Session* pSession = *i;
		i = _sessionList.erase(i);
		closesocket(pSession->_sock);
		delete(pSession);
	}
	closesocket(_listensock);
	WSACleanup();
}

void NetworkManager::AcceptProc()
{
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	Session* newSession = new Session;

	newSession->_sock = accept(_listensock, (SOCKADDR*)&clientaddr, &addrlen);
	if (newSession->_sock == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		delete newSession;
		return;
	}
	newSession->SetSessionAlive();
	_sessionList.push_back(newSession);
	
	if(_pPlayerManager== nullptr)
		_pPlayerManager = PlayerManager::GetInstance();

	_pPlayerManager->CreatePlayer(newSession);

}

void NetworkManager::RecvProc(Session* session)
{
	int recvRet = recv(	session->_sock, 
						session->_recvBuf.GetWriteBufferPtr(), 
						session->_recvBuf.DirectEnqueueSize(), 0);

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			session->SetSessionDead();
			return;
		}
	}
	else if (recvRet == 0)
	{
		session->SetSessionDead();
		return;
	}

	int moveRet = session->_recvBuf.MoveWritePos(recvRet);
	if (recvRet != moveRet)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		session->SetSessionDead();
		return;
	}

	// For Test =========================================================
	char testBuf[DEFAULT_BUF_SIZE];
	int testPeekRet = session->_recvBuf.Peek(testBuf, moveRet);
	testBuf[testPeekRet] = '\0';
	printf("\n\n");
	printf("Recv Test===========================\n\n");
	int idx = 0;
	while (idx < testPeekRet)
	{
		printf("%d ", testBuf[idx]);
		idx++;
	}
	printf("\n\n===================================");
	printf("\n\n");

	if(testPeekRet != moveRet)
	{
		printf("Test Error! Func %s Line %d\n", __func__, __LINE__);
	}
	// ==================================================================
}

void NetworkManager::SendProc(Session* session)
{
	if(session->_sendBuf.GetUseSize() <= 0)
		return;


	int sendRet = send(	session->_sock, 
						session->_sendBuf.GetReadBufferPtr(), 
						session->_sendBuf.DirectDequeueSize(), 0);

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			session-> SetSessionDead();
		}
		return;
	}

	// For Test =======================================================
	char testBuf[DEFAULT_BUF_SIZE];
	int testPeekRet = session->_sendBuf.Peek(testBuf, sendRet);
	testBuf[testPeekRet] = '\0';
	printf("\n\n");
	printf("Send Test===========================\n\n");
	int idx = 0;
	while (idx < testPeekRet)
	{
		printf("%d ", testBuf[idx]);
		idx++;
	}
	printf("\n\n===================================");
	printf("\n\n");

	if (testPeekRet != sendRet)
	{
		printf("Test Error! Func %s Line %d\n", __func__, __LINE__);
	}

	// ===================================================================

	int moveRet = session->_sendBuf.MoveReadPos(sendRet);
	if (sendRet != moveRet)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		session->SetSessionDead();
		return;
	}


}

void NetworkManager::DisconnectDeadSessions()
{
	for (CList<Session*>::iterator i = _sessionList.begin(); i != _sessionList.end();)
	{
		if (!(*i)->GetSessionAlive())
		{
			Session* pSession = *i;
			i = _sessionList.erase(i);
			closesocket(pSession->_sock);
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
		printf("Error! Function %s Line %d\n", __func__, __LINE__);
		pSession->SetSessionDead();
	}
}

void NetworkManager::EnqueueBroadcast(char* msg, int size, Session* pExpSession)
{
	int enqueueRet;

	if (pExpSession == nullptr)
	{
		for (CList<Session*>::iterator i = _sessionList.begin(); i != _sessionList.end(); i++)
		{
			if ((*i)->GetSessionAlive())
			{
				enqueueRet = (*i)->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					printf("Error! Function %s Line %d\n", __func__, __LINE__);
					(*i)->SetSessionDead();
				}
			}
		}
	}
	else
	{
		for (CList<Session*>::iterator i = _sessionList.begin(); i != _sessionList.end(); i++)
		{
			if ((*i)->GetSessionAlive() && pExpSession->_sock != (*i)->_sock)
			{
				enqueueRet = (*i)->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					printf("Error! Function %s Line %d\n", __func__, __LINE__);
					(*i)->SetSessionDead();
				}
			}
		}
	}
}
