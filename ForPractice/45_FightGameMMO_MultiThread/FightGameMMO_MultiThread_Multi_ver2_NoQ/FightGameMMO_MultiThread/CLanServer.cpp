#include "CLanServer.h"
#include "Main.h"
#include "CSystemLog.h"

#include <stdlib.h>
#include <stdio.h>

#ifdef USE_STLS
__declspec (thread) CProfiler* pSTLSProfiler = new CProfiler;
#endif

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

	//_pSessionPool = new CObjectPool<CSession>(dfSESSION_DEFAULT, true);
	_pPacketPool = new CObjectPool<CPacket>(dfPACKET_DEFAULT, false);
	_activeNetworkThreads = new bool[_numOfWorkerThreads];

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
	_sessionMap = CSessionMap::GetInstance();

	// Create Accept Thread
	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, pLanServer, 0, nullptr);
	if (_acceptThread == NULL)
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
		ThreadArg* arg = new ThreadArg(this, i);
		_networkThreads[i] = (HANDLE)_beginthreadex(
			NULL, 0, NetworkThread, arg, 0, nullptr);

		if (_networkThreads[i] == NULL)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			return false;
		}
	}

	::printf("Network Setting Complete\n");
	return true;
}

void CLanServer::NetworkTerminate()
{
	if (!_alive)
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

	// Release All Session
	closesocket(socktmp);
	closesocket(_listenSock);
	unordered_map<__int64, CSession*>::iterator iter = _sessionMap->_map.begin();
	for (; iter != _sessionMap->_map.end();)
	{
		CSession* pSession = iter->second;
		closesocket(pSession->_sock);
		__int64 ID = pSession->_ID;
		delete pSession;
		//_pSessionPool->Free(pSession);
		//::printf("Disconnect Client(ID: % llu)\n", ID);
		iter = _sessionMap->_map.erase(iter);
	}

	WSACleanup();
	CloseHandle(_hNetworkCP);
	CloseHandle(_acceptThread);
	for (int i = 0; i < _numOfWorkerThreads; i++)
		CloseHandle(_networkThreads[i]);
	delete[] _networkThreads;

	::printf("\nNetwork Stop\n");
}

bool CLanServer::Disconnect(__int64 sessionID)
{
	AcquireSRWLockShared(&_sessionMap->_lock);
	unordered_map<__int64, CSession*>::iterator iter = _sessionMap->_map.find(sessionID);
	if (iter == _sessionMap->_map.end())
	{
		ReleaseSRWLockShared(&_sessionMap->_lock);
		return false;
	}
	CSession* pSession = iter->second;

	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&_sessionMap->_lock);

	//pSession->PushStateForDebug(__LINE__);

	pSession->_alive = false;
	PostQueuedCompletionStatus(_hNetworkCP, 1,
		(ULONG_PTR)pSession->_ID, (LPOVERLAPPED)&pSession->_releaseOvl);

	LeaveCriticalSection(&pSession->_cs);

	return true;
}

bool CLanServer::SendPacket(__int64 sessionID, CPacket* packet)
{	
	AcquireSRWLockShared(&_sessionMap->_lock);
	unordered_map<__int64, CSession*>::iterator iter = _sessionMap->_map.find(sessionID);
	if (iter == _sessionMap->_map.end())
	{
		ReleaseSRWLockShared(&_sessionMap->_lock);
		return false;
	}
	CSession* pSession = iter->second;

	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&_sessionMap->_lock);

	if (!pSession->_alive)
	{
		LeaveCriticalSection(&pSession->_cs);
		return false;
	}

	if (packet->IsHeaderEmpty())
	{
		int payloadSize = packet->GetPayloadSize();
		st_PACKET_HEADER header;
		header.Len = payloadSize;

		int putRet = packet->PutHeaderData((char*)&header, dfHEADER_LEN);
		if (putRet != dfHEADER_LEN)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			__debugbreak();
			LeaveCriticalSection(&pSession->_cs);
			return false;
		}
	}

	int packetSize = packet->GetPacketSize();
	int enqueueRet = pSession->_sendBuf.Enqueue(packet->GetPacketReadPtr(), packetSize);

	if (enqueueRet != packetSize)
	{
		/*
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		pSession->PushStateForDebug(__LINE__);
		pSession->PrintStateForDebug();
		ForDebug(pSession->_ID);
		__debugbreak();
		*/

		//::printf("%lld: buffer is full\n", pSession->_ID);
		//pSession->_sendBuf.PrintBufferDataForDebug();

		_NoSendIOCP++;
		pSession->_alive = false;
		PostQueuedCompletionStatus(_hNetworkCP, 1,
			(ULONG_PTR)pSession->_ID, (LPOVERLAPPED)&pSession->_releaseOvl);
		LeaveCriticalSection(&pSession->_cs);

		return false;
	}

	SendPost(pSession);
	LeaveCriticalSection(&pSession->_cs);

	return true;
}

unsigned int __stdcall CLanServer::AcceptThread(void* arg)
{
	PRO_SET(pSTLSProfiler, GetCurrentThreadId());
	CLanServer* pLanServer = (CLanServer*)arg;

	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);

	for (;;)
	{
		// Accept
		SOCKET client_sock = accept(pLanServer->_listenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			__debugbreak();
			break;
		}

		if (g_bShutdown) break;
		if (!pLanServer->_alive) break;
		if (!pLanServer->OnConnectRequest()) continue;

		if (pLanServer->GetSessionCount() >= pLanServer->_sessionMax)
		{     
			closesocket(client_sock);
			continue;
		}

		PRO_BEGIN(L"LS: AcceptThread");

		CSession* pSession = new CSession(pLanServer->_IDGenerator++, client_sock, clientaddr);
		//pSession->PushStateForDebug(__LINE__);

		//CSession* pSession = pLanServer->_pSessionPool->Alloc(
		//	pLanServer->_IDGenerator++, client_sock, clientaddr);
		//pSession->Initialize(sessionID++, client_sock, clientaddr);
		if (pSession == nullptr)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			__debugbreak();
			break;
		}

		AcquireSRWLockExclusive(&pLanServer->_sessionMap->_lock);
		pLanServer->_sessionMap->_map.insert(make_pair(pSession->_ID, pSession));
		ReleaseSRWLockExclusive(&pLanServer->_sessionMap->_lock);
		
		//::printf("Accept New Session (ID: %llu)\n", pSession->_ID);
		pLanServer->_acceptCnt++;

		// Connect Session to IOCP and Post Recv                                      
		CreateIoCompletionPort((HANDLE)pSession->_sock,
			pLanServer->_hNetworkCP, (ULONG_PTR)pSession->_ID, 0);

		PRO_END(L"LS: AcceptThread");

		EnterCriticalSection(&pSession->_cs);
		pLanServer->RecvPost(pSession);
		LeaveCriticalSection(&pSession->_cs);

		pLanServer->OnAcceptClient(pSession->_ID);		
	}

	::printf("Accept Thread Terminate (thread: %d)\n", GetCurrentThreadId());
	return 0;
}

unsigned int __stdcall CLanServer::NetworkThread(void* arg)
{
	PRO_SET(pSTLSProfiler, GetCurrentThreadId());

	ThreadArg* threadArg = (ThreadArg*)arg;
	CLanServer* pLanServer = threadArg->_pServer;
	int num = threadArg->_num;

	int threadID = GetCurrentThreadId();

	for (;;)
	{
		__int64 sessionID;
		DWORD cbTransferred;
		NetworkOverlapped* pNetOvl = new NetworkOverlapped;
		int GQCSRet = GetQueuedCompletionStatus(pLanServer->_hNetworkCP,
			&cbTransferred, (PULONG_PTR)&sessionID, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (pLanServer->_activeNetworkThreads[num] == false)
			pLanServer->_activeNetworkThreads[num] = true;

		if (g_bShutdown) break;
		if (!pLanServer->_alive) break;
		if (pNetOvl->_type == NET_TYPE::RELEASE)
		{
			pLanServer->ReleaseSession(sessionID);
			continue;
		}

		PRO_BEGIN(L"LS: NetworkThread");

		// Check Exception		
		if (GQCSRet == 0 || cbTransferred == 0)
		{
			//pSession->PushStateForDebug(__LINE__);
			AcquireSRWLockShared(&pLanServer->_sessionMap->_lock);
			unordered_map<__int64, CSession*>::iterator iter = pLanServer->_sessionMap->_map.find(sessionID);
			if (iter == pLanServer->_sessionMap->_map.end())
			{
				ReleaseSRWLockShared(&pLanServer->_sessionMap->_lock);
				continue;
			}
			CSession* pSession = iter->second;

			EnterCriticalSection(&pSession->_cs);
			ReleaseSRWLockShared(&pLanServer->_sessionMap->_lock);

			if (GQCSRet == 0)
			{
				DWORD arg1, arg2;
				WSAGetOverlappedResult(pSession->_sock, (LPOVERLAPPED)pNetOvl, &arg1, FALSE, &arg2);
				int err = WSAGetLastError();
				if (err == WSAECONNRESET)
				{
					pLanServer->_10054Cnt++;
				}
				else
				{
					::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
				}
			}

			pSession->_alive = false;
			PostQueuedCompletionStatus(pLanServer->_hNetworkCP, 1,
				(ULONG_PTR)pSession->_ID, (LPOVERLAPPED)&pSession->_releaseOvl);
			LeaveCriticalSection(&pSession->_cs);

			continue;

		}
		else if (pNetOvl->_type == NET_TYPE::RECV)
		{
			//::printf("%d: %llu Complete Recv %d bytes\n", GetCurrentThreadId(), pSession->_ID, cbTransferred);
			pLanServer->HandleRecvCP(sessionID, cbTransferred);
			pLanServer->_recvMsgCnt++;
			
		}
		else if (pNetOvl->_type == NET_TYPE::SEND)
		{
			//::printf("%d: %llu Complete Send %d bytes\n", GetCurrentThreadId(), pSession->_ID, cbTransferred);
			pLanServer->HandleSendCP(sessionID, cbTransferred);
			pLanServer->_sendMsgCnt++;
		}

		//pSession->PushStateForDebug(__LINE__);
		AcquireSRWLockShared(&pLanServer->_sessionMap->_lock);
		unordered_map<__int64, CSession*>::iterator iter = pLanServer->_sessionMap->_map.find(sessionID);
		if (iter == pLanServer->_sessionMap->_map.end())
		{
			ReleaseSRWLockShared(&pLanServer->_sessionMap->_lock);
			continue;
		}
		CSession* pSession = iter->second;

		EnterCriticalSection(&pSession->_cs);
		ReleaseSRWLockShared(&pLanServer->_sessionMap->_lock);

		if (InterlockedDecrement(&pSession->_IOCount) == 0)
		{
			//pSession->PushStateForDebug(__LINE__);
			PostQueuedCompletionStatus(pLanServer->_hNetworkCP, 1,
				(ULONG_PTR)pSession->_ID, (LPOVERLAPPED)&pSession->_releaseOvl);
		}

		LeaveCriticalSection(&pSession->_cs);

		PRO_END(L"LS: NetworkThread");
	}

	::printf("Worker Thread Terminate (thread: %d)\n", threadID);
	return 0;
}

void CLanServer::ReleaseSession(__int64 sessionID)
{
	AcquireSRWLockExclusive(&_sessionMap->_lock);
	unordered_map<__int64, CSession*>::iterator iter = _sessionMap->_map.find(sessionID);
	if (iter == _sessionMap->_map.end())
	{
		ReleaseSRWLockExclusive(&_sessionMap->_lock);
		return;
	}
	CSession* pSession = iter->second;
	_sessionMap->_map.erase(iter);

	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockExclusive(&_sessionMap->_lock);
	LeaveCriticalSection(&pSession->_cs);

	//pSession->PushStateForDebug(__LINE__);
	closesocket(pSession->_sock);
	delete pSession;

	//::printf("%llu: Disconnect Client\n", ID);
	_disconnectCnt++;
	OnReleaseClient(sessionID);
}

void CLanServer::UpdateMonitorData()
{
	_acceptTPS = _acceptCnt;
	_disconnectTPS = _disconnectCnt;
	_recvMsgTPS = _recvMsgCnt;
	_sendMsgTPS = _sendMsgCnt;

	_10054TPS = _10054Cnt;
	_acceptTotal += _acceptTPS;
	_disconnectTotal += _disconnectTPS;

	_activeThreadNum = 0;
	for (int i = 0; i < _numOfWorkerThreads; i++)
	{
		if (_activeNetworkThreads[i] == true)
		{
			_activeThreadNum++;
			_activeNetworkThreads[i] = false;
		}
	}
	_acceptCnt = 0;
	_disconnectCnt = 0;
	_recvMsgCnt = 0;
	_sendMsgCnt = 0;
	_10054Cnt = 0;
}

void CLanServer::HandleRecvCP(__int64 sessionID, int recvBytes)
{	
	AcquireSRWLockShared(&_sessionMap->_lock);
	unordered_map<__int64, CSession*>::iterator iter;
	iter = _sessionMap->_map.find(sessionID);
	if (iter == _sessionMap->_map.end())
	{
		ReleaseSRWLockShared(&_sessionMap->_lock);
		return;
	}
	CSession* pSession = iter->second;

	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&_sessionMap->_lock);

	if (!pSession->_alive)
	{
		LeaveCriticalSection(&pSession->_cs);
		return;
	}

	int moveReadRet = pSession->_recvBuf.MoveWritePos(recvBytes);
	if (moveReadRet != recvBytes)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		__debugbreak();
		LeaveCriticalSection(&pSession->_cs);
		return;
	}

	pSession->PushStateForDebug(__LINE__); 
	int useSize = pSession->_recvBuf.GetUseSize();

	for(;;)
	{
		if (useSize <= dfHEADER_LEN) 
		{
			LeaveCriticalSection(&pSession->_cs);
			break;
		}

		st_PACKET_HEADER header;
		int peekRet = pSession->_recvBuf.Peek((char*)&header, dfHEADER_LEN);
		if (peekRet != dfHEADER_LEN)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			__debugbreak();
			LeaveCriticalSection(&pSession->_cs);
			break;
		}

		if (useSize < dfHEADER_LEN + header.Len)
		{
			LeaveCriticalSection(&pSession->_cs);
			break;
		}

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_LEN);
		if (moveReadRet != dfHEADER_LEN)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			__debugbreak();
			LeaveCriticalSection(&pSession->_cs);
			break;
		}

		CPacket* packet = _pPacketPool->Alloc();
		packet->Clear();
		int dequeueRet = pSession->_recvBuf.Dequeue(packet->GetPayloadWritePtr(), header.Len);
		if (dequeueRet != header.Len)
		{
			::printf("Error! Func %s Line %d\n", __func__, __LINE__);
			__debugbreak();
			LeaveCriticalSection(&pSession->_cs);
			break;
		}

		packet->MovePayloadWritePos(dequeueRet);
		LeaveCriticalSection(&pSession->_cs);

		pSession->PushStateForDebug(__LINE__);
		OnRecv(pSession->_ID, packet); 
		
		AcquireSRWLockShared(&_sessionMap->_lock);
		iter = _sessionMap->_map.find(sessionID);
		if (iter == _sessionMap->_map.end())
		{
			ReleaseSRWLockShared(&_sessionMap->_lock);
			break;
		}
		pSession = iter->second;

		EnterCriticalSection(&pSession->_cs);
		ReleaseSRWLockShared(&_sessionMap->_lock);

		if (!pSession->_alive)
		{
			LeaveCriticalSection(&pSession->_cs);
			break;
		}
		useSize = pSession->_recvBuf.GetUseSize();
	}

	pSession->PushStateForDebug(__LINE__);

	AcquireSRWLockShared(&_sessionMap->_lock);
	iter = _sessionMap->_map.find(sessionID);
	if (iter == _sessionMap->_map.end())
	{
		ReleaseSRWLockShared(&_sessionMap->_lock);
		return;
	}
	pSession = iter->second;

	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&_sessionMap->_lock);

	RecvPost(pSession);
	
	LeaveCriticalSection(&pSession->_cs);
}

void CLanServer::HandleSendCP(__int64 sessionID, int sendBytes)
{
	AcquireSRWLockShared(&_sessionMap->_lock);
	unordered_map<__int64, CSession*>::iterator iter = _sessionMap->_map.find(sessionID);
	if (iter == _sessionMap->_map.end())
	{
		ReleaseSRWLockShared(&_sessionMap->_lock);
		return;
	}
	CSession* pSession = iter->second;

	EnterCriticalSection(&pSession->_cs);
	ReleaseSRWLockShared(&_sessionMap->_lock);

	if (!pSession->_alive)
	{
		LeaveCriticalSection(&pSession->_cs);
		return;
	}

	pSession->PushStateForDebug(__LINE__);

	int moveReadRet = pSession->_sendBuf.MoveReadPos(sendBytes);
	if (moveReadRet != sendBytes)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);		
		LeaveCriticalSection(&pSession->_cs);
		__debugbreak();
		return;
	}

	OnSend(pSession->_ID, sendBytes);
	InterlockedExchange(&pSession->_sendFlag, 0);
	SendPost(pSession);
	LeaveCriticalSection(&pSession->_cs);
}

void CLanServer::RecvPost(CSession* pSession)
{
	if (!pSession->_alive) return;

	DWORD flags = 0;
	DWORD recvBytes = 0;

	int freeSize = pSession->_recvBuf.GetFreeSize();
	pSession->_wsaRecvbuf[0].buf = pSession->_recvBuf.GetWritePtr();
	pSession->_wsaRecvbuf[0].len = pSession->_recvBuf.DirectEnqueueSize();
	pSession->_wsaRecvbuf[1].buf = pSession->_recvBuf.GetFrontPtr();
	pSession->_wsaRecvbuf[1].len = freeSize - pSession->_wsaRecvbuf[0].len;

	ZeroMemory(&pSession->_recvOvl._ovl, sizeof(pSession->_recvOvl._ovl));
	InterlockedIncrement(&pSession->_IOCount);
	//pSession->PushStateForDebug(__LINE__);

	int recvRet = WSARecv(pSession->_sock, pSession->_wsaRecvbuf,
		2, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvOvl, NULL);

	//::printf("%d: %llu Request Recv\n", GetCurrentThreadId(), pSession->_ID);

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET)
			{
				::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
			}
			return;
		}
	}
}

void CLanServer::SendPost(CSession* pSession)
{
	if (!pSession->_alive) return;
	if (pSession->_sendBuf.GetUseSize() == 0)  return;
	if (InterlockedExchange(&pSession->_sendFlag, 1) == 1) return;
	if (pSession->_sendBuf.GetUseSize() == 0)
	{
		InterlockedExchange(&pSession->_sendFlag, 0);
		return;
	}

	DWORD sendBytes;
	int useSize = pSession->_sendBuf.GetUseSize();
	pSession->_wsaSendbuf[0].buf = pSession->_sendBuf.GetReadPtr();
	pSession->_wsaSendbuf[0].len = pSession->_sendBuf.DirectDequeueSize();
	pSession->_wsaSendbuf[1].buf = pSession->_sendBuf.GetFrontPtr();
	pSession->_wsaSendbuf[1].len = useSize - pSession->_wsaSendbuf[0].len;

	ZeroMemory(&pSession->_sendOvl._ovl, sizeof(pSession->_sendOvl._ovl));
	InterlockedIncrement(&pSession->_IOCount);
	pSession->PushStateForDebug(__LINE__);

	int sendRet = WSASend(pSession->_sock, pSession->_wsaSendbuf,
		2, &sendBytes, 0, (LPOVERLAPPED)&pSession->_sendOvl, NULL);
	
	//::printf("%d: %llu Request Send %d bytes\n", GetCurrentThreadId(), pSession->_ID , useSize);

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET)
			{
				::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
			}
			return;
		}
	}
}

