#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 512

int err_quit(const wchar_t* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	return 1;
}

void err_display(const wchar_t* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	wprintf(L"[%s] %s", msg, (wchar_t*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

int recvn(SOCKET s, char* buf, int len, int flags)
{
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0)
	{
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}
	return (len - left);
}

int wmain(int argc, wchar_t* argv[])
{
	int cnt = 0;

	for (;;)
	{
		int retval;

		// Initialize Winsock
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
			return 1;
		//MessageBox(NULL, L"Success to Initialize Winsock!", L"Alert", MB_OK);

		// Generate Socket
		SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET) return err_quit(L"sock()");
		//MessageBox(NULL, L"Success to Generate socket!", L"Alert", MB_OK);

		// Connect
		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		ULONG server_sin_addr;
		inet_pton(AF_INET, SERVERIP, &server_sin_addr);
		serveraddr.sin_addr.s_addr = server_sin_addr;
		serveraddr.sin_port = htons(SERVERPORT);
		retval = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR) return err_quit(L"connect()");

		cnt++;
		printf("put socket no.%d in the backlog queue! \n", cnt);
		
		// Close Socket
		retval = closesocket(sock);
		WSACleanup();
		if (retval == SOCKET_ERROR)
		{
			WSACleanup();
			return err_quit(L"closesocket");
		}
		
	}

	return 0;
}
