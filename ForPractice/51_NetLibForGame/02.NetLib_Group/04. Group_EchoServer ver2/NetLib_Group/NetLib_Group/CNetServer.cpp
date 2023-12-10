#include "CNetServer.h"
#ifdef NETSERVER

#include "ErrorCode.h"
#include "CGroup.h"
#include <stdio.h>
#include <tchar.h>

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>

// 17 bit for Idx (MAX 131072)
// 47 bit for Id (MAX 140737488355328)

CNetServer::CNetServer()
{
	_indexMask = 0b11111111111111111;
	_indexMask <<= __ID_BIT__;
	_idMask = ~_indexMask;

	_releaseFlag._useCount = 0;
	_releaseFlag._releaseFlag = 1;
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
		wchar_t stErrMsg[dfMSG_MAX];
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: WSAStartup Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_WSASTARTUP, stErrMsg);
		return false;
	}

	// Create Listen Sock
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		wchar_t stErrMsg[dfMSG_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Listen sock is INVALID, %d", _T(__FUNCTION__), __LINE__, err);
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
		wchar_t stErrMsg[dfMSG_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Set Linger Option Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_LINGER, stErrMsg);
		return false;
	}

	// Set SndBuf0 Option for Async
	int sndBufSize = 0;
	optRet = setsockopt(_listenSock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(sndBufSize));
	if (optRet == SOCKET_ERROR)
	{
		wchar_t stErrMsg[dfMSG_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Set SendBuf Option Error, %d", _T(__FUNCTION__), __LINE__, err);
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
		wchar_t stErrMsg[dfMSG_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Listen Sock Bind Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTENSOCK_BIND, stErrMsg);
		return false;
	}

	// Start Listen
	int listenRet = listen(_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		wchar_t stErrMsg[dfMSG_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Listen Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTEN, stErrMsg);
		return false;
	}

	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		_emptyIdx.Push(dfSESSION_MAX - i);
		_sessions[i] = new CSession;
	}

	// Thread Setting ===================================================

	// Create IOCP for Network
	_hNetworkCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hNetworkCP == NULL)
	{
		wchar_t stErrMsg[dfMSG_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Create IOCP Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_IOCP, stErrMsg);
		return false;
	}

	// Create Accept Thread
	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, nullptr);
	if (_acceptThread == NULL)
	{
		wchar_t stErrMsg[dfMSG_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Create Accept Thread Error, %d", _T(__FUNCTION__), __LINE__, err);
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
			wchar_t stErrMsg[dfMSG_MAX];
			int err = WSAGetLastError();
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Create Network Thread Error, %d", _T(__FUNCTION__), __LINE__, err);
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
		wchar_t stErrMsg[dfMSG_MAX];
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: NetServer Already Terminate", _T(__FUNCTION__), __LINE__);
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
		wchar_t stErrMsg[dfMSG_MAX];
		int err = ::WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Socket for Wake is INVALID, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_INVALID, stErrMsg);
		return false;
	}

	int connectRet = connect(socktmp, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (connectRet == SOCKET_ERROR)
	{
		wchar_t stErrMsg[dfMSG_MAX];
		int err = ::WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Socket for Wake Connect Error, %d", _T(__FUNCTION__), __LINE__, err);
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
			wchar_t stErrMsg[dfMSG_MAX];
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Accept Error", _T(__FUNCTION__), __LINE__);
			pNetServer->OnError(ERR_ACCEPT, stErrMsg);
			break;
		}

		if (pNetServer->_networkAlive == 1) break;

		InterlockedIncrement(&pNetServer->_acceptCnt);
		long sessionCnt = InterlockedIncrement(&pNetServer->_sessionCnt);

		if (sessionCnt > dfSESSION_MAX)
		{
			closesocket(client_sock);
			InterlockedIncrement(&pNetServer->_disconnectCnt);
			InterlockedDecrement(&pNetServer->_sessionCnt);

			wchar_t stErrMsg[dfMSG_MAX];
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Session Max", _T(__FUNCTION__), __LINE__);
			pNetServer->OnError(DEB_SESSION_MAX, stErrMsg);
			continue;
		}

		if (pNetServer->_emptyIdx.GetUseSize() == 0)
		{
			closesocket(client_sock);
			InterlockedIncrement(&pNetServer->_disconnectCnt);
			InterlockedDecrement(&pNetServer->_sessionCnt);

			wchar_t stErrMsg[dfMSG_MAX];
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: No Empty Index", _T(__FUNCTION__), __LINE__);
			pNetServer->OnError(DEB_SESSION_MAX, stErrMsg);
			continue;
		}

		WCHAR addr[dfADDRESS_LEN] = { L'0' };
		DWORD size = sizeof(addr);
		WSAAddressToStringW((SOCKADDR*)&clientaddr, sizeof(clientaddr), NULL, addr, &size);

		// pNetServer->OnConnectRequest(addr);

		unsigned __int64 sessionID = InterlockedIncrement64(&pNetServer->_sessionID);
		sessionID &= pNetServer->_idMask;
		long idx = pNetServer->_emptyIdx.Pop();
		unsigned __int64 sessionIdx = ((unsigned __int64)idx) << __ID_BIT__;
		sessionID |= sessionIdx;
		CSession* pSession = pNetServer->_sessions[(long)idx];

		pNetServer->IncrementUseCount(pSession);
		pSession->Initialize(sessionID, client_sock, clientaddr);

		// ::printf("%d: Accept Success (%016llx - %016llx)\n", GetCurrentThreadId(), sessionID, pSession->GetID());

		CreateIoCompletionPort((HANDLE)pSession->_sock, pNetServer->_hNetworkCP, (ULONG_PTR)pSession->GetID(), 0);
		pNetServer->RecvPost(pSession);
		pNetServer->OnAcceptClient(sessionID, addr);
		pNetServer->DecrementUseCount(pSession);
	}

	wchar_t stErrMsg[dfMSG_MAX];
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
		__int64 sessionID;
		DWORD cbTransferred;

		int GQCSRet = GetQueuedCompletionStatus(pNetServer->_hNetworkCP,
			&cbTransferred, (PULONG_PTR)&sessionID, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (pNetServer->_networkAlive == 1) break;
		if (pNetOvl->_type == NET_TYPE::RELEASE)
		{
			pNetServer->HandleRelease(sessionID);
			continue;
		}

		CSession* pSession = pNetServer->AcquireSessionUsage(sessionID);
		if (pSession == nullptr) continue;

		// Check Exception
		if (GQCSRet == 0 || cbTransferred == 0)
		{
			if (GQCSRet == 0)
			{
				int err = WSAGetLastError();
				if (err != WSAECONNRESET && err != WSAECONNABORTED &&
					err != WSAENOTSOCK && err != WSAEINTR &&
					err != ERROR_CONNECTION_ABORTED && err != ERROR_NETNAME_DELETED
					&& err != ERROR_OPERATION_ABORTED && err != ERROR_SEM_TIMEOUT)
				{
					wchar_t stErrMsg[dfMSG_MAX];
					swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: GQCS return 0, %d", _T(__FUNCTION__), __LINE__, err);
					pNetServer->OnError(ERR_GQCS_RET0, stErrMsg);
				}
			}
		}
		else if (pNetOvl->_type == NET_TYPE::RECV)
		{
			// ::printf("%016llx (%d): Recv Success\n", sessionID, GetCurrentThreadId());
			pNetServer->HandleRecvCP(pSession, cbTransferred);
		}
		else if (pNetOvl->_type == NET_TYPE::SEND)
		{
			// ::printf("%016llx (%d): Send Success\n", sessionID, GetCurrentThreadId());
			pNetServer->HandleSendCP(pSession, cbTransferred);
		}

		pNetServer->DecrementUseCount(pSession);
		pNetServer->ReleaseSessionUsage(pSession);
	}

	delete pNetOvl;
	wchar_t stErrMsg[dfMSG_MAX];
	swprintf_s(stErrMsg, dfMSG_MAX, L"Network Thread (%d)", threadID);
	pNetServer->OnThreadTerminate(stErrMsg);

	return 0;
}

bool CNetServer::Disconnect(unsigned __int64 sessionID)
{
	CSession* pSession = AcquireSessionUsage(sessionID);
	if (pSession == nullptr) return false;

	// ::printf("%016llx (%d): CNetServer::%s\n", sessionID, GetCurrentThreadId(), __func__);

	pSession->_disconnect = true;
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_recvOvl);
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_sendOvl);

	ReleaseSessionUsage(pSession);
	return true;
}

bool CNetServer::SendPacket(unsigned __int64 sessionID, CPacket* packet, bool disconnect)
{
	CSession* pSession = AcquireSessionUsage(sessionID);
	if (pSession == nullptr) return false;

	// ::printf("%016llx (%d): CNetServer::%s\n", sessionID, GetCurrentThreadId(), __func__);

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
			wchar_t stErrMsg[dfMSG_MAX];
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: CPacket PutHeaderData Error", _T(__FUNCTION__), __LINE__);
			OnError(ERR_PACKET_PUT_HEADER, stErrMsg);
			ReleaseSessionUsage(pSession);
			return false;
		}
	}

	pSession->_sendBuf.Enqueue(packet);
	SendPost(pSession);

	ReleaseSessionUsage(pSession);
	return true;
}

bool CNetServer::MoveGroup(unsigned __int64 sessionID, CGroup* pGroup)
{
	CSession* pSession = AcquireSessionUsage(sessionID);
	if (pSession == nullptr) return false;

	EnterCriticalSection(&pSession->_groupLock);
	pSession->_pGroup = pGroup;
	if (pGroup != nullptr)
		pGroup->_enterSessions.Enqueue(pSession->GetID());
	LeaveCriticalSection(&pSession->_groupLock);

	ReleaseSessionUsage(pSession);
	return true;
}

bool CNetServer::RegisterGroup(CGroup* pGroup)
{
	HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, pGroup->UpdateThread, (void*)pGroup, 0, nullptr);
	if (thread == NULL)
	{
		wchar_t stErrMsg[dfMSG_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Create Group Thread Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_GROUP_THREAD, stErrMsg);
		return false;
	}
	_groupThreads.insert(make_pair(pGroup, thread));
	return true;
}

bool CNetServer::RemoveGroup(CGroup* pGroup)
{
	pGroup->SetDead();
	unordered_map<CGroup*, HANDLE>::iterator it = _groupThreads.find(pGroup);
	HANDLE thread = it->second;
	WaitForSingleObject(thread, INFINITE);
	_groupThreads.erase(it);
	return false;
}

void CNetServer::HandleRecvCP(CSession* pSession, int recvBytes)
{
	int moveWriteRet = pSession->_recvBuf.MoveWritePos(recvBytes);
	if (moveWriteRet != recvBytes)
	{
		wchar_t stErrMsg[dfMSG_MAX];
		Disconnect(pSession->GetID());
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer MoveWritePos Error", _T(__FUNCTION__), __LINE__);
		OnError(ERR_RECVBUF_MOVEWRITEPOS, stErrMsg);
		return;
	}

	int useSize = pSession->_recvBuf.GetUseSize();

	while (useSize > dfHEADER_LEN)
	{
		stHeader header;
		int peekRet = pSession->_recvBuf.Peek((char*)&header, dfHEADER_LEN);
		if (peekRet != dfHEADER_LEN)
		{
			wchar_t stErrMsg[dfMSG_MAX];
			Disconnect(pSession->GetID());
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer Peek Error", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_PEEK, stErrMsg);
			return;
		}

		if (header._code != dfPACKET_CODE)
		{
			wchar_t stErrMsg[dfMSG_MAX];
			Disconnect(pSession->GetID());
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Wrong Packet Code", _T(__FUNCTION__), __LINE__);
			OnDebug(DEB_WRONG_PACKETCODE, stErrMsg);
			return;
		}

		// TO-DO: 잘못된 패킷이 올 경우 대처 필요
		if (dfHEADER_LEN + header._len > pSession->_recvBuf.GetUseSize())
			break;

		int moveReadRet = pSession->_recvBuf.MoveReadPos(dfHEADER_LEN);
		if (moveReadRet != dfHEADER_LEN)
		{
			wchar_t stErrMsg[dfMSG_MAX];
			Disconnect(pSession->GetID());
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, stErrMsg);
			return;
		}

		CPacket* packet = CPacket::Alloc();
		packet->AddUsageCount(1);

		int dequeueRet = pSession->_recvBuf.Dequeue(packet->GetPayloadWritePtr(), header._len);
		if (dequeueRet != header._len)
		{
			Disconnect(pSession->GetID());
			CPacket::Free(packet);

			wchar_t stErrMsg[dfMSG_MAX];
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Buffer Dequeue Error", _T(__FUNCTION__), __LINE__);
			OnDebug(DEB_WRONG_PACKETLEN, stErrMsg);
			return;
		}
		packet->MovePayloadWritePos(dequeueRet);

		if (packet->Decode(header) == false)
		{
			Disconnect(pSession->GetID());
			CPacket::Free(packet);

			wchar_t stErrMsg[dfMSG_MAX];
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Wrong Checksum", _T(__FUNCTION__), __LINE__);
			OnDebug(DEB_WRONG_DECODE, stErrMsg);
			return;
		}

		EnterCriticalSection(&pSession->_groupLock);
		if (pSession->_pGroup == nullptr)
		{
			OnRecv(pSession->GetID(), packet);
			CPacket::Free(packet);
		}
		else
		{
			pSession->_OnRecvQ.Enqueue(packet);
			InterlockedIncrement(&pSession->_pGroup->_recvCnt);
		}
		LeaveCriticalSection(&pSession->_groupLock);

		useSize = pSession->_recvBuf.GetUseSize();
		InterlockedIncrement(&_recvCnt);
	}

	RecvPost(pSession);
}

void CNetServer::HandleSendCP(CSession* pSession, int sendBytes)
{
	int i = 0;
	for (; i < pSession->_sendCount; i++)
	{
		CPacket* packet = pSession->_tempBuf.Dequeue();
		if (packet == nullptr) break;
		CPacket::Free(packet);
	}

	EnterCriticalSection(&pSession->_groupLock);
	if (pSession->_pGroup == nullptr)
	{
		OnSend(pSession->GetID(), sendBytes);
	}
	else
	{
		pSession->_OnSendQ.Enqueue(sendBytes);
		InterlockedAdd(&pSession->_pGroup->_sendCnt, i);
	}
	LeaveCriticalSection(&pSession->_groupLock);
	
	InterlockedAdd(&_sendCnt, i);
	InterlockedExchange(&pSession->_sendFlag, 0);
	SendPost(pSession);
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

	if (pSession->_disconnect)
	{
		return false;
	}

	IncrementUseCount(pSession);
	int recvRet = WSARecv(pSession->_sock, pSession->_wsaRecvbuf,
		dfWSARECVBUF_CNT, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvOvl, NULL);

	// ::printf("%016llx (%d): Recv Request\n", pSession->GetID(), GetCurrentThreadId());

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED && err != WSAEINTR)
			{
				wchar_t stErrMsg[dfMSG_MAX];
				swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Recv Error, %d", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_RECV, stErrMsg);
			}
			DecrementUseCount(pSession);
			return false;
		}
		else if (pSession->_disconnect)
		{
			CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_recvOvl);
			return false;
		}
	}
	return true;
}

bool CNetServer::SendPost(CSession* pSession)
{
	if (pSession->_sendBuf.GetUseSize() == 0) return false;
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

	if (pSession->_disconnect)
	{
		InterlockedExchange(&pSession->_sendFlag, 0);
		return false;
	}

	IncrementUseCount(pSession);
	int sendRet = WSASend(pSession->_sock, pSession->_wsaSendbuf,
		idx, &sendBytes, 0, (LPOVERLAPPED)&pSession->_sendOvl, NULL);

	// ::printf("%016llx (%d): Send Request\n", pSession->GetID(), GetCurrentThreadId());

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED && err != WSAEINTR)
			{
				wchar_t stErrMsg[dfMSG_MAX];
				swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: Send Error, %d", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_SEND, stErrMsg);
			}
			InterlockedExchange(&pSession->_sendFlag, 0);
			DecrementUseCount(pSession);
			return false;
		}
		else if (pSession->_disconnect)
		{
			CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_sendOvl);
			InterlockedExchange(&pSession->_sendFlag, 0);
			return false;
		}
	}

	return true;
}

CSession* CNetServer::AcquireSessionUsage(unsigned __int64 sessionID)
{
	if (sessionID == -1)
	{
		return nullptr;
	}

	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= __ID_BIT__;
	CSession* pSession = _sessions[(long)idx];

	IncrementUseCount(pSession);

	if (pSession->_validFlag._releaseFlag == 1)
	{
		DecrementUseCount(pSession);
		return nullptr;
	}

	if (pSession->GetID() != sessionID)
	{
		DecrementUseCount(pSession);
		return nullptr;
	}

	return pSession;
}

void CNetServer::ReleaseSessionUsage(CSession* pSession)
{
	DecrementUseCount(pSession);
}

void CNetServer::IncrementUseCount(CSession* pSession)
{
	InterlockedIncrement16(&pSession->_validFlag._useCount);
}

void CNetServer::DecrementUseCount(CSession* pSession)
{
	short ret = InterlockedDecrement16(&pSession->_validFlag._useCount);
	if (ret == 0)
	{
		if (InterlockedCompareExchange(&pSession->_validFlag._flag, _releaseFlag._flag, 0) == 0)
		{
			PostQueuedCompletionStatus(_hNetworkCP, 1, (ULONG_PTR)pSession->GetID(), (LPOVERLAPPED)&pSession->_releaseOvl);
			return;
		}
	}
}

void CNetServer::HandleRelease(unsigned __int64 sessionID)
{
	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= __ID_BIT__;
	CSession* pSession = _sessions[(long)idx];

	SOCKET sock = pSession->_sock;
	pSession->Terminate();
	closesocket(sock);
	_emptyIdx.Push(idx);
	OnReleaseClient(sessionID);

	InterlockedIncrement(&_disconnectCnt);
	InterlockedDecrement(&_sessionCnt);
}
#endif