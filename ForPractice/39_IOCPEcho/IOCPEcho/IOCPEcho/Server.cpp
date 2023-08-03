#include "Server.h"
#include "Main.h"
#include <stdlib.h>
#include <stdio.h>

#define dfSERVER_PORT 9000
#define dfBUFSIZE 128

Server::Server()
{
	// Initialize Winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return;
	InitializeCriticalSection(&_cs);

	// Socket Bind, Listen
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(dfSERVER_PORT);

	int bindRet = bind(_listenSock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bindRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	int listenRet = listen(_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
		g_bShutdown = true;
		return;
	}

	_acceptThread = (HANDLE)_beginthreadex(
		NULL, 0, AcceptThread, nullptr, 0, nullptr);

	if (_acceptThread == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}

	// Create IOCP
	_hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hCP == NULL)
	{
		g_bShutdown = true;
		return;
	}

	// Create WorkerThread
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	_workerThreadCnt = (int)si.dwNumberOfProcessors;
	_workerThreads = new HANDLE[_workerThreadCnt];

	for (int i = 0; i < _workerThreadCnt; i++)
	{
		_workerThreads[i] = (HANDLE)_beginthreadex(
			NULL, 0, WorkerThread, _hCP, 0, nullptr);

		if (_workerThreads[i] == NULL)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			return;
		}
	}

	::printf("Complete Setting for Listen!\n");
}

Server::~Server()
{
	unordered_map<int, Session*>::iterator iter = _SessionMap.begin();
	for (; iter != _SessionMap.end();)
	{
		Session* pSession = iter->second;
		iter = _SessionMap.erase(iter);
		closesocket(pSession->_sock);
		delete(pSession);
	}
	
	closesocket(_listenSock);
	WSACleanup();
	WaitForMultipleObjects(_workerThreadCnt, _workerThreads, true, INFINITE);
	WaitForSingleObject(_acceptThread, INFINITE);
	DeleteCriticalSection(&_cs);
	::printf("\nAll Thread Terminate!\n");
}

Server* Server::GetInstance()
{
	static Server _server;
	return &_server;
}

void Server::Monitor()
{
}

unsigned int WINAPI Server::AcceptThread(void* arg)
{
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	Server* pServer = Server::GetInstance();

	while (1)
	{
		// Accept
		SOCKET client_sock = accept(
			pServer->_listenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			continue;
		}

		// Create Session
		Session* pSession = new Session;
		if (pSession == nullptr)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			break;
		}

		pSession->_addr = clientaddr;
		pSession->_sock = client_sock;
		pSession->_ID = pServer->_sessionIDSupplier++;
		pSession->_wsaRecvbuf.buf = pSession->_recvBuf.GetWriteBufferPtr();
		pSession->_wsaRecvbuf.len = pSession->_recvBuf.DirectEnqueueSize();
		pSession->_wsaSendbuf.buf = pSession->_sendBuf.GetReadBufferPtr();
		pSession->_wsaSendbuf.len = pSession->_sendBuf.DirectDequeueSize();

		pSession->_pRecvOvl = new NetworkOverlapped;
		pSession->_pSendOvl = new NetworkOverlapped;
		pSession->_pRecvOvl->_type = NET_TYPE::RECV;
		pSession->_pSendOvl->_type = NET_TYPE::SEND;
		ZeroMemory(&pSession->_pRecvOvl->_ovl, sizeof(pSession->_pRecvOvl->_ovl));
		ZeroMemory(&pSession->_pSendOvl->_ovl, sizeof(pSession->_pSendOvl->_ovl));

		pServer->_SessionMap.insert(make_pair(pSession->_ID, pSession));
		::printf("Accept New Client (ID: %llu)\n", pSession->_ID);

		// Connect Session to IOCP and Post Recv
		CreateIoCompletionPort((HANDLE)pSession->_sock,
			pServer->_hCP, (ULONG_PTR)pSession, 0);
		pServer->RecvPost(pSession);
	}

	::printf("Accept Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

unsigned int WINAPI Server::WorkerThread(void* arg)
{
	HANDLE hCP = (HANDLE)arg;
	Server* pServer = Server::GetInstance();

	while (1)
	{
		Session* pSession;
		DWORD cbTransferred;
		NetworkOverlapped* pNetOvl = new NetworkOverlapped;
		int GQCSRet = GetQueuedCompletionStatus(hCP, &cbTransferred,
			(PULONG_PTR)&pSession, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		// Check Exception
		if (GQCSRet == 0 || cbTransferred == 0)
		{
			if (GQCSRet == 0)
			{
				DWORD arg1, arg2;
				WSAGetOverlappedResult(pSession->_sock, (LPOVERLAPPED)pNetOvl, &arg1, FALSE, &arg2);
				int err = WSAGetLastError();
				if (err != WSAECONNRESET)
				{
					::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
				}
			}
			
			if (InterlockedDecrement(&pSession->_IOCount) == 0)
			{
				pServer->ReleaseSession(pSession);
			}

			continue;
		}

		// Recv
		if (pNetOvl->_type == NET_TYPE::RECV)
		{	
			InterlockedDecrement(&pSession->_IOCount);

			pServer->HandleEchoRecv(pSession, cbTransferred);
			pServer->SendPost(pSession);
			pServer->RecvPost(pSession);
		}

		// Send 
		if (pNetOvl->_type == NET_TYPE::SEND)
		{
			InterlockedDecrement(&pSession->_IOCount);
			pServer->HandleEchoSend(pSession, cbTransferred);
		}
	}

	::printf("Worker Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

int Server::RecvPost(Session* pSession)
{
	DWORD flags = 0;
	DWORD recvBytes = 0;
	ZeroMemory(&pSession->_pRecvOvl->_ovl, sizeof(pSession->_pRecvOvl->_ovl));
	pSession->_wsaRecvbuf.buf = pSession->_recvBuf.GetWriteBufferPtr();
	pSession->_wsaRecvbuf.len = pSession->_recvBuf.DirectEnqueueSize();

	InterlockedIncrement(&pSession->_IOCount);

	int recvRet = WSARecv(pSession->_sock, &pSession->_wsaRecvbuf,
		1, &recvBytes, &flags, (LPOVERLAPPED)pSession->_pRecvOvl, NULL);

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET)
			{
				::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
			}
		
			if (InterlockedDecrement(&pSession->_IOCount) == 0)
			{
				ReleaseSession(pSession);
			}
		}
	}

	return recvRet;
}

int Server::SendPost(Session* pSession)
{
	DWORD sendBytes;
	ZeroMemory(&pSession->_pSendOvl->_ovl, sizeof(pSession->_pSendOvl->_ovl));
	pSession->_wsaSendbuf.buf = pSession->_sendBuf.GetReadBufferPtr();
	pSession->_wsaSendbuf.len = pSession->_sendBuf.DirectDequeueSize();

	InterlockedIncrement(&pSession->_IOCount);

	int sendRet = WSASend(pSession->_sock, &pSession->_wsaSendbuf,
		1, &sendBytes, 0, (LPOVERLAPPED)pSession->_pSendOvl, NULL);

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if(err != WSAECONNRESET)
			{
				::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
			}
		
			if (InterlockedDecrement(&pSession->_IOCount) == 0)
			{
				ReleaseSession(pSession);
			}
		}
	}
	
	return sendRet;
}

void Server::ReleaseSession(Session* pSession)
{
	EnterCriticalSection(&_cs);

	unordered_map<int, Session*>::iterator iter = _SessionMap.find(pSession->_ID);
	if (iter == _SessionMap.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&_cs);
		return;
	}

	_SessionMap.erase(iter);
	closesocket(pSession->_sock);
	__int64 ID = pSession-> _ID;
	delete(pSession);

	LeaveCriticalSection(&_cs);
	::printf("Disconnect Client (ID: %llu)\n", ID);
}

void Server::EnqueueUnicast(char* msg, int size, Session* pSession)
{
	int enqueueRet = pSession->_sendBuf.Enqueue(msg, size);
	if (enqueueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
	}
}

void Server::HandleEchoRecv(Session* pSession, int recvBytes)
{
	//::printf("Success to Recv %dbytes!\n", recvBytes);
	pSession->_recvBuf.MoveWritePos(recvBytes);

	char data[dfBUFSIZE] = { '\0', };
	GetData_Echo(&pSession->_recvBuf, recvBytes, data);
	printf("%llu: %s\n", pSession->_ID, data);

	SerializePacket buffer;
	SetData_Echo(&buffer, recvBytes, data);
	EnqueueUnicast(buffer.GetReadPtr(), recvBytes, pSession);
}

void Server::HandleEchoSend(Session* pSession, int sendBytes)
{
	//::printf("Success to Send %dbytes!\n\n", sendBytes);
	pSession->_sendBuf.MoveReadPos(sendBytes);
}

void Server::GetData_Echo(RingBuffer* recvBuffer, int size, char* data)
{
	SerializePacket buffer;
	int dequeueRet = recvBuffer->Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}
	buffer.MoveWritePos(dequeueRet);
	buffer.GetData(data, size);
}

void Server::SetData_Echo(SerializePacket* buffer, int size, char* data)
{
	buffer->PutData(data, size);
	if (buffer->GetDataSize() != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}
}



