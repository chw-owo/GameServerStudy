#include "CreatePacket.h"

void Create_PACKET_HEADER(
	stPACKET_HEADER &header, 
	uint8 Size, uint8 Type)
{
	header.Code = 0x89;
	header.Size = Size;
	header.Type = Type;
}

void Create_PACKET_SC_CREATE_MY_CHARACTER(
	stPACKET_SC_CREATE_MY_CHARACTER &pakcet, 
	uint32 ID, uint8 Direction, uint16 X, uint16 Y, uint8 HP)
{
	pakcet.ID = ID;
	pakcet.Direction = Direction;
	pakcet.X = X;
	pakcet.Y = Y;
	pakcet.HP = HP;
}

void Create_PACKET_SC_CREATE_OTHER_CHARACTER(
	stPACKET_SC_CREATE_OTHER_CHARACTER& pakcet,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y, uint8 HP)
{
	pakcet.ID = ID;
	pakcet.Direction = Direction;
	pakcet.X = X;
	pakcet.Y = Y;
	pakcet.HP = HP;
}

void Create_PACKET_SC_DELETE_CHARACTER(
	stPACKET_SC_DELETE_CHARACTER& pakcet,
	uint32 ID)
{
	pakcet.ID = ID;
}


void Create_PACKET_CS_MOVE_START(
	stPACKET_CS_MOVE_START& pakcet,
	uint8 Direction, uint16 X, uint16 Y)
{
	pakcet.Direction = Direction;
	pakcet.X = X;
	pakcet.Y = Y;
}

void Create_PACKET_SC_MOVE_START(
	stPACKET_SC_MOVE_START& pakcet,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y)
{
	pakcet.ID = ID;
	pakcet.Direction = Direction;
	pakcet.X = X;
	pakcet.Y = Y;
}

void Create_PACKET_CS_MOVE_STOP(
	stPACKET_CS_MOVE_STOP& pakcet,
	uint8 Direction, uint16 X, uint16 Y)
{
	pakcet.Direction = Direction;
	pakcet.X = X;
	pakcet.Y = Y;
}

void Create_PACKET_SC_MOVE_STOP(
	stPACKET_SC_MOVE_STOP& pakcet,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y)
{
	pakcet.ID = ID;
	pakcet.Direction = Direction;
	pakcet.X = X;
	pakcet.Y = Y;
}

void Create_PACKET_CS_ATTACK1(
	stPACKET_CS_ATTACK1& pakcet,
	uint8 Direction, uint16 X, uint16 Y)
{
	pakcet.Direction = Direction;
	pakcet.X = X;
	pakcet.Y = Y;
}

void Create_PACKET_SC_ATTACK1(
	stPACKET_SC_ATTACK1& pakcet,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y)
{
	pakcet.ID = ID;
	pakcet.Direction = Direction;
	pakcet.X = X;
	pakcet.Y = Y;
}

void Create_PACKET_CS_ATTACK2(
	stPACKET_CS_ATTACK2& pakcet,
	uint8 Direction, uint16 X, uint16 Y)
{
	pakcet.Direction = Direction;
	pakcet.X = X;
	pakcet.Y = Y;
}

void Create_PACKET_SC_ATTACK2(
	stPACKET_SC_ATTACK2& pakcet,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y)
{
	pakcet.ID = ID;
	pakcet.Direction = Direction;
	pakcet.X = X;
	pakcet.Y = Y;
}
void Create_PACKET_CS_ATTACK3(
	stPACKET_CS_ATTACK3& pakcet,
	uint8 Direction, uint16 X, uint16 Y)
{
	pakcet.Direction = Direction;
	pakcet.X = X;
	pakcet.Y = Y;
}

void Create_PACKET_SC_ATTACK3(
	stPACKET_SC_ATTACK3& pakcet,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y)
{
	pakcet.ID = ID;
	pakcet.Direction = Direction;
	pakcet.X = X;
	pakcet.Y = Y;
}


void Create_PACKET_SC_DAMAGE(
	stPACKET_SC_DAMAGE& packet,
	uint32 AttackID, uint32 DamageID, uint8 DamageHP)
{
	packet.AttackID = AttackID;
	packet.DamageID = DamageID;
	packet.DamageHP = DamageHP;
}

void Create_PACKET_CS_SYNC(
	stPACKET_CS_SYNC& packet, 
	uint16 X, uint16 Y)
{
	packet.X = X;
	packet.Y = Y;
}

void Create_PACKET_SC_SYNC(
	stPACKET_SC_SYNC& packet,
	uint16 X, uint16 Y)
{
	packet.X = X;
	packet.Y = Y;
}