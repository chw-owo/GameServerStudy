#pragma comment(lib, "ws2_32")
//#include <winsock.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERDOMAIN L"procademyserver.iptime.org"
#define SERVERIP L"127.0.0.1"
#define SERVERPORT 10010
#define BUFSIZE 512
#define DATABUFMAX 1000

BOOL DomainToIP(WCHAR* szDomain, IN_ADDR* pAddr)
{
	ADDRINFOW* pAddrInfo;
	SOCKADDR_IN* pSockAddr;
	if (GetAddrInfo(szDomain, L"0", NULL, &pAddrInfo) != 0)
	{
		return FALSE;
	}	
	pSockAddr = (SOCKADDR_IN*)pAddrInfo->ai_addr;
	*pAddr = pSockAddr->sin_addr;
	FreeAddrInfo(pAddrInfo);
	return TRUE;
}

struct stPACKETHEADER
{
	DWORD dwPacketCode;
	WCHAR szName[32];
	WCHAR szFileName[128];
	int iFileSize;
};

void SendFile(SOCKET& sock)
{
	int ret;

	// Set Header
	stPACKETHEADER header;
	header.dwPacketCode = 0x11223344;
	wcscpy_s(header.szName, 32, L"ChoHyewon\0");
	wcscpy_s(header.szFileName, 128, L"ProgrammingRule\0");

	// Set Data
	FILE* file;
	ret = fopen_s(&file, "ProgrammingRule.jpg", "rb");
	if (ret != 0)
		printf("Fail to open: %d\n", ret);
	fseek(file, 0, SEEK_END);
	header.iFileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	printf("\n========================\n");
	printf("dwPacketCode: %x\n", header.dwPacketCode);
	wprintf(L"szName: %s\n", header.szName);
	wprintf(L"szFileName: %s\n", header.szFileName);
	printf("iFileSize: %d\n", header.iFileSize);
	printf("========================\n\n");

	char* chData = (char*)malloc(header.iFileSize);
	fread(chData, header.iFileSize, 1, file);
	fclose(file);

	// Send Data to Server
	ret = send(sock, (char*)&header, sizeof(header), 0);
	if (ret == SOCKET_ERROR)
	{
		printf("Fail to Send Header!: %d\n", ret);
		return;
	}

	char* ptr = chData;
	int left = header.iFileSize;

	while (left > 0)
	{
		if(left < DATABUFMAX)
			ret = send(sock, ptr, left, 0);
		else
			ret = send(sock, ptr, DATABUFMAX, 0);

		if (ret == SOCKET_ERROR)
		{
			printf("Fail to Send Data!: %d\n", ret);
			break;
		}
		else 
		{
			printf("[TCP Client] Success to Send %d bytes!\n", ret);
		}
		
		left -= ret;
		ptr += ret;
	}

	printf("\n[TCP Client] Finish to Send All!\n");
	free(chData);
	printf("Success to Free\n");

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
	serveraddr.sin_port = htons(SERVERPORT);

	// Get IP from Domain
	IN_ADDR addr;
	DomainToIP((WCHAR*)SERVERDOMAIN, &addr);
	serveraddr.sin_addr = addr;

	// Get IP
	//InetPton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	ret = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (ret == SOCKET_ERROR) 
	{
		printf("Fail to Connect Socket!: %d\n", ret);
		return 0;
	}

	// Send Data to Server
	SendFile(sock);

	// Receive Data from Server
	char buf[4] = { 0 };
	ret = recv(sock, buf, 4, 0);
	if (ret == SOCKET_ERROR)
	{
		printf("Fail to Receive Data!: %d\n", ret);
	}
	else if (ret == 0)
	{
		printf("Disconnect to Server!\n");
	}
	else
	{
		printf("[TCP Client] Success to Reveice %d bytes!\n", ret);
	}

	printf("[Received Data] %x\n", *((int*)buf));
	
	closesocket(sock);
	WSACleanup();
	return 0;
}