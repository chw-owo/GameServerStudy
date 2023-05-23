#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERPORT 9000
#define BUFSIZE 512

int wmain(int argc, wchar_t* argv[])
{
	int retval;

	// Initialize Winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// Generate Socket
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
	{
		printf("Fail to Generate Socket");
		return 0;
	}

	// Bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) 
	{
		printf("Fail to Bind Socket");
		return 0;
	}

	// Listen
	retval = listen(listen_sock, SOMAXCONN_HINT(SOMAXCONN));
	if (retval == SOCKET_ERROR)
	{
		printf("Fail to Make Socket Listen");
		return 0;
	}

	for(;;)
	{

	}

	// Close Listen Socket
	retval = closesocket(listen_sock);
	if (retval == SOCKET_ERROR)
	{
		printf("Fail to Close Socket");
		WSACleanup();
		return 0;
	}
		
	// Terminate Winsock
	WSACleanup();
	return 0;
}
