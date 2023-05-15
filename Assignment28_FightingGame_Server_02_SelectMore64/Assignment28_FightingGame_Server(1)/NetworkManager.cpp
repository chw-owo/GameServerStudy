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

	_rsetList.push_back(new fd_set);
	_wsetList.push_back(new fd_set);
	
	printf("Setting Complete!\n");
}
int usedWsetNum;
void NetworkManager::Update()
{
	for (CList<fd_set*>::iterator rsetIter = _rsetList.begin(); rsetIter != _rsetList.end(); rsetIter++)
		FD_ZERO((*rsetIter));
	for (CList<fd_set*>::iterator wsetIter = _wsetList.begin(); wsetIter != _wsetList.end(); wsetIter++)
		FD_ZERO((*wsetIter));

	int rsetIdx = 0;
	int wsetIdx = 0;
	CList<fd_set*>::iterator rsetFirstIter = _rsetList.begin();
	FD_SET(_listensock, (*rsetFirstIter));
	rsetIdx++;

	// _sessionList.size() + 1, cuz listen_socket
	int needRset = (((_sessionList.size() + 1) / (FD_SETSIZE + 1)) + 1) - _rsetList.size();
	while(needRset > 0)
	{ 
		_rsetList.push_back(new fd_set);
		needRset--;
	}

	int needWset = (((_sessionList.size()) / (FD_SETSIZE + 1)) + 1) - _wsetList.size();
	while (needWset > 0)
	{
		_wsetList.push_back(new fd_set);
		needWset--;
	}
	
	usedWsetNum = 1;
	for (CList<Session*>::iterator sessionIter = _sessionList.begin(); sessionIter != _sessionList.end(); sessionIter++)
	{
		CList<fd_set*>::iterator rsetIter = _rsetList.begin();
		CList<fd_set*>::iterator wsetIter = _wsetList.begin();

		if (rsetIter == _rsetList.end() || wsetIter == _wsetList.end())
		{
			printf("Error! Function %s Line %d\n", __func__, __LINE__);
			g_bShutdown = true;
			return;
		}

		FD_SET((*sessionIter)->_sock, (*rsetIter));
		rsetIdx++;

		if (rsetIter != _rsetList.begin())
			printf("Success to Set Session in 2nd rset!\n");
		
		if ((*sessionIter)->_sendBuf.GetUseSize() > 0)
		{
			FD_SET((*sessionIter)->_sock, (*wsetIter));
			wsetIdx++;					
		}	

		if (wsetIdx > FD_SETSIZE)
		{
			wsetIter++;
			wsetIdx = 0;
			usedWsetNum++;
		}

		if (rsetIdx > FD_SETSIZE)
		{
			rsetIter++;
			rsetIdx = 0;
		}
	}

	// Select
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;
	int usedWsetIdx = 0;
	CList<Session*>::iterator sessionIter = _sessionList.begin();
	for (CList<fd_set*>::iterator rsetIter = _rsetList.begin(); rsetIter != _rsetList.end();)
	{
		CList<fd_set*>::iterator wsetIter = _wsetList.begin();
		if (usedWsetIdx < usedWsetNum && wsetIter != _wsetList.end())
		{
			int selectRet = select(0, (*rsetIter), (*wsetIter), NULL, &time);
			if (selectRet == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
				printf("session: %d, rset List Size: %d, wset List Size: %d, used wset num: %d\n",
					_sessionList.size(), _rsetList.size(), _wsetList.size(), usedWsetNum);
				g_bShutdown = true;
				return;
			}

			if (selectRet > 0)
			{
				if (FD_ISSET(_listensock, (*rsetIter)))
					AcceptProc();

				for (int i = 0; i < FD_SETSIZE; i++)
				{
					if (sessionIter == _sessionList.end())
						break;

					if (FD_ISSET((*sessionIter)->_sock, (*wsetIter)))
						SendProc((*sessionIter));

					if (FD_ISSET((*sessionIter)->_sock, (*rsetIter)))
						RecvProc((*sessionIter));

					sessionIter++;
				}
				wsetIter++;
				rsetIter++;
				usedWsetIdx++;
			}
		}
		else
		{
			int selectRet = select(0, (*rsetIter), NULL, NULL, &time);
			if (selectRet == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				printf("Error! Function %s Line %d: %d\n", __func__, __LINE__, err);
				printf("session: %d, rset List Size: %d, wset List Size: %d, used wset num: %d\n",
					_sessionList.size(), _rsetList.size(), _wsetList.size(), usedWsetNum);
				g_bShutdown = true;
				return;
			}

			if (selectRet > 0)
			{
				for (int i = 0; i < FD_SETSIZE; i++)
				{
					if (sessionIter == _sessionList.end())
						break;

					if (FD_ISSET((*sessionIter)->_sock, (*rsetIter)))
						RecvProc((*sessionIter));

					sessionIter++;
				}
				rsetIter++;
			}
		}

		if (sessionIter == _sessionList.end())
			break;
	}

	DisconnectDeadSessions();

	// For Test
	if (GetAsyncKeyState(VK_RETURN) & 0x0001)
	{
		printf("session: %d, rset List Size: %d, wset List Size: %d, used wset num: %d\n",
			_sessionList.size(), _rsetList.size(), _wsetList.size(), usedWsetNum);

	}
}

void NetworkManager::Terminate()
{
	// For Test
	printf("session: %d, rset List Size: %d, wset List Size: %d, used wset num: %d\n",
		_sessionList.size(), _rsetList.size(), _wsetList.size(), usedWsetNum);

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

	fd_set* pSet;
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
