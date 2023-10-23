#include "CLanServer.h"
#include "ErrorCode.h"
#include <stdio.h>
#include <tchar.h>

__declspec (thread) wchar_t g_lanErrMsg[dfMSG_MAX] = { '\0' };
#define ID_BIT_SIZE 49

CLanServer::CLanServer()
{
	_indexMask = 0b111111111111111; 
	_indexMask <<= ID_BIT_SIZE;
	_idMask = ~_indexMask;

	for (int i = 0; i < dfSESSION_MAX; i++)
		_emptyIndex[i] = i;
}


bool CLanServer::NetworkInitialize(const wchar_t* IP, short port, int numOfThreads, bool nagle)
{
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
		swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: WSAStartup Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_WSASTARTUP, g_lanErrMsg);		
		return false;
	}

	// Create Listen Sock
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Listen sock is INVALID, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTENSOCK_INVALID, g_lanErrMsg);
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
		swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Set Linger Option Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_LINGER, g_lanErrMsg);
		return false;
	}

	// Set SndBuf0 Option for Async
	int sndBufSize = 0;
	optRet = setsockopt(_listenSock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(sndBufSize));
	if (optRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Set SendBuf Option Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_SNDBUF_0, g_lanErrMsg);
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
		swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Listen Sock Bind Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTENSOCK_BIND, g_lanErrMsg);
		return false;
	}

	// Start Listen
	int listenRet = listen(_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Listen Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTEN, g_lanErrMsg);
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
		swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Create IOCP Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_IOCP, g_lanErrMsg);
		return false;
	}

	// Create Accept Thread
	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, nullptr);
	if (_acceptThread == NULL)
	{
		int err = WSAGetLastError();
		swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Create Accept Thread Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_ACCEPT_THREAD, g_lanErrMsg);
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
			swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Create Network Thread Error, %d\n", _T(__FUNCTION__), __LINE__, err);
			OnError(ERR_CREATE_NETWORK_THREAD, g_lanErrMsg);
			return false;
		}
	}

	OnInitialize();
	return true;
}

bool CLanServer::NetworkTerminate()
{
	if (InterlockedExchange(& _networkAlive, 1) != 0)
	{
		swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: LanServer Already Terminate\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_ALREADY_TERMINATE, g_lanErrMsg);
		return false;
	}

	// Terminate Network Threads
	for (int i = 0; i < _numOfThreads; i++)
		PostQueuedCompletionStatus(_hNetworkCP, 0, 0, 0);
	WaitForMultipleObjects(_numOfThreads, _networkThreads, true, INFINITE);

	// Terminate Accept Thread
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	InetPton(AF_INET, _IP, &serveraddr.sin_addr);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(_port);

	// Awake Accept Thread if It's blocked
	SOCKET socktmp = socket(AF_INET, SOCK_STREAM, 0);
	if (socktmp == INVALID_SOCKET)
	{
		int err = ::WSAGetLastError();
		swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Temp Socket is INVALID, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_INVALID, g_lanErrMsg);
		return false;
	}

	int connectRet = connect(socktmp, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (connectRet == SOCKET_ERROR)
	{
		int err = ::WSAGetLastError();
		swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Temp Socket Connect Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_CONNECT, g_lanErrMsg);
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

bool CLanServer::Disconnect(__int64 sessionID)
{
	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= ID_BIT_SIZE;
	CSession* pSession = _sessions[(short)idx];

	if (pSession->_validFlag._releaseFlag == 1) return false;
	if (pSession->_ID != sessionID) return false;

	bool ret = false;
	while (!ret) ret = DecrementIOCount(pSession);
	pSession->_disconnect = true;

	long debugIdx = InterlockedIncrement(&pSession->_debugIdx);
	pSession->_debugData[debugIdx % dfSESSION_DEBUG_MAX].LeaveLog(
		0, pSession->_validFlag._IOCount, pSession->_validFlag._releaseFlag, pSession->_ID, sessionID);

	return true;
}

bool CLanServer::SendPacket(__int64 sessionID, CPacket* packet)
{
	// ::printf("%lld: SendPacket Try\n", (sessionID & _idMask));

	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= ID_BIT_SIZE;
	CSession* pSession = _sessions[(short)idx];

	if (!IncrementIOCount(pSession)) return false;
	if (pSession->_validFlag._releaseFlag == 1) 
	{
		DecrementIOCount(pSession);
		return false;
	}
	if (pSession->_ID != sessionID) 
	{
		DecrementIOCount(pSession);
		return false;
	}

	if (packet->IsHeaderEmpty())
	{
		short payloadSize = packet->GetPayloadSize();
		stHeader header;
		header.Len = payloadSize;

		int putRet = packet->PutHeaderData((char*)&header, dfHEADER_LEN);
		if (putRet != dfHEADER_LEN)
		{
			DecrementIOCount(pSession);
			swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: CPacket PutHeaderData Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_PACKET_PUT_HEADER, g_lanErrMsg);
			return false;
		}
	}
	
	pSession->_sendBuf.Enqueue(packet);
	if (!SendPost(pSession)) 
	{
		DecrementIOCount(pSession);
		return false;
	}

	return true;
}

unsigned int __stdcall CLanServer::AcceptThread(void* arg)
{
	CLanServer* pLanServer = (CLanServer*)arg;
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);

	for (;;)
	{
		// Accept
		SOCKET client_sock = accept(pLanServer->_listenSock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Accept Error\n", _T(__FUNCTION__), __LINE__);
			pLanServer->OnError(ERR_ACCEPT, g_lanErrMsg);
			break;
		}

		if (pLanServer-> _networkAlive == 1) break;

		long sessionCnt = InterlockedIncrement(&pLanServer->_sessionCnt);
		if (sessionCnt > dfSESSION_MAX)
		{
			closesocket(client_sock);
			continue;
		}

		__int64 sessionID = InterlockedIncrement64(&pLanServer->_sessionID);
		sessionID &= pLanServer->_idMask;

		long emptyIndexPos = InterlockedIncrement(&pLanServer->_emptyIndexPos);
		if (emptyIndexPos >= dfSESSION_MAX)
		{
			closesocket(client_sock);
			continue;
		}

		unsigned __int64 idx = pLanServer->_emptyIndex[emptyIndexPos];
		unsigned __int64 sessionIdx = idx << ID_BIT_SIZE;
		sessionID |= sessionIdx;

		CSession* pSession = pLanServer->_sessions[(short)idx]; 
		pSession->Initialize(sessionID, client_sock, clientaddr);
		pLanServer->_acceptCnt++;

		long debugIdx = InterlockedIncrement(&pSession->_debugIdx);
		pSession->_debugData[debugIdx % dfSESSION_DEBUG_MAX].LeaveLog(
			1, pSession->_validFlag._IOCount, pSession->_validFlag._releaseFlag, pSession->_ID, sessionID);

		// ::printf("%lld: Accept Success\n", (sessionID & pLanServer->_idMask));

		// Connect Session to IOCP and Post Recv                                      
		CreateIoCompletionPort((HANDLE)pSession->_sock, pLanServer->_hNetworkCP, (ULONG_PTR)pSession, 0);
		
		pLanServer->IncrementIOCount(pSession);
		pLanServer->RecvPost(pSession);
		pLanServer->OnAcceptClient(pSession->_ID);
	}

	swprintf_s(g_lanErrMsg, dfMSG_MAX, L"Accept Thread (%d)\n", GetCurrentThreadId());
	pLanServer->OnThreadTerminate(g_lanErrMsg);
	return 0;
}

unsigned int __stdcall CLanServer::NetworkThread(void* arg)
{
	CLanServer* pLanServer = (CLanServer*)arg;
	int threadID = GetCurrentThreadId();

	for (;;)
	{
		CSession* pSession;
		DWORD cbTransferred;
		NetworkOverlapped* pNetOvl = new NetworkOverlapped;
		int GQCSRet = GetQueuedCompletionStatus(pLanServer->_hNetworkCP,
			&cbTransferred, (PULONG_PTR)&pSession, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (pLanServer-> _networkAlive == 1) break;
		
		// Check Exception		
		if (GQCSRet == 0 || cbTransferred == 0)
		{
			if (GQCSRet == 0)
			{
				int err = WSAGetLastError();
				if (err != WSAECONNRESET && err != WSAECONNABORTED &&
					err != ERROR_CONNECTION_ABORTED && err != ERROR_NETNAME_DELETED && err != ERROR_OPERATION_ABORTED)
				{
					swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: GQCS return 0, %d\n", _T(__FUNCTION__), __LINE__, err);
					pLanServer->OnError(ERR_GQCS_RET0, g_lanErrMsg);
				}
			}
		}
		else if (pNetOvl->_type == NET_TYPE::RELEASE)
		{
			pLanServer->ReleaseSession(pSession->_ID);
			continue;
		}
		else if (pNetOvl->_type == NET_TYPE::RECV)
		{
			pLanServer->HandleRecvCP(pSession->_ID, cbTransferred);
			pLanServer->_recvMsgCnt++;
		}
		else if (pNetOvl->_type == NET_TYPE::SEND)
		{
			pLanServer->HandleSendCP(pSession->_ID, cbTransferred);
			pLanServer->_sendMsgCnt++;
		}

		pLanServer->DecrementIOCount(pSession);
	}

	swprintf_s(g_lanErrMsg, dfMSG_MAX, L"Network Thread (%d)\n", threadID);
	pLanServer->OnThreadTerminate(g_lanErrMsg);

	return 0;
}

bool CLanServer::ReleaseSession(__int64 sessionID)
{
	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= ID_BIT_SIZE;
	CSession* pSession = _sessions[(short)idx];

	if (InterlockedCompareExchange(&pSession->_validFlag._flag, 1, 0) != 0) 
	{
		return false;
	}
	if (pSession->_ID != sessionID) return false;

	long debugIdx = InterlockedIncrement(&pSession->_debugIdx);
	pSession->_debugData[debugIdx % dfSESSION_DEBUG_MAX].LeaveLog(
		2, pSession->_validFlag._IOCount, pSession->_validFlag._releaseFlag, pSession->_ID, sessionID);

	closesocket(pSession->_sock);
	pSession->Terminate();
	_disconnectCnt++;

	long emptyIndexPos = InterlockedDecrement(&_emptyIndexPos);
	emptyIndexPos++;
	if (emptyIndexPos < 0) // Debug
	{		
		::printf("emptyIndexPos == -1\n"); 
		__debugbreak();
	}

	_emptyIndex[emptyIndexPos] = (int)idx;
	InterlockedDecrement(&_sessionCnt);

	OnReleaseClient(sessionID);
	return true;
}

bool CLanServer::HandleRecvCP(__int64 sessionID, int recvBytes)
{
	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= ID_BIT_SIZE;
	CSession* pSession = _sessions[(short)idx];

	if (!IncrementIOCount(pSession)) return false;
	if (pSession->_validFlag._releaseFlag == 1) 
	{
		DecrementIOCount(pSession);
		return false;
	}
	if (pSession->_ID != sessionID) 
	{
		DecrementIOCount(pSession);
		return false;
	}

	int moveReadRet = pSession->_recvBuf.MoveWritePos(recvBytes);
	if (moveReadRet != recvBytes)
	{
		DecrementIOCount(pSession);
		swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer MoveWritePos Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_RECVBUF_MOVEWRITEPOS, g_lanErrMsg);
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
			DecrementIOCount(pSession);
			swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer Peek Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_PEEK, g_lanErrMsg);
			return false;
		}

		if (useSize < dfHEADER_LEN + header.Len) break;

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_LEN);
		if (moveReadRet != dfHEADER_LEN)
		{
			DecrementIOCount(pSession);
			swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, g_lanErrMsg);
			return false;
		}

		CPacket* packet = new CPacket;
		packet->Clear();
		int dequeueRet = pSession->_recvBuf.Dequeue(packet->GetPayloadWritePtr(), header.Len);
		if (dequeueRet != header.Len)
		{
			DecrementIOCount(pSession);
			swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer Dequeue Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_DEQ, g_lanErrMsg);
			return false;
		}

		packet->MovePayloadWritePos(dequeueRet);
		OnRecv(pSession->_ID, packet);
		delete packet;
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

	int recvRet = WSARecv(pSession->_sock, pSession->_wsaRecvbuf,
		dfWSARECVBUF_CNT, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvOvl, NULL);
	// ::printf("%lld: Recv Try\n", (pSession->_ID & _idMask));

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED)
			{
				swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Recv Error, %d\n", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_RECV, g_lanErrMsg);
			}

			DecrementIOCount(pSession);
		}
	}

	return true;
}

bool CLanServer::HandleSendCP(__int64 sessionID, int sendBytes)
{
	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= ID_BIT_SIZE;
	CSession* pSession = _sessions[(short)idx];

	if (!IncrementIOCount(pSession)) return false;
	if (pSession->_validFlag._releaseFlag == 1) 
	{
		DecrementIOCount(pSession);
		return false;
	}
	if (pSession->_ID != sessionID) 
	{
		DecrementIOCount(pSession);
		return false;
	}

	for (int i = 0; i < pSession->_sendCount; i++)
	{
		CPacket* packet = pSession->_tempBuf.Dequeue();
		if (packet == nullptr) break;
		if (packet->DecrementUsageCount() == 0) delete packet;
	}

	OnSend(pSession->_ID, sendBytes);
	InterlockedExchange(&pSession->_sendFlag, 0);
	if (!SendPost(pSession))
	{
		DecrementIOCount(pSession);
		return false;
	}

	return true;
}

bool CLanServer::SendPost(CSession* pSession)
{
	if (pSession->_sendBuf.GetUseSize() == 0)  return false;
	if (InterlockedExchange(&pSession->_sendFlag, 1) == 1) return false;
	if (pSession->_sendBuf.GetUseSize() == 0)
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
	int sendRet = WSASend(pSession->_sock, pSession->_wsaSendbuf,
		idx, &sendBytes, 0, (LPOVERLAPPED)&pSession->_sendOvl, NULL);

	// ::printf("%lld: Send Try\n", (pSession->_ID & _idMask));

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED)
			{
				swprintf_s(g_lanErrMsg, dfMSG_MAX, L"%s[%d]: Send Error, %d\n", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_SEND, g_lanErrMsg);
			}

			DecrementIOCount(pSession);
		}
	}
	return true;
}

bool CLanServer::IncrementIOCount(CSession* pSession)
{
	if (pSession->_disconnect) return false;

	InterlockedIncrement16(&pSession->_validFlag._IOCount);
	return true;
}

bool CLanServer::DecrementIOCount(CSession* pSession)
{
	if (pSession->_disconnect) return false;

	if (InterlockedDecrement16(&pSession->_validFlag._IOCount) == 0)
	{
		PostQueuedCompletionStatus(_hNetworkCP, 1,
			(ULONG_PTR)pSession, (LPOVERLAPPED)&pSession->_releaseOvl);
		return true;
	}
	return false;
}