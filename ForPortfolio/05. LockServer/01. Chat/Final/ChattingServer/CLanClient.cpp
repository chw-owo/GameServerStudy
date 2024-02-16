#include "CLanClient.h"
#include "ErrorCode.h"
#include "CLanMsg.h"
#include <stdio.h>
#include <tchar.h>

bool CLanClient::NetworkInitialize(const wchar_t* IP, short port, int numOfThreads, int numOfRunnings, bool nagle, bool monitorServer)
{
	// Option Setting ====================================================

	wcscpy_s(_IP, 10, IP);
	_port = port;
	_numOfThreads = numOfThreads;
	_nagle = nagle;
	_mm = new CMonitorManager(monitorServer);

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

	// Create Sock
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		int err = ::WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Socket is INVALID, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_TEMPSOCK_INVALID, stErrMsg);
		return false;
	}
	_client = new CClientSession(sock);

	// Set Linger Option
	LINGER optval;
	optval.l_onoff = 1;
	optval.l_linger = 0;
	int optRet = setsockopt(_client->_sock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
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
	optRet = setsockopt(_client->_sock, SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(sndBufSize));
	if (optRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Set SendBuf Option Error, %d", _T(__FUNCTION__), __LINE__, err);
		OnError(ERR_SET_SNDBUF_0, stErrMsg);
		return false;
	}

	// Connect Socket
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	InetPton(AF_INET, _IP, &serveraddr.sin_addr);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(_port);
	int connectRet = connect(_client->_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (connectRet == SOCKET_ERROR)
	{
		int err = ::WSAGetLastError();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Socket Connect Error, %d", _T(__FUNCTION__), __LINE__, err);
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
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Create IOCP Error, %d", _T(__FUNCTION__), __LINE__, err);
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
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Create Network Thread Error, %d", _T(__FUNCTION__), __LINE__, err);
			OnError(ERR_CREATE_NETWORK_THREAD, stErrMsg);
			return false;
		}
	}

	CreateIoCompletionPort((HANDLE)_client->_sock, _hNetworkCP, (ULONG_PTR)nullptr, 0);
	RecvPost();

	OnInitialize();

	return true;
}

void CLanClient::NetworkTerminate()
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

bool CLanClient::Disconnect()
{
	// ::printf("%d: Disconnect\n", GetCurrentThreadId());

	_client->_disconnect = true;
	CancelIoEx((HANDLE)_client->_sock, (LPOVERLAPPED)&_client->_recvComplOvl);
	CancelIoEx((HANDLE)_client->_sock, (LPOVERLAPPED)&_client->_sendComplOvl);

	if (InterlockedExchange(&_networkAlive, 1) != 0)
	{
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: LanClient Already Terminate", _T(__FUNCTION__), __LINE__);
		OnError(ERR_ALREADY_TERMINATE, stErrMsg);
		return false;
	}
	return true;
}

bool CLanClient::SendPacket(CLanSendPacket* packet)
{
	// // ::printf("%d: Send Packet\n", GetCurrentThreadId());

	if (_client->_disconnect)
		return false;

	// // ::printf("%d: Send Packet (%016llx - %016llx)\n", GetCurrentThreadId(), sessionID, _client->GetID());
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
			return false;
		}
	}

	_client->_sendBuf.push(packet);
	if (SendCheck()) SendPost();
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
					swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: GQCS return 0, %d", _T(__FUNCTION__), __LINE__, err);
					pLanClient->OnError(ERR_GQCS_RET0, stErrMsg);
				}
			}
		}
		else if (pNetOvl->_type == NET_TYPE::RECV_COMPLETE)
		{
			// ::printf("%d: Recv Complete\n", GetCurrentThreadId());
			pLanClient->HandleRecvCP(cbTransferred);
		}
		else if (pNetOvl->_type == NET_TYPE::SEND_COMPLETE)
		{
			// ::printf("%d: Send Complete\n", GetCurrentThreadId());
			pLanClient->HandleSendCP(cbTransferred);
		}

		pLanClient->DecrementUseCount();
	}

	// delete pNetOvl;
	wchar_t stErrMsg[dfERR_MAX];
	swprintf_s(stErrMsg, dfERR_MAX, L"Network Thread (%d)", threadID);
	pLanClient->OnThreadTerminate(stErrMsg);
	return 0;
}

bool CLanClient::HandleRecvCP(int recvBytes)
{
	CLanRecvPacket* recvBuf = _client->_recvBuf;
	int moveWriteRet = recvBuf->MovePayloadWritePos(recvBytes);
	if (moveWriteRet != recvBytes)
	{
		Disconnect();
		wchar_t stErrMsg[dfERR_MAX];
		swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer MoveWritePos Error\n", _T(__FUNCTION__), __LINE__);
		OnError(ERR_RECVBUF_MOVEWRITEPOS, stErrMsg);
		return false;
	}

	int cnt = 0;
	int useSize = recvBuf->GetPayloadSize();

	while (useSize > dfLANHEADER_LEN)
	{
		// // ::printf("%016llx: Payload %d\n", _client->GetID(), recvBuf->GetPayloadReadPos());

		stLanHeader* header = (stLanHeader*)recvBuf->GetPayloadReadPtr();

		if (dfLANHEADER_LEN + header->_len > useSize) break;

		int moveReadRet1 = recvBuf->MovePayloadReadPos(dfLANHEADER_LEN);
		if (moveReadRet1 != dfLANHEADER_LEN)
		{
			Disconnect();
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, stErrMsg);
			return false;
		}

		// // ::printf("%016llx: Header %d\n", _client->GetID(), recvBuf->GetPayloadReadPos());

		CLanMsg* recvLanPacket = CLanMsg::Alloc(recvBuf);
		recvBuf->AddUsageCount(1);
		OnRecv(recvLanPacket);
		cnt++;

		int moveReadRet2 = recvBuf->MovePayloadReadPos(header->_len);
		if (moveReadRet2 != header->_len)
		{
			Disconnect();
			wchar_t stErrMsg[dfERR_MAX];
			swprintf_s(stErrMsg, dfERR_MAX, L"%s[%d]: Recv Buffer MoveReadPos Error", _T(__FUNCTION__), __LINE__);
			OnError(ERR_RECVBUF_MOVEREADPOS, stErrMsg);
			return false;
		}

		useSize = recvBuf->GetPayloadSize();
	}

	_client->_recvBuf = CLanRecvPacket::Alloc();
	_client->_recvBuf->AddUsageCount(1);

	_client->_recvBuf->CopyRecvBuf(recvBuf);
	CLanRecvPacket::Free(recvBuf);

	RecvPost();
	return true;
}

bool CLanClient::RecvPost()
{
	DWORD flags = 0;
	DWORD recvBytes = 0;

	_client->_wsaRecvbuf[0].buf = _client->_recvBuf->GetPayloadWritePtr();
	_client->_wsaRecvbuf[0].len = _client->_recvBuf->GetRemainPayloadSize();

	ZeroMemory(&_client->_recvComplOvl._ovl, sizeof(_client->_recvComplOvl._ovl));

	if (_client->_disconnect)
	{
		return false;
	}

	IncrementUseCount();
	int recvRet = WSARecv(_client->_sock, _client->_wsaRecvbuf,
		dfWSARECVBUF_CNT, &recvBytes, &flags, (LPOVERLAPPED)&_client->_recvComplOvl, NULL);

	// ::printf("%d: Recv Request\n", GetCurrentThreadId());

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
			DecrementUseCount();
			return false;
		}
		else if (_client->_disconnect)
		{
			CancelIoEx((HANDLE)_client->_sock, (LPOVERLAPPED)&_client->_recvComplOvl);
			return false;
		}
	}

	return true;
}

bool CLanClient::HandleSendCP(int sendBytes)
{
	for (int i = 0; i < _client->_sendCount; i++)
	{
		CLanSendPacket* packet = _client->_tempBuf.front();
		_client->_tempBuf.pop();
		if (packet == nullptr) break;
		CLanSendPacket::Free(packet);
	}

	OnSend(sendBytes);
	InterlockedExchange(&_client->_sendFlag, 0);
	if (SendCheck()) SendPost();
	return true;
}

bool CLanClient::SendCheck()
{
	if (_client->_sendBuf.size() == 0) return false;
	if (InterlockedExchange(&_client->_sendFlag, 1) == 1) return false;
	if (_client->_sendBuf.size() == 0)
	{
		InterlockedExchange(&_client->_sendFlag, 0);
		return false;
	}
	return true;
}

bool CLanClient::SendPost()
{
	int idx = 0;
	int useSize = _client->_sendBuf.size();

	for (; idx < useSize; idx++)
	{
		if (idx == dfWSASENDBUF_CNT) break;
		CLanSendPacket* packet = _client->_sendBuf.front();
		_client->_sendBuf.pop();
		if (packet == nullptr) break;

		_client->_wsaSendbuf[idx].buf = packet->GetLanPacketReadPtr();
		_client->_wsaSendbuf[idx].len = packet->GetLanPacketSize();
		_client->_tempBuf.push(packet);
	}
	_client->_sendCount = idx;

	DWORD sendBytes;
	ZeroMemory(&_client->_sendComplOvl._ovl, sizeof(_client->_sendComplOvl._ovl));

	if (_client->_disconnect)
	{
		InterlockedExchange(&_client->_sendFlag, 0);
		return false;
	}

	IncrementUseCount();
	int sendRet = WSASend(_client->_sock, _client->_wsaSendbuf,
		idx, &sendBytes, 0, (LPOVERLAPPED)&_client->_sendComplOvl, NULL);

	// ::printf("%d: Send Request\n", GetCurrentThreadId());

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
			InterlockedExchange(&_client->_sendFlag, 0);
			DecrementUseCount();
			return false;
		}
		else if (_client->_disconnect)
		{
			CancelIoEx((HANDLE)_client->_sock, (LPOVERLAPPED)&_client->_sendComplOvl);
			InterlockedExchange(&_client->_sendFlag, 0);
			return false;
		}
	}

	return true;
}

void CLanClient::HandleRelease()
{
	SOCKET sock = _client->_sock;
	delete _client;
	closesocket(sock);
}

void CLanClient::IncrementUseCount()
{
	InterlockedIncrement(&_client->_IOCount);
}

void CLanClient::DecrementUseCount()
{
	short ret = InterlockedDecrement(&_client->_IOCount);
	if (ret == 0)
	{
		PostQueuedCompletionStatus(_hNetworkCP, 1, 0, (LPOVERLAPPED)&_client->_releaseOvl);
	}
}