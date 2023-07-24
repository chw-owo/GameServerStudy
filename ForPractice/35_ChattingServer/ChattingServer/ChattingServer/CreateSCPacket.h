#pragma once
#include "Protocol.h"
#include "SerializePacket.h"

void CreatePacket_HEADER(st_PACKET_HEADER& header, BYTE checkSum, WORD Type, WORD Size);
int CreatePacket_RES_LOGIN(SerializePacket* buffer, BYTE result, int userID);
int CreatePacket_RES_ROOM_LIST(SerializePacket* buffer, int roomID, short roomNameLen, wchar_t* roomName, BYTE headcount, wchar_t* userNames);
int CreatePacket_RES_ROOM_CREATE(SerializePacket* buffer, int roomID, short roomNameLen, wchar_t* roomName);
int CreatePacket_RES_ROOM_ENTER(SerializePacket* buffer, int roomID, short roomNameLen, wchar_t* roomName, BYTE headcount, wchar_t* userNames, int* userNums);
int CreatePacket_RES_CHAT(SerializePacket* buffer, int userID, short msgLen, wchar_t* msg);
int CreatePacket_RES_ROOM_LEAVE(SerializePacket* buffer, int userID);
int CreatePacket_RES_ROOM_DELETE(SerializePacket* buffer, int roomID);
int CreatePacket_RES_USER_ENTER(SerializePacket* buffer, wchar_t* userName, int userID);

