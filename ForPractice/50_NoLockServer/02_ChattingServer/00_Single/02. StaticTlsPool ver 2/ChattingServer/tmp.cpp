/*
#include "CNetServer.h"
#ifdef NETSERVER

#include "ErrorCode.h"
#include <stdio.h>
#include <tchar.h>

// 17 bit for Idx (MAX 131072)
// 47 bit for Id (MAX 140737488355328)
#define ID_BIT_SIZE 47
__declspec(thread) wchar_t* stErrMsg = new wchar_t[dfMSG_MAX];

CNetServer::CNetServer()
{
	_indexMask = 0b11111111111111111;
	_indexMask <<= ID_BIT_SIZE;
	_idMask = ~_indexMask;
}

bool CNetServer::NetworkInitialize(const wchar_t* IP, short port, int numOfThreads, bool nagle)
{
	// Option Setting ====================================================

	wcscpy_s(_IP, 10, IP);
	_port = port;
	_numOfThreads = numOfThreads;
	_nagle = nagle;

	// Network Setting ===================================================

	// Initialize Winsock
	WSADATA wsa;
	int startRet = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (startRet != 0)
	{
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: WSAStartup Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_WSASTARTUP, stErrMsg);
		return false;
	}

	// Create Listen Sock
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Listen sock is INVALID, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTENSOCK_INVALID, stErrMsg);
		return false;
	}

	// Set Linger Option
	LINGER optval;
	optval.l_onoff = 1;
	optval.l_linger = 0;
	int optRet = setsockopt(_listenSock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
	if (optRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Set Linger Option Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_LINGER, stErrMsg);
		return false;
	}

	// Set SndBuf0 Option for Async
	int sndBufSize = 0;
	optRet = setsockopt(_listenSock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(sndBufSize));
	if (optRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Set SendBuf Option Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_SNDBUF_0, stErrMsg);
		return false;
	}

	// Listen Socket Bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(_port);
	int bindRet = bind(_listenSock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bindRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Listen Sock Bind Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTENSOCK_BIND, stErrMsg);
		return false;
	}

	// Start Listen
	int listenRet = listen(_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Listen Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTEN, stErrMsg);
		return false;
	}

	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		_emptyIdx.Push(i);
		_sessions[i] = new CSession;
	}

	// Thread Setting ===================================================

	// Create IOCP for Network
	_hNetworkCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hNetworkCP == NULL)
	{
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Create IOCP Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_IOCP, stErrMsg);
		return false;
	}

	// Create Accept Thread
	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, nullptr);
	if (_acceptThread == NULL)
	{
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Create Accept Thread Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_ACCEPT_THREAD, stErrMsg);
		return false;
	}

	// Create Network Thread
	_networkThreads = new HANDLE[_numOfThreads];
	for (int i = 0; i < _numOfThreads; i++)
	{
		_networkThreads[i] = (HANDLE)_beginthreadex(NULL, 0, NetworkThread, this, 0, nullptr);
		if (_networkThreads[i] == NULL)
		{
			int err = WSAGetLastError();
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Create Network Thread Error, %d\n", _T(__FUNCTION__), __LINE__, err);
			OnError(ERR_CREATE_NETWORK_THREAD, stErrMsg);
			return false;
		}
	}

	OnInitialize();

	return true;
}

bool CNetServer::NetworkTerminate()
{
	if (InterlockedExchange(&_networkAlive, 1) != 0)
	{
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: LanServer Already Terminate\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_ALREADY_TERMINATE, stErrMsg);
		return false;
	}

	// Terminate Network Threads
	for (int i = 0; i < _numOfThreads; i++)
		PostQueuedCompletionStatus(_hNetworkCP, 0, 0, 0);
	WaitForMultipleObjects(_numOfThreads, _networkThreads, true, INFINITE);

	// Terminate Accept Thread
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	InetPton(AF_INET, L"127.0.0.1", &serveraddr.sin_addr);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(_port);

	// Awake Accept Thread if It's blocked
	SOCKET socktmp = socket(AF_INET, SOCK_STREAM, 0);
	if (socktmp == INVALID_SOCKET)
	{
		int err = ::WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Socket for Wake is INVALID, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_INVALID, stErrMsg);
		return false;
	}

	int connectRet = connect(socktmp, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (connectRet == SOCKET_ERROR)
	{
		int err = ::WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Socket for Wake Connect Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_CONNECT, stErrMsg);
		return false;
	}
	WaitForSingleObject(_acceptThread, INFINITE);

	// Release All Session
	closesocket(socktmp);
	closesocket(_listenSock);
	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		CSession* pSession = _sessions[i];
		closesocket(pSession->_sock);
		delete pSession;
	}

	WSACleanup();
	CloseHandle(_hNetworkCP);
	CloseHandle(_acceptThread);
	for (int i = 0; i < _numOfThreads; i++)
		CloseHandle(_networkThreads[i]);
	delete[] _networkThreads;

	OnTerminate();
}

bool CNetServer::Disconnect(__int64 sessionID)
{
	if (sessionID == -1) return false;

	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= ID_BIT_SIZE;
	CSession* pSession = _sessions[(long)idx];

	if (!IncrementUseCount(pSession))
	{
		return false;
	}
	if (pSession->_validFlag._releaseFlag == 1)
	{
		return false;
	}
	if (pSession->_ID != sessionID)
	{
		DecrementUseCount(pSession);
		return false;
	}
	if (InterlockedExchange16(&pSession->_disconnect, 1) == 1)
	{
		DecrementUseCount(pSession);
		return false;
	}

	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_recvOvl);
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_sendOvl);
	DecrementUseCount(pSession);

	return true;
}

bool CNetServer::SendPacket(__int64 sessionID, CPacket* packet)
{
	// ::printf("%lld (%d): Request Send Packet\n", (sessionID & _idMask), GetCurrentThreadId());
	if (sessionID == -1) return false;

	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= ID_BIT_SIZE;
	CSession* pSession = _sessions[(long)idx];

	if (!IncrementUseCount(pSession))
	{
		return false;
	}
	if (pSession->_validFlag._releaseFlag == 1)
	{
		return false;
	}
	if (pSession->_ID != sessionID)
	{
		DecrementUseCount(pSession);
		return false;
	}

	if (packet->IsHeaderEmpty())
	{
		short payloadSize = packet->GetPayloadSize();
		stHeader header;
		header._len = payloadSize;
		header._randKey = rand() % 256;
		packet->Encode(header);

		int putRet = packet->PutHeaderData((char*)&header, dfHEADER_LEN);
		if (putRet != dfHEADER_LEN)
		{
			DecrementUseCount(pSession);
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: CPacket PutHeaderData Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_PACKET_PUT_HEADER, stErrMsg);
			return false;
		}
	}

	pSession->_sendBuf.Enqueue(packet);
	SendPost(pSession);
	DecrementUseCount(pSession);
	return true;
}

unsigned int __stdcall CNetServer::AcceptThread(void* arg)
{
	CNetServer* pNetServer = (CNetServer*)arg;
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);

	for (;;)
	{
		// Accept
		SOCKET client_sock = accept(pNetServer->_listenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Accept Error\n", _T(__FUNCTION__), __LINE__);
			pNetServer->OnError(ERR_ACCEPT, stErrMsg);
			break;
		}
		InterlockedIncrement(&pNetServer->_acceptCnt);

		if (pNetServer->_networkAlive == 1) break;
		long sessionCnt = InterlockedIncrement(&pNetServer->_sessionCnt);
		if (sessionCnt > dfSESSION_MAX)
		{
			closesocket(client_sock);
			InterlockedIncrement(&pNetServer->_disconnectCnt);
			InterlockedDecrement(&pNetServer->_sessionCnt);

			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Session Max\n", _T(__FUNCTION__), __LINE__);
			pNetServer->OnError(DEB_SESSION_MAX, stErrMsg);

			continue;
		}

		if (pNetServer->_emptyIdx.GetUseSize() == 0)
		{
			closesocket(client_sock);
			InterlockedIncrement(&pNetServer->_disconnectCnt);

			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: No Empty Index\n", _T(__FUNCTION__), __LINE__);
			pNetServer->OnError(DEB_SESSION_MAX, stErrMsg);

			continue;
		}

		__int64 sessionID = InterlockedIncrement64(&pNetServer->_sessionID);
		sessionID &= pNetServer->_idMask;
		unsigned __int64 idx = pNetServer->_emptyIdx.Pop();
		unsigned __int64 sessionIdx = idx << ID_BIT_SIZE;
		sessionID |= sessionIdx;

		CSession* pSession = pNetServer->_sessions[(long)idx];
		pSession->Initialize(sessionID, client_sock, clientaddr);

		if (!pNetServer->IncrementUseCount(pSession))
		{
			continue;
		}
		if (pSession->_validFlag._releaseFlag == 1)
		{
			continue;
		}
		if (pSession->_ID != sessionID)
		{
			pNetServer->DecrementUseCount(pSession);
			continue;
		}

		// ::printf("%lld (%d): Accept Success\n", (sessionID & pNetServer->_idMask), GetCurrentThreadId());

		// Connect Session to IOCP and Post Recv
		CreateIoCompletionPort((HANDLE)pSession->_sock, pNetServer->_hNetworkCP, (ULONG_PTR)pSession->_ID, 0);
		pNetServer->RecvPost(pSession);
		pNetServer->OnAcceptClient(pSession->_ID);
		pNetServer->DecrementUseCount(pSession);
	}

	swprintf_s(stErrMsg, dfMSG_MAX, L"Accept Thread (%d)", GetCurrentThreadId());
	pNetServer->OnThreadTerminate(stErrMsg);

	return 0;
}

unsigned int __stdcall CNetServer::NetworkThread(void* arg)
{
	CNetServer* pNetServer = (CNetServer*)arg;
	int threadID = GetCurrentThreadId();
	NetworkOverlapped* pNetOvl = new NetworkOverlapped;

	for (;;)
	{
		long long sessionID;
		DWORD cbTransferred;

		int GQCSRet = GetQueuedCompletionStatus(pNetServer->_hNetworkCP,
			&cbTransferred, (PULONG_PTR)&sessionID, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (pNetServer->_networkAlive == 1) break;

		if (sessionID == -1) continue;
		if (pNetOvl->_type == NET_TYPE::RELEASE)
		{
			// ::printf("%lld (%d): Release Request\n", (pSession->_ID & pNetServer->_idMask), GetCurrentThreadId());
			pNetServer->ReleaseSession(sessionID);
			continue;
		}

		unsigned __int64 idx = sessionID & pNetServer->_indexMask;
		idx >>= ID_BIT_SIZE;
		CSession* pSession = pNetServer->_sessions[(long)idx];

		bool connected = pNetServer->IncrementUseCount(pSession, false);
		if (pSession->_validFlag._releaseFlag == 1)
		{
			continue;
		}
		if (pSession->_ID != sessionID)
		{
			pNetServer->DecrementUseCount(pSession);
			continue;
		}

		// Check Exception
		if (GQCSRet == 0 || cbTransferred == 0)
		{
			if (GQCSRet == 0)
			{
				int err = WSAGetLastError();
				if (err != WSAECONNRESET && err != WSAECONNABORTED && err != WSAENOTSOCK && err != WSAEINTR &&
					err != ERROR_CONNECTION_ABORTED && err != ERROR_NETNAME_DELETED && err != ERROR_OPERATION_ABORTED)
				{
					swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: GQCS return 0, %d\n", _T(__FUNCTION__), __LINE__, err);
					pNetServer->OnError(ERR_GQCS_RET0, stErrMsg);
				}
			}
		}
		else if (pNetOvl->_type == NET_TYPE::RECV)
		{
			// ::printf("%lld (%d): Recv Success\n", (pSession->_ID & pNetServer->_idMask), GetCurrentThreadId());
			pNetServer->HandleRecvCP(pSession, cbTransferred);
		}
		else if (pNetOvl->_type == NET_TYPE::SEND)
		{
			// ::printf("%lld (%d): Send Success\n", (pSession->_ID & pNetServer->_idMask), GetCurrentThreadId());
			pNetServer->HandleSendCP(pSession, cbTransferred);
		}

		pNetServer->DecrementUseCount(pSession);
		pNetServer->DecrementUseCount(pSession);
	}

	delete pNetOvl;
	swprintf_s(stErrMsg, dfMSG_MAX, L"Network Thread (%d)", threadID);
	pNetServer->OnThreadTerminate(stErrMsg);

	return 0;
}

bool CNetServer::ReleaseSession(__int64 sessionID)
{
	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= ID_BIT_SIZE;
	CSession* pSession = _sessions[(long)idx];

	if (InterlockedCompareExchange(&pSession->_validFlag._flag, 1, 0) != 0)
	{
		return false;
	}
	if (pSession->_ID != sessionID)
	{
		InterlockedExchange16(&pSession->_validFlag._releaseFlag, 0);
		return false;
	}

	// ::printf("%lld (%d): Release Success\n", (pSession->_ID & _idMask), GetCurrentThreadId());

	SOCKET sock = pSession->_sock;
	OnReleaseClient(pSession->_ID);
	pSession->Terminate();

	closesocket(sock);
	_emptyIdx.Push((long)idx);
	InterlockedIncrement(&_disconnectCnt);
	InterlockedDecrement(&_sessionCnt);
	return true;
}

bool CNetServer::HandleRecvCP(CSession* pSession, int recvBytes)
{
	int moveReadRet = pSession->_recvBuf.MoveWritePos(recvBytes);
	if (moveReadRet != recvBytes)
	{
		Disconnect(pSession->_ID);
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer MoveWritePos Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_RECVBUF_MOVEWRITEPOS, stErrMsg);
		return false;
	}

	int useSize = pSession->_recvBuf.GetUseSize();

	for (;;)
	{
		if (useSize <= dfHEADER_LEN) break;

		stHeader header;
		int peekRet = pSession->_recvBuf.Peek((char*)&header, dfHEADER_LEN);
		if (peekRet != dfHEADER_LEN)
		{
			Disconnect(pSession->_ID);
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer Peek Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_PEEK, stErrMsg);
			return false;
		}

		if (header._code != dfPACKET_CODE)
		{
			Disconnect(pSession->_ID);
			// swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Wrong Packet Code\n", _T(__FUNCTION__), __LINE__);
			// OnDebug(DEB_WRONG_PACKETCODE, stErrMsg);
			return false;
		}


		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_LEN);
		if (moveReadRet != dfHEADER_LEN)
		{
			Disconnect(pSession->_ID);
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, stErrMsg);
			return false;
		}

		CPacket* packet = CPacket::Alloc();
		packet->Clear();
		packet->AddUsageCount(1);
		int dequeueRet = pSession->_recvBuf.Dequeue(packet->GetPayloadWritePtr(), header._len);
		if (dequeueRet != header._len)
		{
			Disconnect(pSession->_ID);
			CPacket::Free(packet);
			// swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer Dequeue Error\n", _T(__FUNCTION__), __LINE__);
			// OnDebug(DEB_WRONG_PACKETLEN, stErrMsg);
			return false;
		}
		packet->MovePayloadWritePos(dequeueRet);

		if (packet->Decode(header) == false)
		{
			Disconnect(pSession->_ID);
			CPacket::Free(packet);
			// swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Wrong Checksum\n", _T(__FUNCTION__), __LINE__);
			// OnDebug(DEB_WRONG_DECODE, stErrMsg);
			return false;
		}

		OnRecv(pSession->_ID, packet);
		CPacket::Free(packet);

		useSize = pSession->_recvBuf.GetUseSize();
	}

	if (!RecvPost(pSession)) return false;
	return true;
}

bool CNetServer::RecvPost(CSession* pSession)
{
	DWORD flags = 0;
	DWORD recvBytes = 0;

	int freeSize = pSession->_recvBuf.GetFreeSize();
	pSession->_wsaRecvbuf[0].buf = pSession->_recvBuf.GetWritePtr();
	pSession->_wsaRecvbuf[0].len = pSession->_recvBuf.DirectEnqueueSize();
	pSession->_wsaRecvbuf[1].buf = pSession->_recvBuf.GetFrontPtr();
	pSession->_wsaRecvbuf[1].len = freeSize - pSession->_wsaRecvbuf[0].len;

	ZeroMemory(&pSession->_recvOvl._ovl, sizeof(pSession->_recvOvl._ovl));

	if (!IncrementUseCount(pSession))
	{
		return false;
	}
	int recvRet = WSARecv(pSession->_sock, pSession->_wsaRecvbuf,
		dfWSARECVBUF_CNT, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvOvl, NULL);
	// ::printf("%lld (%d): Recv Request\n", (pSession->_ID & _idMask), GetCurrentThreadId());

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED &&
				err != WSAENOTSOCK && err != WSAEINTR)
			{
				swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Error, %d\n", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_RECV, stErrMsg);
			}
			DecrementUseCount(pSession);
			return false;
		}
	}

	InterlockedIncrement(&_recvMsgCnt);
	return true;
}

bool CNetServer::HandleSendCP(CSession* pSession, int sendBytes)
{
	for (int i = 0; i < pSession->_sendCount; i++)
	{
		CPacket* packet = pSession->_tempBuf.Dequeue();
		if (packet == nullptr) break;
		CPacket::Free(packet);
	}

	OnSend(pSession->_ID, sendBytes);
	InterlockedExchange(&pSession->_sendFlag, 0);
	if (!SendPost(pSession)) return false;

	return true;
}

bool CNetServer::SendPost(CSession* pSession)
{
	if (pSession->_disconnect == 1) return false;
	if (pSession->_sendBuf.GetUseSize() == 0) return false;
	if (InterlockedExchange(&pSession->_sendFlag, 1) == 1) return false;
	if (pSession->_disconnect == 1 || pSession->_sendBuf.GetUseSize() == 0)
	{
		InterlockedExchange(&pSession->_sendFlag, 0);
		return false;
	}

	int idx = 0;
	int useSize = pSession->_sendBuf.GetUseSize();

	for (; idx < useSize; idx++)
	{
		if (idx == dfWSASENDBUF_CNT) break;
		CPacket* packet = pSession->_sendBuf.Dequeue();
		if (packet == nullptr) break;

		pSession->_wsaSendbuf[idx].buf = packet->GetPacketReadPtr();
		pSession->_wsaSendbuf[idx].len = packet->GetPacketSize();
		pSession->_tempBuf.Enqueue(packet);
	}
	pSession->_sendCount = idx;

	DWORD sendBytes;
	ZeroMemory(&pSession->_sendOvl._ovl, sizeof(pSession->_sendOvl._ovl));

	if (!IncrementUseCount(pSession))
	{
		InterlockedExchange(&pSession->_sendFlag, 0);
		return false;
	}
	int sendRet = WSASend(pSession->_sock, pSession->_wsaSendbuf,
		idx, &sendBytes, 0, (LPOVERLAPPED)&pSession->_sendOvl, NULL);

	// ::printf("%lld (%d): Send Request\n", (pSession->_ID & _idMask), GetCurrentThreadId());

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED &&
				err != WSAENOTSOCK && err != WSAEINTR)
			{
				swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Send Error, %d\n", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_SEND, stErrMsg);
			}

			InterlockedExchange(&pSession->_sendFlag, 0);
			DecrementUseCount(pSession);
			return false;
		}
	}

	InterlockedIncrement(&_sendMsgCnt);
	return true;
}

bool CNetServer::IncrementUseCount(CSession* pSession, bool check)
{
	bool ret = true;
	if (pSession->_disconnect == 1)
	{
		if (check) return false;
		ret = false;
	}
	InterlockedIncrement16(&pSession->_validFlag._useCount);
	return ret;
}

void CNetServer::DecrementUseCount(CSession* pSession)
{
	if (InterlockedDecrement16(&pSession->_validFlag._useCount) == 0)
	{
		PostQueuedCompletionStatus(_hNetworkCP, 1, (ULONG_PTR)pSession->_ID, (LPOVERLAPPED)&pSession->_releaseOvl);
	}
}
#endif 
*/