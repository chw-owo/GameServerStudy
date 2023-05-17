#include "NetworkManager.h"
#include "PlayerManager.h"
#include "Main.h"
#include <stdio.h>
#include <windows.h>

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
	_rsetList.push_back(new FD_SET);
	_wsetList.push_back(new FD_SET);
	printf("Setting Complete!\n");
}

void NetworkManager::Update()
{
	int rsetSessionNum = 0;
	int wsetSessionNum = 0;
	CList<FD_SET*>::iterator rsetIter = _rsetList.begin();
	CList<FD_SET*>::iterator wsetIter = _wsetList.begin();

	FD_ZERO(*rsetIter);
	FD_ZERO(*wsetIter);
	FD_SET(_listensock, *rsetIter);
	rsetSessionNum++;

	for (CList<Session*>::iterator i = _sessionList.begin(); i != _sessionList.end(); i++)
	{
		FD_SET((*i)->_sock, *rsetIter);
		rsetSessionNum++;
		if (rsetSessionNum > FD_SETSIZE)
		{
			rsetSessionNum = 0;
			_rsetList.push_back(new FD_SET);
			rsetIter++;
		}

		if ((*i)->_sendBuf.GetUseSize() > 0)
		{
			FD_SET((*i)->_sock, *wsetIter);
			wsetSessionNum++;
			if (wsetSessionNum > FD_SETSIZE)
			{
				wsetSessionNum = 0;
				_wsetList.push_back(new FD_SET);
				wsetIter++;
			}
		}
	}

	// Select
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	rsetIter = _rsetList.begin();
	wsetIter = _wsetList.begin();

	// iter++만 하고 나면 select에서 10022 예외가 터지는데 원인이 뭔지 모르겠다...
	// 안되는 게 당연했음 ㅠ 스레드 당 64개 제한이라 여러개 쓰려면 멀티스레드로 만들어야 한다
	// 이후에 멀티스레드 수업 진행한 다음에 수정하도록 하자

	while (wsetIter != _wsetList.end())
	{
		int selectRet = select(0, *rsetIter, *wsetIter, NULL, &time);
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
				if (FD_ISSET((*i)->_sock, *wsetIter))
					SendProc((*i));
			}

			if (FD_ISSET(_listensock, *rsetIter))
				AcceptProc();

			for (CList<Session*>::iterator i = _sessionList.begin(); i != _sessionList.end(); i++)
			{
				if (FD_ISSET((*i)->_sock, *rsetIter))
					RecvProc((*i));
			}

			DisconnectDeadSessions();
		}

		wsetIter++;
		rsetIter++;
	}

	while (rsetIter != _rsetList.end())
	{
		int selectRet = select(0, *rsetIter, NULL, NULL, &time);
		if (selectRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
			g_bShutdown = true;
			return;
		}
		else if (selectRet > 0)
		{

			if (FD_ISSET(_listensock, *rsetIter))
				AcceptProc();

			for (CList<Session*>::iterator i = _sessionList.begin(); i != _sessionList.end(); i++)
			{
				if (FD_ISSET((*i)->_sock, *rsetIter))
					RecvProc((*i));
			}

			DisconnectDeadSessions();
		}
		rsetIter++;
	}

	if (GetAsyncKeyState(VK_RETURN) & 0x0001)
	{
		// For Test
		printf("session: %d, rset List Size: %d, wset List Size: %d\n",
			_sessionList.size(), _rsetList.size(), _wsetList.size());
	}
}

void NetworkManager::Terminate()
{
	// For Test
	printf("session: %d, rset List Size: %d, wset List Size: %d\n",
		_sessionList.size(), _rsetList.size(), _wsetList.size());

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

	FD_SET* pSet;
	while (!_rsetList.empty())
	{
		pSet = _rsetList.pop_back();
		delete(pSet);
	}

	while (!_wsetList.empty())
	{
		pSet = _wsetList.pop_back();
		delete(pSet);
	}
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
	char buf[MAX_BUF_SIZE];
	int recvRet = recv(session->_sock, buf, MAX_BUF_SIZE, 0);
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

	int enqueueRet = session->_recvBuf.Enqueue(buf, recvRet);
	if (recvRet != enqueueRet)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		session->SetSessionDead();
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
		session->SetSessionDead();
		return;
	}

	int sendRet = send(session->_sock, buf, peekRet, 0);
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

	session->_sendBuf.MoveReadPos(sendRet);
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
