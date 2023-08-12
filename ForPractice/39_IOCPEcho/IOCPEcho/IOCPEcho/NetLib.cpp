#include "NetLib.h"
#include "Main.h"
#include <stdlib.h>
#include <stdio.h>

/* TO-DO
ㅎㅎ 돌고 돌아 send 0 byte 문제로~
*/

NetLib::NetLib()
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

	LINGER optval;
	optval.l_onoff = 1;
	optval.l_linger = 0;
	int optRet = setsockopt(_listenSock, SOL_SOCKET, SO_LINGER,
		(char*)&optval, sizeof(optval));
	if (optRet == SOCKET_ERROR)
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

	InitializeSRWLock(&g_SessionMapLock);
	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, nullptr, 0, nullptr);
	if (_acceptThread == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}

	// Create IOCP for Release Session
	_hReleaseCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hReleaseCP == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}

	_releaseThread = (HANDLE)_beginthreadex(NULL, 0, ReleaseThread, _hReleaseCP, 0, nullptr);
	if (_releaseThread == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}

	// Create IOCP for Network
	_hNetworkCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hNetworkCP == NULL)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
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
		unsigned int id;
		_networkThreads[i] = (HANDLE)_beginthreadex(
			NULL, 0, NetworkThread, _hNetworkCP, 0, &id);

		if (_networkThreads[i] == NULL)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			return;
		}

		ThreadIDMap.insert(make_pair(id, i));
	}

	::printf("Complete Setting\n");
}

NetLib::~NetLib()
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
	PostQueuedCompletionStatus(_releaseThread, 0, 0, 0);

	WaitForMultipleObjects(_networkThreadCnt, _networkThreads, true, INFINITE);
	WaitForSingleObject(_acceptThread, INFINITE);
	WaitForSingleObject(_releaseThread, INFINITE);

	::printf("\nAll Thread Terminate!\n");
}

NetLib* NetLib::GetInstance()
{
	static NetLib _netLib;
	return &_netLib;
}

unsigned int WINAPI NetLib::AcceptThread(void* arg)
{
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	NetLib* pNetLib = NetLib::GetInstance();

	while (1)
	{
		// Accept
		SOCKET client_sock = accept(
			pNetLib->_listenSock, (SOCKADDR*)&clientaddr, &addrlen);

		if (g_bShutdown) break;

		if (client_sock == INVALID_SOCKET)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			continue;
		}

		// Create Session
		Session* pSession = new Session(
			NetLib::GetInstance()->_sessionIDSupplier++, client_sock, clientaddr);
		if (pSession == nullptr)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		AcquireSRWLockExclusive(&g_SessionMapLock);
		g_SessionMap.insert(make_pair(pSession->_ID, pSession));
		ReleaseSRWLockExclusive(&g_SessionMapLock);

		::printf("Accept New Session (ID: %llu)\n", pSession->_ID);

		// Connect Session to IOCP and Post Recv
		CreateIoCompletionPort((HANDLE)pSession->_sock,
			pNetLib->_hNetworkCP, (ULONG_PTR)pSession, 0);

		pNetLib->RecvPost(pSession->_ID, GetCurrentThreadId());
	}

	::printf("Accept Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

unsigned int WINAPI NetLib::NetworkThread(void* arg)
{
	HANDLE hNetworkCP = (HANDLE)arg;
	NetLib* pNetLib = NetLib::GetInstance();
	unordered_map<unsigned int, int>:: iterator iter 
		= pNetLib->ThreadIDMap.find(GetCurrentThreadId());
	int threadID = iter->second;

	while (1)
	{
		Session* pSession;
		DWORD cbTransferred;
		NetworkOverlapped* pNetOvl = new NetworkOverlapped;
		int GQCSRet = GetQueuedCompletionStatus(hNetworkCP, &cbTransferred,
			(PULONG_PTR)&pSession, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (g_bShutdown) break;

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

		// Recv
		else if (pNetOvl->_type == NET_TYPE::RECV)
		{	
			//::printf("%llu: Complete Recv %d bytes (thread: %d)\n", pSession->_ID, cbTransferred, threadID);
			pNetLib->HandleRecvCP(pSession->_ID, cbTransferred, threadID);
		}

		// Send 
		else if (pNetOvl->_type == NET_TYPE::SEND)
		{
			//::printf("%d: Complete Send %d bytes\n", threadID, cbTransferred);
			pNetLib->HandleSendCP(pSession->_ID, cbTransferred, threadID);
		}
		else
		{
			printf("Wrong Type: %d\n", pNetOvl->_type);
		}

		if (InterlockedDecrement(&pSession->_IOCount) == 0)
		{
			PostQueuedCompletionStatus(
				pNetLib->_hReleaseCP, 0,(ULONG_PTR)pSession, 0);
		}
	}

	::printf("Worker Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

unsigned int WINAPI NetLib::ReleaseThread(void* arg)
{
	HANDLE hReleaseCP = (HANDLE)arg;
	NetLib* pNetLib = NetLib::GetInstance();

	while (1)
	{
		Session* pSession;
		DWORD cbTransferred;
		OVERLAPPED* pOvl = nullptr;

		int GQCSRet = GetQueuedCompletionStatus(hReleaseCP, &cbTransferred,
			(PULONG_PTR)&pSession, &pOvl, INFINITE);

		if (g_bShutdown) break;

		AcquireSRWLockExclusive(&g_SessionMapLock);
		unordered_map<int, Session*>::iterator iter = g_SessionMap.find(pSession->_ID);
		if (iter == g_SessionMap.end())
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			ReleaseSRWLockExclusive(&g_SessionMapLock);
			return 0;
		}
		g_SessionMap.erase(iter);
		ReleaseSRWLockExclusive(&g_SessionMapLock);

		EnterCriticalSection(&pSession->_cs);
		LeaveCriticalSection(&pSession->_cs);

		closesocket(pSession->_sock);
		__int64 ID = pSession->_ID;
		delete(pSession);

		::printf("Disconnect Client (ID: %llu)\n", ID);
	}
	::printf("Release Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}


// pSession으로 바로 받아도 되지만 동기화 연습 겸 ID 전달 -> map에서 탐색하는 구조로
void NetLib::HandleRecvCP(__int64 sessionID, int recvBytes, int threadID)
{	
	AcquireSRWLockShared(&g_SessionMapLock);
	unordered_map<int, Session*>::iterator iter = g_SessionMap.find(sessionID);
	if (iter == g_SessionMap.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		ReleaseSRWLockShared(&g_SessionMapLock);
		return;
	}

	Session* pSession = iter->second;

	EnterCriticalSection(&pSession->_cs);

	ReleaseSRWLockShared(&g_SessionMapLock);

	int moveReadRet = pSession->_recvBuf.MoveWritePos(recvBytes);
	if (moveReadRet != recvBytes)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&pSession->_cs);
		return;
	}
	LeaveCriticalSection(&pSession->_cs);

	RecvDataToMsg(pSession->_ID, threadID); // include onMessage, SendMsg, MsgToSendData
	RecvPost(pSession->_ID, threadID);
}

void NetLib::HandleSendCP(__int64 sessionID, int sendBytes, int threadID)
{
	AcquireSRWLockShared(&g_SessionMapLock);

	unordered_map<int, Session*>::iterator iter = g_SessionMap.find(sessionID);
	if (iter == g_SessionMap.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		ReleaseSRWLockShared(&g_SessionMapLock);
		return;
	}

	Session* pSession = iter->second;

	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&g_SessionMapLock);

	int moveReadRet = pSession->_sendBuf.MoveReadPos(sendBytes);
	if (moveReadRet != sendBytes)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&pSession->_cs);
		return;
	}
	LeaveCriticalSection(&pSession->_cs);

	if (InterlockedDecrement(&pSession->_sendFlag) != 0)
	{
		//::printf("%d: Call SendPost in %s (sendFlag: %d)\n", 
		//	threadID, __func__, pSession->_sendFlag);
		SendPost(pSession->_ID, threadID);
	}
}

void NetLib::RecvPost(__int64 sessionID, int threadID)
{
	DWORD flags = 0;
	DWORD recvBytes = 0;

	AcquireSRWLockShared(&g_SessionMapLock);

	unordered_map<int, Session*>::iterator iter = g_SessionMap.find(sessionID);
	if (iter == g_SessionMap.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		ReleaseSRWLockShared(&g_SessionMapLock);
		return;
	}

	Session* pSession = iter->second;
	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&g_SessionMapLock);

	int freeSize = pSession->_recvBuf.GetFreeSize();
	pSession->_wsaRecvbuf[0].buf = pSession->_recvBuf.GetWriteBufferPtr();
	pSession->_wsaRecvbuf[0].len = pSession->_recvBuf.DirectEnqueueSize();
	pSession->_wsaRecvbuf[1].buf = pSession->_recvBuf.GetBufferFrontPtr();
	pSession->_wsaRecvbuf[1].len = freeSize - pSession->_wsaRecvbuf[0].len;

	ZeroMemory(&pSession->_recvOvl._ovl, sizeof(pSession->_recvOvl._ovl));
	InterlockedIncrement(&pSession->_IOCount);
	
	int recvRet = WSARecv(pSession->_sock, pSession->_wsaRecvbuf,
		2, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvOvl, NULL);

	//::printf("%llu: Request Recv (thread: %d)\n", pSession->_ID, threadID);

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

	LeaveCriticalSection(&pSession->_cs);
}

void NetLib::SendPost(__int64 sessionID, int threadID)
{
	AcquireSRWLockShared(&g_SessionMapLock);

	unordered_map<int, Session*>::iterator iter = g_SessionMap.find(sessionID);
	if (iter == g_SessionMap.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		ReleaseSRWLockShared(&g_SessionMapLock);
		return;
	}

	Session* pSession = iter->second;
	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&g_SessionMapLock);

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

	//::printf("%d: Request Send %d bytes\n", threadID, useSize);

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

	LeaveCriticalSection(&pSession->_cs);
}

void NetLib::RecvDataToMsg(__int64 sessionID, int threadID)
{
	AcquireSRWLockShared(&g_SessionMapLock);

	unordered_map<int, Session*>::iterator iter = g_SessionMap.find(sessionID);
	if (iter == g_SessionMap.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		ReleaseSRWLockShared(&g_SessionMapLock);
		return;
	}

	Session* pSession = iter->second;
	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&g_SessionMapLock);

	int useSize = pSession->_recvBuf.GetUseSize();

	while (useSize > 0)
	{
		if (useSize <= dfHEADER_LEN)
			break;

		stHeader header;
		int peekRet = pSession->_recvBuf.Peek((char*)&header, dfHEADER_LEN);
		if (peekRet != dfHEADER_LEN)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		if (useSize < dfHEADER_LEN + header._shLen)
			break;

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_LEN);
		if (moveReadRet != dfHEADER_LEN)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		SerializePacket packet;
		int dequeueRet = pSession->_recvBuf.Dequeue(packet.GetPayloadWritePtr(), header._shLen);
		if (dequeueRet != header._shLen)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		packet.MovePayloadWritePos(dequeueRet);
		useSize = pSession->_recvBuf.GetUseSize();
		LeaveCriticalSection(&pSession->_cs);
		Server::GetInstance()->onMessage(pSession->_ID, &packet, threadID);
		EnterCriticalSection(&pSession->_cs);
	}

	LeaveCriticalSection(&pSession->_cs);
}

void NetLib::MsgToSendData(__int64 sessionID, SerializePacket* packet, int threadID)
{
	AcquireSRWLockShared(&g_SessionMapLock);

	unordered_map<int, Session*>::iterator iter = g_SessionMap.find(sessionID);
	if (iter == g_SessionMap.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		ReleaseSRWLockShared(&g_SessionMapLock);
		return;
	}
	
	Session* pSession = iter->second;
	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&g_SessionMapLock);

	// This func is called,
	// So should not lock the session

	int payloadSize = packet->GetPayloadSize();
	stHeader header;
	header._shLen = payloadSize;
	int putRet = packet->PutHeaderData((char*)&header, sizeof(header));
	if (putRet != sizeof(header))
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&pSession->_cs);
		return;
	}

	int packetSize = packet->GetPacketSize();
	int enqueueRet = pSession->_sendBuf.Enqueue(
		packet->GetPacketReadPtr(), packetSize);

	if (enqueueRet != packetSize)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&pSession->_cs);
		return;
	}

	if (InterlockedIncrement(&pSession->_sendFlag) == 1)
	{
		//::printf("%d: Call SendPost in %s (sendFlag: %d)\n",
		//	threadID, __func__, pSession->_sendFlag);

		LeaveCriticalSection(&pSession->_cs);
		SendPost(pSession->_ID, threadID);
	}
	else
	{
		//::printf("%d: Wait For SendPost (sendFlag: %d)\n", threadID, pSession->_sendFlag);
		LeaveCriticalSection(&pSession->_cs);
	}

}
