#include "Network.h"
#include "ErrorHandler.h"
#include <stdio.h>

#define IP L"0.0.0.0"
#define PORT 5000

NetworkManager::NetworkManager()
{
	CMemoryPoolT<Session>_SessionPool(64, false);
	//TO-DO: _SessionPool.Alloc()으로 바꾸기
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
		AssertCrash();
	}

	// Create Socket
	_listensock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listensock == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		AssertCrash();
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
		AssertCrash();
	}

	// Listen
	listenRet = listen(_listensock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		AssertCrash();
	}

	// Set Non-Blocking Mode
	u_long on = 1;
	ioctRet = ioctlsocket(_listensock, FIONBIO, &on);
	if (ioctRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		AssertCrash();
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
		AssertCrash();
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
		_SessionPool.Free(pSession); //delete(pSession);
	}
	closesocket(_listensock);
	WSACleanup();
}

void NetworkManager::AcceptProc()
{
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	Session* newSession = _SessionPool.Alloc();//new Session; 

	newSession->_sock = accept(_listensock, (SOCKADDR*)&clientaddr, &addrlen);
	if (newSession->_sock == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		_SessionPool.Free(newSession); //delete newSession;
		return;
	}

	newSession->_ID = (0xffffffff & (clientaddr.sin_addr.s_addr));
	newSession->_ID <<= 16;
	newSession->_ID |= (0xffff & (clientaddr.sin_port));
	newSession->_ID |= 0x0089000000000000;
	newSession->SetSessionAlive();

	printf("Accept! Session ID: %llx\n", newSession->_ID);
	_newSessionList.push_back(newSession);
}

void NetworkManager::RecvProc(Session* session)
{
	int recvRet = recv(session->_sock,
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

	if (moveRet > 0)
	{
		printf("Recv Success!\n");
	}

}

void NetworkManager::SendProc(Session* session)
{
	if (session->_sendBuf.GetUseSize() <= 0)
		return;

	int sendRet = send(session->_sock,
		session->_sendBuf.GetReadBufferPtr(),
		session->_sendBuf.DirectDequeueSize(), 0);

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			session->SetSessionDead();
		}
		return;
	}

	int moveRet = session->_sendBuf.MoveReadPos(sendRet);
	if (sendRet != moveRet)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		session->SetSessionDead();
		return;
	}

	if (moveRet > 0)
	{
		printf("Send Success!\n");
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
			_SessionPool.Free(pSession); //delete(pSession);
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

void NetworkManager::EnqueueMulticast(char* msg, int size, Session* pSessionList, int SessionNum)
{
	Session* pSession = pSessionList;
	for (int i = 1; i < SessionNum; i++)
	{
		int enqueueRet = pSession->_sendBuf.Enqueue(msg, size);
		if (enqueueRet != size)
		{
			printf("Error! Function %s Line %d\n", __func__, __LINE__);
			pSession->SetSessionDead();
		}
		pSession++;
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
