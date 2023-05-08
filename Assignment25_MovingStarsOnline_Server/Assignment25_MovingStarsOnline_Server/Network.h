#pragma once
#include "Player.h"
#include <ws2tcpip.h>
#include <iostream>

#define SERVERPORT 3000
#define MSGSIZE 16
#define XMAX 81
#define YMAX 24

extern SOCKET listen_sock;
extern COORD stCoord;
extern HANDLE hConsole;
inline void SetCursor();

void AcceptProc();
void RecvProc(Player* player);
void SetBuffer(Player* player);
void SendProc(Player* player);

void SendUnicast(char* msg, Player* player);
void SendBroadcast(char* msg, Player* expPlayer = nullptr);
void Disconnect(Player* player);
void DeleteDeadPlayers();


