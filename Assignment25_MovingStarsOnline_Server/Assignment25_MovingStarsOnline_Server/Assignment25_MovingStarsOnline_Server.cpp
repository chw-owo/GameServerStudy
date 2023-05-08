#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <iostream>
#include <stdarg.h>
#include "List.h"
#include "Message.h"
#include "Player.h"
#include "Network.h"

int main(int argc, char* argv[])
{
	// Initialize
	int err;
	int bindRet;
	int ioctRet;
	int listenRet;
	int selectRet;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// Create Socket
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET)
	{
		err = ::WSAGetLastError();
		SetCursor();
		printf("Error! Line %d: %d\n", __LINE__, err);
		return 0;
	}

	// Bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, L"0.0.0.0", &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);

	bindRet = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (bindRet == SOCKET_ERROR)
	{
		err = ::WSAGetLastError();
		SetCursor();
		printf("Error! Line %d: %d\n", __LINE__, err);
		return 0;
	}

	// Listen
	listenRet = listen(listen_sock, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		err = ::WSAGetLastError();
		SetCursor();
		printf("Error! Line %d: %d\n", __LINE__, err);
		return 0;
	}

	// Set Non-Blocking Mode
	u_long on = 1;
	ioctRet = ioctlsocket(listen_sock, FIONBIO, &on);
	if (ioctRet == SOCKET_ERROR)
	{
		err = ::WSAGetLastError();
		SetCursor();
		printf("Error! Line %d: %d\n", __LINE__, err);
		return 0;
	}

	FD_SET rset;
	FD_SET wset;


	CONSOLE_CURSOR_INFO stConsoleCursor;
	stConsoleCursor.bVisible = FALSE;
	stConsoleCursor.dwSize = 1;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorInfo(hConsole, &stConsoleCursor);
	char screenBuffer[YMAX][XMAX] = { ' ', };

	printf("Setting Complete!\n");
	while (1)
	{
		// Network ============================================
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(listen_sock, &rset);
		for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end(); i++)
		{
			FD_SET((*i)->sock, &rset);
			if((*i)->sendBuf.GetUseSize() > 0)
				FD_SET((*i)->sock, &wset);
		}

		// Select 
		selectRet = select(0, &rset, &wset, NULL, NULL);
		if (selectRet == SOCKET_ERROR)
		{
			err = ::WSAGetLastError();
			SetCursor();
			printf("Error! Line %d: %d\n", __LINE__, err);
			break;
		}
		else if (selectRet > 0)
		{
			for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end(); i++)
			{
				if (FD_ISSET((*i)->sock, &wset) && (*i)->alive)
					SendProc((*i));
			}

			if (FD_ISSET(listen_sock, &rset))
				AcceptProc();

			for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end(); i++)
			{
				if (FD_ISSET((*i)->sock, &rset) && (*i)->alive)
				{
					RecvProc((*i));
					SetBuffer((*i));
				}
			}

			if (deleted = true)
				DeleteDeadPlayers();
		}

		// Render ==============================================


		// Buffer Clear
		memset(screenBuffer, ' ', YMAX * XMAX);
		for (int i = 0; i < YMAX; i++)
		{
			screenBuffer[i][XMAX - 1] = '\0';
		}

		// Set Buffer
		for (CList<Player*>::iterator i = gPlayerList.begin(); i != gPlayerList.end(); i++)
		{
			if ((*i)->alive)
			{
				if ((*i)->X > XMAX - 2) (*i)->X = XMAX - 2;
				else if ((*i)->X <= 0) (*i)->X = 0;
				if ((*i)->Y > YMAX - 1) (*i)->Y = YMAX - 1;
				else if ((*i)->Y <= 0) (*i)->Y = 0;

				screenBuffer[(*i)->Y][(*i)->X] = '*';
			}
		}
		sprintf_s(screenBuffer[0], "Connect Client: %02d\n", gPlayerList.size());

		/*
		int idx = 0;
		for (CList<Player*>::iterator iter = gPlayerList.begin(); iter != gPlayerList.end(); ++iter)
		{
			idx++;
			sprintf_s(screenBuffer[idx], XMAX, "%d: %d-%d", (*iter)->ID, (*iter)->X, (*iter)->Y);
		}
		*/

		// Buffer Flip
		for (int i = 0; i < YMAX; i++)
		{
			stCoord.X = 0;
			stCoord.Y = i;
			SetConsoleCursorPosition(hConsole, stCoord);
			printf(screenBuffer[i]);
		}
	}

	closesocket(listen_sock);
	WSACleanup();
	return 0;
}
