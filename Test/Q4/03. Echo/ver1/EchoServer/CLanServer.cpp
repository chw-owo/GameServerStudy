#include "CLanServer.h"
#ifdef LANSERVER

#include "ErrorCode.h"
#include "CLanGroup.h"
#include <stdio.h>
#include <tchar.h>

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>

// 17 bit for Idx (MAX 131072)
// 47 bit for Id (MAX 140737488355328)

CLanServer::CLanServer()
{
	_indexMask = 0b11111111111111111;
	_indexMask <<= __ID_BIT__;
	_idMask = ~_indexMask;

	_releaseFlag._useCount = 0;
	_releaseFlag._releaseFlag = 1;
}

bool CLanServer::NetworkInitialize(const wchar_t* IP, short port, long sendTime, int numOfThreads, int numOfRunnings, bool nagle)
{
	// Option Setting ====================================================

	wcscpy_s(_IP, 10, IP);
	_port = port;
	_nagle = nagle;
	_numOfThreads = numOfThreads;
	_numOfRunnings = numOfRunnings;
	_sendTime = sendTime;

	_pReleaseQ = new CLockFreeQueue<unsigned __int64>;

	// Network Setting ===================================================

	// Initialize Winsock
	WSADATA wsa;
	int startRet = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (startRet != 0)
	{
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: WSAStartup Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_WSASTARTUP, stErrMsg);
		return false;
	}

	// Create Listen Sock
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		wchar_t stErrMsg[dfERR_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Listen sock is INVALID, %d", _T(__FUNCTION__), __LINE__, err);
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
		wchar_t stErrMsg[dfERR_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Set Linger Option Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_LINGER, stErrMsg);
		return false;
	}

	// Set SndBuf0 Option for Async
	int sndBufSize = 0;
	optRet = setsockopt(_listenSock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(sndBufSize));
	if (optRet == SOCKET_ERROR)
	{
		wchar_t stErrMsg[dfERR_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Set SendBuf Option Error, %d", _T(__FUNCTION__), __LINE__, err);
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
		wchar_t stErrMsg[dfERR_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Listen Sock Bind Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTENSOCK_BIND, stErrMsg);
		return false;
	}

	// Start Listen
	int listenRet = listen(_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		wchar_t stErrMsg[dfERR_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Listen Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTEN, stErrMsg);
		return false;
	}

	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		_emptyIdx.Push(dfSESSION_MAX - i);
		_sessions[i] = new CLanSession;
	}

	// Thread Setting ===================================================

	// Create IOCP for Network
	_hNetworkCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, _numOfRunnings);
	if (_hNetworkCP == NULL)
	{
		wchar_t stErrMsg[dfERR_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Create IOCP Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_IOCP, stErrMsg);
		return false;
	}

	// Create Accept Thread
	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, nullptr);
	if (_acceptThread == NULL)
	{
		wchar_t stErrMsg[dfERR_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Create Accept Thread Error, %d", _T(__FUNCTION__), __LINE__, err);
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
			wchar_t stErrMsg[dfERR_MAX];
			int err = WSAGetLastError();
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Create Network Thread Error, %d", _T(__FUNCTION__), __LINE__, err);
			OnError(ERR_CREATE_NETWORK_THREAD, stErrMsg);
			return false;
		}
	}


	// Create Release Thread
	_releaseThread = (HANDLE)_beginthreadex(NULL, 0, ReleaseThread, this, 0, nullptr);
	if (_releaseThread == NULL)
	{
		int err = WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Create Release Thread Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_RELEASE_THREAD, stErrMsg);
		return false;
	}

	if (_sendTime != 0)
	{
		_sendThread = (HANDLE)_beginthreadex(NULL, 0, SendThread, this, 0, nullptr);
		if (_sendThread == NULL)
		{
			int err = WSAGetLastError();
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Create Send Thread Error, %d", _T(__FUNCTION__), __LINE__, err);
			OnError(ERR_CREATE_SEND_THREAD, stErrMsg);
			return false;
		}
	}


	OnInitialize();
	return true;
}

bool CLanServer::NetworkTerminate()
{
	if (InterlockedExchange(&_networkAlive, 1) != 0)
	{
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: LanServer Already Terminate", _T(__FUNCTION__), __LINE__);
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
		wchar_t stErrMsg[dfERR_MAX];
		int err = ::WSAGetLastError();
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Socket for Wake is INVALID, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_INVALID, stErrMsg);
		return false;
	}

	int connectRet = connect(socktmp, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (connectRet == SOCKET_ERROR)
	{
		wchar_t stErrMsg[dfERR_MAX];
		int err = ::WSAGetLastError();
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Socket for Wake Connect Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_CONNECT, stErrMsg);
		return false;
	}
	WaitForSingleObject(_acceptThread, INFINITE);

	// Release All Session
	closesocket(socktmp);
	closesocket(_listenSock);
	for (int i = 0; i < dfSESSION_MAX; i++)
	{
		CLanSession* pSession = _sessions[i];
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
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Accept Error", _T(__FUNCTION__), __LINE__);
			pLanServer->OnError(ERR_ACCEPT, stErrMsg);
			break;
		}

		if (pLanServer->_networkAlive == 1) break;

		InterlockedIncrement(&pLanServer->_acceptCnt);
		long sessionCnt = InterlockedIncrement(&pLanServer->_sessionCnt);

		if (sessionCnt > dfSESSION_MAX)
		{
			closesocket(client_sock);
			InterlockedIncrement(&pLanServer->_disconnectCnt);
			InterlockedDecrement(&pLanServer->_sessionCnt);

			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Session Max", _T(__FUNCTION__), __LINE__);
			pLanServer->OnError(DEB_SESSION_MAX, stErrMsg);
			continue;
		}

		if (pLanServer->_emptyIdx.GetUseSize() == 0)
		{
			closesocket(client_sock);
			InterlockedIncrement(&pLanServer->_disconnectCnt);
			InterlockedDecrement(&pLanServer->_sessionCnt);

			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: No Empty Index", _T(__FUNCTION__), __LINE__);
			pLanServer->OnError(DEB_SESSION_MAX, stErrMsg);
			continue;
		}

		WCHAR addr[dfADDRESS_LEN] = { L'0' };
		DWORD size = dfADDRESS_LEN;
		WSAAddressToStringW((SOCKADDR*)&clientaddr, sizeof(clientaddr), NULL, addr, &size);

		// pLanServer->OnConnectRequest(addr);

		unsigned __int64 sessionID = InterlockedIncrement64(&pLanServer->_sessionID);
		sessionID &= pLanServer->_idMask;
		long idx = pLanServer->_emptyIdx.Pop();
		unsigned __int64 sessionIdx = ((unsigned __int64)idx) << __ID_BIT__;
		sessionID |= sessionIdx;
		CLanSession* pSession = pLanServer->_sessions[(long)idx];

		pLanServer->IncrementUseCount(pSession);
		pSession->Initialize(sessionID, client_sock, clientaddr);

		// ::printf("%d: Accept Success (%016llx - %016llx)\n", GetCurrentThreadId(), sessionID, pSession->GetID());

		CreateIoCompletionPort((HANDLE)pSession->_sock, pLanServer->_hNetworkCP, (ULONG_PTR)pSession->GetID(), 0);
		pLanServer->RecvPost(pSession);
		pLanServer->OnAcceptClient(sessionID, addr);
		pLanServer->DecrementUseCount(pSession);
	}

	wchar_t stErrMsg[dfERR_MAX];
	swprintf_s(stErrMsg, dfERR_MAX, L"Accept Thread (%d)", GetCurrentThreadId());
	pLanServer->OnThreadTerminate(stErrMsg);

	return 0;
}

unsigned int __stdcall CLanServer::NetworkThread(void* arg)
{
	CLanServer* pLanServer = (CLanServer*)arg;
	int threadID = GetCurrentThreadId();
	NetworkOverlapped* pNetOvl = new NetworkOverlapped;

	for (;;)
	{
		__int64 sessionID;
		DWORD cbTransferred;

		int GQCSRet = GetQueuedCompletionStatus(pLanServer->_hNetworkCP,
			&cbTransferred, (PULONG_PTR)&sessionID, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (pLanServer->_networkAlive == 1) break;
		CLanSession* pSession = pLanServer->AcquireSessionUsage(sessionID);
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
					wchar_t stErrMsg[dfERR_MAX];
					swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: GQCS return 0, %d", _T(__FUNCTION__), __LINE__, err);
					pLanServer->OnError(ERR_GQCS_RET0, stErrMsg);
				}
			}
			pLanServer->DecrementUseCount(pSession);
		}
		else
		{
			switch (pNetOvl->_type)
			{
			case NET_TYPE::RECV_COMPLETE:
				pLanServer->HandleRecvCP(pSession, cbTransferred);
				pLanServer->DecrementUseCount(pSession);
				break;

			case NET_TYPE::SEND_COMPLETE:
				pLanServer->HandleSendCP(pSession, cbTransferred);
				pLanServer->DecrementUseCount(pSession);
				break;

			case NET_TYPE::SEND_POST:
				pLanServer->SendPost(pSession);
				break;
			}
		}

		pLanServer->ReleaseSessionUsage(pSession);
	}

	delete pNetOvl;
	wchar_t stErrMsg[dfERR_MAX];
	swprintf_s(stErrMsg, dfERR_MAX, L"Network Thread (%d)", threadID);
	pLanServer->OnThreadTerminate(stErrMsg);

	return 0;
}

unsigned int __stdcall CLanServer::SendThread(void* arg)
{
	CLanServer* pLanServer = (CLanServer*)arg;
	pLanServer->_oldTick = timeGetTime();

	for (;;)
	{
		pLanServer->SleepForFixedSend();

		for (int i = 0; i < dfSESSION_MAX; i++)
		{
			unsigned __int64 sessionID = pLanServer->_sessions[i]->GetID();
			CLanSession* pSession = pLanServer->AcquireSessionUsage(sessionID);
			if (pSession == nullptr) continue;

			if (pLanServer->SendCheck(pSession))
			{
				pLanServer->SendPost(pSession);
			}
			pLanServer->ReleaseSessionUsage(pSession);
		}
	}

	wchar_t stErrMsg[dfERR_MAX];
	swprintf_s(stErrMsg, dfERR_MAX, L"Send Thread");
	pLanServer->OnThreadTerminate(stErrMsg);

	return 0;
}

unsigned int __stdcall CLanServer::ReleaseThread(void* arg)
{
	CLanServer* pLanServer = (CLanServer*)arg;
	long undesired = 0;

	for (;;)
	{
		WaitOnAddress(&pLanServer->_releaseSignal, &undesired, sizeof(long), INFINITE);
		if (pLanServer->_networkAlive == 1) break;

		while (pLanServer->_pReleaseQ->GetUseSize() > 0)
		{
			unsigned __int64 sessionID = pLanServer->_pReleaseQ->Dequeue();
			if (sessionID == 0) break;

			unsigned __int64 sessionIdx = sessionID & pLanServer->_indexMask;
			sessionIdx >>= __ID_BIT__;
			CLanSession* pSession = pLanServer->_sessions[(long)sessionIdx];

			SOCKET sock = pSession->_sock;

			EnterCriticalSection(&pSession->_groupLock);
			CLanGroup* pGroup = pSession->_pGroup;
			pSession->Terminate();
			LeaveCriticalSection(&pSession->_groupLock);
			if (pGroup != nullptr)
			{
				long ret = InterlockedIncrement(&pGroup->_signal);
				if (ret == 1) WakeByAddressSingle(&pGroup->_signal);
			}

			closesocket(sock);
			pLanServer->_emptyIdx.Push(sessionIdx);

			InterlockedIncrement(&pLanServer->_disconnectCnt);
			InterlockedDecrement(&pLanServer->_sessionCnt);

			pLanServer->OnReleaseClient(sessionID);
			InterlockedDecrement(&pLanServer->_releaseSignal);
		}
	}

	return 0;
}

bool CLanServer::Disconnect(unsigned __int64 sessionID)
{
	CLanSession* pSession = AcquireSessionUsage(sessionID);
	if (pSession == nullptr) return false;

	// ::printf("%d: Disconnect (%016llx - %016llx)\n", GetCurrentThreadId(), sessionID, pSession->GetID());

	pSession->_disconnect = true;
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_recvComplOvl);
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_sendComplOvl);
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_sendPostOvl);

	ReleaseSessionUsage(pSession);
	return true;
}

bool CLanServer::SendPacket(unsigned __int64 sessionID, CLanPacket* packet, bool disconnect)
{
	CLanSession* pSession = AcquireSessionUsage(sessionID);
	if (pSession == nullptr) return false;

	// ::printf("%016llx (%d): CLanServer::%s\n", sessionID, GetCurrentThreadId(), __func__);

	if (packet->IsHeaderEmpty())
	{
		short payloadSize = packet->GetPayloadSize();
		stLanHeader header;
		header._len = payloadSize;

		int putRet = packet->PutHeaderData((char*)&header, dfLANHEADER_LEN);
		if (putRet != dfLANHEADER_LEN)
		{
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: CPacket PutHeaderData Error", _T(__FUNCTION__), __LINE__);
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

bool CLanServer::MoveGroup(unsigned __int64 sessionID, CLanGroup* pGroup)
{
	CLanSession* pSession = AcquireSessionUsage(sessionID);
	if (pSession == nullptr) return false;

	EnterCriticalSection(&pSession->_groupLock);
	pSession->_pGroup = pGroup;
	if (pGroup != nullptr)
	{
		pGroup->_enterSessions.Enqueue(pSession->GetID());
		long ret = InterlockedIncrement(&pGroup->_signal);
		if (ret == 1) WakeByAddressSingle(&pGroup->_signal);
	}
	LeaveCriticalSection(&pSession->_groupLock);

	ReleaseSessionUsage(pSession);
	return true;
}

bool CLanServer::RegisterGroup(CLanGroup* pGroup)
{
	HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, pGroup->UpdateThread, (void*)pGroup, 0, nullptr);
	if (thread == NULL)
	{
		wchar_t stErrMsg[dfERR_MAX];
		int err = WSAGetLastError();
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Create Group Thread Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_GROUP_THREAD, stErrMsg);
		return false;
	}
	_groupThreads.insert(make_pair(pGroup, thread));
	return true;
}

bool CLanServer::RemoveGroup(CLanGroup* pGroup)
{
	pGroup->SetDead();
	unordered_map<CLanGroup*, HANDLE>::iterator it = _groupThreads.find(pGroup);
	HANDLE thread = it->second;
	WaitForSingleObject(thread, INFINITE);
	_groupThreads.erase(it);
	return false;
}

void CLanServer::HandleRecvCP(CLanSession* pSession, int recvBytes)
{
	CLanPacket* recvBuf = pSession->_recvBuf;
	int moveWriteRet = recvBuf->MovePayloadWritePos(recvBytes);
	if (moveWriteRet != recvBytes)
	{
		Disconnect(pSession->GetID());
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer MoveWritePos Error", _T(__FUNCTION__), __LINE__);
		OnError(ERR_RECVBUF_MOVEWRITEPOS, stErrMsg);
		return;
	}

	int cnt = 0;
	int useSize = recvBuf->GetPayloadSize();

	EnterCriticalSection(&pSession->_groupLock);

	while (useSize > dfNETHEADER_LEN)
	{
		stNetHeader* header = (stNetHeader*)recvBuf->GetPayloadReadPtr();
		if (dfNETHEADER_LEN + header->_len > useSize) break;

		int moveReadRet1 = recvBuf->MovePayloadReadPos(dfLANHEADER_LEN);
		if (moveReadRet1 != dfLANHEADER_LEN)
		{
			Disconnect(pSession->GetID());
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, stErrMsg);
			LeaveCriticalSection(&pSession->_groupLock);
			return;
		}

		CRecvLanPacket* packet = CRecvLanPacket::Alloc(recvBuf);
		recvBuf->AddUsageCount(1);
		cnt++;

		if (pSession->_pGroup == nullptr)
		{
			OnRecv(pSession->GetID(), packet);
			CRecvLanPacket::Free(packet);
		}
		else
		{
			pSession->_OnRecvQ.Enqueue(packet);
			long ret = InterlockedIncrement(&pSession->_pGroup->_signal);
			if (ret == 1) WakeByAddressSingle(&pSession->_pGroup->_signal);
		}

		int moveReadRet2 = recvBuf->MovePayloadReadPos(header->_len);
		if (moveReadRet2 != header->_len)
		{
			Disconnect(pSession->GetID());
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, stErrMsg);
			LeaveCriticalSection(&pSession->_groupLock);
			return;
		}

		useSize = recvBuf->GetPayloadSize();
	}

	pSession->_recvBuf = CLanPacket::Alloc();
	pSession->_recvBuf->AddUsageCount(1);
	pSession->_recvBuf->CopyRecvBuf(recvBuf);
	CLanPacket::Free(recvBuf);

	InterlockedAdd(&_recvCnt, cnt);
	if (pSession->_pGroup != nullptr)
		InterlockedAdd(&pSession->_pGroup->_recvCnt, cnt);

	LeaveCriticalSection(&pSession->_groupLock);
	RecvPost(pSession);
}

void CLanServer::HandleSendCP(CLanSession* pSession, int sendBytes)
{
	int i = 0;
	for (; i < pSession->_sendCount; i++)
	{
		CLanPacket* packet = pSession->_tempBuf.Dequeue();
		if (packet == nullptr) break;

		if (packet->_pGroup == nullptr)
		{
			OnSend(pSession->GetID());
		}
		else
		{
			packet->_pGroup->_OnSendQ.Enqueue(pSession->GetID());
			long ret = InterlockedIncrement(&pSession->_pGroup->_signal);
			if (ret == 1) WakeByAddressSingle(&pSession->_pGroup->_signal);
		}

		CLanPacket::Free(packet);
	}

	InterlockedExchange(&pSession->_sendFlag, 0);
	if (_sendTime == 0 && SendCheck(pSession))
	{
		SendPost(pSession);
	}
}

bool CLanServer::RecvPost(CLanSession* pSession)
{
	DWORD flags = 0;
	DWORD recvBytes = 0;

	pSession->_wsaRecvbuf[0].buf = pSession->_recvBuf->GetPayloadWritePtr();
	pSession->_wsaRecvbuf[0].len = pSession->_recvBuf->GetRemainPayloadSize();
	ZeroMemory(&pSession->_recvComplOvl._ovl, sizeof(pSession->_recvComplOvl._ovl));

	if (pSession->_disconnect)
	{
		return false;
	}

	IncrementUseCount(pSession);
	int recvRet = WSARecv(pSession->_sock, pSession->_wsaRecvbuf,
		dfWSARECVBUF_CNT, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvComplOvl, NULL);

	// ::printf("%016llx (%d): Recv Request\n", pSession->GetID(), GetCurrentThreadId());

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED && err != WSAEINTR)
			{
				wchar_t stErrMsg[dfERR_MAX];
				swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Error, %d", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_RECV, stErrMsg);
			}
			DecrementUseCount(pSession);
			return false;
		}
		else if (pSession->_disconnect)
		{
			CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_recvComplOvl);
			return false;
		}
	}
	return true;
}

bool CLanServer::SendPost(CLanSession* pSession)
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
		CLanPacket* packet = pSession->_sendBuf.Dequeue();
		if (packet == nullptr) break;

		pSession->_wsaSendbuf[idx].buf = packet->GetLanPacketReadPtr();
		pSession->_wsaSendbuf[idx].len = packet->GetLanPacketSize();
		pSession->_tempBuf.Enqueue(packet);

		InterlockedIncrement(&_sendCnt);
		if (packet->_pGroup != nullptr)
			InterlockedIncrement(&packet->_pGroup->_sendCnt);
	}
	pSession->_sendCount = idx;

	DWORD sendBytes;
	ZeroMemory(&pSession->_sendComplOvl._ovl, sizeof(pSession->_sendComplOvl._ovl));

	if (pSession->_disconnect)
	{
		InterlockedExchange(&pSession->_sendFlag, 0);
		return false;
	}

	IncrementUseCount(pSession);
	int sendRet = WSASend(pSession->_sock, pSession->_wsaSendbuf,
		idx, &sendBytes, 0, (LPOVERLAPPED)&pSession->_sendComplOvl, NULL);

	// ::printf("%016llx (%d): Send Request\n", pSession->GetID(), GetCurrentThreadId());

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED && err != WSAEINTR)
			{
				wchar_t stErrMsg[dfERR_MAX];
				swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Send Error, %d", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_SEND, stErrMsg);
			}
			InterlockedExchange(&pSession->_sendFlag, 0);
			DecrementUseCount(pSession);
			return false;
		}
		else if (pSession->_disconnect)
		{
			CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_sendComplOvl);
			InterlockedExchange(&pSession->_sendFlag, 0);
			return false;
		}
	}

	return true;
}

bool CLanServer::SendCheck(CLanSession* pSession)
{
	if (pSession->_sendBuf.GetUseSize() == 0) return false;
	if (InterlockedExchange(&pSession->_sendFlag, 1) == 1) return false;
	if (pSession->_sendBuf.GetUseSize() == 0)
	{
		InterlockedExchange(&pSession->_sendFlag, 0);
		return false;
	}
	return true;
}

CLanSession* CLanServer::AcquireSessionUsage(unsigned __int64 sessionID)
{
	if (sessionID == -1)
	{
		return nullptr;
	}

	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= __ID_BIT__;
	CLanSession* pSession = _sessions[(long)idx];

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

inline void CLanServer::SleepForFixedSend()
{
	if ((timeGetTime() - _oldTick) < _sendTime)
		Sleep(_sendTime - (timeGetTime() - _oldTick));
	_oldTick += _sendTime;
}

void CLanServer::ReleaseSessionUsage(CLanSession* pSession)
{
	DecrementUseCount(pSession);
}

void CLanServer::IncrementUseCount(CLanSession* pSession)
{
	InterlockedIncrement16(&pSession->_validFlag._useCount);
}

void CLanServer::DecrementUseCount(CLanSession* pSession)
{
	short ret = InterlockedDecrement16(&pSession->_validFlag._useCount);
	if (ret == 0)
	{
		if (InterlockedCompareExchange(&pSession->_validFlag._flag, _releaseFlag._flag, 0) == 0)
		{
			_pReleaseQ->Enqueue(pSession->GetID());
			InterlockedIncrement(&_releaseSignal);
			WakeByAddressSingle(&_releaseSignal);
			return;
		}
	}
}

#endif