#include "CLanServer.h"
#include "ErrorCode.h"
#include <stdio.h>
#include <tchar.h>

bool CLanServer::NetworkInitialize(const wchar_t* IP, short port, int numOfThreads, bool nagle)
{
	// Option Setting ====================================================

	wcscpy_s(_IP, 10, IP);
	_port = port;
	_numOfThreads = numOfThreads;
	_nagle = nagle;

	// Network Setting ===================================================

	_pPacketPool = new CLockFreePool<CPacket>(dfPACKET_DEFAULT, false);

	// Initialize Winsock
	WSADATA wsa;
	int startRet = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (startRet != 0)
	{
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: WSAStartup Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_WSASTARTUP, _msg);		
		return false;
	}

	// Create Listen Sock
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Listen sock is INVALID, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTENSOCK_INVALID, _msg);
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
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Set Linger Option Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_LINGER, _msg);
		return false;
	}

	// Set SndBuf0 Option for Async
	int sndBufSize = 0;
	optRet = setsockopt(_listenSock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(sndBufSize));
	if (optRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Set SendBuf Option Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_SNDBUF_0, _msg);
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
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Listen Sock Bind Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTENSOCK_BIND, _msg);
		return false;
	}

	// Start Listen
	int listenRet = listen(_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Listen Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTEN, _msg);
		return false;
	}

	// Thread Setting ===================================================

	// Create IOCP for Network
	_hNetworkCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hNetworkCP == NULL)
	{
		int err = WSAGetLastError();
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Create IOCP Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_IOCP, _msg);
		return false;
	}

	// Create Accept Thread
	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, nullptr);
	if (_acceptThread == NULL)
	{
		int err = WSAGetLastError();
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Create Accept Thread Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_ACCEPT_THREAD, _msg);
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
			swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Create Network Thread Error, %d\n", _T(__FUNCTION__), __LINE__, err);
			OnError(ERR_CREATE_NETWORK_THREAD, _msg);
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
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: LanServer Already Terminate\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_ALREADY_TERMINATE, _msg);
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
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Temp Socket is INVALID, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_INVALID, _msg);
		return false;
	}

	int connectRet = connect(socktmp, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (connectRet == SOCKET_ERROR)
	{
		int err = ::WSAGetLastError();
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Temp Socket Connect Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_CONNECT, _msg);
		return false;
	}
	WaitForSingleObject(_acceptThread, INFINITE);

	// Release All Session
	closesocket(socktmp);
	closesocket(_listenSock);
	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		CSession* pSession = _sessions[dfSESSION_MAX];
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

// TO-DO
bool CLanServer::Disconnect(__int64 sessionID)
{
	int idx = sessionID & _idxKey;	
	CSession* pSession = _sessions[idx];

	// TO-DO

	return false;	
}

bool CLanServer::SendPacket(__int64 sessionID, CPacket* packet)
{
	int idx = sessionID & _idxKey;
	CSession* pSession = _sessions[idx];

	if (packet->IsHeaderEmpty())
	{
		int payloadSize = packet->GetPayloadSize();
		stHeader header;
		header.Len = payloadSize;

		int putRet = packet->PutHeaderData((char*)&header, dfHEADER_LEN);
		if (putRet != dfHEADER_LEN)
		{
			swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: CPacket PutHeaderData Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_PACKET_PUT_HEADER, _msg);
			return false;
		}
	}

	int packetSize = packet->GetPacketSize();
	int enqueueRet = pSession->_sendBuf.Enqueue(packet->GetPacketReadPtr(), packetSize);
	if (enqueueRet != packetSize)
	{
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Send Buffer Enqueue Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_SENDBUF_ENQ, _msg);
		return false;
	}

	SendPost(pSession);
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
			swprintf_s(pLanServer->_msg, dfMSG_MAX, L"%s[%d]: Accept Error\n", _T(__FUNCTION__), __LINE__);
			pLanServer->OnError(ERR_ACCEPT, pLanServer->_msg);
			break;
		}

		if (pLanServer-> _networkAlive == 1) break;

		if (pLanServer->GetSessionCount() >= dfSESSION_MAX)
		{
			closesocket(client_sock);
			continue;
		}

		__int64 sessionID = InterlockedIncrement64(&pLanServer->_sessionID);
		sessionID &= pLanServer->_idKey;
		int idx = 0;

		// TO-DO
		// Get Empty Session Idx
		// sessionID |= idx;

		CSession* pSession = pLanServer->_sessions[idx];
		pSession->Initialize(sessionID, client_sock, clientaddr);
		pLanServer->_acceptCnt++;

		// Connect Session to IOCP and Post Recv                                      
		CreateIoCompletionPort((HANDLE)pSession->_sock, pLanServer->_hNetworkCP, (ULONG_PTR)pSession, 0);

		pLanServer->RecvPost(pSession);
		pLanServer->OnAcceptClient(pSession->_ID);
	}

	swprintf_s(pLanServer->_msg, dfMSG_MAX, L"Accept Thread (%d)\n", GetCurrentThreadId());
	pLanServer->OnThreadTerminate(pLanServer->_msg);
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
		if (pNetOvl->_type == NET_TYPE::RELEASE)
		{
			pLanServer->ReleaseSession(pSession->_ID);
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
					swprintf_s(pLanServer->_msg, dfMSG_MAX, L"%s[%d]: GQCS return 0, %d\n", _T(__FUNCTION__), __LINE__, err);
					pLanServer->OnError(ERR_GQCS_RET0, pLanServer->_msg);
				}
			}
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

		if (InterlockedDecrement(&pSession->_IOCount) == 0)
		{
			PostQueuedCompletionStatus(pLanServer->_hNetworkCP, 1,
				(ULONG_PTR)pSession->_ID, (LPOVERLAPPED)&pSession->_releaseOvl);
		}
	}

	swprintf_s(pLanServer->_msg, dfMSG_MAX, L"Network Thread (%d)\n", threadID);
	pLanServer->OnThreadTerminate(pLanServer->_msg);

	return 0;
}

bool CLanServer::ReleaseSession(__int64 sessionID)
{
	// TO-DO: Session Validation Check
	int idx = sessionID & _idxKey;
	CSession* pSession = _sessions[idx];

	if (InterlockedExchange(&pSession->_socketAlive, 1) != 0)
	{
		return false;
	}

	closesocket(pSession->_sock);
	delete pSession;
	_disconnectCnt++;

	OnReleaseClient(sessionID);
}

bool CLanServer::HandleRecvCP(__int64 sessionID, int recvBytes)
{
	// TO-DO: Session Validation Check
	int idx = sessionID & _idxKey;
	CSession* pSession = _sessions[idx];
	int moveReadRet = pSession->_recvBuf.MoveWritePos(recvBytes);
	if (moveReadRet != recvBytes)
	{
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Recv Buffer MoveWritePos Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_RECVBUF_MOVEWRITEPOS, _msg);
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
			swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Recv Buffer Peek Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_PEEK, _msg);
			return false;
		}

		if (useSize < dfHEADER_LEN + header.Len) break;

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_LEN);
		if (moveReadRet != dfHEADER_LEN)
		{
			swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, _msg);
			return false;
		}

		CPacket* packet = _pPacketPool->Alloc();
		packet->Clear();
		int dequeueRet = pSession->_recvBuf.Dequeue(packet->GetPayloadWritePtr(), header.Len);
		if (dequeueRet != header.Len)
		{
			swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Recv Buffer Dequeue Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_DEQ, _msg);
			return false;
		}

		packet->MovePayloadWritePos(dequeueRet);
		OnRecv(pSession->_ID, packet);
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
	InterlockedIncrement(&pSession->_IOCount);

	int recvRet = WSARecv(pSession->_sock, pSession->_wsaRecvbuf,
		2, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvOvl, NULL);

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED)
			{
				swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Recv Error, %d\n", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_RECV, _msg);
			}

			if (InterlockedDecrement(&pSession->_IOCount) == 0)
			{
				PostQueuedCompletionStatus(_hNetworkCP, 1,
					(ULONG_PTR)pSession->_ID, (LPOVERLAPPED)&pSession->_releaseOvl);
			}

			return false;
		}
	}

	return true;
}

bool CLanServer::HandleSendCP(__int64 sessionID, int sendBytes)
{
	// TO-DO: Session Validation Check
	int idx = sessionID & _idxKey;
	CSession* pSession = _sessions[idx];
	int moveReadRet = pSession->_sendBuf.MoveReadPos(sendBytes);
	if (moveReadRet != sendBytes)
	{
		swprintf_s(_msg, dfMSG_MAX, L"%s[%d]:Send Buffer MoveReadPos Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_RECVBUF_MOVEREADPOS, _msg);
		return false;
	}

	OnSend(pSession->_ID, sendBytes);
	InterlockedExchange(&pSession->_sendFlag, 0);
	SendPost(pSession);

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

	DWORD sendBytes;
	int useSize = pSession->_sendBuf.GetUseSize();
	pSession->_wsaSendbuf[0].buf = pSession->_sendBuf.GetReadPtr();
	pSession->_wsaSendbuf[0].len = pSession->_sendBuf.DirectDequeueSize();
	pSession->_wsaSendbuf[1].buf = pSession->_sendBuf.GetFrontPtr();
	pSession->_wsaSendbuf[1].len = useSize - pSession->_wsaSendbuf[0].len;

	ZeroMemory(&pSession->_sendOvl._ovl, sizeof(pSession->_sendOvl._ovl));
	InterlockedIncrement(&pSession->_IOCount);

	int sendRet = WSASend(pSession->_sock, pSession->_wsaSendbuf,
		2, &sendBytes, 0, (LPOVERLAPPED)&pSession->_sendOvl, NULL);

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED)
			{
				swprintf_s(_msg, dfMSG_MAX, L"%s[%d]: Send Error, %d\n", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_SEND, _msg);
			}

			if (InterlockedDecrement(&pSession->_IOCount) == 0)
			{
				PostQueuedCompletionStatus(_hNetworkCP, 1,
					(ULONG_PTR)pSession->_ID, (LPOVERLAPPED)&pSession->_releaseOvl);
			}

			return false;
		}
	}

	return true;
}
