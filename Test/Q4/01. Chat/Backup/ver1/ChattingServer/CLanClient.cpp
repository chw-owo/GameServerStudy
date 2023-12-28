#include "CLanClient.h"

/*
#include "ErrorCode.h"
#include <stdio.h>
#include <tchar.h>

void CLanClient::Initialize()
{

}

void CLanClient::Terminate()
{
	// Terminate Network Threads
	for (int i = 0; i < _numOfThreads; i++)
		PostQueuedCompletionStatus(_hNetworkCP, 0, 0, 0);
	WaitForMultipleObjects(_numOfThreads, _networkThreads, true, INFINITE);

	WSACleanup();
	CloseHandle(_hNetworkCP);
	for (int i = 0; i < _numOfThreads; i++)
		CloseHandle(_networkThreads[i]);
	delete[] _networkThreads;

	OnTerminate();
}

bool CLanClient::Connect(const wchar_t* IP, short port, int numOfThreads, int numOfRunnings, bool nagle)
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
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: WSAStartup Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_WSASTARTUP, stErrMsg);
		return false;
	}

	// Create Bind Sock
	_clientSession->_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (_clientSession->_sock == INVALID_SOCKET)
	{
		int err = ::WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Socket is INVALID, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_INVALID, stErrMsg);
		return false;
	}

	// Set Linger Option
	LINGER optval;
	optval.l_onoff = 1;
	optval.l_linger = 0;
	int optRet = setsockopt(_clientSession->_sock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
	if (optRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Set Linger Option Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_LINGER, stErrMsg);
		return false;
	}

	// Set SndBuf0 Option for Async
	int sndBufSize = 0;
	optRet = setsockopt(_clientSession->_sock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(sndBufSize));
	if (optRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Set SendBuf Option Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_SNDBUF_0, stErrMsg);
		return false;
	}

	// Listen Socket Bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	InetPton(AF_INET, L"127.0.0.1", &serveraddr.sin_addr);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(_port);
	int bindRet = bind(_clientSession->_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bindRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Listen Sock Bind Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_LISTENSOCK_BIND, stErrMsg);
		return false;
	}

	// Try to Connect
	int connectRet = connect(_clientSession->_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (connectRet == SOCKET_ERROR)
	{
		int err = ::WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Socket Connect Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_CONNECT, stErrMsg);
		return false;
	}

	// Thread Setting ===================================================

	// Create IOCP for Network
	_hNetworkCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (_hNetworkCP == NULL)
	{
		int err = WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Create IOCP Error, %d\n", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_CREATE_IOCP, stErrMsg);
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
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Create Network Thread Error, %d\n", _T(__FUNCTION__), __LINE__, err);
			OnError(ERR_CREATE_NETWORK_THREAD, stErrMsg);
			return false;
		}
	}

	OnInitialize();

	return true;
}

bool CLanClient::Disconnect()
{
	// _clientSession->LeaveLog(200, sessionID, _clientSession->_validFlag._useCount, _clientSession->_validFlag._releaseFlag);
	// ::printf("%d: Disconnect (%016llx - %016llx)\n", GetCurrentThreadId(), sessionID, _clientSession->GetID());

	_clientSession->_disconnect = true;
	CancelIoEx((HANDLE)_clientSession->_sock, (LPOVERLAPPED)&_clientSession->_recvOvl);
	CancelIoEx((HANDLE)_clientSession->_sock, (LPOVERLAPPED)&_clientSession->_sendOvl);

	if (InterlockedExchange(&_networkAlive, 1) != 0)
	{
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: LanClient Already Terminate\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_ALREADY_TERMINATE, stErrMsg);
		return false;
	}
	return true;
}

bool CLanClient::SendPacket(CPacket* packet)
{
	if (_clientSession->_disconnect)
		return false;

	// ::printf("%d: Send Packet (%016llx - %016llx)\n", GetCurrentThreadId(), sessionID, _clientSession->GetID());
	if (packet->IsHeaderEmpty())
	{
		short payloadSize = packet->GetPayloadSize();
		stHeader header;
		header._len = payloadSize;

		int putRet = packet->PutHeaderData((char*)&header, dfHEADER_LEN);
		if (putRet != dfHEADER_LEN)
		{
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: CPacket PutHeaderData Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_PACKET_PUT_HEADER, stErrMsg);
			return false;
		}
	}

	_clientSession->_sendBuf.Enqueue(packet);
	SendPost();
	return true;
}

unsigned int __stdcall CLanClient::NetworkThread(void* arg)
{
	CLanClient* pLanClient = (CLanClient*)arg;
	int threadID = GetCurrentThreadId();
	NetworkOverlapped* pNetOvl = new NetworkOverlapped;

	for (;;)
	{
		long tmp;
		DWORD cbTransferred;
		int GQCSRet = GetQueuedCompletionStatus(pLanClient->_hNetworkCP,
			&cbTransferred, (PULONG_PTR)&tmp, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		if (pLanClient->_networkAlive == 1) break;
		if (pNetOvl->_type == NET_TYPE::RELEASE)
		{
			pLanClient->HandleRelease();
			break;
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
					wchar_t stErrMsg[dfERR_MAX];
					swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: GQCS return 0, %d\n", _T(__FUNCTION__), __LINE__, err);
					pLanClient->OnError(ERR_GQCS_RET0, stErrMsg);
				}
			}
		}
		else if (pNetOvl->_type == NET_TYPE::RECV)
		{
			// ::printf("%d: Recv Success (%016llx - %016llx)\n", GetCurrentThreadId(), sessionID, _clientSession->GetID());
			pLanClient->HandleRecvCP(cbTransferred);
		}
		else if (pNetOvl->_type == NET_TYPE::SEND)
		{
			// ::printf("%d: Send Success (%016llx - %016llx)\n", GetCurrentThreadId(), sessionID, _clientSession->GetID());
			pLanClient->HandleSendCP(cbTransferred);
		}

		long ret = InterlockedDecrement16(&pLanClient->_clientSession->_validFlag._useCount);
		if (ret == 0)
		{
			PostQueuedCompletionStatus(pLanClient->_hNetworkCP, 0, 0, (LPOVERLAPPED)&pLanClient->_clientSession->_releaseOvl);
		}
	}

	delete pNetOvl;
	wchar_t stErrMsg[dfERR_MAX];
	swprintf_s(stErrMsg, dfERR_MAX, L"Network Thread (%d)", threadID);
	pLanClient->OnThreadTerminate(stErrMsg);
	return 0;
}

bool CLanClient::HandleRecvCP(int recvBytes)
{
	int moveReadRet = _clientSession->_recvBuf.MoveWritePos(recvBytes);
	if (moveReadRet != recvBytes)
	{
		Disconnect();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer MoveWritePos Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_RECVBUF_MOVEWRITEPOS, stErrMsg);
		return false;
	}

	int useSize = _clientSession->_recvBuf.GetUseSize();

	for (;;)
	{
		if (useSize <= dfHEADER_LEN) break;

		stHeader header;
		int peekRet = _clientSession->_recvBuf.Peek((char*)&header, dfHEADER_LEN);
		if (peekRet != dfHEADER_LEN)
		{
			Disconnect();
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer Peek Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_PEEK, stErrMsg);
			return false;
		}

		int moveReadRet = _clientSession->_recvBuf.MoveReadPos(dfHEADER_LEN);
		if (moveReadRet != dfHEADER_LEN)
		{
			Disconnect();
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error\n", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, stErrMsg);
			return false;
		}

		CPacket* packet = CPacket::Alloc();
		packet->Clear();
		packet->AddUsageCount(1);
		int dequeueRet = _clientSession->_recvBuf.Dequeue(packet->GetPayloadWritePtr(), header._len);
		if (dequeueRet != header._len)
		{
			Disconnect();
			CPacket::Free(packet);
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer Dequeue Error\n", _T(__FUNCTION__), __LINE__);
			OnDebug(DEB_WRONG_PACKETLEN, stErrMsg);
			return false;
		}
		packet->MovePayloadWritePos(dequeueRet);

		OnRecv(packet);
		CPacket::Free(packet);

		useSize = _clientSession->_recvBuf.GetUseSize();
	}

	if (!RecvPost()) return false;
	return true;
}

bool CLanClient::RecvPost()
{
	DWORD flags = 0;
	DWORD recvBytes = 0;

	int freeSize = _clientSession->_recvBuf.GetFreeSize();
	_clientSession->_wsaRecvbuf[0].buf = _clientSession->_recvBuf.GetWritePtr();
	_clientSession->_wsaRecvbuf[0].len = _clientSession->_recvBuf.DirectEnqueueSize();
	_clientSession->_wsaRecvbuf[1].buf = _clientSession->_recvBuf.GetFrontPtr();
	_clientSession->_wsaRecvbuf[1].len = freeSize - _clientSession->_wsaRecvbuf[0].len;

	ZeroMemory(&_clientSession->_recvOvl._ovl, sizeof(_clientSession->_recvOvl._ovl));

	if (_clientSession->_disconnect)
	{
		return false;
	}

	InterlockedIncrement16(&_clientSession->_validFlag._useCount);
	int recvRet = WSARecv(_clientSession->_sock, _clientSession->_wsaRecvbuf,
		dfWSARECVBUF_CNT, &recvBytes, &flags, (LPOVERLAPPED)&_clientSession->_recvOvl, NULL);

	// ::printf("%d: Recv Request (%016llx)\n", GetCurrentThreadId(), _clientSession->GetID());

	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED && err != WSAEINTR)
			{
				wchar_t stErrMsg[dfERR_MAX];
				swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Error, %d\n", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_RECV, stErrMsg);
			}
			InterlockedDecrement16(&_clientSession->_validFlag._useCount);
			return false;
		}
		else if (_clientSession->_disconnect)
		{
			CancelIoEx((HANDLE)_clientSession->_sock, (LPOVERLAPPED)&_clientSession->_recvOvl);
			return false;
		}
	}

	InterlockedIncrement(&_recvCnt);
	return true;
}

bool CLanClient::HandleSendCP(int sendBytes)
{
	for (int i = 0; i < _clientSession->_sendCount; i++)
	{
		CPacket* packet = _clientSession->_tempBuf.Dequeue();
		if (packet == nullptr) break;
		CPacket::Free(packet);
	}

	OnSend(sendBytes);
	InterlockedExchange(&_clientSession->_sendFlag, 0);
	if (!SendPost()) return false;

	return true;
}

bool CLanClient::SendPost()
{
	if (_clientSession->_sendBuf.GetUseSize() == 0) return false;
	if (InterlockedExchange(&_clientSession->_sendFlag, 1) == 1) return false;
	if (_clientSession->_sendBuf.GetUseSize() == 0)
	{
		InterlockedExchange(&_clientSession->_sendFlag, 0);
		return false;
	}

	int idx = 0;
	int useSize = _clientSession->_sendBuf.GetUseSize();

	for (; idx < useSize; idx++)
	{
		if (idx == dfWSASENDBUF_CNT) break;
		CPacket* packet = _clientSession->_sendBuf.Dequeue();
		if (packet == nullptr) break;

		_clientSession->_wsaSendbuf[idx].buf = packet->GetPacketReadPtr();
		_clientSession->_wsaSendbuf[idx].len = packet->GetPacketSize();
		_clientSession->_tempBuf.Enqueue(packet);
	}
	_clientSession->_sendCount = idx;

	DWORD sendBytes;
	ZeroMemory(&_clientSession->_sendOvl._ovl, sizeof(_clientSession->_sendOvl._ovl));

	if (_clientSession->_disconnect)
	{
		InterlockedExchange(&_clientSession->_sendFlag, 0);
		return false;
	}

	InterlockedIncrement16(&_clientSession->_validFlag._useCount);
	int sendRet = WSASend(_clientSession->_sock, _clientSession->_wsaSendbuf,
		idx, &sendBytes, 0, (LPOVERLAPPED)&_clientSession->_sendOvl, NULL);

	// ::printf("%d: Send Request (%016llx)\n", GetCurrentThreadId(), _clientSession->GetID());

	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != ERROR_IO_PENDING)
		{
			if (err != WSAECONNRESET && err != WSAECONNABORTED && err != WSAEINTR)
			{
				wchar_t stErrMsg[dfERR_MAX];
				swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Send Error, %d\n", _T(__FUNCTION__), __LINE__, err);
				OnError(ERR_SEND, stErrMsg);
			}
			InterlockedExchange(&_clientSession->_sendFlag, 0);
			InterlockedDecrement16(&_clientSession->_validFlag._useCount);
			return false;
		}
		else if (_clientSession->_disconnect)
		{
			CancelIoEx((HANDLE)_clientSession->_sock, (LPOVERLAPPED)&_clientSession->_recvOvl);
			InterlockedExchange(&_clientSession->_sendFlag, 0);
			return false;
		}
	}

	InterlockedIncrement(&_sendCnt);
	return true;
}

void CLanClient::HandleRelease()
{
	SOCKET sock = _clientSession->_sock;
	_clientSession->Terminate();
	closesocket(sock);
}
*/