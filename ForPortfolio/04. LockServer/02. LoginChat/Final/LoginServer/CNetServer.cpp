#include "CNetServer.h"
#ifdef NETSERVER

#include "ErrorCode.h"
#include <stdio.h>
#include <tchar.h>


CNetServer::CNetServer()
{

}

bool CNetServer::NetworkInitialize(const wchar_t* IP, short port, long sendTime, int numOfThreads, int numOfRunnings, bool nagle, bool monitorServer)
{
	// Option Setting ====================================================

	wcscpy_s(_IP, 10, IP);
	_port = port;
	_sendTime = sendTime;
	_numOfThreads = numOfThreads;
	_numOfRunnings = numOfRunnings;
	_nagle = nagle;

	InitializeSRWLock(&_releaseLock);
	InitializeSRWLock(&_sessionsLock);
	_mm = new CMonitorManager(monitorServer);
	_pSessionPool = new CObjectPool<CNetSession>(dfSESSION_MAX, false);

	// Network Setting ===================================================

	// Initialize Winsock
	WSADATA wsa;
	int startRet = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (startRet != 0)
	{
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: WSAStartup Error", _T(__FUNCTION__), __LINE__);
		OnError(ERR_WSASTARTUP, stErrMsg);
		return false;
	}

	// Create Listen Sock
	_listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSock == INVALID_SOCKET)
	{
		int err = WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
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
		int err = WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Set Linger Option Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_LINGER, stErrMsg);
		return false;
	}

	// Set SndBuf0 Option for Async
	int sndBufSize = 0;
	optRet = setsockopt(_listenSock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(sndBufSize));
	if (optRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
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
		int err = WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Listen Sock Bind Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTENSOCK_BIND, stErrMsg);
		return false;
	}

	// Start Listen
	int listenRet = listen(_listenSock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Listen Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTEN, stErrMsg);
		return false;
	}

	// Thread Setting ===================================================

	// Create IOCP for Network
	_hNetworkCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, _numOfRunnings);
	if (_hNetworkCP == NULL)
	{
		int err = WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Create IOCP Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_IOCP, stErrMsg);
		return false;
	}

	// Create Accept Thread
	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, nullptr);
	if (_acceptThread == NULL)
	{
		int err = WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
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
			int err = WSAGetLastError();
			wchar_t stErrMsg[dfERR_MAX];
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

bool CNetServer::NetworkTerminate()
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
		int err = ::WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Socket for Wake is INVALID, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_INVALID, stErrMsg);
		return false;
	}

	int connectRet = connect(socktmp, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (connectRet == SOCKET_ERROR)
	{
		int err = ::WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Socket for Wake Connect Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_CONNECT, stErrMsg);
		return false;
	}
	WaitForSingleObject(_acceptThread, INFINITE);

	// Release All Session
	closesocket(socktmp);
	closesocket(_listenSock);

	unordered_map<unsigned __int64, CNetSession*>::iterator it = _sessions.begin();
	for (; it != _sessions.end(); it++)
	{
		CNetSession* pSession = it->second;
		closesocket(pSession->_sock);
		_pSessionPool->Free(pSession);
	}

	WSACleanup();
	CloseHandle(_hNetworkCP);
	CloseHandle(_acceptThread);
	for (int i = 0; i < _numOfThreads; i++)
		CloseHandle(_networkThreads[i]);
	delete[] _networkThreads;

	OnTerminate();
}

bool CNetServer::Disconnect(unsigned __int64 sessionID)
{
	CNetSession* pSession = AcquireSessionUsage(sessionID);
	if (pSession == nullptr) return false;

	// ::printf("Disconnect\n");

	pSession->_disconnect = true;
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_recvComplOvl);
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_sendComplOvl);
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_sendPostOvl);

	ReleaseSessionUsage(pSession);
	return true;
}

bool CNetServer::SendPacket(unsigned __int64 sessionID, CNetSendPacket* packet)
{
	CNetSession* pSession = AcquireSessionUsage(sessionID);
	if (pSession == nullptr) return false;

	if (packet->IsHeaderEmpty())
	{
		short payloadSize = packet->GetPayloadSize();
		stNetHeader header;
		header._len = payloadSize;
		header._randKey = rand() % 256;
		packet->Encode(header, packet->GetPayloadPtr());

		int putRet = packet->PutHeaderData((char*)&header, dfNETHEADER_LEN);
		if (putRet != dfNETHEADER_LEN)
		{
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: CNetPacket PutHeaderData Error", _T(__FUNCTION__), __LINE__);
			OnError(ERR_PACKET_PUT_HEADER, stErrMsg);
			ReleaseSessionUsage(pSession);
			return false;
		}
	}

	pSession->_sendBuf.push(packet);
	if (_sendTime == 0 && SendCheck(pSession))
	{
		// SendPost(pSession);
		PostQueuedCompletionStatus(_hNetworkCP, 1, (ULONG_PTR)pSession->GetID(), (LPOVERLAPPED)&pSession->_sendPostOvl);
	}

	ReleaseSessionUsage(pSession);
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
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Accept Error", _T(__FUNCTION__), __LINE__);
			pNetServer->OnError(ERR_ACCEPT, stErrMsg);
			break;
		}
		if (pNetServer->_networkAlive == 1) break;
		pNetServer->_acceptCnt++;

		if (pNetServer->_sessions.size() >= dfSESSION_MAX)
		{
			closesocket(client_sock);
			InterlockedIncrement(&pNetServer->_disconnectCnt);

			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Session Max", _T(__FUNCTION__), __LINE__);
			pNetServer->OnError(DEB_SESSION_MAX, stErrMsg);
			continue;
		}

		WCHAR addr[dfADDRESS_LEN] = { L'0', };
		DWORD size = dfADDRESS_LEN;
		WSAAddressToStringW((SOCKADDR*)&clientaddr, sizeof(clientaddr), NULL, addr, &size);

		unsigned __int64 sessionID = pNetServer->_sessionID++;
		CNetSession* pSession = pNetServer->_pSessionPool->Alloc();
		pSession->Initialize(sessionID, client_sock, clientaddr);
		CreateIoCompletionPort((HANDLE)pSession->_sock, pNetServer->_hNetworkCP, (ULONG_PTR)pSession->GetID(), 0);

		AcquireSRWLockExclusive(&pNetServer->_sessionsLock);
		pNetServer->_sessions.insert(make_pair(sessionID, pSession));
		ReleaseSRWLockExclusive(&pNetServer->_sessionsLock);

		AcquireSRWLockExclusive(&pSession->_lock);
		pNetServer->RecvPost(pSession);
		ReleaseSRWLockExclusive(&pSession->_lock);

		pNetServer->OnAcceptClient(sessionID, addr);
	}

	wchar_t stErrMsg[dfERR_MAX];
	swprintf_s(stErrMsg, dfERR_MAX, L"Accept Thread (%d)", GetCurrentThreadId());
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
		unsigned __int64 sessionID;
		DWORD cbTransferred;

		int GQCSRet = GetQueuedCompletionStatus(pNetServer->_hNetworkCP,
			&cbTransferred, (PULONG_PTR)&sessionID, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (pNetServer->_networkAlive == 1) break;
		CNetSession* pSession = pNetServer->AcquireSessionUsage(sessionID);
		if (pSession == nullptr) continue;

		if (GQCSRet == 0 || cbTransferred == 0)
		{
			if (GQCSRet == 0)
			{
				int err = GetLastError();
				if (err != WSAECONNRESET && err != WSAECONNABORTED && err != WSAENOTSOCK && err != WSAEINTR &&
					err != ERROR_OPERATION_ABORTED && err != ERROR_SEM_TIMEOUT && err != ERROR_SUCCESS &&
					err != ERROR_CONNECTION_ABORTED && err != ERROR_NETNAME_DELETED)
				{
					wchar_t stErrMsg[dfERR_MAX];
					swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: GQCS return 0, %d", _T(__FUNCTION__), __LINE__, err);
					pNetServer->OnError(ERR_GQCS_RET0, stErrMsg);
				}
			}
			pNetServer->DecrementIOCount(pSession);
		}
		else
		{
			switch (pNetOvl->_type)
			{
			case NET_TYPE::RECV_COMPLETE:
				pNetServer->HandleRecvCP(pSession, cbTransferred);
				pNetServer->DecrementIOCount(pSession);
				break;

			case NET_TYPE::SEND_COMPLETE:
				pNetServer->HandleSendCP(pSession, cbTransferred);
				pNetServer->DecrementIOCount(pSession);
				break;

			case NET_TYPE::SEND_POST:
				pNetServer->SendPost(pSession);
				break;
			}
		}

		pNetServer->ReleaseSessionUsage(pSession);
	}

	delete pNetOvl;
	wchar_t stErrMsg[dfERR_MAX];
	swprintf_s(stErrMsg, dfERR_MAX, L"Network Thread (%d)", threadID);
	pNetServer->OnThreadTerminate(stErrMsg);

	return 0;
}

unsigned int __stdcall CNetServer::SendThread(void* arg)
{
	CNetServer* pNetServer = (CNetServer*)arg;
	pNetServer->_oldTick = timeGetTime();

	for (;;)
	{
		pNetServer->SleepForFixedSend();

		AcquireSRWLockShared(&pNetServer->_sessionsLock);
		unordered_map<unsigned __int64, CNetSession*>::iterator it = pNetServer->_sessions.begin();
		for (; it != pNetServer->_sessions.end(); it++)
		{
			CNetSession* pSession = it->second;
			AcquireSRWLockExclusive(&pSession->_lock);
			if (pNetServer->SendCheck(pSession))
			{
				pNetServer->SendPost(pSession);
			}
			ReleaseSRWLockExclusive(&pSession->_lock);
		}
		ReleaseSRWLockShared(&pNetServer->_sessionsLock);
	}

	wchar_t stErrMsg[dfERR_MAX];
	swprintf_s(stErrMsg, dfERR_MAX, L"Send Thread");
	pNetServer->OnThreadTerminate(stErrMsg);

	return 0;
}

void CNetServer::SleepForFixedSend()
{
	if ((timeGetTime() - _oldTick) < _sendTime)
		Sleep(_sendTime - (timeGetTime() - _oldTick));
	_oldTick += _sendTime;
}

unsigned int __stdcall CNetServer::ReleaseThread(void* arg)
{
	CNetServer* pNetServer = (CNetServer*)arg;
	long undesired = 0;

	for (;;)
	{
		WaitOnAddress(&pNetServer->_releaseSignal, &undesired, sizeof(long), INFINITE);
		if (pNetServer->_networkAlive == 1) break;

		while (pNetServer->_releaseQ.size() > 0)
		{
			AcquireSRWLockExclusive(&pNetServer->_releaseLock);
			unsigned __int64 sessionID = pNetServer->_releaseQ.front();
			pNetServer->_releaseQ.pop();
			ReleaseSRWLockExclusive(&pNetServer->_releaseLock);

			// ::printf("Release\n");

			AcquireSRWLockExclusive(&pNetServer->_sessionsLock);
			unordered_map<unsigned __int64, CNetSession*>::iterator it = pNetServer->_sessions.find(sessionID);
			if (it == pNetServer->_sessions.end())
			{
				ReleaseSRWLockExclusive(&pNetServer->_sessionsLock);
				continue;
			}
			CNetSession* pSession = it->second;
			pNetServer->_sessions.erase(it);

			AcquireSRWLockExclusive(&pSession->_lock);
			ReleaseSRWLockExclusive(&pNetServer->_sessionsLock);
			ReleaseSRWLockExclusive(&pSession->_lock);

			closesocket(pSession->_sock);
			pSession->Terminate();
			pNetServer->_pSessionPool->Free(pSession);

			InterlockedIncrement(&pNetServer->_disconnectCnt);
			pNetServer->OnReleaseClient(sessionID);
			InterlockedDecrement(&pNetServer->_releaseSignal);
		}
	}

	return 0;
}

bool CNetServer::HandleRecvCP(CNetSession* pSession, int recvBytes)
{
	CNetRecvPacket* recvBuf = pSession->_recvBuf;
	int moveWriteRet = recvBuf->MovePayloadWritePos(recvBytes);
	if (moveWriteRet != recvBytes)
	{
		Disconnect(pSession->GetID());
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer MoveWritePos Error", _T(__FUNCTION__), __LINE__);
		OnError(ERR_RECVBUF_MOVEWRITEPOS, stErrMsg);
		return false;
	}

	int cnt = 0;
	int useSize = recvBuf->GetPayloadSize();

	while (useSize > dfNETHEADER_LEN)
	{
		// ::printf("%016llx: Payload %d\n", pSession->GetID(), recvBuf->GetPayloadReadPos());

		stNetHeader* header = (stNetHeader*)recvBuf->GetPayloadReadPtr();
		if (header->_code != dfPACKET_CODE)
		{
			Disconnect(pSession->GetID());
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Wrong NetPacket Code", _T(__FUNCTION__), __LINE__);
			OnDebug(DEB_WRONG_PACKETCODE, stErrMsg);
			return false;
		}

		if (dfNETHEADER_LEN + header->_len > useSize) break;

		int moveReadRet1 = recvBuf->MovePayloadReadPos(dfNETHEADER_LEN);
		if (moveReadRet1 != dfNETHEADER_LEN)
		{
			Disconnect(pSession->GetID());
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, stErrMsg);
			return false;
		}

		if (recvBuf->Decode(*header, recvBuf->GetPayloadReadPtr()) == false)
		{
			Disconnect(pSession->GetID());
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Wrong Checksum", _T(__FUNCTION__), __LINE__);
			OnDebug(DEB_WRONG_DECODE, stErrMsg);
			return false;
		}

		CNetMsg* recvMsg = CNetMsg::Alloc(recvBuf);
		recvBuf->AddUsageCount(1);
		OnRecv(pSession->GetID(), recvMsg);
		cnt++;

		int moveReadRet2 = recvBuf->MovePayloadReadPos(header->_len);
		if (moveReadRet2 != header->_len)
		{
			Disconnect(pSession->GetID());
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, stErrMsg);
			return false;
		}

		useSize = recvBuf->GetPayloadSize();
	}

	pSession->_recvBuf = CNetRecvPacket::Alloc();
	pSession->_recvBuf->AddUsageCount(1);
	pSession->_recvBuf->CopyRecvBuf(recvBuf);
	CNetRecvPacket::Free(recvBuf);

	InterlockedIncrement(&_recvCnt);
	RecvPost(pSession);
	return true;
}

bool CNetServer::RecvPost(CNetSession* pSession)
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

	IncrementIOCount(pSession);
	int recvRet = WSARecv(pSession->_sock, pSession->_wsaRecvbuf,
		dfWSARECVBUF_CNT, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvComplOvl, NULL);

	// ::printf("%d: Recv Request\n");

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

			DecrementIOCount(pSession);
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

bool CNetServer::HandleSendCP(CNetSession* pSession, int sendBytes)
{
	int cnt = 0;
	while (pSession->_tempBuf.size() > 0)
	{
		CNetSendPacket* packet = pSession->_tempBuf.front();
		pSession->_tempBuf.pop();
		CNetSendPacket::Free(packet);
		cnt++;
	}

	InterlockedAdd(&_sendCnt, cnt);

	OnSend(pSession->GetID(), sendBytes);
	InterlockedExchange(&pSession->_sendFlag, 0);
	if (_sendTime == 0 && SendCheck(pSession))
	{
		SendPost(pSession);
	}

	return true;
}

bool CNetServer::SendCheck(CNetSession* pSession)
{
	if (pSession->_sendBuf.size() == 0) return false;
	if (InterlockedExchange(&pSession->_sendFlag, 1) == 1) return false;
	if (pSession->_sendBuf.size() == 0)
	{
		InterlockedExchange(&pSession->_sendFlag, 0);
		return false;
	}
	return true;
}

bool CNetServer::SendPost(CNetSession* pSession)
{
	int idx = 0;

	while (pSession->_sendBuf.size() > 0)
	{
		CNetSendPacket* packet = pSession->_sendBuf.front();
		pSession->_sendBuf.pop();
		pSession->_tempBuf.push(packet);

		pSession->_wsaSendbuf[idx].buf = packet->GetNetPacketReadPtr();
		pSession->_wsaSendbuf[idx].len = packet->GetNetPacketSize();
		if ((++idx) == dfWSASENDBUF_CNT) break;
	}
	pSession->_sendCount = idx;

	DWORD sendBytes;
	ZeroMemory(&pSession->_sendComplOvl._ovl, sizeof(pSession->_sendComplOvl._ovl));

	if (pSession->_disconnect)
	{
		InterlockedExchange(&pSession->_sendFlag, 0);
		return false;
	}

	IncrementIOCount(pSession);
	int sendRet = WSASend(pSession->_sock, pSession->_wsaSendbuf,
		idx, &sendBytes, 0, (LPOVERLAPPED)&pSession->_sendComplOvl, NULL);

	// ::printf("%d: Send Request (%016llx)\n", GetCurrentThreadId(), pSession->GetID());

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
			DecrementIOCount(pSession);
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

CNetSession* CNetServer::AcquireSessionUsage(unsigned __int64 sessionID)
{
	AcquireSRWLockShared(&_sessionsLock);
	unordered_map<unsigned __int64, CNetSession*>::iterator it = _sessions.find(sessionID);
	if (it == _sessions.end())
	{
		ReleaseSRWLockShared(&_sessionsLock);
		return nullptr;
	}

	CNetSession* pSession = it->second;
	AcquireSRWLockExclusive(&pSession->_lock);
	ReleaseSRWLockShared(&_sessionsLock);

	return pSession;
}

void CNetServer::ReleaseSessionUsage(CNetSession* pSession)
{
	ReleaseSRWLockExclusive(&pSession->_lock);
}

void CNetServer::IncrementIOCount(CNetSession* pSession)
{
	InterlockedIncrement(&pSession->_IOCount);
}

void CNetServer::DecrementIOCount(CNetSession* pSession)
{
	if (InterlockedDecrement(&pSession->_IOCount) == 0)
	{
		AcquireSRWLockExclusive(&_releaseLock);
		_releaseQ.push(pSession->GetID());
		ReleaseSRWLockExclusive(&_releaseLock);

		InterlockedIncrement(&_releaseSignal);
		WakeByAddressSingle(&_releaseSignal);
	}
}

#endif

