#include "CreateSCPacket.h"
 

void Create_Packet_HEADER(stPACKET_HEADER &header, char Size, char Type)
{
	header.Code = 0x89;
	header.Size = Size;
	header.Type = Type;
}

int Create_Packet_SC_CREATE_MY_CHARACTER(SerializePacket* buffer,
	int ID, char Direction, short X, short Y, char HP)
{
	int size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y) + sizeof(HP);
	stPACKET_HEADER header;
	Create_Packet_HEADER(header, size, dfPACKET_SC_CREATE_MY_CHARACTER);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;
	*buffer << HP;

	return size + HEADER_SIZE;
}

int Create_Packet_SC_CREATE_OTHER_CHARACTER(SerializePacket* buffer,
	int ID, char Direction, short X, short Y, char HP)
{
	int size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y) + sizeof(HP);
	stPACKET_HEADER header;
	Create_Packet_HEADER(header, size, dfPACKET_SC_CREATE_OTHER_CHARACTER);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;
	*buffer << HP;

	return size + HEADER_SIZE;
}


int Create_Packet_SC_DELETE_CHARACTER(SerializePacket* buffer, int ID)
{
	int size = sizeof(ID);
	stPACKET_HEADER header;
	Create_Packet_HEADER(header, size, dfPACKET_SC_DELETE_CHARACTER);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;

	return size + HEADER_SIZE;
}


int Create_Packet_SC_MOVE_START(SerializePacket* buffer,
	int ID, char Direction, short X, short Y)
{
	int size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y);
	stPACKET_HEADER header;
	Create_Packet_HEADER(header, size, dfPACKET_SC_MOVE_START);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;

	return size + HEADER_SIZE;
}


int Create_Packet_SC_MOVE_STOP(SerializePacket* buffer,
	int ID, char Direction, short X, short Y)
{
	int size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y);
	stPACKET_HEADER header;
	Create_Packet_HEADER(header, size, dfPACKET_SC_MOVE_STOP);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;

	return size + HEADER_SIZE;
}


int Create_Packet_SC_ATTACK1(SerializePacket* buffer,
	int ID, char Direction, short X, short Y)
{
	int size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y);
	stPACKET_HEADER header;
	Create_Packet_HEADER(header, size, dfPACKET_SC_ATTACK1);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;

	return size + HEADER_SIZE;
}


int Create_Packet_SC_ATTACK2(SerializePacket* buffer,
	int ID, char Direction, short X, short Y)
{
	int size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y);
	stPACKET_HEADER header;
	Create_Packet_HEADER(header, size, dfPACKET_SC_ATTACK2);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;

	return size + HEADER_SIZE;
}


int Create_Packet_SC_ATTACK3(SerializePacket* buffer,
	int ID, char Direction, short X, short Y)
{
	int size = sizeof(ID) + sizeof(Direction) + sizeof(X) + sizeof(Y);
	stPACKET_HEADER header;
	Create_Packet_HEADER(header, size, dfPACKET_SC_ATTACK3);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << ID;
	*buffer << Direction;
	*buffer << X;
	*buffer << Y;

	return size + HEADER_SIZE;
}



int Create_Packet_SC_DAMAGE(SerializePacket* buffer,
	int AttackID, int DamageID, char DamageHP)
{
	int size = sizeof(AttackID) + sizeof(DamageID) + sizeof(DamageHP);
	stPACKET_HEADER header;
	Create_Packet_HEADER(header, size, dfPACKET_SC_DAMAGE);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << AttackID;
	*buffer << DamageID;
	*buffer << DamageHP;

	return size + HEADER_SIZE;
}


int Create_Packet_SC_SYNC(SerializePacket* buffer, short X, short Y)
{
	int size = sizeof(X) + sizeof(Y);
	stPACKET_HEADER header;
	Create_Packet_HEADER(header, size, dfPACKET_SC_SYNC);

	buffer->PutData((char*)&header, HEADER_SIZE);

	*buffer << X;
	*buffer << Y;

	return size + HEADER_SIZE;
}
