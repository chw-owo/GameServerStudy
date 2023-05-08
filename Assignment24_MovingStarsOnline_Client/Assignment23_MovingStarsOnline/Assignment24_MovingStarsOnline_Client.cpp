#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <iostream>
#include "Message.h"
#include "List.h"
#include "Player.h"

#define SERVERPORT 3000
#define MSGSIZE 16
#define IPMAX 16

SOCKET sock;
HANDLE hConsole;
int main(int argc, char* argv[])
{
	// Initialize Winsock
	int err;
	COORD stCoord;
	stCoord.X = 0;
	stCoord.Y = YMAX;
	SetConsoleCursorPosition(hConsole, stCoord);

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// Create Socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		err = ::WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Line %d: %d", __LINE__, err);
			return 0;
		}
	}

	// Get IP Address to connect
	WCHAR IPbuf[IPMAX];
	wprintf(L"Enter IP Address to connect: ");
	wscanf_s(L"%s", IPbuf, IPMAX);
	wprintf(L"\n");

	// Connect Socket to Server
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	InetPton(AF_INET, IPbuf, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);

	int ret;
	ret = connect(sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (ret == SOCKET_ERROR)
	{
		err = ::WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Line %d: %d", __LINE__, err);
			return 0;
		}
	}

	// Set Socket Non-Blocking Mode
	u_long on = 1;
	ret = ioctlsocket(sock, FIONBIO, &on);
	if (ret == SOCKET_ERROR)
	{
		err = ::WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			printf("Error! Line %d: %d", __LINE__, err);
			return 0;
		}
	}

	// Setting For Players
	CList<Player*>::iterator iter;

	// Setting For Network
	char recvBuf[MSGSIZE] = { 0 };
	FD_SET rset, wset;

	// Setting For Keyboard IO
	__int32 prevX = 0;
	__int32 prevY = 0;
	clock_t timecheck = clock();

	// Setting For Render
	
	CONSOLE_CURSOR_INFO stConsoleCursor;
	stConsoleCursor.bVisible = FALSE;
	stConsoleCursor.dwSize = 1;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorInfo(hConsole, &stConsoleCursor);
	char screenBuffer[YMAX][XMAX] = { ' ', };
	
	while (1)
	{
		// Network =================================
#pragma region Network

		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(sock, &rset);
		FD_SET(sock, &wset);

		// Select 
		TIMEVAL time;
		time.tv_usec = 0;
		ret = select(0, &rset, &wset, NULL, &time);
		if (ret == SOCKET_ERROR)
		{
			err = WSAGetLastError();
			if (err != WSAEWOULDBLOCK)
			{
				printf("Error! Line %d: %d", __LINE__, err);
				break;
			}
		}
		else if (ret > 0)
		{
			if (FD_ISSET(sock, &wset))
			{
				// Send Data to Server	
				if (gMyPlayer != nullptr &&
					(gMyPlayer->X != prevX || gMyPlayer->Y != prevY))
				{
					MSG_MOVE Msg;
					Msg.type = TYPE_MOVE;
					Msg.ID = gMyPlayer->ID;
					Msg.X = gMyPlayer->X;
					Msg.Y = gMyPlayer->Y;

					ret = send(sock, (char*)&Msg, MSGSIZE, 0);
					if (ret == SOCKET_ERROR)
					{
						err = WSAGetLastError();
						if (err != WSAEWOULDBLOCK)
						{
							printf("Error! Line %d: %d", __LINE__, err);
							break;
						}
					}

					prevX = gMyPlayer->X;
					prevY = gMyPlayer->Y;
				}
			}
			

			// Reveice Data from Server
			if (FD_ISSET(sock, &rset))
			{
				ret = recv(sock, recvBuf, MSGSIZE, 0);
				if (ret == SOCKET_ERROR)
				{
					err = WSAGetLastError();
					if (err != WSAEWOULDBLOCK)
					{
						printf("Error! Line %d: %d", __LINE__, err);
						break;
					}
				}

				MSG_TYPE* pType = (MSG_TYPE*)recvBuf;
				switch (*pType)
				{
				case TYPE_ID:
				{
					MSG_ID* pMsg = (MSG_ID*)recvBuf;
					ID = pMsg->ID;
					break;
				}

				case TYPE_CREATE:
				{
					MSG_CREATE* pMsg = (MSG_CREATE*)recvBuf;
					if (ID == pMsg->ID && gMyPlayer == nullptr)
					{
						gMyPlayer = new Player;
						gMyPlayer->ID = pMsg->ID;
						gMyPlayer->X = pMsg->X;
						gMyPlayer->Y = pMsg->Y;
					}
					else if (ID != pMsg->ID)
					{
						Player* player = new Player;
						player->ID = pMsg->ID;
						player->X = pMsg->X;
						player->Y = pMsg->Y;
						gPlayerList.push_back(player);
					}
					break;
				}

				case TYPE_DELETE:
				{
					MSG_DELETE* pMsg = (MSG_DELETE*)recvBuf;
					if (gMyPlayer != nullptr && gMyPlayer->ID == pMsg->ID)
					{
						delete gMyPlayer;
						gMyPlayer = nullptr;
					}
					else if (gMyPlayer == nullptr || gMyPlayer->ID != pMsg->ID)
					{
						for (iter = gPlayerList.begin(); iter != gPlayerList.end(); )
						{
							if ((*iter)->ID == pMsg->ID)
							{
								delete (*iter);
								iter = gPlayerList.erase(iter);
								break;
							}
							else
							{
								++iter;
							}
						}
					}
					break;
				}

				case TYPE_MOVE:
				{
					MSG_MOVE* pMsg = (MSG_MOVE*)recvBuf;

					for (iter = gPlayerList.begin(); iter != gPlayerList.end(); ++iter)
					{
						if ((*iter)->ID == pMsg->ID)
						{
							(*iter)->X = pMsg->X;
							(*iter)->Y = pMsg->Y;
							break;
						}
					}
					break;
				}

				default:
					break;
				}
			}
		}

#pragma endregion 

		// Keyboard IO ===================================
#pragma region KeyboardIO

		if (gMyPlayer != nullptr && clock() - timecheck > 30)
		{
			if (GetAsyncKeyState(VK_LEFT) && gMyPlayer->X > 0)
				gMyPlayer->X--;
			if (GetAsyncKeyState(VK_RIGHT) && gMyPlayer->X < XMAX - 2)
				gMyPlayer->X++;
			if (GetAsyncKeyState(VK_UP) && gMyPlayer->Y > 0)
				gMyPlayer->Y--;
			if (GetAsyncKeyState(VK_DOWN) && gMyPlayer->Y < YMAX - 2)
				gMyPlayer->Y++;

			timecheck = clock();
		}

#pragma endregion

		// Render ==================================
#pragma region Render

// Buffer Clear
		memset(screenBuffer, ' ', YMAX * XMAX);
		for (int i = 0; i < YMAX; i++)
		{
			screenBuffer[i][XMAX - 1] = '\0';
		}

		// Set Buffer
		
		if (gMyPlayer != nullptr)
		{
			if (gMyPlayer->X > XMAX - 2) gMyPlayer->X = XMAX - 2;
			else if (gMyPlayer->X <= 0) gMyPlayer->X = 0;
			if (gMyPlayer->Y > YMAX - 1) gMyPlayer->Y = YMAX - 1;
			else if (gMyPlayer->Y <= 0) gMyPlayer->Y = 0;
			screenBuffer[gMyPlayer->Y][gMyPlayer->X] = '*';
		}
			

		for (iter = gPlayerList.begin(); iter != gPlayerList.end(); ++iter)
		{
			if ((*iter)->X > XMAX - 2) (*iter)->X = XMAX - 2;
			else if ((*iter)->X <= 0) (*iter)->X = 0;
			if ((*iter)->Y > YMAX - 1) (*iter)->Y = YMAX - 1;
			else if ((*iter)->Y <= 0) (*iter)->Y = 0;

			screenBuffer[(*iter)->Y][(*iter)->X] = '*';
		}
			
		
		/*
		int idx = 0;
		if (gMyPlayer != nullptr)
			sprintf_s(screenBuffer[idx], XMAX, "My: %d-%d", gMyPlayer->X, gMyPlayer->Y);

		for (iter = gPlayerList.begin(); iter != gPlayerList.end(); ++iter)
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
		
	#pragma endregion
	}


	closesocket(sock);
	WSACleanup();

	// Delete Players' Data
	if (gMyPlayer != nullptr)
		delete gMyPlayer; 
	for (iter = gPlayerList.begin(); iter != gPlayerList.end(); ++iter)
		delete (*iter);

	return 0;
}
