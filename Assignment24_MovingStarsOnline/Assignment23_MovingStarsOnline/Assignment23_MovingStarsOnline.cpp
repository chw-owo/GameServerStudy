#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <iostream>
#include "List.h"

#define SERVERPORT 3000
#define MSGSIZE 16

#define PLAYERMAX 32
#define IPMAX 16
#define XMAX 81
#define YMAX 24

struct Player
{
	__int32 ID = 0;
	__int32 X = 0;
	__int32 Y = 0;
};

SOCKET sock;
HANDLE hConsole;

__int32 ID = -1;
Player* gMyPlayer = nullptr;
CList<Player*> gPlayerList;
char gScreenBuffer[YMAX][XMAX] = { ' ', };

enum MSG_TYPE
{
	TYPE_ID, 
	TYPE_CREATE, 
	TYPE_DELETE,
	TYPE_MOVE
};

struct MSG_ID
{
	__int32 type;
	__int32 ID;
	__int32 none1;
	__int32 none2;
};

struct MSG_CREATE
{
	__int32 type;
	__int32 ID;
	__int32 X;
	__int32 Y;
};

struct MSG_DELETE
{
	__int32 type;
	__int32 ID;
	__int32 none1;
	__int32 none2;
};

struct MSG_MOVE
{
	__int32 type;
	__int32 ID;
	__int32 X;
	__int32 Y;
};

// Render Alert Message
void AlertMsg(const char* alertMsg)
{
	char alertBuf[XMAX];
	memset(alertBuf, ' ', XMAX);
	strcpy_s(alertBuf, XMAX, alertMsg);
	
	COORD stCoord;
	stCoord.X = 0;
	stCoord.Y = YMAX;
	SetConsoleCursorPosition(hConsole, stCoord);
	printf(alertBuf);
}

void HandleError(int line)
{
	int err = ::WSAGetLastError();
	if (err == WSAECONNRESET)
	{
		AlertMsg("Server terminated!(code: 10054) Bye~");
		return;
	}
	char alertMsg[XMAX];
	sprintf_s(alertMsg, XMAX, "Error! Line %d: %d", line, err);
	AlertMsg(alertMsg);
}
#define _HandleError HandleError(__LINE__)

int main(int argc, char* argv[])
{
	// Initialize Winsock
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// Create Socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		_HandleError;
		return 0;
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
		_HandleError;
		return 0;
	}

	// Set Socket Non-Blocking Mode
	u_long on = 1;
	ret = ioctlsocket(sock, FIONBIO, &on);
	if (ret == SOCKET_ERROR)
	{
		_HandleError;
		return 0;
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
		time.tv_usec = 300;
		ret = select(0, &rset, &wset, NULL, &time);
		if (ret == SOCKET_ERROR)
		{
			_HandleError;
			break;
		}
		else if (ret > 0)
		{
			// Send Data to Server
			if (FD_ISSET(sock, &wset))
			{
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
						_HandleError;
						continue;
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
					_HandleError;
					continue;
				}
				else if (ret == 0)
				{
					AlertMsg("Disconnect to Server!");
					break;
				}

				int* pType = (int*)recvBuf;
				switch (*pType)
				{
				case TYPE_ID:
				{
					MSG_ID* pMsg = (MSG_ID*)recvBuf;
					ID = pMsg->ID;
					AlertMsg("Success to Alloc ID!");
					break;
				}
					
				case TYPE_CREATE:
				{
					MSG_CREATE* pMsg = (MSG_CREATE*)recvBuf;
					if (ID == pMsg->ID && gMyPlayer == nullptr)
					{
						gMyPlayer = new Player;
						gMyPlayer->ID = pMsg->ID;
						AlertMsg("Success to Create My Data!");
					}
					else if (ID != pMsg->ID)
					{
						Player* player = new Player;
						player->ID = pMsg->ID;
						gPlayerList.push_back(player);
						AlertMsg("Other player come in!");
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
						AlertMsg("Success to Delete My Data!");
					}
					else if (gMyPlayer == nullptr || gMyPlayer->ID != pMsg->ID)
					{
						for (iter = gPlayerList.begin(); iter != gPlayerList.end(); )
						{
							if ((*iter)->ID == pMsg->ID)
							{
								delete (*iter);
								iter = gPlayerList.erase(iter);
								AlertMsg("Other player go out!");
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
						if (pMsg->ID == (*iter)->ID)
						{
							(*iter)->X = pMsg->X;
							(*iter)->Y = pMsg->Y;
							AlertMsg("Success to Get Move Data!");
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

		if (gMyPlayer != nullptr && clock() - timecheck > 50)
		{
			if (GetAsyncKeyState(VK_LEFT) && gMyPlayer->X > 0)
				gMyPlayer->X--;
			else if (GetAsyncKeyState(VK_RIGHT) && gMyPlayer->X < XMAX - 2)
				gMyPlayer->X++;
			else if (GetAsyncKeyState(VK_UP) && gMyPlayer->Y > 0)
				gMyPlayer->Y--;
			else if (GetAsyncKeyState(VK_DOWN) && gMyPlayer->Y < YMAX - 2)
				gMyPlayer->Y++;

			timecheck = clock();
		}
		
		#pragma endregion

		// Render ==================================
		#pragma region Render

		// Buffer Clear
		memset(gScreenBuffer, ' ', YMAX * XMAX);
		for (int i = 0; i < YMAX; i++)
		{
			gScreenBuffer[i][XMAX - 1] = '\0';
		}

		// Set Buffer
		if (gMyPlayer != nullptr)
			gScreenBuffer[gMyPlayer->Y][gMyPlayer->X] = '*';
		for (iter = gPlayerList.begin(); iter != gPlayerList.end(); ++iter)
			gScreenBuffer[(*iter)->Y][(*iter)->X] = '*';

		// Buffer Flip
		for (int i = 0; i < YMAX; i++)
		{
			COORD stCoord;
			stCoord.X = 0;
			stCoord.Y = i;
			SetConsoleCursorPosition(hConsole, stCoord);
			printf(gScreenBuffer[i]);
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