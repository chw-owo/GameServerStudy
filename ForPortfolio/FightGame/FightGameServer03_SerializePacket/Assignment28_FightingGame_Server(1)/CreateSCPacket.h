#pragma once
#include "PacketDefine.h"
#include "SerializePacket.h"

void Create_Packet_HEADER(stPACKET_HEADER& header, char Size, char Type);

int Create_Packet_SC_CREATE_MY_CHARACTER(SerializePacket* buffer,
	int ID, char Direction, short X, short Y, char HP);

int Create_Packet_SC_CREATE_OTHER_CHARACTER(SerializePacket* buffer,
	int ID, char Direction, short X, short Y, char HP);

int Create_Packet_SC_DELETE_CHARACTER(SerializePacket* buffer, int ID);

int Create_Packet_SC_MOVE_START(SerializePacket* buffer,
	int ID, char Direction, short X, short Y);

int Create_Packet_SC_MOVE_STOP(SerializePacket* buffer,
	int ID, char Direction, short X, short Y);

int Create_Packet_SC_ATTACK1(SerializePacket* buffer,
	int ID, char Direction, short X, short Y);

int Create_Packet_SC_ATTACK2(SerializePacket* buffer,
	int ID, char Direction, short X, short Y);

int Create_Packet_SC_ATTACK3(SerializePacket* buffer,
	int ID, char Direction, short X, short Y);

int Create_Packet_SC_DAMAGE(SerializePacket* buffer,
	int AttackID, int DamageID, char DamageHP);

int Create_Packet_SC_SYNC(SerializePacket* buffer, short X, short Y);