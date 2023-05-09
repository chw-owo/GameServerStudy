#pragma once
#include "RingBuffer.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <ws2tcpip.h>
#include "stdio.h"

#define UM_SOCKET (WM_USER+1)
struct Session
{
	SOCKET sock;
	RingBuffer recvBuf;
	RingBuffer sendBuf;
	bool bConnected = false;
	bool bCanWrite = false;
};

extern Session g_Session;

#pragma pack(1)
struct Header
{
	unsigned short len;
};

struct DRAW_PACKET
{
	int iStartX;
	int iStartY;
	int iEndX;
	int iEndY;
};
#pragma pack(pop)

bool InitialSocket(HWND hWnd);
void ProcessSocketMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void HandleReadEvent();
void HandleWriteEvent();
void HandleCloseEvent();

void EnqueueSendData(char* data, int len);
void SendUnicast();

// For Debugging
//#define NETWORK_DEBUG
inline void DebugTest(const char* func, int line)
{
#ifdef NETWORK_DEBUG
	printf("Success %s: line %d\n", func, line);
#endif
}
#define _DebugTest DebugTest(__func__, __LINE__)