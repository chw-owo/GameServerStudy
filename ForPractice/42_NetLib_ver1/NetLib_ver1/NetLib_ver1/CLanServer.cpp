#include "CLanServer.h"
#include "Main.h"
#include <stdlib.h>
#include <stdio.h>

bool CLanServer::NetworkStart(const wchar_t* IP, short port,
				int numOfWorkerThreads, bool nagle, int sessionMax)
{
	// Option Setting ====================================================
	
	wcscpy_s(_IP, 10, IP);
	_port = port;
	_numOfWorkerThreads = numOfWorkerThreads;
	_nagle = nagle;
	_sessionMax = sessionMax;

	// Network Setting ===================================================

	// Initialize Winsock
	WSADATA wsa;
	int startRet = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (startRet != 0)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		return false;
	}

	// Socket Bind, Listen
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		return false;
	}

	LINGER optval;
	optval.l_onoff = 1;
	optval.l_linger = 0;
	int optRet = setsockopt(_listenSock, SOL_SOCKET, SO_LINGER,
		(char*)&optval, sizeof(optval));
	if (optRet == SOCKET_ERROR)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		return false;
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(_port);

	int bindRet = bind(_listenSock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bindRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
		return false;
	}

	int listenRet = listen(_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
		return false;
	}

	// Thread Setting ===================================================

	_alive = true;
	CLanServer* pLanServer = this;
	InitializeSRWLock(&_SessionMapLock);

	// Create Accept Thread
	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, pLanServer, 0, nullptr);
	if (_acceptThread == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		return false;
	}

	// Create IOCP for Release
	_hReleaseCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hReleaseCP == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		return false;
	}

	// Create Release Thread
	_releaseThread = (HANDLE)_beginthreadex(NULL, 0, ReleaseThread, pLanServer, 0, nullptr);
	if (_releaseThread == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		return false;
	}

	// Create IOCP for Network
	_hNetworkCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hNetworkCP == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		return false;
	}

	// Create Network Thread
	_networkThreads = new HANDLE[_numOfWorkerThreads];
	for (int i = 0; i < _numOfWorkerThreads; i++)
	{
		_networkThreads[i] = (HANDLE)_beginthreadex(
			NULL, 0, NetworkThread, pLanServer, 0, nullptr);

		if (_networkThreads[i] == NULL)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			return false;
		}
	}

	::printf("Network Start\n");
	return true;
}

void CLanServer::NetworkStop()
{
	if(!_alive)
	{
		::printf("Invalid Request. It's already stopped\n");
		return;
	}

	_alive = false;

	// Terminate Network Threads
	for (int i = 0; i < _numOfWorkerThreads; i++)
		PostQueuedCompletionStatus(_hNetworkCP, 0, 0, 0);
	WaitForMultipleObjects(_numOfWorkerThreads, _networkThreads, true, INFINITE);

	// Terminate Accept Thread
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	InetPton(AF_INET, _IP, &serveraddr.sin_addr);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(_port);

	SOCKET socktmp = socket(AF_INET, SOCK_STREAM, 0);
	if (socktmp == INVALID_SOCKET)
	{
		int err = ::WSAGetLastError();
		::printf("Error! Line %d: %d", __LINE__, err);
		return;
	}

	int connectRet = connect(socktmp, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (connectRet == SOCKET_ERROR)
	{
		int err = ::WSAGetLastError();
		::printf("Error! Line %d: %d", __LINE__, err);
		return;
	}
	WaitForSingleObject(_acceptThread, INFINITE);

	// Terminate Release Threads
	PostQueuedCompletionStatus(_hReleaseCP, 0, 0, 0);
	WaitForSingleObject(_releaseThread, INFINITE);

	// Release All Session
	closesocket(socktmp);
	closesocket(_listenSock);
	unordered_map<__int64, CSession*>::iterator iter = _SessionMap.begin();
	for (; iter != _SessionMap.end();)
	{
		CSession* pSession = iter->second;
		closesocket(pSession->_sock);
		__int64 ID = pSession->_ID;
		delete(pSession);
		//::printf("Disconnect Client(ID: % llu)\n", ID);
		iter = _SessionMap.erase(iter);
	}

	_sessionCnt = 0;
	WSACleanup();

	CloseHandle(_hReleaseCP);
	CloseHandle(_hNetworkCP);
	CloseHandle(_acceptThread);
	CloseHandle(_releaseThread);
	for (int i = 0; i < _numOfWorkerThreads; i++)
		CloseHandle(_networkThreads[i]);
	delete[] _networkThreads;

	::printf("\nNetwork Stop\n");
}

bool CLanServer::Disconnect(__int64 sessionID)
{
	// TO-DO:
	// 잘 동작하는지 테스트 필요
	AcquireSRWLockShared(&_SessionMapLock);

	unordered_map<__int64, CSession*>::iterator iter = _SessionMap.find(sessionID);
	if (iter == _SessionMap.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		ReleaseSRWLockShared(&_SessionMapLock);
		return false;
	}

	CSession* pSession = iter->second;
	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&_SessionMapLock);

	PostQueuedCompletionStatus(_hNetworkCP, 0, (ULONG_PTR)pSession, 0);
	PostQueuedCompletionStatus(_hNetworkCP, 0, (ULONG_PTR)pSession, 0);

	LeaveCriticalSection(&pSession->_cs);
	return true;
}

bool CLanServer::SendPacket(__int64 sessionID, CPacket* packet)
{
	AcquireSRWLockShared(&_SessionMapLock);

	unordered_map<__int64, CSession*>::iterator iter = _SessionMap.find(sessionID);
	if (iter == _SessionMap.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		ReleaseSRWLockShared(&_SessionMapLock);
		return false;
	}

	CSession* pSession = iter->second;
	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&_SessionMapLock);

	int payloadSize = packet->GetPayloadSize();
	stLanHeader header;
	header._shLen = payloadSize;
	int putRet = packet->PutHeaderData((char*)&header, sizeof(header));
	if (putRet != sizeof(header))
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&pSession->_cs);
		return false;
	}

	int packetSize = packet->GetPacketSize();
	int enqueueRet = pSession->_sendBuf.Enqueue(
		packet->GetPacketReadPtr(), packetSize);

	if (enqueueRet != packetSize)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&pSession->_cs);
		return false;
	}

	SendPost(pSession);

	LeaveCriticalSection(&pSession->_cs);
	return true;
}

unsigned int __stdcall CLanServer::AcceptThread(void* arg)
{
	CLanServer* pLanServer = (CLanServer*)arg;

	int sessionID = 0;
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);

	for (;;)
	{
		// Accept
		SOCKET client_sock = accept(pLanServer->_listenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		if (g_bShutdown) break;
		if (!pLanServer->_alive) break;
		if (!pLanServer->OnConnectRequest()) continue;

		if (pLanServer->_sessionCnt >= pLanServer->_sessionMax)
		{
			closesocket(client_sock);
			continue;
		}
		
		// Create Session
		CSession* pSession = new CSession(sessionID++, client_sock, clientaddr);
		if (pSession == nullptr)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		AcquireSRWLockExclusive(&pLanServer->_SessionMapLock);
		pLanServer->_SessionMap.insert(make_pair(pSession->_ID, pSession));
		ReleaseSRWLockExclusive(&pLanServer->_SessionMapLock);

		//::printf("Accept New Session (ID: %llu)\n", pSession->_ID);
		InterlockedIncrement(&pLanServer->_sessionCnt);
		pLanServer->_acceptCnt++;
		pLanServer->OnAcceptClient();

		// Connect Session to IOCP and Post Recv
		CreateIoCompletionPort((HANDLE)pSession->_sock,
			pLanServer->_hNetworkCP, (ULONG_PTR)pSession, 0);

		pLanServer->RecvPost(pSession);
	}

	::printf("Accept Thread Terminate (thread: %d)\n", GetCurrentThreadId());
	return 0;
}

unsigned int __stdcall CLanServer::NetworkThread(void* arg)
{
	CLanServer* pLanServer = (CLanServer*)arg;
	int threadID = GetCurrentThreadId();

	for(;;)
	{
		CSession* pSession;
		DWORD cbTransferred;
		NetworkOverlapped* pNetOvl = new NetworkOverlapped;
		int GQCSRet = GetQueuedCompletionStatus(pLanServer->_hNetworkCP, 
			&cbTransferred, (PULONG_PTR)&pSession, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (g_bShutdown) break;
		if (!pLanServer->_alive) break;

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
		}
		else if (pNetOvl->_type == NET_TYPE::RECV)
		{
			//::printf("%llu: Complete Recv %d bytes (%d)\n", pSession->_ID, cbTransferred, threadID);
			pLanServer->_recvMsgCnt++;
			pLanServer->HandleRecvCP(pSession->_ID, cbTransferred);
		}
		else if (pNetOvl->_type == NET_TYPE::SEND)
		{
			//::printf("%llu: Complete Send %d bytes (%d)\n", pSession->_ID, cbTransferred, threadID);
			pLanServer->_sendMsgCnt++;
			pLanServer->HandleSendCP(pSession->_ID, cbTransferred);
		}

		EnterCriticalSection(&pSession->_cs);
		if (InterlockedDecrement(&pSession->_IOCount) == 0)
		{
			PostQueuedCompletionStatus(
				pLanServer->_hReleaseCP, 0, (ULONG_PTR)pSession, 0);
		}
		LeaveCriticalSection(&pSession->_cs);
	}

	::printf("Worker Thread Terminate (thread: %d)\n", threadID);
	return 0;
}

unsigned int __stdcall CLanServer::ReleaseThread(void* arg)
{
	CLanServer* pLanServer = (CLanServer*)arg;
	int threadID = GetCurrentThreadId();

	for(;;)
	{
		CSession* pSession;
		DWORD cbTransferred;
		OVERLAPPED* pOvl = nullptr;

		int GQCSRet = GetQueuedCompletionStatus(pLanServer->_hReleaseCP, 
			&cbTransferred, (PULONG_PTR)&pSession, &pOvl, INFINITE);

		if (g_bShutdown) break;
		if (!pLanServer->_alive) break;

		AcquireSRWLockExclusive(&pLanServer->_SessionMapLock);
		unordered_map<__int64, CSession*>::iterator iter 
			= pLanServer->_SessionMap.find(pSession->_ID);
		if (iter == pLanServer->_SessionMap.end())
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			ReleaseSRWLockExclusive(&pLanServer->_SessionMapLock);
			return 0;
		}

		pLanServer->_SessionMap.erase(iter);
		ReleaseSRWLockExclusive(&pLanServer->_SessionMapLock);

		EnterCriticalSection(&pSession->_cs);
		LeaveCriticalSection(&pSession->_cs);

		closesocket(pSession->_sock);
		__int64 ID = pSession->_ID;
		delete(pSession);

		//::printf("%llu: Disconnect Client\n", ID);
		pLanServer->_disconnectCnt++;
		InterlockedDecrement(&pLanServer->_sessionCnt);
		pLanServer->OnReleaseClient(ID);
	}
	::printf("Release Thread Terminate (thread: %d)\n", threadID);
	return 0;
}

void CLanServer::UpdateMonitorData()
{
	_acceptTPS = _acceptCnt;
	_disconnectTPS = _disconnectCnt;
	_recvMsgTPS = _recvMsgCnt;
	_sendMsgTPS = _sendMsgCnt;
		
	_acceptTotal += _acceptTPS;
	_disconnectTotal += _disconnectTPS;

	_acceptCnt = 0;
	_disconnectCnt = 0;
	_recvMsgCnt = 0;
	_sendMsgCnt = 0;
}

void CLanServer::HandleRecvCP(__int64 sessionID, int recvBytes)
{
	AcquireSRWLockShared(&_SessionMapLock);
	unordered_map<__int64, CSession*>::iterator iter = _SessionMap.find(sessionID);
	if (iter == _SessionMap.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		ReleaseSRWLockShared(&_SessionMapLock);
		return;
	}

	CSession* pSession = iter->second;
	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&_SessionMapLock);

	int moveReadRet = pSession->_recvBuf.MoveWritePos(recvBytes);
	if (moveReadRet != recvBytes)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&pSession->_cs);
		return;
	}

	int useSize = pSession->_recvBuf.GetUseSize();
	while (useSize > 0)
	{
		if (useSize <= dfLAN_HEADER_LEN)
			break;

		stLanHeader header;
		int peekRet = pSession->_recvBuf.Peek((char*)&header, dfLAN_HEADER_LEN);
		if (peekRet != dfLAN_HEADER_LEN)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		if (useSize < dfLAN_HEADER_LEN + header._shLen)
			break;

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfLAN_HEADER_LEN);
		if (moveReadRet != dfLAN_HEADER_LEN)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		CPacket packet(dfLAN_HEADER_LEN);
		int dequeueRet = pSession->_recvBuf.Dequeue(packet.GetPayloadWritePtr(), header._shLen);
		if (dequeueRet != header._shLen)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		packet.MovePayloadWritePos(dequeueRet);
		LeaveCriticalSection(&pSession->_cs);

		OnRecv(pSession->_ID, &packet);
		
		EnterCriticalSection(&pSession->_cs);
		useSize = pSession->_recvBuf.GetUseSize();
	}

	RecvPost(pSession);
	LeaveCriticalSection(&pSession->_cs);

}

void CLanServer::HandleSendCP(__int64 sessionID, int sendBytes)
{
	AcquireSRWLockShared(&_SessionMapLock);

	unordered_map<__int64, CSession*>::iterator iter = _SessionMap.find(sessionID);
	if (iter == _SessionMap.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		ReleaseSRWLockShared(&_SessionMapLock);
		return;
	}

	CSession* pSession = iter->second;
	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&_SessionMapLock);

	int moveReadRet = pSession->_sendBuf.MoveReadPos(sendBytes);
	if (moveReadRet != sendBytes)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&pSession->_cs);
		return;
	}

	OnSend(pSession->_ID, sendBytes);
	InterlockedExchange(&pSession->_sendFlag, 0);
	SendPost(pSession);		

	LeaveCriticalSection(&pSession->_cs);
}

void CLanServer::RecvPost(CSession* pSession)
{
	DWORD flags = 0;
	DWORD recvBytes = 0;

	int freeSize = pSession->_recvBuf.GetFreeSize();
	pSession->_wsaRecvbuf[0].buf = pSession->_recvBuf.GetWriteBufferPtr();
	pSession->_wsaRecvbuf[0].len = pSession->_recvBuf.DirectEnqueueSize();
	pSession->_wsaRecvbuf[1].buf = pSession->_recvBuf.GetBufferFrontPtr();
	pSession->_wsaRecvbuf[1].len = freeSize - pSession->_wsaRecvbuf[0].len;

	ZeroMemory(&pSession->_recvOvl._ovl, sizeof(pSession->_recvOvl._ovl));
	InterlockedIncrement(&pSession->_IOCount);

	int recvRet = WSARecv(pSession->_sock, pSession->_wsaRecvbuf,
		2, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvOvl, NULL);

	//::printf("%llu: Request Recv (%d)\n", pSession->_ID, GetCurrentThreadId());

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET)
			{
				::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
			}
			LeaveCriticalSection(&pSession->_cs);
			return;
		}
	}
}

void CLanServer::SendPost(CSession* pSession)
{
	if (pSession->_sendBuf.GetUseSize() == 0) return;
	if(InterlockedExchange(&pSession->_sendFlag, 1) == 1) return;
	if (pSession->_sendBuf.GetUseSize() == 0)
	{
		InterlockedExchange(&pSession->_sendFlag, 0);
		return;
	}

	DWORD sendBytes;

	int useSize = pSession->_sendBuf.GetUseSize();
	pSession->_wsaSendbuf[0].buf = pSession->_sendBuf.GetReadBufferPtr();
	pSession->_wsaSendbuf[0].len = pSession->_sendBuf.DirectDequeueSize();
	pSession->_wsaSendbuf[1].buf = pSession->_sendBuf.GetBufferFrontPtr();
	pSession->_wsaSendbuf[1].len = useSize - pSession->_wsaSendbuf[0].len;

	ZeroMemory(&pSession->_sendOvl._ovl, sizeof(pSession->_sendOvl._ovl));
	InterlockedIncrement(&pSession->_IOCount);
	InterlockedExchange(&pSession->_sendFlag, 1);

	int sendRet = WSASend(pSession->_sock, pSession->_wsaSendbuf,
		2, &sendBytes, 0, (LPOVERLAPPED)&pSession->_sendOvl, NULL);

	//::printf("%llu: Request Send %d bytes (%d)\n", pSession->_ID , useSize, GetCurrentThreadId());

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET)
			{
				::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
			}
			LeaveCriticalSection(&pSession->_cs);
			return;
		}
	}
}

