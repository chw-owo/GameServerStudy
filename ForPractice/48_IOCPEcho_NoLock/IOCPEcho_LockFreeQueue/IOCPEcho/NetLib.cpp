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

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	
	CONSOLE_CURSOR_INFO stConsoleCursor;
	stConsoleCursor.bVisible = FALSE;
	stConsoleCursor.dwSize = 1;
	SetConsoleCursorInfo(hConsole, &stConsoleCursor);

	COORD stCoord;
	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		_unuseIdxs.Enqueue(i);
		_sessions[i] = new Session(i);

		stCoord.X = 0;
		stCoord.Y = 0;
		SetConsoleCursorPosition(hConsole, stCoord);
		::printf("Ready... %d/%d\n", i, dfSESSION_MAX);
	}

	stCoord.X = 0;
	stCoord.Y = 0;
	SetConsoleCursorPosition(hConsole, stCoord);
	::printf("Ready... %d/%d\n", dfSESSION_MAX, dfSESSION_MAX);

	_packetPool = new LockFreePool<SerializePacket>(0, true);

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
	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		Session* pSession = _sessions[i];
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
		int idx = pNetLib->_unuseIdxs.Dequeue();
		if (idx == -1) 
		{
			::printf("Session Max!!\n");
			continue;
		}
		Session* pSession = pNetLib->_sessions[idx];
		pSession->Initialize(pNetLib->_sessionIDSupplier++, client_sock, clientaddr);
		// ::printf("Accept Socket! Idx: %d, ID: %lld\n", idx, pSession->_sessionID);

		// Connect Session to IOCP and Post Recv
		CreateIoCompletionPort((HANDLE)pSession->_sock, pNetLib->_hNetworkCP, (ULONG_PTR)idx, 0);		
		pNetLib->RecvPost(pSession);
	}

	::printf("Accept Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

unsigned int WINAPI NetLib::NetworkThread(void* arg)
{
	NetLib* pNetLib = (NetLib*)arg;

	while (1)
	{
		int idx;
		DWORD cbTransferred;
		NetworkOverlapped* pNetOvl = new NetworkOverlapped;
		int GQCSRet = GetQueuedCompletionStatus(pNetLib->_hNetworkCP,
			&cbTransferred, (PULONG_PTR)&idx, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		Session* pSession = pNetLib->_sessions[idx];
		if (pSession->_sessionID == -1) continue;

		if (g_bShutdown) break;
		if (pNetOvl->_type == NET_TYPE::RELEASE)
		{
			pNetLib->ReleaseSession(pSession);
			continue;
		}

		// Check Exception		
		if (GQCSRet == 0 || cbTransferred == 0)
		{
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
		}
		else if (pNetOvl->_type == NET_TYPE::RECV)
		{
			// ::printf("%d: Complete Recv %d bytes\n", GetCurrentThreadId(), cbTransferred);
			pNetLib->HandleRecvCP(pSession, cbTransferred);

		}
		else if (pNetOvl->_type == NET_TYPE::SEND)
		{
			// ::printf("%d: Complete Send %d bytes\n", GetCurrentThreadId(), cbTransferred);
			pNetLib->HandleSendCP(pSession, cbTransferred);
		}

		if (InterlockedDecrement(&pSession->_IOCount) == 0)
		{
			PostQueuedCompletionStatus(pNetLib->_hNetworkCP, 1,
				(ULONG_PTR)idx, (LPOVERLAPPED)&pSession->_releaseOvl);
		}
	}

	::printf("Worker Thread Terminate (ID: %d)\n", GetCurrentThreadId());
	return 0;
}

void NetLib::HandleRecvCP(Session* pSession, int recvBytes)
{	
	int moveReadRet = pSession->_recvBuf.MoveWritePos(recvBytes);
	if (moveReadRet != recvBytes) __debugbreak();
	int useSize = pSession->_recvBuf.GetUseSize();

	for (;;)
	{
		if (useSize <= dfHEADER_LEN) break;

		stHeader header;
		int peekRet = pSession->_recvBuf.Peek((char*)&header, dfHEADER_LEN);
		if (peekRet != dfHEADER_LEN) __debugbreak();

		if (useSize < dfHEADER_LEN + header._shLen) break;

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_LEN);
		if (moveReadRet != dfHEADER_LEN) __debugbreak();

		SerializePacket* packet = _packetPool->Alloc();
		
		int dequeueRet = pSession->_recvBuf.Dequeue(packet->GetPayloadWritePtr(), header._shLen);
		if (dequeueRet != header._shLen) __debugbreak();
		packet->MovePayloadWritePos(dequeueRet);

		OnRecv(pSession->_sessionID, packet);

		_packetPool->Free(packet);
		useSize = pSession->_recvBuf.GetUseSize();
	}

	RecvPost(pSession);
}

void NetLib::HandleSendCP(Session* pSession, int sendBytes)
{
	for (int i = 0; i < pSession->_sendCnt; i++)
	{
		SerializePacket* packet = pSession->_tempBuf.Dequeue();
		if (packet == nullptr) break;
		_packetPool->Free(packet);
	}

	InterlockedExchange(&pSession->_sendFlag, 0);
	SendPost(pSession);
}

void NetLib::ReleaseSession(Session* pSession)
{
	pSession->Clear();
	_unuseIdxs.Enqueue(pSession->_idx);
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

			if (InterlockedDecrement(&pSession->_IOCount) == 0)
			{
				PostQueuedCompletionStatus(_hNetworkCP, 1,
					(ULONG_PTR)pSession->_idx, (LPOVERLAPPED)&pSession->_releaseOvl);
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
		SerializePacket* packet = pSession->_sendBuf.Dequeue();

		if (packet == nullptr) break;
		pSession->_wsaSendbuf[idx].buf = packet->GetPacketReadPtr();
		pSession->_wsaSendbuf[idx].len = packet->GetPacketSize();
		pSession->_tempBuf.Enqueue(packet);
	}

	pSession->_sendCnt = idx;
	ZeroMemory(&pSession->_sendOvl._ovl, sizeof(pSession->_sendOvl._ovl));
	
	DWORD sendBytes;
	InterlockedIncrement(&pSession->_IOCount);
	int sendRet = WSASend(pSession->_sock, pSession->_wsaSendbuf,
		idx, &sendBytes, 0, (LPOVERLAPPED)&pSession->_sendOvl, NULL);

	// ::printf("%d: Request Send %d packet\n", GetCurrentThreadId(), idx);

	if (sendRet == SOCKET_ERROR)
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
				PostQueuedCompletionStatus(_hNetworkCP, 1,
					(ULONG_PTR)pSession->_idx, (LPOVERLAPPED)&pSession->_releaseOvl);
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

	Session* pSession = nullptr;
	int i = 0;
	for (; i < dfSESSION_MAX; i++)
	{
		if (_sessions[i]->_sessionID == sessionID)
		{
			pSession = _sessions[i];
			break;
		}
	}

	if (pSession == nullptr) return;
	pSession->_sendBuf.Enqueue(packet);
	SendPost(pSession);
}
