#include "NetworkManager.h"
#include "PlayerManager.h"
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

bool NetworkManager::Initialize()
{
	int err;
	int bindRet;
	int listenRet;
	int ioctRet;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	// Create Socket
	_listensock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listensock == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		return false;
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
		return false;
	}

	// Listen
	listenRet = listen(_listensock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		return false;
	}

	// Set Non-Blocking Mode
	u_long on = 1;
	ioctRet = ioctlsocket(_listensock, FIONBIO, &on);
	if (ioctRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		return false;
	}

	printf("Setting Complete!\n");
	return true;
}

bool NetworkManager::Update()
{
	int err;
	int selectRet;

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
	selectRet = select(0, &_rset, &_wset, NULL, NULL);
	if (selectRet == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
		return false;
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

	return true;
}

void NetworkManager::Terminate()
{
	//To-do: Delete All Players
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
	newSession->_disconnect = false;
	_sessionList.push_back(newSession);
	
	if(_pPlayerManager== nullptr)
		_pPlayerManager = PlayerManager::GetInstance();

	_pPlayerManager->CreatePlayer(newSession);

}

void NetworkManager::RecvProc(Session* session)
{
	char buf[MAX_BUF_SIZE];
	int recvRet = recv(session->_sock, buf, MAX_BUF_SIZE, 0);
	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			session->_disconnect = true;
			return;
		}
	}
	else if (recvRet == 0)
	{
		session->_disconnect = true;(session);
		return;
	}

	int enqueueRet = session->_recvBuf.Enqueue(buf, recvRet);
	if (recvRet != enqueueRet)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		session->_disconnect = true;
		return;
	}
}

void NetworkManager::SendProc(Session* session)
{
	if(session->_sendBuf.GetUseSize() <= 0)
		return;

	char buf[MAX_BUF_SIZE];
	int sendBufSize = session->_sendBuf.GetUseSize();

	int peekRet = session->_sendBuf.Peek(buf, sendBufSize);
	if (peekRet != sendBufSize)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		session->_disconnect = true;
		return;
	}

	int sendRet = send(session->_sock, buf, peekRet, 0);
	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			session->_disconnect = true;
		}
		return;
	}

	session->_sendBuf.MoveReadPos(sendRet);
}

void NetworkManager::DisconnectDeadSessions()
{
	for (CList<Session*>::iterator i = _sessionList.begin(); i != _sessionList.end();)
	{
		if ((*i)->_disconnect)
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
		pSession->_disconnect = true;
	}
}

void NetworkManager::EnqueueBroadcast(char* msg, int size, Session* pExpSession)
{
	int enqueueRet;

	if (pExpSession == nullptr)
	{
		for (CList<Session*>::iterator i = _sessionList.begin(); i != _sessionList.end(); i++)
		{
			if (!(*i)->_disconnect)
			{
				enqueueRet = (*i)->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					printf("Error! Function %s Line %d\n", __func__, __LINE__);
					(*i)->_disconnect = true;
				}
			}
		}
	}
	else
	{
		for (CList<Session*>::iterator i = _sessionList.begin(); i != _sessionList.end(); i++)
		{
			if (!(*i)->_disconnect && pExpSession->_sock != (*i)->_sock)
			{
				enqueueRet = (*i)->_sendBuf.Enqueue(msg, size);
				if (enqueueRet != size)
				{
					printf("Error! Function %s Line %d\n", __func__, __LINE__);
					(*i)->_disconnect = true;
				}
			}
		}
	}
}
