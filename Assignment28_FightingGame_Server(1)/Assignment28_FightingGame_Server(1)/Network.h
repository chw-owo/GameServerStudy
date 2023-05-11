#pragma once

bool NetworkInit();
bool NetworkProc();
void NetworkTerminate();

void AcceptProc();
void RecvProc(Player* player);
void SetBuffer(Player* player);
void SendProc(Player* player);

void SendUnicast(char* msg, Player* player);
void SendBroadcast(char* msg, Player* expPlayer = nullptr);
void Disconnect(Player* player);
void DeleteDeadPlayers();


