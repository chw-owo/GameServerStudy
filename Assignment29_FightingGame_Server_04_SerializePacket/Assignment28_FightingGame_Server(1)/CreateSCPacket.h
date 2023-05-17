#pragma once
#include "PacketDefine.h"
#include "SerializeBuffer.h"

void Create_PACKET_HEADER(stPACKET_HEADER& header, uint8 Size, uint8 Type);

int Create_PACKET_SC_CREATE_MY_CHARACTER(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y, int8 HP);

int Create_PACKET_SC_CREATE_OTHER_CHARACTER(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y, int8 HP);

int Create_PACKET_SC_DELETE_CHARACTER(SerializeBuffer* buffer, uint32 ID);

int Create_PACKET_SC_MOVE_START(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y);

int Create_PACKET_SC_MOVE_STOP(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y);

int Create_PACKET_SC_ATTACK1(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y);

int Create_PACKET_SC_ATTACK2(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y);

int Create_PACKET_SC_ATTACK3(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y);

int Create_PACKET_SC_DAMAGE(SerializeBuffer* buffer,
	uint32 AttackID, uint32 DamageID, uint8 DamageHP);

int Create_PACKET_SC_SYNC(SerializeBuffer* buffer, uint16 X, uint16 Y);