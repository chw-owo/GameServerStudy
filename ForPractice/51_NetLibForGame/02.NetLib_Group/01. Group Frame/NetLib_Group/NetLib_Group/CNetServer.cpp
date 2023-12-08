#include "CNetServer.h"
#ifdef NETSERVER

#include "ErrorCode.h"
#include "CGroup.h"
#include <stdio.h>
#include <tchar.h>

// 17 bit for Idx (MAX 131072)
// 47 bit for Id (MAX 140737488355328)
__declspec(thread) wchar_t* stErrMsg = new wchar_t[dfMSG_MAX];

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
		swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: NetServer Already Terminate\n", _T(__FUNCTION__), __LINE__);
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

		if (pNetServer->_networkAlive == 1) break;

		InterlockedIncrement(&pNetServer->_acceptCnt);
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
			InterlockedDecrement(&pNetServer->_sessionCnt);
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: No Empty Index\n", _T(__FUNCTION__), __LINE__);
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

		pNetServer->IncrementUseCount(pSession, __LINE__);
		pSession->Initialize(sessionID, client_sock, clientaddr);

		// ::printf("%d: Accept Success (%016llx - %016llx)\n", GetCurrentThreadId(), sessionID, pSession->GetID());

		CreateIoCompletionPort((HANDLE)pSession->_sock, pNetServer->_hNetworkCP, (ULONG_PTR)pSession->GetID(), 0);
		pNetServer->RecvPost(pSession);
		pNetServer->OnAcceptClient(sessionID, addr);
		pNetServer->DecrementUseCount(pSession, __LINE__);
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
		__int64 sessionID;
		DWORD cbTransferred;

		int GQCSRet = GetQueuedCompletionStatus(pNetServer->_hNetworkCP,
			&cbTransferred, (PULONG_PTR)&sessionID, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (pNetServer->_networkAlive == 1) break;
		switch (pNetOvl->_type)
		{
		case NET_TYPE::RELEASE:
			pNetServer->HandleRelease(sessionID);
			break;

		case NET_TYPE::ENTER:
			pNetServer->HandleEnter(sessionID);
			break;

		default:
			pNetServer->HandleIOCP(sessionID, GQCSRet, cbTransferred, pNetOvl);
			break;
		}
	}

	delete pNetOvl;
	swprintf_s(stErrMsg, dfMSG_MAX, L"Network Thread (%d)", threadID);
	pNetServer->OnThreadTerminate(stErrMsg);

	return 0;
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
	InterlockedIncrement(&_disconnectCnt);
	InterlockedDecrement(&_sessionCnt);

	OnReleaseClient(sessionID);
}

void CNetServer::HandleEnter(unsigned __int64 sessionID)
{
	unsigned __int64 idx = sessionID & _indexMask;
	idx >>= __ID_BIT__;
	CSession* pSession = _sessions[(long)idx];

	// TO-DO

	OnEnterGroup(sessionID);
}

void CNetServer::HandleIOCP(unsigned __int64 sessionID, int GQCSRet, DWORD cbTransferred, NetworkOverlapped* pNetOvl)
{
	CSession* pSession = AcquireSessionUsage(sessionID, __LINE__);
	if (pSession == nullptr) return;
	if (pSession->_group != nullptr) return;

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
				swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: GQCS return 0, %d\n", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_GQCS_RET0, stErrMsg);
			}
		}
	}
	else if (pNetOvl->_type == NET_TYPE::RECV)
	{
		// ::printf("%d: Recv Success (%016llx - %016llx)\n", GetCurrentThreadId(), sessionID, pSession->GetID());
		HandleRecvCP(pSession, cbTransferred);
	}
	else if (pNetOvl->_type == NET_TYPE::SEND)
	{
		// ::printf("%d: Send Success (%016llx - %016llx)\n", GetCurrentThreadId(), sessionID, pSession->GetID());
		HandleSendCP(pSession, cbTransferred);
	}

	DecrementUseCount(pSession, __LINE__);
	ReleaseSessionUsage(pSession, __LINE__);
}

bool CNetServer::Disconnect(unsigned __int64 sessionID)
{
	CSession* pSession = AcquireSessionUsage(sessionID, __LINE__);
	if (pSession == nullptr) return false;

	pSession->_disconnect = true;
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_recvOvl);
	CancelIoEx((HANDLE)pSession->_sock, (LPOVERLAPPED)&pSession->_sendOvl);

	ReleaseSessionUsage(pSession, __LINE__);
	return true;
}

bool CNetServer::SendPacket(unsigned __int64 sessionID, CPacket* packet, bool disconnect)
{
	CSession* pSession = AcquireSessionUsage(sessionID, __LINE__);
	if (pSession == nullptr) return false;

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
			swprintf_s(stErrMsg, dfMSG_MAX, L"%s[%d]: CPacket PutHeaderData Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_PACKET_PUT_HEADER, stErrMsg);
			ReleaseSessionUsage(pSession, __LINE__);
			return false;
		}
	}

	pSession->_sendBuf.Enqueue(packet);
	SendPost(pSession);

	ReleaseSessionUsage(pSession, __LINE__);
	return true;
}

bool CNetServer::MoveGroup(unsigned __int64 sessionID, CGroup* pGroup)
{
	CSession* pSession = AcquireSessionUsage(sessionID, __LINE__);
	if (pSession == nullptr) return false;

	CGroup* pCurGroup = pSession->_group;
	InterlockedExchangePointer((PVOID*)pSession->_group, pGroup);
	pGroup->_enterSessions.Enqueue(pSession);

	ReleaseSessionUsage(pSession, __LINE__);
	return true;
}

bool CNetServer::DeferPacket(unsigned __int64 sessionID, CPacket* packet)
{
	CSession* pSession = AcquireSessionUsage(sessionID, __LINE__);
	if (pSession == nullptr) return false;

	pSession->_deferedPacket.Enqueue(packet);
	
	return true;
}

bool CNetServer::HandleRecvCP(CSession* pSession, int recvBytes)
{
	return false;
}

bool CNetServer::HandleSendCP(CSession* pSession, int sendBytes)
{
	return false;
}

bool CNetServer::RecvPost(CSession* pSession)
{
	return false;
}

bool CNetServer::SendPost(CSession* pSession)
{
	return false;
}

CSession* CNetServer::AcquireSessionUsage(unsigned __int64 sessionID, int line)
{
	return nullptr;
}

void CNetServer::ReleaseSessionUsage(CSession* pSession, int line)
{
}

void CNetServer::IncrementUseCount(CSession* pSession, int line)
{
}

void CNetServer::DecrementUseCount(CSession* pSession, int line)
{
}
#endif