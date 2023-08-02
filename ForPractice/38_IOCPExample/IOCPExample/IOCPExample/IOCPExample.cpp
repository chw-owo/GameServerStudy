#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define dfSERVER_PORT 9000
#define dfBUFSIZE 512
#define dfADDRESS_BUFSIZE 33
struct Session
{
	OVERLAPPED overlapped;
	SOCKET sock;
	char buf[dfBUFSIZE + 1];
	int recvBytes;
	int sendBytes;
	WSABUF wsabuf;
	char addrBuf[dfADDRESS_BUFSIZE] = { '\0', };
};

DWORD WINAPI WorkerThread(LPVOID arg);

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
	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
	{
		hThread = CreateThread(NULL, 0, WorkerThread, hCP, 0, NULL);
		if (hThread == NULL) return 1;
		CloseHandle(hThread);

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

	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	DWORD recvBytes, flags;

	while (1)
	{
		// Accept
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			printf("Error! %s(%d)\n", __func__, __LINE__);
			break;
		}

		char addrBuf[dfADDRESS_BUFSIZE] = { '\0', };
		inet_ntop(AF_INET, &clientaddr.sin_addr, addrBuf, dfADDRESS_BUFSIZE);
		printf("Accept Client! %s:%d\n", addrBuf, ntohs(clientaddr.sin_port));

		// Connect Socket - IOCP
		CreateIoCompletionPort((HANDLE)client_sock, hCP, client_sock, 0);

		// Create Session
		Session* pSession = new Session;
		if (pSession == nullptr)
		{
			printf("Error! %s(%d)\n", __func__, __LINE__);
			break;
		}

		ZeroMemory(&pSession->overlapped, sizeof(pSession->overlapped));
		pSession->sock = client_sock;
		pSession->recvBytes = 0;
		pSession->sendBytes = 0;
		pSession->wsabuf.buf = pSession->buf;
		pSession->wsabuf.len = dfBUFSIZE;
		strcpy_s(pSession->addrBuf, dfADDRESS_BUFSIZE, addrBuf);

		// Start Async IO
		flags = 0;
		int recvRet = WSARecv(client_sock, &pSession->wsabuf, 
			1, &recvBytes, &flags, &pSession->overlapped, NULL);
		if (recvRet == SOCKET_ERROR)
		{
			int err = WSAGetLastError();
			if(err != ERROR_IO_PENDING)
			{
				printf("Error! %s(%d): %d\n", __func__, __LINE__, err);
				break;
			}
			continue;
		}

		// Terminate Winsock
		WSACleanup();
		return 0;
	}
}

DWORD WINAPI WorkerThread(LPVOID arg)
{
	HANDLE hCP = (HANDLE)arg;

	while (1)
	{
		// Wait for Async IO Complete
		DWORD cbTransferred;
		SOCKET client_sock;
		Session* pSession;
		int GQCSRet = GetQueuedCompletionStatus(hCP, &cbTransferred, 
			&client_sock, (LPOVERLAPPED*)&pSession, INFINITE);

		// Get Client Info
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(pSession->sock, (SOCKADDR*)&clientaddr, &addrlen);

		// Check Async IO Result
		if (GQCSRet == 0 || cbTransferred == 0)
		{
			if (GQCSRet == 0)
			{
				DWORD tmp1, tmp2;
				WSAGetOverlappedResult(pSession->sock,
					&pSession->overlapped, &tmp1, FALSE, &tmp2);
				printf("Error! %s(%d)\n", __func__, __LINE__);
			}

			closesocket(pSession->sock);
			printf("Disconnect Client! %s:%d\n",
				pSession->addrBuf, ntohs(clientaddr.sin_port));
			delete pSession;
			continue;
		}

		// Update Recv-Send Bytes
		if (pSession->recvBytes == 0)
		{
			pSession->recvBytes = cbTransferred;
			pSession->sendBytes = 0;

			// Print Recv Data
			pSession->buf[pSession->recvBytes] = '\0';
			printf("[%s:%d] %s\n", pSession->addrBuf, ntohs(clientaddr.sin_port), pSession->buf);
		}
		else
		{
			pSession->sendBytes += cbTransferred;
		}

		// Send Data
		if (pSession->recvBytes > pSession->sendBytes)
		{
			ZeroMemory(&pSession->overlapped, sizeof(pSession->overlapped));
			pSession->wsabuf.buf = pSession->buf + pSession->sendBytes;
			pSession->wsabuf.len = pSession->recvBytes - pSession->sendBytes;

			DWORD sendBytes;
			int sendRet = WSASend(pSession->sock, &pSession->wsabuf,
				1, &sendBytes, 0, &pSession->overlapped, NULL);
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
		}
		// Recv Data
		else
		{
			pSession->recvBytes = 0;

			ZeroMemory(&pSession->overlapped, sizeof(pSession->overlapped));
			pSession->wsabuf.buf = pSession->buf;
			pSession->wsabuf.len = dfBUFSIZE;
			
			DWORD recvBytes;
			DWORD flags = 0;
			int recvRet = WSARecv(pSession->sock, &pSession->wsabuf,
				1, &recvBytes, &flags, &pSession->overlapped, NULL);
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
	}

	return 0;
}
