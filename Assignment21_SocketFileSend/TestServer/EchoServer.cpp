#pragma comment(lib, "ws2_32")
//#include <winsock.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERPORT 10010
#define BUFSIZE 512

int main(int argc, char* argv[])
{
	int ret;

	// Initialize Winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// Create Socket
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
	{
		printf("Fail to Create Socket!\n");
		return 0;
	}

	// Bind Socket
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, L"0.0.0.0", &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	ret = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (ret == SOCKET_ERROR)
	{
		printf("Fail to Bind Socket!: %d\n", ret);
		return 0;
	}

	// Set Socket to Listen
	ret = listen(listen_sock, SOMAXCONN);
	if (ret == SOCKET_ERROR)
	{
		printf("Fail to Set Socket to Listen!: %d\n", ret);
		return 0;
	}

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
			printf("Fail to Accept Client!: %d\n", ret);
			break;
		}

		WCHAR szClientIP[16] = { 0 };
		InetNtop(AF_INET, &clientaddr.sin_addr, szClientIP, 16);
		wprintf(L"\n[TCP Server] Accept Client: IP=%s, port=%d\n",
			szClientIP, ntohs(clientaddr.sin_port));

		// Receive Data from Client
		while (1)
		{
			ret = recv(client_sock, buf, BUFSIZE, 0);
			if (ret == SOCKET_ERROR)
			{
				printf("Fail to Receive Data from Client!: %d\n", ret);
				break;
			}
			else if (ret == 0)
			{
				printf("Disconnect to Client!\n");
				break;
			}

			// Print Received Data
			buf[ret] = '\0';
			wprintf(L"[TCP/%s:%d] %s\n", 
				szClientIP, ntohs(clientaddr.sin_port), buf);

			// Send Data to Client
			ret = send(client_sock, buf, ret, 0);
			if (ret == SOCKET_ERROR)
			{
				printf("Fail to Send Data to Client!: %d\n", ret);
				break;
			}
		}

		// Close Client Socket
		closesocket(client_sock);
		wprintf(L"[TCP Server] Close client socket: IP=%s, port=%d\n",
			szClientIP, ntohs(clientaddr.sin_port));
	}

	// Close Server Socket
	closesocket(listen_sock);
	WSACleanup();
	return 0;
}
