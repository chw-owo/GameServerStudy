#include "Network.h"
#include "stdio.h"
#pragma comment(lib, "Ws2_32.lib")

#define SERVERIP L"192.168.30.13"
#define SERVERPORT 25000
#define HEADER_LEN sizeof(Header)

Session g_Session;

bool InitialSocket(HWND hWnd)
{
	int ret;
	int err;
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	// Create Socket
	g_Session.sock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_Session.sock == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			return false;
		}
	}

	// Connect Socket to Server
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);

	// AsyncSelect the Socket
	ret = WSAAsyncSelect(g_Session.sock, hWnd, UM_SOCKET,
				FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE);
	if (ret == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		return false;
	}

	// Try Connect
	ret = connect(g_Session.sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (ret == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			return false;
		}
	}

	_DebugTest;

	return true;
}

void ProcessSocketMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);

	if (WSAGETSELECTERROR(lParam))
	{
		if (WSAGETSELECTERROR(lParam) != WSAEWOULDBLOCK)
		{
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__,
				WSAGETSELECTERROR(lParam));
			HandleCloseEvent();
			return;
		}
	}

	switch (WSAGETSELECTEVENT(lParam))
	{
	case FD_CONNECT:
		g_Session.bConnected = true;
		break;

	case FD_READ:
		if (g_Session.bConnected)
			HandleReadEvent(hWnd);
		break;

	case FD_WRITE:
		if (g_Session.bConnected)
			HandleWriteEvent();
		break;

	case FD_CLOSE:
		if (g_Session.bConnected)
			HandleCloseEvent();
		break;
	}
}

HPEN g_hPen;

void HandleReadEvent(HWND hWnd)
{
	char buf[MAX_BUF_SIZE];
	int recvRet = recv(g_Session.sock, buf, MAX_BUF_SIZE, 0);
	if (recvRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
			return;
		}
	}

	int enqueueRet = g_Session.recvBuf.Enqueue(buf, recvRet);
	if (recvRet != enqueueRet)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		return;
	}

	int useSize = g_Session.recvBuf.GetUseSize();
	while (useSize > 0)
	{
		if (useSize <= HEADER_LEN)
			break;

		Header header;
		int peekRet = g_Session.recvBuf.Peek((char*)&header, HEADER_LEN);
		if (peekRet != HEADER_LEN)
		{
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return;
		}

		if (useSize < HEADER_LEN + header.len)
			break;

		DRAW_PACKET drawPacket;
		int moveReadRet = g_Session.recvBuf.MoveReadPos(HEADER_LEN);
		if (moveReadRet != HEADER_LEN)
		{
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return;
		}

		int dequeueRet = g_Session.recvBuf.Dequeue((char*)&drawPacket, header.len);
		if (dequeueRet != header.len)
		{
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			return;
		}

		printf("Other %d bytes: %d-%d, %d-%d\n", header.len,
			drawPacket.iStartX, drawPacket.iStartY,
			drawPacket.iEndX, drawPacket.iEndY);

		HDC hdc = GetDC(hWnd);
		HPEN hPen = (HPEN)SelectObject(hdc, g_hPen);
		MoveToEx(hdc, drawPacket.iStartX, drawPacket.iStartY, NULL);
		LineTo(hdc, drawPacket.iEndX, drawPacket.iEndY);
		SelectObject(hdc, hPen);
		ReleaseDC(hWnd, hdc);

		useSize = g_Session.recvBuf.GetUseSize();

	}

	_DebugTest;
}

void HandleWriteEvent()
{
	g_Session.bCanWrite = true;
	SendUnicast();
	_DebugTest;
}

void HandleCloseEvent()
{
	closesocket(g_Session.sock);
	WSACleanup();
	_DebugTest;
}

void SendUnicast()
{
	if (!g_Session.bCanWrite || g_Session.sendBuf.GetUseSize() <= 0)
		return;

	char buf[MAX_BUF_SIZE];
	int sendBufSize = g_Session.sendBuf.GetUseSize();

	int peekRet = g_Session.sendBuf.Peek(buf, sendBufSize);
	if (peekRet != sendBufSize)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		return;
	}

	int sendRet = send(g_Session.sock, buf, peekRet, 0);
	if (sendRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Func %s Line %d: %d\n", __func__, __LINE__, err);
		}
		return;
	}

	g_Session.sendBuf.MoveReadPos(sendRet);
	_DebugTest;
}


void EnqueueSendData(char* data, unsigned short len)
{
	int enqueueRet;

	// Enqueue Header
	Header header;
	header.len = len;
	enqueueRet = g_Session.sendBuf.Enqueue((char*)&header, HEADER_LEN);
	if (enqueueRet != HEADER_LEN)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		return;
	}

	// Enqueue Payload
	enqueueRet = g_Session.sendBuf.Enqueue(data, len);
	if (enqueueRet != len)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		return;
	}

	DRAW_PACKET* drawPacket = (DRAW_PACKET*)data;
	printf("Me: %d-%d, %d-%d\n",
		drawPacket->iStartX, drawPacket->iStartY,
		drawPacket->iEndX, drawPacket->iEndY);

	_DebugTest;
}
