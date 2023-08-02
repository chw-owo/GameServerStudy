#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "RingBuffer.h"

using namespace std;
#define dfSERVER_PORT 9000
#define dfBUFSIZE 128

enum class NET_TYPE
{
	SEND = 0,
	RECV
};

struct NetworkOverlapped
{
    OVERLAPPED ovl;
	NET_TYPE type;
};

struct Session
{
	SOCKET _sock;
	SOCKADDR_IN _addr;
	NetworkOverlapped* _pRecvOvl;
	NetworkOverlapped* _pSendOvl;

	WSABUF _wsaRecvbuf;
	WSABUF _wsaSendbuf;
	RingBuffer _recvBuf;
	RingBuffer _sendBuf;
};

vector<Session*> _allSessions;

unsigned WINAPI WorkerThread(void* arg);
int main()
{
	// Initialize Winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	// Create IOCP
	HANDLE hCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hCP == NULL) return 1;

	// Create WorkerThread
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int threadCnt = (int)si.dwNumberOfProcessors * 2;
	HANDLE* arrThread = new HANDLE[threadCnt];
	for (int i = 0; i < threadCnt; i++)
	{
		arrThread[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, hCP, 0, nullptr);
		if (arrThread[i] == NULL)
		{
			printf("Error! %s(%d)\n", __func__, __LINE__);
			return 1;
		}
	}

	// Socket Bind, Listen
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
	{
		printf("Error! %s(%d)\n", __func__, __LINE__);
		return 0;
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(dfSERVER_PORT);

	int bindRet = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bindRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
		return 0;
	}

	int listenRet = listen(listen_sock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
		return 0;
	}

	printf("Complete Setting for Listen!\n");
	
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);

	while (1)
	{
		// Accept
		SOCKET client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			printf("Error! %s(%d)\n", __func__, __LINE__);
			break;
		}

		Session* pSession = new Session;
		if (pSession == nullptr)
		{
			printf("Error! %s(%d)\n", __func__, __LINE__);
			break;
		}

		pSession->_sock = client_sock;
		pSession->_addr = clientaddr;
		pSession->_wsaRecvbuf.buf = pSession->_recvBuf.GetWriteBufferPtr();
		pSession->_wsaRecvbuf.len = pSession->_recvBuf.DirectEnqueueSize();
		pSession->_wsaSendbuf.buf = pSession->_sendBuf.GetReadBufferPtr();
		pSession->_wsaSendbuf.len = pSession->_sendBuf.DirectDequeueSize();

		pSession->_pRecvOvl = new NetworkOverlapped;
		pSession->_pSendOvl = new NetworkOverlapped;
		pSession->_pRecvOvl->type = NET_TYPE::RECV;
		pSession->_pSendOvl->type = NET_TYPE::SEND;
		ZeroMemory(&pSession->_pRecvOvl->ovl, sizeof(pSession->_pRecvOvl->ovl));
		ZeroMemory(&pSession->_pSendOvl->ovl, sizeof(pSession->_pSendOvl->ovl));

		_allSessions.push_back(pSession);
		printf("Accept New Client!\n");

		// IOCP
		CreateIoCompletionPort((HANDLE)pSession->_sock, hCP, (ULONG_PTR)pSession, 0);
		
		// Async Recv
		DWORD flags = 0;
		DWORD recvBytes = 0;

		int recvRet = WSARecv(pSession->_sock, &pSession->_wsaRecvbuf,
			1, &recvBytes, &flags, (LPOVERLAPPED)pSession->_pRecvOvl, NULL);

		if (recvRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
				break;
			}
			continue;
		}
	}

	WSACleanup();
	WaitForMultipleObjects(threadCnt, arrThread, true, INFINITE);
	printf("\nAll Thread Terminate!\n");
}

unsigned WINAPI  WorkerThread(void* arg)
{
	HANDLE hCP = (HANDLE)arg;

	while (1)
	{
		Session* pSession;
		DWORD cbTransferred;
		NetworkOverlapped* pNetOvl = new NetworkOverlapped;
		int GQCSRet = GetQueuedCompletionStatus(hCP, &cbTransferred,
			(PULONG_PTR)&pSession, (LPOVERLAPPED*)&pNetOvl, INFINITE);

		// Check Exception
		if (GQCSRet == 0 || cbTransferred == 0)
		{
			if (GQCSRet == 0)
			{
				DWORD arg1, arg2;
				WSAGetOverlappedResult(pSession->_sock, (LPOVERLAPPED)pNetOvl, &arg1, FALSE, &arg2);
				printf("Error! %s(%d)\n", __func__, __LINE__);
			}
			closesocket(pSession->_sock);
			printf("Disconnect Client\n");
			continue;
		}

		// Recv
		if(pNetOvl->type == NET_TYPE::RECV)
		{
			// Handle Recvd Message (Echo)
			printf("Success to Recv %dbytes!\n", cbTransferred);
			pSession->_recvBuf.MoveWritePos(cbTransferred);

			char buf[dfBUFSIZE] = { '\0', };
			int peekRet = pSession->_recvBuf.Peek(buf, cbTransferred);
			if (peekRet != cbTransferred)
			{
				printf("Error! %s(%d)\n", __func__, __LINE__);
				break;
			}
			printf("%s\n", buf);

			int dequeueRet = pSession->_recvBuf.Dequeue(
				pSession->_sendBuf.GetWriteBufferPtr(), cbTransferred);
			if (dequeueRet != cbTransferred)
			{
				printf("Error! %s(%d)\n", __func__, __LINE__);
				break;
			}

			printf("MoveWritePos: %d\n", dequeueRet);
			pSession->_sendBuf.MoveWritePos(dequeueRet);

			// Send
			DWORD sendBytes;
			ZeroMemory(&pSession->_pSendOvl->ovl, sizeof(pSession->_pSendOvl->ovl));
			pSession->_wsaSendbuf.buf = pSession->_sendBuf.GetReadBufferPtr();
			pSession->_wsaSendbuf.len = pSession->_sendBuf.DirectDequeueSize();
			printf("DirectDequeueSize: %d\n", pSession->_wsaSendbuf.len);
			printf("Used Size: %d\n", pSession->_sendBuf.GetUseSize());

			int sendRet = WSASend(pSession->_sock, &pSession->_wsaSendbuf,
				1, &sendBytes, 0, (LPOVERLAPPED)pSession->_pSendOvl, NULL);
			if (sendRet == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err != ERROR_IO_PENDING)
				{
					printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
					break;
				}
				continue;
			}

			// Recv
			DWORD flags = 0;
			DWORD recvBytes;
			ZeroMemory(&pSession->_pRecvOvl->ovl, sizeof(pSession->_pRecvOvl->ovl));
			pSession->_wsaRecvbuf.buf = pSession->_recvBuf.GetWriteBufferPtr();
			pSession->_wsaRecvbuf.len = pSession->_recvBuf.DirectEnqueueSize();

			int recvRet = WSARecv(pSession->_sock, &pSession->_wsaRecvbuf,
				1, &recvBytes, &flags, (LPOVERLAPPED)pSession->_pRecvOvl, NULL);
			if (recvRet == SOCKET_ERROR)
			{
				int err = WSAGetLastError();
				if (err != ERROR_IO_PENDING)
				{
					printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
					break;
				}
				continue;
			}

		}

		// Send 
		else if (pNetOvl->type == NET_TYPE::SEND)
		{
			printf("Success to Send %dbytes!\n\n", cbTransferred);
			pSession->_recvBuf.MoveReadPos(cbTransferred); 
			// TO-DO: 여기서 used size =0이 됨
		}

		// Exception
		else
		{
			printf("OVERLAPPED TYPE: %d\n", pNetOvl->type);
		}
	} 

	return 0;
}