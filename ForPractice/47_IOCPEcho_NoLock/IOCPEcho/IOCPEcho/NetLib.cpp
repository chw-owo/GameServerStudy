#include "NetLib.h"
#include "Main.h"
#include <stdlib.h>
#include <stdio.h>

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

	int optRet;
	LINGER optval;
	optval.l_onoff = 1;
	optval.l_linger = 0;
	optRet = setsockopt(_listenSock, SOL_SOCKET, SO_LINGER,
		(char*)&optval, sizeof(optval));
	if (optRet == SOCKET_ERROR)
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}

	int sndBufSize = 0;
	optRet = setsockopt(_listenSock, SOL_SOCKET, SO_SNDBUF,
		(char*)&sndBufSize, sizeof(sndBufSize));
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

	_sessionMap = SessionMap::GetInstance();

	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, nullptr);
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
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}

	// Create networkThread
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	_networkThreadCnt = (int)si.dwNumberOfProcessors / 2;
	_networkThreads = new HANDLE[_networkThreadCnt];

	for (int i = 0; i < _networkThreadCnt; i++)
	{
		_networkThreads[i] = (HANDLE)_beginthreadex(NULL, 0, NetworkThread, this, 0, nullptr);

		if (_networkThreads[i] == NULL)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			return;
		}
	}

	::printf("Complete Setting\n");
}

NetLib::~NetLib()
{
	unordered_map<__int64, Session*>::iterator iter = _sessionMap->_map.begin();
	for (; iter != _sessionMap->_map.end();)
	{
		Session* pSession = iter->second;
		iter = _sessionMap->_map.erase(iter);
		closesocket(pSession->_sock);
		delete(pSession);
	}
	
	closesocket(_listenSock);
	WSACleanup();

	for(int i = 0; i < _networkThreadCnt; i++)
	{
		PostQueuedCompletionStatus(_hNetworkCP, 0, 0, 0);
	}

	WaitForMultipleObjects(_networkThreadCnt, _networkThreads, true, INFINITE);
	WaitForSingleObject(_acceptThread, INFINITE);

	::printf("\nAll Thread Terminate!\n");
}

unsigned int WINAPI NetLib::AcceptThread(void* arg)
{
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	NetLib* pNetLib = (NetLib*)arg;
	SessionMap* sessionMap = pNetLib->_sessionMap;

	for(;;)
	{
		// Accept
		SOCKET client_sock = accept(pNetLib->_listenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			continue;
		}
		if (g_bShutdown) break;


		// Create Session
		Session* pSession = new Session(
			pNetLib->_sessionIDSupplier++, client_sock, clientaddr);
		if (pSession == nullptr)
		{
			::printf("Error! %s(%d)\n", __func__, __LINE__);
			g_bShutdown = true;
			break;
		}

		EnterCriticalSection(&sessionMap->_cs);
		sessionMap->_map.insert(make_pair(pSession->_ID, pSession));
		LeaveCriticalSection(&sessionMap->_cs);

		//::printf("Accept New Session (ID: %llu)\n", pSession->_ID);

		// Connect Session to IOCP and Post Recv
		CreateIoCompletionPort((HANDLE)pSession->_sock, pNetLib->_hNetworkCP, (ULONG_PTR)pSession->_ID, 0);
		pNetLib->RecvPost(pSession);
	}

	::printf("Accept Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

unsigned int WINAPI NetLib::NetworkThread(void* arg)
{
	NetLib* pNetLib = (NetLib*)arg;
	SessionMap* sessionMap = pNetLib->_sessionMap;
	while (1)
	{
		__int64 sessionID;
		DWORD cbTransferred;
		NetworkOverlapped* pNetOvl = new NetworkOverlapped;
		int GQCSRet = GetQueuedCompletionStatus(pNetLib->_hNetworkCP,
			&cbTransferred, (PULONG_PTR)&sessionID, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (g_bShutdown) break;
		if (pNetOvl->_type == NET_TYPE::RELEASE)
		{
			pNetLib->ReleaseSession(sessionID);
			continue;
		}

		// Check Exception		
		if (GQCSRet == 0 || cbTransferred == 0)
		{
			EnterCriticalSection(&sessionMap->_cs);
			unordered_map<__int64, Session*>::iterator iter =sessionMap->_map.find(sessionID);
			if (iter == sessionMap->_map.end())
			{
				LeaveCriticalSection(&sessionMap->_cs);
				continue;
			}
			Session* pSession = iter->second;

			EnterCriticalSection(&pSession->_cs);
			LeaveCriticalSection(&sessionMap->_cs);

			if (GQCSRet == 0)
			{
				DWORD arg1, arg2;
				WSAGetOverlappedResult(pSession->_sock, (LPOVERLAPPED)pNetOvl, &arg1, FALSE, &arg2);
				int err = WSAGetLastError();
				if (err != WSAECONNRESET && err != WSAECONNABORTED && err != WSAENOTSOCK)
				{
					::printf("%s[%d]: Connecton End, %d\n", __FUNCTION__, __LINE__, err);
				}
			}
			LeaveCriticalSection(&pSession->_cs);
		}
		else if (pNetOvl->_type == NET_TYPE::RECV)
		{
			// ::printf("%d: Complete Recv %d bytes\n", GetCurrentThreadId(), cbTransferred);
			pNetLib->HandleRecvCP(sessionID, cbTransferred);

		}
		else if (pNetOvl->_type == NET_TYPE::SEND)
		{
			// ::printf("%d: Complete Send %d bytes\n", GetCurrentThreadId(), cbTransferred);
			pNetLib->HandleSendCP(sessionID, cbTransferred);
		}

		EnterCriticalSection(&sessionMap->_cs);
		unordered_map<__int64, Session*>::iterator iter = sessionMap->_map.find(sessionID);
		if (iter == sessionMap->_map.end())
		{
			LeaveCriticalSection(&sessionMap->_cs);
			continue;
		}
		Session* pSession = iter->second;

		EnterCriticalSection(&pSession->_cs);
		LeaveCriticalSection(&sessionMap->_cs);

		if (InterlockedDecrement(&pSession->_IOCount) == 0)
		{
			PostQueuedCompletionStatus(pNetLib->_hNetworkCP, 1,
				(ULONG_PTR)pSession->_ID, (LPOVERLAPPED)&pSession->_releaseOvl);
		}

		LeaveCriticalSection(&pSession->_cs);
	}

	::printf("Worker Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

void NetLib::HandleRecvCP(__int64 sessionID, int recvBytes)
{	
	EnterCriticalSection(&_sessionMap->_cs);
	unordered_map<__int64, Session*>::iterator iter;
	iter = _sessionMap->_map.find(sessionID);
	if (iter == _sessionMap->_map.end())
	{
		LeaveCriticalSection(&_sessionMap->_cs);
		return;
	}
	Session* pSession = iter->second;

	EnterCriticalSection(&pSession->_cs);
	LeaveCriticalSection(&_sessionMap->_cs);

	int moveReadRet = pSession->_recvBuf.MoveWritePos(recvBytes);
	if (moveReadRet != recvBytes)
	{
		__debugbreak();
		LeaveCriticalSection(&pSession->_cs);
		return;
	}

	int useSize = pSession->_recvBuf.GetUseSize();

	for (;;)
	{
		if (useSize <= dfHEADER_LEN)
		{
			LeaveCriticalSection(&pSession->_cs);
			break;
		}

		stHeader header;
		int peekRet = pSession->_recvBuf.Peek((char*)&header, dfHEADER_LEN);
		if (peekRet != dfHEADER_LEN)
		{
			__debugbreak();
			LeaveCriticalSection(&pSession->_cs);
			break;
		}

		if (useSize < dfHEADER_LEN + header._shLen)
		{
			LeaveCriticalSection(&pSession->_cs);
			break;
		}

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_LEN);
		if (moveReadRet != dfHEADER_LEN)
		{
			__debugbreak();
			LeaveCriticalSection(&pSession->_cs);
			break;
		}

		SerializePacket* packet = new SerializePacket;
		int dequeueRet = pSession->_recvBuf.Dequeue(packet->GetPayloadWritePtr(), header._shLen);
		if (dequeueRet != header._shLen)
		{
			__debugbreak();
			LeaveCriticalSection(&pSession->_cs);
			break;
		}

		packet->MovePayloadWritePos(dequeueRet);
		LeaveCriticalSection(&pSession->_cs);

		OnRecv(pSession->_ID, packet);
		delete packet;

		EnterCriticalSection(&_sessionMap->_cs);
		iter = _sessionMap->_map.find(sessionID);
		if (iter == _sessionMap->_map.end())
		{
			LeaveCriticalSection(&_sessionMap->_cs);
			break;
		}
		pSession = iter->second;

		EnterCriticalSection(&pSession->_cs);
		LeaveCriticalSection(&_sessionMap->_cs);
		useSize = pSession->_recvBuf.GetUseSize();
	}

	EnterCriticalSection(&_sessionMap->_cs);
	iter = _sessionMap->_map.find(sessionID);
	if (iter == _sessionMap->_map.end())
	{
		LeaveCriticalSection(&_sessionMap->_cs);
		return;
	}
	pSession = iter->second;

	EnterCriticalSection(&pSession->_cs);
	LeaveCriticalSection(&_sessionMap->_cs);

	RecvPost(pSession);

	LeaveCriticalSection(&pSession->_cs);
}

void NetLib::HandleSendCP(__int64 sessionID, int sendBytes)
{
	EnterCriticalSection(&_sessionMap->_cs);

	unordered_map<__int64, Session*>::iterator iter = _sessionMap->_map.find(sessionID);
	if (iter == _sessionMap->_map.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&_sessionMap->_cs);
		return;
	}
	Session* pSession = iter->second;

	EnterCriticalSection(&pSession->_cs);
	LeaveCriticalSection(&_sessionMap->_cs);

	for (int i = 0; i < pSession->_sendCnt; i++)
	{
		SerializePacket* packet = pSession->_sendBuf.GetReadPosData(i);
		delete packet;
	}

	int moveReadRet = pSession->_sendBuf.MoveReadPos(pSession->_sendCnt);
	if (moveReadRet != pSession->_sendCnt)
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&pSession->_cs);
		return;
	}

	InterlockedExchange(&pSession->_sendFlag, 0);
	SendPost(pSession);
	LeaveCriticalSection(&pSession->_cs);
}

void NetLib::ReleaseSession(__int64 sessionID)
{
	EnterCriticalSection(&_sessionMap->_cs);
	unordered_map<__int64, Session*>::iterator iter = _sessionMap->_map.find(sessionID);
	if (iter == _sessionMap->_map.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&_sessionMap->_cs);
		return;
	}
	Session* pSession = iter->second;
	_sessionMap->_map.erase(iter);
	LeaveCriticalSection(&_sessionMap->_cs);

	EnterCriticalSection(&pSession->_cs);
	LeaveCriticalSection(&pSession->_cs);

	closesocket(pSession->_sock);
	__int64 ID = pSession->_ID;
	delete(pSession);
}

void NetLib::RecvPost(Session* pSession)
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
		WSA_RCVBUF_MAX, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvOvl, NULL);

	// ::printf("%d: Request Recv\n", GetCurrentThreadId());

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

void NetLib::SendPost(Session* pSession)
{
	if (pSession->_sendBuf.GetUseSize() == 0)  return;
	if (InterlockedExchange(&pSession->_sendFlag, 1) == 1) return;
	if (pSession->_sendBuf.GetUseSize() == 0)
	{
		InterlockedExchange(&pSession->_sendFlag, 0);
		return;
	}

	int idx = 0;
	int useSize = pSession->_sendBuf.GetUseSize();
	for (; idx < useSize; idx++)
	{
		if (idx == WSA_SNDBUF_MAX) break;
		SerializePacket* packet = pSession->_sendBuf.GetReadPosData(idx);
		pSession->_wsaSendbuf[idx].buf = packet->GetPacketReadPtr();
		pSession->_wsaSendbuf[idx].len = packet->GetPacketSize();
	}
	pSession->_sendCnt = idx;

	ZeroMemory(&pSession->_sendOvl._ovl, sizeof(pSession->_sendOvl._ovl));
	InterlockedIncrement(&pSession->_IOCount);

	DWORD sendBytes;
	int sendRet = WSASend(pSession->_sock, pSession->_wsaSendbuf,
		idx, &sendBytes, 0, (LPOVERLAPPED)&pSession->_sendOvl, NULL);

	// ::printf("%d: Request Send %d bytes\n", GetCurrentThreadId(), useSize);

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err == ERROR_IO_PENDING)
		{
			// _IOPendCnt++;
		}
		else
		{
			if (err != WSAECONNRESET)
			{
				::printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
			}
			return;
		}
	}
}

void NetLib::Disconnect(__int64 sessionID)
{
}

void NetLib::SendPacket(__int64 sessionID, SerializePacket* packet)
{
	int payloadSize = packet->GetPayloadSize();
	stHeader header;
	header._shLen = payloadSize;
	int putRet = packet->PutHeaderData((char*)&header, sizeof(header));
	if (putRet != sizeof(header))
	{
		::printf("Error! Func %s Line %d\n", __func__, __LINE__);
		g_bShutdown = true;
		return;
	}

	EnterCriticalSection(&_sessionMap->_cs);
	unordered_map<__int64, Session*>::iterator iter = _sessionMap->_map.find(sessionID);
	if (iter == _sessionMap->_map.end())
	{
		::printf("Error! %s(%d)\n", __func__, __LINE__);
		g_bShutdown = true;
		LeaveCriticalSection(&_sessionMap->_cs);
		return;
	}

	Session* pSession = iter->second;
	EnterCriticalSection(&pSession->_cs);
	LeaveCriticalSection(&_sessionMap->_cs);

	pSession->_sendBuf.SetWritePosData(packet);
	int enqueueRet = pSession->_sendBuf.MoveWritePos(1);
	if (enqueueRet != 1)
	{
		::printf("Error! Func %s Line %d: return %d\n", __func__, __LINE__, enqueueRet);
		__debugbreak();
		g_bShutdown = true;
		return;
	}

	SendPost(pSession);
	LeaveCriticalSection(&pSession->_cs);
}
