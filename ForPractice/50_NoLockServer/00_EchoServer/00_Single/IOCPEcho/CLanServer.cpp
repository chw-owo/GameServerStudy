#include "CLanServer.h"
#ifdef LANSERVER

#include "ErrorCode.h"
#include <stdio.h>
#include <tchar.h>

#define ID_BIT_SIZE 49

CLanServer::CLanServer()
{
	_indexMask = 0b111111111111111;
	_indexMask <<= ID_BIT_SIZE;
	_idMask = ~_indexMask;

	for (int i = 0; i < dfSESSION_MAX; i++)
		_emptyIdx.Push(i);
}

bool CLanServer::NetworkInitialize(const wchar_t* IP, short port, int numOfThreads, bool nagle)
{
	wchar_t* _errMsg = new wchar_t[dfMSG_MAX];

	// Option Setting ====================================================

	wcscpy_s(_IP, 10, IP);
	_port = port;
	_numOfThreads = numOfThreads;
	_nagle = nagle;

	// Network Setting ===================================================

	// _pPacketPool = new CLockFreePool<CPacket>(0, false);

	// Initialize Winsock
	WSADATA wsa;
	int startRet = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (startRet != 0)
	{
		swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: WSAStartup Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_WSASTARTUP, _errMsg);
		delete[] _errMsg;
		return false;
	}

	// Create Listen Sock
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Listen sock is INVALID, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTENSOCK_INVALID, _errMsg);
		delete[] _errMsg;
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
		swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Set Linger Option Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_LINGER, _errMsg);
		delete[] _errMsg;
		return false;
	}

	// Set SndBuf0 Option for Async
	int sndBufSize = 0;
	optRet = setsockopt(_listenSock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(sndBufSize));
	if (optRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Set SendBuf Option Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_SNDBUF_0, _errMsg);
		delete[] _errMsg;
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
		swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Listen Sock Bind Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTENSOCK_BIND, _errMsg);
		delete[] _errMsg;
		return false;
	}

	// Start Listen
	int listenRet = listen(_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Listen Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTEN, _errMsg);
		delete[] _errMsg;
		return false;
	}

	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		_sessions[i] = new CSession;
	}

	// Thread Setting ===================================================

	// Create IOCP for Network
	_hNetworkCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hNetworkCP == NULL)
	{
		int err = WSAGetLastError();
		swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Create IOCP Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_IOCP, _errMsg);
		delete[] _errMsg;
		return false;
	}

	// Create Accept Thread
	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, nullptr);
	if (_acceptThread == NULL)
	{
		int err = WSAGetLastError();
		swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Create Accept Thread Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_ACCEPT_THREAD, _errMsg);
		delete[] _errMsg;
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
			swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Create Network Thread Error, %d\n", _T(__FUNCTION__), __LINE__, err);
			OnError(ERR_CREATE_NETWORK_THREAD, _errMsg);
			delete[] _errMsg;
			return false;
		}
	}

	OnInitialize();
	delete[] _errMsg;
	return true;
}

bool CLanServer::NetworkTerminate()
{
	wchar_t* _errMsg = new wchar_t[dfMSG_MAX];

	if (InterlockedExchange(&_networkAlive, 1) != 0)
	{
		swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: LanServer Already Terminate\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_ALREADY_TERMINATE, _errMsg);
		delete[] _errMsg;
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
		swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Socket for Wake is INVALID, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_INVALID, _errMsg);
		delete[] _errMsg;
		return false;
	}

	int connectRet = connect(socktmp, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (connectRet == SOCKET_ERROR)
	{
		int err = ::WSAGetLastError();
		swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Socket for Wake Connect Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_CONNECT, _errMsg);
		delete[] _errMsg;
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

	delete[] _errMsg;
}

bool CLanServer::Disconnect(__int64 sessionID)
{
	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= ID_BIT_SIZE;
	CSession* pSession = _sessions[(short)idx];

	if (pSession->_validFlag._releaseFlag == 1) return false;
	if (pSession->_ID != sessionID) return false;

	pSession->_disconnect = true;
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_recvOvl);
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_sendOvl);

	return true;
}

bool CLanServer::SendPacket(__int64 sessionID, CPacket* packet)
{
	// ::printf("%lld (%d): Request Send Packet\n", (sessionID & _idMask), GetCurrentThreadId());

	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= ID_BIT_SIZE;
	CSession* pSession = _sessions[(short)idx];

	if (!IncrementIOCount(pSession, __LINE__, sessionID)) return false;
	if (pSession->_validFlag._releaseFlag == 1)
	{
		DecrementIOCount(pSession, __LINE__, sessionID);
		return false;
	}
	if (pSession->_ID != sessionID)
	{
		DecrementIOCount(pSession, __LINE__, sessionID);
		return false;
	}

	if (packet->IsHeaderEmpty())
	{
		short payloadSize = packet->GetPayloadSize();
		stHeader header;
		header._len = payloadSize;

		int putRet = packet->PutHeaderData((char*)&header, dfHEADER_LEN);
		if (putRet != dfHEADER_LEN)
		{
			wchar_t* _errMsg = new wchar_t[dfMSG_MAX];
			DecrementIOCount(pSession, __LINE__, sessionID);
			swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: CPacket PutHeaderData Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_PACKET_PUT_HEADER, _errMsg);
			delete[] _errMsg;

			return false;
		}
	}

	pSession->_sendBuf.Enqueue(packet);

	SendPost(pSession);
	DecrementIOCount(pSession, __LINE__, sessionID);

	return true;
}

unsigned int __stdcall CLanServer::AcceptThread(void* arg)
{
	CLanServer* pNetServer = (CLanServer*)arg;
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	wchar_t* _errMsg = new wchar_t[dfMSG_MAX];

	for (;;)
	{
		// Accept
		SOCKET client_sock = accept(pNetServer->_listenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Accept Error\n", _T(__FUNCTION__), __LINE__);
			pNetServer->OnError(ERR_ACCEPT, _errMsg);
			break;
		}

		if (pNetServer->_networkAlive == 1) break;

		long sessionCnt = InterlockedIncrement(&pNetServer->_sessionCnt);
		if (sessionCnt > dfSESSION_MAX)
		{
			closesocket(client_sock);
			InterlockedDecrement(&pNetServer->_sessionCnt);
			continue;
		}

		__int64 sessionID = InterlockedIncrement64(&pNetServer->_sessionID);
		sessionID &= pNetServer->_idMask;

		if (pNetServer->_emptyIdx.GetUseSize() == 0)
		{
			closesocket(client_sock);
			continue;
		}

		unsigned __int64 idx = pNetServer->_emptyIdx.Pop();
		unsigned __int64 sessionIdx = idx << ID_BIT_SIZE;
		sessionID |= sessionIdx;

		CSession* pSession = pNetServer->_sessions[(short)idx];
		pSession->Initialize(sessionID, client_sock, clientaddr);
		InterlockedIncrement(&pNetServer->_acceptCnt);

		//::printf("%lld (%d): Accept Success\n", (sessionID & pNetServer->_idMask), GetCurrentThreadId());

		// Connect Session to IOCP and Post Recv                                      
		CreateIoCompletionPort((HANDLE)pSession->_sock, pNetServer->_hNetworkCP, (ULONG_PTR)pSession, 0);

		pNetServer->RecvPost(pSession);
		pNetServer->OnAcceptClient(pSession->_ID);
	}

	swprintf_s(_errMsg, dfMSG_MAX, L"Accept Thread (%d)", GetCurrentThreadId());
	pNetServer->OnThreadTerminate(_errMsg);
	delete[] _errMsg;
	return 0;
}

unsigned int __stdcall CLanServer::NetworkThread(void* arg)
{
	CLanServer* pNetServer = (CLanServer*)arg;
	int threadID = GetCurrentThreadId();
	wchar_t* _errMsg = new wchar_t[dfMSG_MAX];

	for (;;)
	{
		CSession* pSession;
		DWORD cbTransferred;
		NetworkOverlapped* pNetOvl = new NetworkOverlapped;
		int GQCSRet = GetQueuedCompletionStatus(pNetServer->_hNetworkCP,
			&cbTransferred, (PULONG_PTR)&pSession, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (pNetServer->_networkAlive == 1) break;

		// Check Exception		
		if (GQCSRet == 0 || cbTransferred == 0)
		{
			if (GQCSRet == 0)
			{
				int err = WSAGetLastError();
				if (err != WSAECONNRESET && err != WSAECONNABORTED && err != WSAENOTSOCK &&
					err != ERROR_CONNECTION_ABORTED && err != ERROR_NETNAME_DELETED && err != ERROR_OPERATION_ABORTED)
				{
					swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: GQCS return 0, %d\n", _T(__FUNCTION__), __LINE__, err);
					pNetServer->OnError(ERR_GQCS_RET0, _errMsg);
				}
			}
		}
		else if (pNetOvl->_type == NET_TYPE::RELEASE)
		{
			// ::printf("%lld (%d): Release Request\n", (pSession->_ID & pNetServer->_idMask), GetCurrentThreadId());
			pNetServer->ReleaseSession(pSession->_ID);
			continue;
		}
		else if (pNetOvl->_type == NET_TYPE::RECV)
		{
			//::printf("%lld (%d): Recv Success\n", (pSession->_ID & pNetServer->_idMask), GetCurrentThreadId());
			pNetServer->HandleRecvCP(pSession->_ID, cbTransferred);
			pNetServer->_recvMsgCnt++;
		}
		else if (pNetOvl->_type == NET_TYPE::SEND)
		{
			//::printf("%lld (%d): Send Success\n", (pSession->_ID & pNetServer->_idMask), GetCurrentThreadId());
			pNetServer->HandleSendCP(pSession->_ID, cbTransferred);
			pNetServer->_sendMsgCnt++;
		}

		pNetServer->DecrementIOCount(pSession, __LINE__, 0);
	}

	swprintf_s(_errMsg, dfMSG_MAX, L"Network Thread (%d)", threadID);
	pNetServer->OnThreadTerminate(_errMsg);
	delete[] _errMsg;

	return 0;
}

bool CLanServer::ReleaseSession(__int64 sessionID)
{
	if (sessionID == -1) return false;

	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= ID_BIT_SIZE;
	CSession* pSession = _sessions[(short)idx];

	long debugIdx = InterlockedIncrement(&pSession->_debugIdx);
	pSession->_debugData[debugIdx % dfSESSION_DEBUG_MAX].LeaveLog(
		0, GetCurrentThreadId(), pSession->_validFlag._IOCount, pSession->_validFlag._releaseFlag, pSession->_ID, sessionID);

	if (InterlockedCompareExchange(&pSession->_validFlag._flag, 1, 0) != 0)
	{
		return false;
	}
	if (pSession->_ID != sessionID) return false;

	debugIdx = InterlockedIncrement(&pSession->_debugIdx);
	pSession->_debugData[debugIdx % dfSESSION_DEBUG_MAX].LeaveLog(
		1, GetCurrentThreadId(), pSession->_validFlag._IOCount, pSession->_validFlag._releaseFlag, pSession->_ID, sessionID);

	//::printf("%lld (%d): Release Success\n", (pSession->_ID & _idMask), GetCurrentThreadId());

	closesocket(pSession->_sock);
	pSession->Terminate();
	InterlockedIncrement(&_disconnectCnt);

	_emptyIdx.Push(idx);
	InterlockedDecrement(&_sessionCnt);

	OnReleaseClient(sessionID);
	return true;
}

bool CLanServer::HandleRecvCP(__int64 sessionID, int recvBytes)
{
	if (sessionID == -1) return false;
	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= ID_BIT_SIZE;
	CSession* pSession = _sessions[(short)idx];

	if (pSession->_validFlag._releaseFlag == 1) return false;
	if (pSession->_ID != sessionID) return false;

	int moveReadRet = pSession->_recvBuf.MoveWritePos(recvBytes);
	if (moveReadRet != recvBytes)
	{
		wchar_t* _errMsg = new wchar_t[dfMSG_MAX];
		swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer MoveWritePos Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_RECVBUF_MOVEWRITEPOS, _errMsg);
		delete[] _errMsg;
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
			wchar_t* _errMsg = new wchar_t[dfMSG_MAX];
			swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer Peek Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_PEEK, _errMsg);
			delete[] _errMsg;
			return false;
		}

		if (useSize < dfHEADER_LEN + header._len) break;

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_LEN);
		if (moveReadRet != dfHEADER_LEN)
		{
			wchar_t* _errMsg = new wchar_t[dfMSG_MAX];
			swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, _errMsg);
			delete[] _errMsg;
			return false;
		}

		CPacket* packet = CPacket::Alloc();
		packet->Clear();
		packet->AddUsageCount(1);
		int dequeueRet = pSession->_recvBuf.Dequeue(packet->GetPayloadWritePtr(), header._len);
		if (dequeueRet != header._len)
		{
			wchar_t* _errMsg = new wchar_t[dfMSG_MAX];
			swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer Dequeue Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_DEQ, _errMsg);
			delete[] _errMsg;
			return false;
		}
		packet->MovePayloadWritePos(dequeueRet);

		OnRecv(pSession->_ID, packet);
		CPacket::Free(packet);

		useSize = pSession->_recvBuf.GetUseSize();
	}

	RecvPost(pSession);
	return true;
}

bool CLanServer::RecvPost(CSession* pSession)
{
	DWORD flags = 0;
	DWORD recvBytes = 0;

	int freeSize = pSession->_recvBuf.GetFreeSize();
	pSession->_wsaRecvbuf[0].buf = pSession->_recvBuf.GetWritePtr();
	pSession->_wsaRecvbuf[0].len = pSession->_recvBuf.DirectEnqueueSize();
	pSession->_wsaRecvbuf[1].buf = pSession->_recvBuf.GetFrontPtr();
	pSession->_wsaRecvbuf[1].len = freeSize - pSession->_wsaRecvbuf[0].len;

	ZeroMemory(&pSession->_recvOvl._ovl, sizeof(pSession->_recvOvl._ovl));

	if (!IncrementIOCount(pSession, __LINE__, 0)) return false;
	int recvRet = WSARecv(pSession->_sock, pSession->_wsaRecvbuf,
		dfWSARECVBUF_CNT, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvOvl, NULL);
	//::printf("%lld (%d): Recv Request\n", (pSession->_ID & _idMask), GetCurrentThreadId());

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED && err != WSAENOTSOCK)
			{
				wchar_t* _errMsg = new wchar_t[dfMSG_MAX];
				swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Recv Error, %d\n", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_RECV, _errMsg);
				delete[] _errMsg;
			}

			DecrementIOCount(pSession, __LINE__, 0);
		}
	}

	return true;
}

bool CLanServer::HandleSendCP(__int64 sessionID, int sendBytes)
{
	if (sessionID == -1) return false;
	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= ID_BIT_SIZE;
	CSession* pSession = _sessions[(short)idx];

	if (pSession->_validFlag._releaseFlag == 1) return false;
	if (pSession->_ID != sessionID) return false;

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

bool CLanServer::SendPost(CSession* pSession)
{
	if (pSession->_sendBuf.GetUseSize() == 0) return false;
	if (InterlockedExchange(&pSession->_sendFlag, 1) == 1) return false;
	if (pSession->_sendBuf.GetUseSize() == 0)
	{
		InterlockedExchange(&pSession->_sendFlag, 0);
		return false;
	}

	/*
	for (;;)
	{
		if (pSession->_sendBuf.GetUseSize() == 0) return false;
		if (InterlockedExchange(&pSession->_sendFlag, 1) == 1) continue;
		if (pSession->_sendBuf.GetUseSize() == 0)
		{
			InterlockedExchange(&pSession->_sendFlag, 0);
			return false;
		}
		break;
	}
	*/

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

	if (!IncrementIOCount(pSession, __LINE__, 0)) return false;
	int sendRet = WSASend(pSession->_sock, pSession->_wsaSendbuf,
		idx, &sendBytes, 0, (LPOVERLAPPED)&pSession->_sendOvl, NULL);

	//::printf("%lld (%d): Send Request\n", (pSession->_ID & _idMask), GetCurrentThreadId());

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED && err != WSAENOTSOCK)
			{
				wchar_t* _errMsg = new wchar_t[dfMSG_MAX];
				swprintf_s(_errMsg, dfMSG_MAX, L"%s[%d]: Send Error, %d\n", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_SEND, _errMsg);
				delete[] _errMsg;
			}

			DecrementIOCount(pSession, __LINE__, 0);
			return false;
		}
	}
	return true;
}

bool CLanServer::IncrementIOCount(CSession* pSession, int line, int sessionID)
{
	long debugIdx = InterlockedIncrement(&pSession->_debugIdx);
	pSession->_debugData[debugIdx % dfSESSION_DEBUG_MAX].LeaveLog(
		line, GetCurrentThreadId(), pSession->_validFlag._IOCount, pSession->_validFlag._releaseFlag, pSession->_ID, sessionID);

	if (pSession->_disconnect) return false;
	InterlockedIncrement16(&pSession->_validFlag._IOCount);
	return true;
}

bool CLanServer::DecrementIOCount(CSession* pSession, int line, int sessionID)
{
	long debugIdx = InterlockedIncrement(&pSession->_debugIdx);
	pSession->_debugData[debugIdx % dfSESSION_DEBUG_MAX].LeaveLog(
		line, GetCurrentThreadId(), pSession->_validFlag._IOCount, pSession->_validFlag._releaseFlag, pSession->_ID, sessionID);

	if (InterlockedDecrement16(&pSession->_validFlag._IOCount) == 0)
	{
		PostQueuedCompletionStatus(_hNetworkCP, 1,
			(ULONG_PTR)pSession, (LPOVERLAPPED)&pSession->_releaseOvl);
		return true;
	}
	return false;
}
#endif