#include <stdio.h>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define SERVERIP L"255.255.255.255"
#define MSGSIZE 10
#define BUFSIZE 64

#define _HandleError HandleError(__LINE__)
void HandleError(int line)
{
	int err = ::WSAGetLastError();
	wprintf(L"Error! line %d, %d\n", line, err);
}

int main()
{
	// Initialize WSA
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// Create Socket
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
	{
		_HandleError;
		return 0;
	}

	// Set Broadcast Option
	int ret;
	BOOL bEnable = TRUE;
	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&bEnable, sizeof(bEnable));
	if (ret == SOCKET_ERROR)
	{
		_HandleError;
		return 0;
	}

	int time = 200;
	ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&time, sizeof(time));
	if (ret == SOCKET_ERROR)
	{
		_HandleError;
		return 0;
	}

	// Set Address
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, SERVERIP, &serveraddr.sin_addr);

	char msg[MSGSIZE]
		= { 0xff, 0xee, 0xdd, 0xaa, 0x00, 0x99, 0x77, 0x55, 0x33, 0x11 };

	for (int i = 10001; i < 20000; i++)
	{
		// Reset Port
		serveraddr.sin_port = htons(i);

		SOCKADDR_IN peeraddr;
		int addrlen = sizeof(peeraddr);

		// Send Data
		ret = sendto(sock, msg, MSGSIZE, 0, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
		if (ret == SOCKET_ERROR)
		{
			_HandleError;
			return 0;
		}

		char buf[BUFSIZE];

		// Recieve Data
		ret = recvfrom(sock, buf, BUFSIZE, 0, (SOCKADDR*)&peeraddr, &addrlen);
		if (ret == -1)
		{
			continue;
		}
		else if (ret == SOCKET_ERROR)
		{
			_HandleError;
			return 0;
		}
		
		WCHAR szClientIP[16] = { 0 };
		InetNtop(AF_INET, &peeraddr.sin_addr, szClientIP, 16);
		
		WCHAR* roomName = (WCHAR*)buf;
		roomName[ret / 2] = L'\0';
		wprintf(L"[UDP/%s:%d] %s\n", szClientIP, ntohs(peeraddr.sin_port), roomName);
	}

	// Close Socket
	closesocket(sock);
	WSACleanup();
	return 0;
}
