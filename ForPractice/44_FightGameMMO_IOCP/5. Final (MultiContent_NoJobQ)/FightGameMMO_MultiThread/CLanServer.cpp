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

	// Initialize Winsock
	WSADATA wsa;
	int startRet = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (startRet != 0)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: WSAStartup Error\n",
			_T(__FUNCTION__), __LINE__);

		::wprintf(L"%s[%d]: WSAStartup Error\n",
			_T(__FUNCTION__), __LINE__);

		__debugbreak();
		return false;
	}

	// Socket Bind, Listen
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		int err = WSAGetLastError();

		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: listen sock is INVALIED, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		::wprintf(L"%s[%d]: listen sock is INVALIED, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		__debugbreak();
		return false;
	}

	LINGER optval;
	optval.l_onoff = 1;
	optval.l_linger = 0;
	int optRet = setsockopt(_listenSock, SOL_SOCKET, SO_LINGER,
		(char*)&optval, sizeof(optval));
	if (optRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();

		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: bind Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		::wprintf(L"%s[%d]: bind Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);
		
		__debugbreak();
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
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: bind Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		::wprintf(L"%s[%d]: bind Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		__debugbreak();
		return false;
	}

	int listenRet = listen(_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: listen Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		::wprintf(L"%s[%d]: listen Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		__debugbreak();
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
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Begin Accept Thread Error\n",
			_T(__FUNCTION__), __LINE__ );

		::wprintf(L"%s[%d]: Begin Accept Thread Error\n",
			_T(__FUNCTION__), __LINE__);

		__debugbreak();
		return false;
	}

	// Create IOCP for Network
	_hNetworkCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hNetworkCP == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Create IOCP Error\n",
			_T(__FUNCTION__), __LINE__);

		::wprintf(L"%s[%d]: Create IOCP Error\n",
			_T(__FUNCTION__), __LINE__);

		__debugbreak();
		return false;
	}

	// Create Network Thread
	_networkThreads = new HANDLE[_numOfWorkerThreads];
	for (int i = 0; i < _numOfWorkerThreads; i++)
	{
		_networkThreads[i] = (HANDLE)_beginthreadex(NULL, 0, NetworkThread, this, 0, nullptr);
		if (_networkThreads[i] == NULL)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: Begin Network Thread Error\n",
				_T(__FUNCTION__), __LINE__);

			::wprintf(L"%s[%d]: Begin Network Thread Error\n",
				_T(__FUNCTION__), __LINE__);

			__debugbreak();
			return false;
		}
	}

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Setting Complete\n");
	::wprintf(L"Network Setting Complete\n");
	return true;
}

void CLanServer::NetworkTerminate()
{
	if (!_alive)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Invalid Request. It's already stopped\n",
			_T(__FUNCTION__), __LINE__);

		::wprintf(L"%s[%d]: Invalid Request. It's already stopped\n",
			_T(__FUNCTION__), __LINE__);

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
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: socket Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		::wprintf(L"%s[%d]: socket Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		__debugbreak();
		return;
	}

	int connectRet = connect(socktmp, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (connectRet == SOCKET_ERROR)
	{
		int err = ::WSAGetLastError();
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: connect Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		::wprintf(L"%s[%d]: connect Error, %d\n",
			_T(__FUNCTION__), __LINE__, err);

		__debugbreak();
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
		//::wprintf("Disconnect Client(ID: % llu)\n", ID);
		iter = _sessionMap->_map.erase(iter);
	}

	WSACleanup();
	CloseHandle(_hNetworkCP);
	CloseHandle(_acceptThread);
	for (int i = 0; i < _numOfWorkerThreads; i++)
		CloseHandle(_networkThreads[i]);
	delete[] _networkThreads;

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Terminate.\n");
	::wprintf(L"Network Terminate.\n");
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

	// pSession->PushStateForDebug(__LINE__); 

	if(pSession->_socketAlive)
	{
		_serverDisconnCnt++;
		pSession->_socketAlive = false;
		closesocket(pSession->_sock);
	}

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

	if (!pSession->_socketAlive)
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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: Packet Return Error\n",
				_T(__FUNCTION__), __LINE__);

			::wprintf(L"%s[%d]: Packet Return Error\n",
				_T(__FUNCTION__), __LINE__);

			__debugbreak();
			LeaveCriticalSection(&pSession->_cs);
			return false;
		}
	}

	int packetSize = packet->GetPacketSize();
	int enqueueRet = pSession->_sendBuf.Enqueue(packet->GetPacketReadPtr(), packetSize);

	if (enqueueRet != packetSize)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: Packet Return Error\n",
			_T(__FUNCTION__), __LINE__);

		::wprintf(L"%s[%d]: Packet Return Error\n",
			_T(__FUNCTION__), __LINE__);

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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: accept Error\n",
				_T(__FUNCTION__), __LINE__);

			::wprintf(L"%s[%d]: accept Error\n",
				_T(__FUNCTION__), __LINE__);

			__debugbreak();
			break;
		}

		if (g_bShutdown) break;
		if (!pLanServer->_alive) break;
		// if (!pLanServer->OnConnectRequest()) continue;

		if (pLanServer->GetSessionCount() >= pLanServer->_sessionMax)
		{     
			closesocket(client_sock);
			continue;
		}

		PRO_BEGIN(L"LS: AcceptThread");

		CSession* pSession = new CSession(
			pLanServer->_IDGenerator++, client_sock, clientaddr);

		if (pSession == nullptr)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: new Session Error\n",
				_T(__FUNCTION__), __LINE__);

			::wprintf(L"%s[%d]: new Session Error\n",
				_T(__FUNCTION__), __LINE__);

			__debugbreak();
			break;
		}

		AcquireSRWLockExclusive(&pLanServer->_sessionMap->_lock);
		pLanServer->_sessionMap->_map.insert(make_pair(pSession->_ID, pSession));
		
		EnterCriticalSection(&pSession->_cs);
		ReleaseSRWLockExclusive(&pLanServer->_sessionMap->_lock);
		
		//::printf("Accept New Session (ID: %llu)\n", pSession->_ID);
		pLanServer->_acceptCnt++;

		// Connect Session to IOCP and Post Recv                                      
		CreateIoCompletionPort((HANDLE)pSession->_sock,
			pLanServer->_hNetworkCP, (ULONG_PTR)pSession->_ID, 0);

		PRO_END(L"LS: AcceptThread");

		pLanServer->RecvPost(pSession);
		LeaveCriticalSection(&pSession->_cs);

		pLanServer->OnAcceptClient(pSession->_ID);		
	}


	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, 
		L"Accept Thread Terminate (thread: %d)\n", GetCurrentThreadId());
	::wprintf(L"Accept Thread Terminate (thread: %d)\n", GetCurrentThreadId());

	return 0;
}

unsigned int __stdcall CLanServer::NetworkThread(void* arg)
{
	PRO_SET(pSTLSProfiler, GetCurrentThreadId());

	CLanServer* pLanServer = (CLanServer*)arg;
	int threadID = GetCurrentThreadId();
	NetworkOverlapped* pNetOvl = new NetworkOverlapped;

	for (;;)
	{
		__int64 sessionID;
		DWORD cbTransferred;

		int GQCSRet = GetQueuedCompletionStatus(pLanServer->_hNetworkCP,
			&cbTransferred, (PULONG_PTR)&sessionID, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (g_bShutdown) break;
		if (!pLanServer->_alive) break;
		if (pNetOvl->_type == NET_TYPE::RELEASE)
		{
			pLanServer->ReleaseSession(pLanServer, sessionID);
			continue;
		}

		PRO_BEGIN(L"LS: NetworkThread");

		// Check Exception		
		if (GQCSRet == 0 || cbTransferred == 0)
		{
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
				if (err != WSAECONNRESET && err != WSAECONNABORTED && err != WSAENOTSOCK)
				{
					LOG(L"FightGame", CSystemLog::DEBUG_LEVEL,
						L"%s[%d]: Connecton End, %d\n",
						_T(__FUNCTION__), __LINE__, err);

					::wprintf(L"%s[%d]: Connecton End, %d\n",
						_T(__FUNCTION__), __LINE__, err);
				}
			}
			LeaveCriticalSection(&pSession->_cs);
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
			PostQueuedCompletionStatus(pLanServer->_hNetworkCP, 1,
				(ULONG_PTR)pSession->_ID, (LPOVERLAPPED)&pSession->_releaseOvl);
		}

		// pSession->PushStateForDebug(__LINE__);

		LeaveCriticalSection(&pSession->_cs);
		PRO_END(L"LS: NetworkThread");
	}

	delete pNetOvl;
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL,
		L"Worker Thread Terminate (thread: %d)\n", GetCurrentThreadId());
	::wprintf(L"Worker Thread Terminate (thread: %d)\n", GetCurrentThreadId());

	return 0;
}

void CLanServer::ReleaseSession(CLanServer* pLanServer, __int64 sessionID)
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

	// pSession->PushStateForDebug(__LINE__); 
	if(pSession->_socketAlive) 
	{
		pLanServer->_clientDisconnCnt++;
		pSession->_socketAlive = false;
		closesocket(pSession->_sock);
	}
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
	_clientDisconnTPS = _clientDisconnCnt;
	_serverDisconnTPS = _serverDisconnCnt;
	_acceptTotal += _acceptTPS;
	_disconnectTotal += _disconnectTPS;

	_acceptCnt = 0;
	_disconnectCnt = 0;
	_recvMsgCnt = 0;
	_sendMsgCnt = 0;
	_clientDisconnCnt = 0;
	_serverDisconnCnt = 0;
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

	if (!pSession->_socketAlive)
	{
		LeaveCriticalSection(&pSession->_cs);
		return;
	}

	int moveReadRet = pSession->_recvBuf.MoveWritePos(recvBytes);
	if (moveReadRet != recvBytes)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: recv buffer Return Error\n",
			_T(__FUNCTION__), __LINE__);

		::wprintf(L"%s[%d]: recv buffer Return Error\n",
			_T(__FUNCTION__), __LINE__);

		__debugbreak();
		LeaveCriticalSection(&pSession->_cs);
		return;
	}

	// pSession->PushStateForDebug(__LINE__); 
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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: recv buffer Return Error\n",
				_T(__FUNCTION__), __LINE__);

			::wprintf(L"%s[%d]: recv buffer Return Error\n",
				_T(__FUNCTION__), __LINE__);

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
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: recv buffer Return Error\n",
				_T(__FUNCTION__), __LINE__);

			::wprintf(L"%s[%d]: recv buffer Return Error\n",
				_T(__FUNCTION__), __LINE__);

			__debugbreak();
			LeaveCriticalSection(&pSession->_cs);
			break;
		}

		CPacket* packet = _pPacketPool->Alloc();
		packet->Clear();
		int dequeueRet = pSession->_recvBuf.Dequeue(packet->GetPayloadWritePtr(), header.Len);
		if (dequeueRet != header.Len)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
				L"%s[%d]: recv buffer Return Error\n",
				_T(__FUNCTION__), __LINE__);

			::wprintf(L"%s[%d]: recv buffer Return Error\n",
				_T(__FUNCTION__), __LINE__);

			__debugbreak();
			LeaveCriticalSection(&pSession->_cs);
			break;
		}

		packet->MovePayloadWritePos(dequeueRet);
		LeaveCriticalSection(&pSession->_cs);

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

		if (!pSession->_socketAlive)
		{
			LeaveCriticalSection(&pSession->_cs);
			break;
		}
		useSize = pSession->_recvBuf.GetUseSize();
	} 

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

	if (!pSession->_socketAlive)
	{
		LeaveCriticalSection(&pSession->_cs);
		return;
	}

	// pSession->PushStateForDebug(__LINE__); 
	int moveReadRet = pSession->_sendBuf.MoveReadPos(sendBytes);
	if (moveReadRet != sendBytes)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
			L"%s[%d]: send buffer Return Error\n",
			_T(__FUNCTION__), __LINE__);

		::wprintf(L"%s[%d]: send buffer Return Error\n",
			_T(__FUNCTION__), __LINE__);

		__debugbreak();
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
	pSession->_wsaRecvbuf[0].buf = pSession->_recvBuf.GetWritePtr();
	pSession->_wsaRecvbuf[0].len = pSession->_recvBuf.DirectEnqueueSize();
	pSession->_wsaRecvbuf[1].buf = pSession->_recvBuf.GetFrontPtr();
	pSession->_wsaRecvbuf[1].len = freeSize - pSession->_wsaRecvbuf[0].len;

	ZeroMemory(&pSession->_recvOvl._ovl, sizeof(pSession->_recvOvl._ovl));
	
	InterlockedIncrement(&pSession->_IOCount);
	// pSession->PushStateForDebug(__LINE__); 

	int recvRet = WSARecv(pSession->_sock, pSession->_wsaRecvbuf,
		2, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvOvl, NULL);

	//::printf("%d: %llu Request Recv\n", GetCurrentThreadId(), pSession->_ID);

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED)
			{
				LOG(L"FightGame", CSystemLog::DEBUG_LEVEL,
					L"%s[%d]: Connection End, %d\n",
					_T(__FUNCTION__), __LINE__, err);

				::wprintf(L"%s[%d]: Connection End, %d\n",
					_T(__FUNCTION__), __LINE__, err);
			}

			if (InterlockedDecrement(&pSession->_IOCount) == 0)
			{
				PostQueuedCompletionStatus(_hNetworkCP, 1,
					(ULONG_PTR)pSession->_ID, (LPOVERLAPPED)&pSession->_releaseOvl);
			}

			return;
		}
	}
}

void CLanServer::SendPost(CSession* pSession)
{
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
	// pSession->PushStateForDebug(__LINE__); 

	int sendRet = WSASend(pSession->_sock, pSession->_wsaSendbuf,
		2, &sendBytes, 0, (LPOVERLAPPED)&pSession->_sendOvl, NULL);
	
	//::printf("%d: %llu Request Send %d bytes\n", GetCurrentThreadId(), pSession->_ID , useSize);

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED)
			{
				LOG(L"FightGame", CSystemLog::DEBUG_LEVEL,
					L"%s[%d]: Connection End, %d\n",
					_T(__FUNCTION__), __LINE__, err);

				::wprintf(L"%s[%d]: Connection End, %d\n",
					_T(__FUNCTION__), __LINE__, err);
			}

			if (InterlockedDecrement(&pSession->_IOCount) == 0)
			{
				PostQueuedCompletionStatus(_hNetworkCP, 1,
					(ULONG_PTR)pSession->_ID, (LPOVERLAPPED)&pSession->_releaseOvl);
			}
		}
	}
}

