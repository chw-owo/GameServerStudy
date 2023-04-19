#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

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

int wmain(int argc, wchar_t* argv[])
{
	int retval;

	// Initialize Winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;
	//MessageBox(NULL, L"Success to Initialize Winsock!", L"Alert", MB_OK);

	// Generate Socket
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) return err_quit(L"sock()");
	//MessageBox(NULL, L"Success to Generate Listen socket!", L"Alert", MB_OK);

	// Bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) return err_quit(L"bind()");

	// Listen
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) return err_quit(L"listen()");

	/*
	// Create Var for Client
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];

	while (1)
	{
		// Accept Client Socket
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			err_display(L"accept()");
			break;
		}

		char client_sin_addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(clientaddr.sin_addr), client_sin_addr, INET_ADDRSTRLEN);

		printf("\n[TCP Server] Accpet Client: IP = %s, port = %d\n",
			client_sin_addr, ntohs(clientaddr.sin_port));

		while (1)
		{
			// Receive Data from Client 
			retval = recv(client_sock, buf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display(L"recv()");
				break;
			}
			else if (retval == 0)
			{
				break;
			}

			// Print out the Data 
			buf[retval] = '\0';
			printf("\n[TCP %s:%d] %s\n",
				client_sin_addr, ntohs(clientaddr.sin_port), buf);

			// Send Data to Client
			retval = send(client_sock, buf, retval, 0);
			if (retval == SOCKET_ERROR)
			{
				err_display(L"send()");
				break;
			}
		}

		// Close Client Socket
		closesocket(client_sock);
		printf("\n[TCP Server] Close Client Socket: IP = %s, port = %d\n",
			client_sin_addr, ntohs(clientaddr.sin_port));
	}
	*/

	for(;;)
	{

	}

	// Close Listen Socket
	retval = closesocket(listen_sock);
	if (retval == SOCKET_ERROR)
	{
		WSACleanup();
		return err_quit(L"closesocket");
	}
		
	// Terminate Winsock
	WSACleanup();
	return 0;
}
