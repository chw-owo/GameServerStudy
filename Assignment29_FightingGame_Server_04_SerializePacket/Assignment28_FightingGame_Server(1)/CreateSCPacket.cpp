#include "CreateSCPacket.h"
#include "Typedef.h"

void Create_PACKET_HEADER(stPACKET_HEADER &header, uint8 Size, uint8 Type)
{
	header.Code = 0x89;
	header.Size = Size;
	header.Type = Type;
}

int Create_PACKET_SC_CREATE_MY_CHARACTER(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y, int8 HP)
{
	uint32 size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y) + sizeof(HP);
	stPACKET_HEADER header;
	Create_PACKET_HEADER(header, size, dfPACKET_SC_CREATE_MY_CHARACTER);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;
	*buffer << HP;

	return size + HEADER_SIZE;
}

int Create_PACKET_SC_CREATE_OTHER_CHARACTER(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y, int8 HP)
{
	uint32 size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y) + sizeof(HP);
	stPACKET_HEADER header;
	Create_PACKET_HEADER(header, size, dfPACKET_SC_CREATE_OTHER_CHARACTER);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;
	*buffer << HP;

	return size + HEADER_SIZE;
}


int Create_PACKET_SC_DELETE_CHARACTER(SerializeBuffer* buffer, uint32 ID)
{
	uint32 size = sizeof(ID);
	stPACKET_HEADER header;
	Create_PACKET_HEADER(header, size, dfPACKET_SC_DELETE_CHARACTER);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;

	return size + HEADER_SIZE;
}


int Create_PACKET_SC_MOVE_START(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y)
{
	uint32 size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y);
	stPACKET_HEADER header;
	Create_PACKET_HEADER(header, size, dfPACKET_SC_MOVE_START);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;

	return size + HEADER_SIZE;
}


int Create_PACKET_SC_MOVE_STOP(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y)
{
	uint32 size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y);
	stPACKET_HEADER header;
	Create_PACKET_HEADER(header, size, dfPACKET_SC_MOVE_STOP);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;

	return size + HEADER_SIZE;
}


int Create_PACKET_SC_ATTACK1(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y)
{
	uint32 size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y);
	stPACKET_HEADER header;
	Create_PACKET_HEADER(header, size, dfPACKET_SC_ATTACK1);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;

	return size + HEADER_SIZE;
}


int Create_PACKET_SC_ATTACK2(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y)
{
	uint32 size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y);
	stPACKET_HEADER header;
	Create_PACKET_HEADER(header, size, dfPACKET_SC_ATTACK2);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;

	return size + HEADER_SIZE;
}


int Create_PACKET_SC_ATTACK3(SerializeBuffer* buffer,
	uint32 ID, uint8 Direction, uint16 X, uint16 Y)
{
	uint32 size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y);
	stPACKET_HEADER header;
	Create_PACKET_HEADER(header, size, dfPACKET_SC_ATTACK3);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;

	return size + HEADER_SIZE;
}



int Create_PACKET_SC_DAMAGE(SerializeBuffer* buffer,
	uint32 AttackID, uint32 DamageID, uint8 DamageHP)
{
	uint32 size = sizeof(AttackID) + sizeof(DamageID) + sizeof(DamageHP);
	stPACKET_HEADER header;
	Create_PACKET_HEADER(header, size, dfPACKET_SC_DAMAGE);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << AttackID;
	*buffer << DamageID;
	*buffer << DamageHP;

	return size + HEADER_SIZE;
}


int Create_PACKET_SC_SYNC(SerializeBuffer* buffer, uint16 X, uint16 Y)
{
	uint32 size = sizeof(X) + sizeof(Y);
	stPACKET_HEADER header;
	Create_PACKET_HEADER(header, size, dfPACKET_SC_SYNC);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << X;
	*buffer << Y;

	return size + HEADER_SIZE;
}
