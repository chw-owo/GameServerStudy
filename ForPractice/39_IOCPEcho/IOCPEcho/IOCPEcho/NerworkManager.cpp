#include "NetworkManager.h"
#include "Main.h"
#include <stdlib.h>
#include <stdio.h>

#define dfSERVER_PORT 9000
#define dfBUFSIZE 128

NetworkManager::NetworkManager()
{
	// Initialize Winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return;

	// Socket Bind, Listen
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}

	SOCKADDR_IN NetworkManageraddr;
	ZeroMemory(&NetworkManageraddr, sizeof(NetworkManageraddr));
	NetworkManageraddr.sin_family = AF_INET;
	NetworkManageraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	NetworkManageraddr.sin_port = htons(dfSERVER_PORT);

	int bindRet = bind(_listenSock, (SOCKADDR*)&NetworkManageraddr, sizeof(NetworkManageraddr));
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

	// Create IOCP for Network
	_hNetworkCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hNetworkCP == NULL)
	{
		g_bShutdown = true;
		return;
	}

	// Create networkThread
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int cpuCnt = (int)si.dwNumberOfProcessors;
	_networkThreadCnt = cpuCnt;
	_networkThreads = new HANDLE[_networkThreadCnt];

	for (int i = 0; i < _networkThreadCnt; i++)
	{
		_networkThreads[i] = (HANDLE)_beginthreadex(
			NULL, 0, NetworkThread, _hNetworkCP, 0, nullptr);

		if (_networkThreads[i] == NULL)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			return;
		}
	}

	// Create IOCP for Echo
	_hEchoCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hEchoCP == NULL)
	{
		g_bShutdown = true;
		return;
	}

	// Create EchoThread
	_echoThreadCnt = cpuCnt;
	_echoThreads = new HANDLE[_echoThreadCnt];

	for (int i = 0; i < _echoThreadCnt; i++)
	{
		_echoThreads[i] = (HANDLE)_beginthreadex(
			NULL, 0, EchoThread, _hEchoCP, 0, nullptr);

		if (_echoThreads[i] == NULL)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			return;
		}
	}

	::printf("Complete Setting\n");
}

NetworkManager::~NetworkManager()
{
	unordered_map<int, Session*>::iterator iter = g_SessionMap.begin();
	for (; iter != g_SessionMap.end();)
	{
		Session* pSession = iter->second;
		iter = g_SessionMap.erase(iter);
		closesocket(pSession->_sock);
		delete(pSession);
	}
	
	closesocket(_listenSock);
	WSACleanup();

	for(int i = 0; i < _networkThreadCnt; i++)
	{
		PostQueuedCompletionStatus(_hNetworkCP, 0, 0, 0);
	}

	for (int i = 0; i < _echoThreadCnt; i++)
	{
		PostQueuedCompletionStatus(_hEchoCP, 0, 0, 0);
	}

	WaitForMultipleObjects(_networkThreadCnt, _networkThreads, true, INFINITE);
	WaitForMultipleObjects(_echoThreadCnt, _echoThreads, true, INFINITE);
	WaitForSingleObject(_acceptThread, INFINITE);

	::printf("\nAll Thread Terminate!\n");
}

NetworkManager* NetworkManager::GetInstance()
{
	static NetworkManager _networkManager;
	return &_networkManager;
}

unsigned int WINAPI NetworkManager::AcceptThread(void* arg)
{
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	NetworkManager* pNetworkManager = NetworkManager::GetInstance();

	while (1)
	{
		if (g_bShutdown)
			break;

		// Accept
		SOCKET client_sock = accept(
			pNetworkManager->_listenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			continue;
		}

		// Create Session
		Session* pSession = new Session(
			NetworkManager::GetInstance()->_sessionIDSupplier++, client_sock, clientaddr);
		if (pSession == nullptr)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			break;
		}

		AcquireSRWLockExclusive(&g_SessionMapLock);
		g_SessionMap.insert(make_pair(pSession->_ID, pSession));
		ReleaseSRWLockExclusive(&g_SessionMapLock);

		::printf("Accept New Client (ID: %llu)\n", pSession->_ID);

		// Connect Session to IOCP and Post Recv
		CreateIoCompletionPort((HANDLE)pSession->_sock,
			pNetworkManager->_hNetworkCP, (ULONG_PTR)pSession, 0);

		pNetworkManager->RecvPost(pSession);
	}

	::printf("Accept Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

unsigned int WINAPI NetworkManager::NetworkThread(void* arg)
{
	HANDLE hNetworkCP = (HANDLE)arg;
	NetworkManager* pNetworkManager = NetworkManager::GetInstance();

	while (1)
	{
		Session* pSession;
		DWORD cbTransferred;
		NetworkOverlapped* pNetOvl = new NetworkOverlapped;
		int GQCSRet = GetQueuedCompletionStatus(hNetworkCP, &cbTransferred,
			(PULONG_PTR)&pSession, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (g_bShutdown)
		{
			break;
		}

		// Check Exception
		else if (GQCSRet == 0 || cbTransferred == 0)
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
		}

		// Recv
		else if (pNetOvl->_type == NET_TYPE::RECV)
		{	
			pNetworkManager->HandleEchoRecv(pSession, cbTransferred);
			pNetworkManager->RecvPost(pSession);
		}

		// Send 
		else if (pNetOvl->_type == NET_TYPE::SEND)
		{
			InterlockedDecrement(&pSession->_sendFlag);
			pNetworkManager->HandleEchoSend(pSession, cbTransferred);
		}

		if (InterlockedDecrement(&pSession->_IOCount) == 0)
		{
			pNetworkManager->ReleaseSession(pSession);
		}
	}

	::printf("Worker Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

unsigned int __stdcall NetworkManager::EchoThread(void* arg)
{
	HANDLE hEchoCP = (HANDLE)arg;
	NetworkManager* pNetworkManager = NetworkManager::GetInstance();

	while (1)
	{
		int recvBytes;
		__int64 sessionID;
		OVERLAPPED* pOvl = nullptr;

		int GQCSRet = GetQueuedCompletionStatus(hEchoCP, 
			(LPDWORD)&recvBytes, (PULONG_PTR)&sessionID, (LPOVERLAPPED*)&pOvl, INFINITE);

		if (g_bShutdown)
		{
			break;
		}

		AcquireSRWLockShared(&g_SessionMapLock);

		unordered_map<int, Session*>::iterator iter = g_SessionMap.find(sessionID);
		if (iter == g_SessionMap.end())
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		Session* pSession = iter->second;

		AcquireSRWLockExclusive(&pSession->_lock);
		ReleaseSRWLockShared(&g_SessionMapLock);

		char data[dfBUFSIZE] = { '\0', };
		pNetworkManager->GetData_Echo(&pSession->_recvBuf, recvBytes, data);
		printf("(Echo Thread) %llu: %s\n", pSession->_ID, data);

		SerializePacket buffer;
		pNetworkManager->SetData_Echo(&buffer, recvBytes, data);
		pNetworkManager->EnqueueUnicast(buffer.GetReadPtr(), recvBytes, pSession);
		pNetworkManager->SendPost(pSession);

		ReleaseSRWLockExclusive(&pSession->_lock);
	}

	::printf("Echo Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

int NetworkManager::RecvPost(Session* pSession)
{
	DWORD flags = 0;
	DWORD recvBytes = 0;
	WSABUF wsaRecvbuf;

	wsaRecvbuf.buf = pSession->_recvBuf.GetWriteBufferPtr();
	wsaRecvbuf.len = pSession->_recvBuf.DirectEnqueueSize();
	ZeroMemory(&pSession->_recvOvl._ovl, sizeof(pSession->_recvOvl._ovl));

	InterlockedIncrement(&pSession->_IOCount);
	int recvRet = WSARecv(pSession->_sock, &wsaRecvbuf,
		1, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvOvl, NULL);

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET)
			{
				::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
			}
		}
	}

	return recvRet;
}

int NetworkManager::SendPost(Session* pSession)
{
	if (pSession->_sendFlag != 0)
		return -1;

	DWORD sendBytes;
	WSABUF wsaSendbuf;

	wsaSendbuf.buf = pSession->_sendBuf.GetReadBufferPtr();
	wsaSendbuf.len = pSession->_sendBuf.DirectDequeueSize();
	ZeroMemory(&pSession->_sendOvl._ovl, sizeof(pSession->_sendOvl._ovl));

	InterlockedIncrement(&pSession->_sendFlag);
	InterlockedIncrement(&pSession->_IOCount);
	int sendRet = WSASend(pSession->_sock, &wsaSendbuf,
		1, &sendBytes, 0, (LPOVERLAPPED)&pSession->_sendOvl, NULL);

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if(err != WSAECONNRESET)
			{
				::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
			}
		}
	}
	
	return sendRet;
}

void NetworkManager::ReleaseSession(Session* pSession)
{
	AcquireSRWLockExclusive(&g_SessionMapLock);
	unordered_map<int, Session*>::iterator iter = g_SessionMap.find(pSession->_ID);
	if (iter == g_SessionMap.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}
	g_SessionMap.erase(iter);

	AcquireSRWLockExclusive(&pSession->_lock);
	ReleaseSRWLockExclusive(&g_SessionMapLock);

	closesocket(pSession->_sock);
	__int64 ID = pSession-> _ID;
	delete(pSession);
	//ReleaseSRWLockExclusive(&pSession->_lock);

	::printf("Disconnect Client (ID: %llu)\n", ID);
}

void NetworkManager::EnqueueUnicast(char* msg, int size, Session* pSession)
{
	int enqueueRet = pSession->_sendBuf.Enqueue(msg, size);
	if (enqueueRet != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
	}
}

void NetworkManager::HandleEchoRecv(Session* pSession, int recvBytes)
{
	//::printf("Success to Recv %dbytes!\n", recvBytes);
	pSession->_recvBuf.MoveWritePos(recvBytes);
	PostQueuedCompletionStatus(_hEchoCP, recvBytes, pSession->_ID, 0);
}

void NetworkManager::HandleEchoSend(Session* pSession, int sendBytes)
{
	//::printf("Success to Send %dbytes!\n\n", sendBytes);
	pSession->_sendBuf.MoveReadPos(sendBytes);
}

void NetworkManager::GetData_Echo(RingBuffer* recvBuffer, int size, char* data)
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

void NetworkManager::SetData_Echo(SerializePacket* buffer, int size, char* data)
{
	buffer->PutData(data, size);
	if (buffer->GetDataSize() != size)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}
}



