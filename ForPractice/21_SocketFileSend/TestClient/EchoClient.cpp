#pragma comment(lib, "ws2_32")
//#include <winsock.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERIP L"127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 512

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

int main(int argc, char* argv[])
{
	int ret;

	// Initialize Winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// Create Socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		printf("Fail to Create Socket!\n");
		return 0;
	}

	// Connect Socket to Server
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	ret = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (ret == SOCKET_ERROR)
	{
		printf("Fail to Connect Socket!: %d\n", ret);
		return 0;
	}

	char buf[BUFSIZE + 1];
	int len;

	while (1)
	{
		// Set Data to Send 
		printf("\nEnter Text to Send: ");
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			break;
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			break;

		// Send Data to Server
		ret = send(sock, buf, strlen(buf), 0);
		if (ret == SOCKET_ERROR)
		{
			printf("Fail to Send Data!: %d\n", ret);
			break;
		}
		printf("[TCP Client] Success to Send %d bytes!\n", ret);

		// Reveice Data from Server
		ret = recvn(sock, buf, ret, 0);
		if (ret == SOCKET_ERROR)
		{
			printf("Fail to Receive Data!: %d\n", ret);
			break;
		}
		else if (ret == 0)
		{
			printf("Disconnect to Server!\n");
			break;
		}

		buf[ret] = '\0';
		printf("[TCP Client] Success to Reveice %d bytes!\n", ret);
		printf("[Received Data] %s\n", buf);
	}
	closesocket(sock);
	WSACleanup();
	return 0;
}