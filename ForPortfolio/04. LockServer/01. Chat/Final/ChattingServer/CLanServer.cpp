#include "CLanServer.h"
#ifdef LANSERVER

#include "ErrorCode.h"
#include <stdio.h>
#include <tchar.h>

CLanServer::CLanServer()
{

}

bool CLanServer::NetworkInitialize(const wchar_t* IP, short port, int numOfThreads, int numOfRunnings, bool nagle, bool monitorServer)
{
	// Member Setting ====================================================

	wcscpy_s(_IP, 10, IP);
	_port = port;
	_numOfThreads = numOfThreads;
	_numOfRunnings = numOfRunnings;
	_nagle = nagle;

	_mm = new CMonitorManager(monitorServer);
	_pSessionPool = new CObjectPool<CLanSession>(dfSESSION_MAX, false);

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

	// Create IOCP for Lanwork
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

	// Create Lanwork Thread
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

	// Terminate Lanwork Threads
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
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Socket for Wake Connect Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_CONNECT, stErrMsg);
		return false;
	}
	WaitForSingleObject(_acceptThread, INFINITE);

	// Release All Session
	closesocket(socktmp);
	closesocket(_listenSock);

	unordered_map<unsigned __int64, CLanSession*>::iterator it = _sessions.begin();
	for (; it != _sessions.end(); it++)
	{
		CLanSession* pSession = it->second;
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

bool CLanServer::Disconnect(unsigned __int64 sessionID)
{
	CLanSession* pSession = AcquireSessionUsage(sessionID);
	if (pSession == nullptr) return false;

	pSession->_disconnect = true;
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_recvComplOvl);
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_sendComplOvl);
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_sendPostOvl);
	
	ReleaseSessionUsage(pSession);
	return true;
}

bool CLanServer::SendPacket(unsigned __int64 sessionID, CLanSendPacket* packet)
{
	CLanSession* pSession = AcquireSessionUsage(sessionID);
	if (pSession == nullptr) return false;

	if (packet->IsHeaderEmpty())
	{
		short payloadSize = packet->GetPayloadSize();
		stLanHeader header;
		header._len = payloadSize;

		int putRet = packet->PutHeaderData((char*)&header, dfLANHEADER_LEN);
		if (putRet != dfLANHEADER_LEN)
		{
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: CLanPacket PutHeaderData Error", _T(__FUNCTION__), __LINE__);
			OnError(ERR_PACKET_PUT_HEADER, stErrMsg);
			ReleaseSessionUsage(pSession);
			return false;
		}
	}

	pSession->_sendBuf.push(packet);
	if (SendCheck(pSession))
	{
		// SendPost(pSession);
		PostQueuedCompletionStatus(_hNetworkCP, 1, (ULONG_PTR)pSession->GetID(), (LPOVERLAPPED)&pSession->_sendPostOvl);
	}

	ReleaseSessionUsage(pSession);
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
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Accept Error", _T(__FUNCTION__), __LINE__);
			pLanServer->OnError(ERR_ACCEPT, stErrMsg);
			break;
		}
		if (pLanServer->_networkAlive == 1) break;
		pLanServer->_acceptCnt++;

		if (pLanServer->_sessions.size() >= dfSESSION_MAX)
		{
			closesocket(client_sock);
			InterlockedIncrement(&pLanServer->_disconnectCnt);

			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Session Max", _T(__FUNCTION__), __LINE__);
			pLanServer->OnError(DEB_SESSION_MAX, stErrMsg);
			continue;
		}

		unsigned __int64 sessionID = pLanServer->_sessionID++;
		CLanSession* pSession = pLanServer->_pSessionPool->Alloc();
		pSession->Initialize(sessionID, client_sock, clientaddr);

		AcquireSRWLockExclusive(&pLanServer->_sessionsLock);
		pLanServer->_sessions.insert(make_pair(sessionID, pSession));
		ReleaseSRWLockExclusive(&pLanServer->_sessionsLock);

		// ::printf("Accept\n");

		CreateIoCompletionPort((HANDLE)pSession->_sock, pLanServer->_hNetworkCP, (ULONG_PTR)pSession->GetID(), 0);
		pLanServer->RecvPost(pSession);
		pLanServer->OnAcceptClient(sessionID);

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
		unsigned __int64 sessionID;
		DWORD cbTransferred;

		int GQCSRet = GetQueuedCompletionStatus(pLanServer->_hNetworkCP,
			&cbTransferred, (PULONG_PTR)&sessionID, (LPOVERLAPPED*)&pNetOvl, INFINITE);
		
		if (pLanServer->_networkAlive == 1) break;
		if (pNetOvl->_type == NET_TYPE::RELEASE)
		{
			pLanServer->HandleRelease(sessionID);
			continue;
		}

		CLanSession* pSession = pLanServer->AcquireSessionUsage(sessionID);
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
					pLanServer->OnError(ERR_GQCS_RET0, stErrMsg);
				}
			}

			pLanServer->DecrementIOCount(pSession);
		}
		else
		{
			switch (pNetOvl->_type)
			{
			case NET_TYPE::RECV_COMPLETE:
				pLanServer->HandleRecvCP(pSession, cbTransferred);
				pLanServer->DecrementIOCount(pSession);
				break;

			case NET_TYPE::SEND_COMPLETE:
				pLanServer->HandleSendCP(pSession, cbTransferred);
				pLanServer->DecrementIOCount(pSession);
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
	swprintf_s(stErrMsg, dfERR_MAX, L"Lanwork Thread (%d)", threadID);
	pLanServer->OnThreadTerminate(stErrMsg);

	return 0;
}


bool CLanServer::HandleRecvCP(CLanSession* pSession, int recvBytes)
{
	CLanRecvPacket* recvBuf = pSession->_recvBuf;
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

	while (useSize > dfLANHEADER_LEN)
	{
		// ::printf("%016llx: Payload %d\n", pSession->GetID(), recvBuf->GetPayloadReadPos());

		stLanHeader* header = (stLanHeader*)recvBuf->GetPayloadReadPtr();
		if (dfLANHEADER_LEN + header->_len > useSize) break;

		int moveReadRet1 = recvBuf->MovePayloadReadPos(dfLANHEADER_LEN);
		if (moveReadRet1 != dfLANHEADER_LEN)
		{
			Disconnect(pSession->GetID());
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, stErrMsg);
			return false;
		}

		// ::printf("%016llx: Header %d\n", pSession->GetID(), recvBuf->GetPayloadReadPos());

		CLanMsg* recvLanPacket = CLanMsg::Alloc(recvBuf);
		recvBuf->AddUsageCount(1);
		OnRecv(pSession->GetID(), recvLanPacket);
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

	pSession->_recvBuf = CLanRecvPacket::Alloc();
	pSession->_recvBuf->AddUsageCount(1);
	pSession->_recvBuf->CopyRecvBuf(recvBuf);
	CLanRecvPacket::Free(recvBuf);

	InterlockedAdd(&_recvCnt, cnt);
	RecvPost(pSession);
	return true;
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

	IncrementIOCount(pSession);
	int recvRet = WSARecv(pSession->_sock, pSession->_wsaRecvbuf,
		dfWSARECVBUF_CNT, &recvBytes, &flags, (LPOVERLAPPED)&pSession->_recvComplOvl, NULL);

	// ::printf("%d: Recv Request (%016llx)\n", GetCurrentThreadId(), pSession->GetID());

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

bool CLanServer::HandleSendCP(CLanSession* pSession, int sendBytes)
{
	while (pSession->_tempBuf.size() > 0)
	{
		CLanSendPacket* packet = pSession->_tempBuf.front();
		pSession->_tempBuf.pop();
		CLanSendPacket::Free(packet);
	}

	OnSend(pSession->GetID(), sendBytes);
	if (SendCheck(pSession)) SendPost(pSession);
	return true;
}

bool CLanServer::SendCheck(CLanSession* pSession)
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

bool CLanServer::SendPost(CLanSession* pSession)
{
	int idx = 0;

	while (pSession->_sendBuf.size() > 0)
	{
		idx++;
		if (idx == dfWSASENDBUF_CNT) break;
		CLanSendPacket* packet = pSession->_sendBuf.front();
		pSession->_sendBuf.pop();

		pSession->_wsaSendbuf[idx].buf = packet->GetLanPacketReadPtr();
		pSession->_wsaSendbuf[idx].len = packet->GetLanPacketSize();
		pSession->_tempBuf.push(packet);
	}
	pSession->_sendCount = idx;

	DWORD sendBytes;
	ZeroMemory(&pSession->_sendComplOvl._ovl, sizeof(pSession->_sendComplOvl._ovl));

	if (pSession->_disconnect)
	{
		InterlockedExchange(&pSession->_sendFlag, 0);
		return false;
	}

	InterlockedAdd(&_sendCnt, idx);
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

void CLanServer::HandleRelease(unsigned __int64 sessionID)
{
	AcquireSRWLockExclusive(&_sessionsLock);
	unordered_map<unsigned __int64, CLanSession*>::iterator it = _sessions.find(sessionID);
	if (it == _sessions.end())
	{
		ReleaseSRWLockExclusive(&_sessionsLock);
		return;
	}
	CLanSession* pSession = it->second;
	_sessions.erase(it);

	AcquireSRWLockExclusive(&pSession->_lock);
	ReleaseSRWLockExclusive(&_sessionsLock);
	ReleaseSRWLockExclusive(&pSession->_lock);

	closesocket(pSession->_sock);
	pSession->Terminate();
	_pSessionPool->Free(pSession);

	InterlockedIncrement(&_disconnectCnt);
	OnReleaseClient(sessionID);
}

CLanSession* CLanServer::AcquireSessionUsage(unsigned __int64 sessionID)
{
	AcquireSRWLockShared(&_sessionsLock);

	unordered_map<unsigned __int64, CLanSession*>::iterator it = _sessions.find(sessionID);
	if (it == _sessions.end())
	{
		ReleaseSRWLockShared(&_sessionsLock);
		return nullptr;
	}

	CLanSession* pSession = it->second;

	AcquireSRWLockExclusive(&pSession->_lock);
	ReleaseSRWLockShared(&_sessionsLock);
	return pSession;
}

void CLanServer::ReleaseSessionUsage(CLanSession* pSession)
{
	ReleaseSRWLockExclusive(&pSession->_lock);
}

void CLanServer::IncrementIOCount(CLanSession* pSession)
{
	InterlockedIncrement(&pSession->_IOCount);
}

void CLanServer::DecrementIOCount(CLanSession* pSession)
{
	if (InterlockedDecrement(&pSession->_IOCount) == 0)
	{
		PostQueuedCompletionStatus(_hNetworkCP, 1, (ULONG_PTR)pSession->GetID(), (LPOVERLAPPED)&pSession->_releaseOvl);
	}
}

#endif

