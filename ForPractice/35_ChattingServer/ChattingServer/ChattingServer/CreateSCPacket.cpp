#include "CreateSCPacket.h"
#include "SerializePacket.h"
#include <stdio.h>

void CreatePacket_HEADER(st_PACKET_HEADER& header, BYTE checkSum, WORD type, WORD size)
{
	header.byCode = dfPACKET_CODE;
	header.byCheckSum = checkSum;
	header.wMsgType = type;
	header.wPayloadSize = size;
}

int CreatePacket_RES_LOGIN(SerializePacket* buffer, BYTE result, int userID)
{
	st_PACKET_HEADER header;
	int size = sizeof(result) + sizeof(userID);
	CreatePacket_HEADER(header, 0, df_RES_LOGIN, size);

	buffer->PutData((char*)&header, dfHEADER_SIZE);
	*buffer << result;
	*buffer << userID;

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		printf("Create Packet Error, %d != %d: func %s, line %d - ",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}

int CreatePacket_RES_ROOM_LIST(SerializePacket* buffer, int roomID, short roomNameLen, wchar_t* roomName, BYTE headcount, wchar_t* userNames)
{
	st_PACKET_HEADER header;
	int size = sizeof(roomID) + sizeof(roomNameLen) + roomNameLen
		+ sizeof(headcount) + headcount * sizeof(wchar_t) * dfNICK_MAX_LEN;
	CreatePacket_HEADER(header, 0, df_RES_ROOM_LIST, size);

	buffer->PutData((char*)&header, dfHEADER_SIZE);
	*buffer << roomID;
	*buffer << roomNameLen;
	buffer->PutData((char*)roomName, roomNameLen);
	*buffer << headcount;
	buffer->PutData((char*)userNames, headcount * sizeof(wchar_t) * dfNICK_MAX_LEN);

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		printf("Create Packet Error, %d != %d: func %s, line %d - ",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}


int CreatePacket_RES_ROOM_CREATE(SerializePacket* buffer, int roomID, short roomNameLen, wchar_t* roomName)
{
	st_PACKET_HEADER header;
	int size = sizeof(roomID) + sizeof(roomNameLen) + roomNameLen;
	CreatePacket_HEADER(header, 0, df_RES_ROOM_CREATE, size);

	buffer->PutData((char*)&header, dfHEADER_SIZE);
	*buffer << roomID;
	*buffer << roomNameLen;
	buffer->PutData((char*)roomName, roomNameLen);

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		printf("Create Packet Error, %d != %d: func %s, line %d - ",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}


int CreatePacket_RES_ROOM_ENTER(SerializePacket* buffer, int roomID, short roomNameLen, wchar_t* roomName, BYTE headcount, wchar_t* userNames, int* userNums)
{
	st_PACKET_HEADER header;
	int size = sizeof(roomID) + sizeof(roomNameLen) + roomNameLen
		+ sizeof(headcount) + headcount * (sizeof(wchar_t) * dfNICK_MAX_LEN + sizeof(int));
	CreatePacket_HEADER(header, 0, df_RES_ROOM_ENTER, size);

	buffer->PutData((char*)&header, dfHEADER_SIZE);
	*buffer << roomID;
	*buffer << roomNameLen;
	buffer->PutData((char*)roomName, roomNameLen);
	*buffer << headcount;
	for (int i = 0; i < headcount; i++)
	{
		buffer->PutData((char*)(&userNames[i]), sizeof(wchar_t) * dfNICK_MAX_LEN);
		buffer->PutData((char*)(&userNums[i]), sizeof(int));
	}

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		printf("Create Packet Error, %d != %d: func %s, line %d - ",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}


int CreatePacket_RES_CHAT(SerializePacket* buffer, int userID, short msgLen, wchar_t* msg)
{
	st_PACKET_HEADER header;
	int size = sizeof(userID) + sizeof(msgLen) + msgLen;
	CreatePacket_HEADER(header, 0, df_RES_CHAT, size);

	buffer->PutData((char*)&header, dfHEADER_SIZE);
	*buffer << userID;
	*buffer << msgLen;
	buffer->PutData((char*)msg, msgLen);

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		printf("Create Packet Error, %d != %d: func %s, line %d - ",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}

int CreatePacket_RES_ROOM_LEAVE(SerializePacket* buffer, int userID)
{
	st_PACKET_HEADER header;
	int size = sizeof(userID);
	CreatePacket_HEADER(header, 0, df_RES_ROOM_LEAVE, size);

	buffer->PutData((char*)&header, dfHEADER_SIZE);
	*buffer << userID;

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		printf("Create Packet Error, %d != %d: func %s, line %d - ",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}


int CreatePacket_RES_ROOM_DELETE(SerializePacket* buffer, int roomID)
{
	st_PACKET_HEADER header;
	int size = sizeof(roomID);
	CreatePacket_HEADER(header, 0, df_RES_ROOM_DELETE, size);

	buffer->PutData((char*)&header, dfHEADER_SIZE);
	*buffer << roomID;

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		printf("Create Packet Error, %d != %d: func %s, line %d - ",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}


int CreatePacket_RES_USER_ENTER(SerializePacket* buffer, wchar_t* userName, int userID)
{
	st_PACKET_HEADER header;
	int size = sizeof(wchar_t) * dfNICK_MAX_LEN + sizeof(userID);
	CreatePacket_HEADER(header, 0, df_RES_LOGIN, size);

	buffer->PutData((char*)&header, dfHEADER_SIZE);
	buffer->PutData((char*)userName, sizeof(wchar_t) * dfNICK_MAX_LEN);
	*buffer << userID;

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		printf("Create Packet Error, %d != %d: func %s, line %d - ",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}
