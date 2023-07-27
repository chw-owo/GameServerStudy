#include "CreateSCPacket.h"
#include <stdio.h>

void CreatePacket_HEADER(st_PACKET_HEADER& header, BYTE checkSum, WORD type, WORD size)
{
	header.byCode = dfPACKET_CODE;
	header.byCheckSum = checkSum;
	header.wMsgType = type;
	header.wPayloadSize = size;
}

BYTE GetChecksum(WORD type, SerializePacket* payload)
{
	int sum = 0;
	BYTE checksum = 0;

	sum += (type & 0x00ff);
	sum += (type & 0xff00);
	
	int payloadSize = payload->GetDataSize();
	unsigned char* tmp = new unsigned char[payloadSize];
	payload->PeekData((char*)tmp, payloadSize);

	for (WORD i = 0; i < payloadSize; i++)
	{
		sum += tmp[i];
	}

	checksum = sum % 256;
	return checksum;
}

int CreatePacket_RES_LOGIN(SerializePacket* buffer, BYTE result, int userID)
{
	int size;
	SerializePacket payload;

	if (result == df_RESULT_LOGIN_OK)
	{
		size = sizeof(result) + sizeof(userID);
		payload << result;
		payload << userID;
	}
	else
	{
		size = sizeof(result);
		payload << result;
	}

	if (payload.GetDataSize() != size)
	{
		::printf("Create Payload Error, %d != %d: func %s, line %d\n",
			payload.GetDataSize(), size, __func__, __LINE__);
	}

	st_PACKET_HEADER header;
	BYTE checksum = GetChecksum(df_RES_LOGIN, &payload);
	CreatePacket_HEADER(header, checksum, df_RES_LOGIN, size);
	buffer->PutData((char*)&header, dfHEADER_SIZE);
	buffer->PutData(payload.GetReadPtr(), size);

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}
	
	return buffer->GetDataSize();
}

int CreatePacket_RES_ROOM_LIST(SerializePacket* buffer, 
	int roomCnt, int* roomIDs, short* roomNameLens, wchar_t* roomNames, BYTE* headcounts, wchar_t* userNames)
{
	int size;
	SerializePacket payload;

	if (roomCnt == 0)
	{
		size = sizeof(roomCnt);
		payload << roomCnt;
	}
	else 
	{
		int roomNamesTotalSize = 0;
		int userNamesTotalSize = 0;
		for (int i = 0; i < roomCnt; i++)
		{
			roomNamesTotalSize += roomNameLens[i];
			userNamesTotalSize += headcounts[i];
		}

		userNamesTotalSize *= (sizeof(wchar_t) * dfNICK_MAX_LEN);

		size = sizeof(roomCnt) +
			roomCnt * (sizeof(int) + sizeof(short) + sizeof(BYTE)) +
			roomNamesTotalSize + userNamesTotalSize;
		
		payload << roomCnt;
		int roomNamesIdx = 0;
		int userNamesIdx = 0;
		for (int i = 0; i < roomCnt; i++)
		{
			payload << roomIDs[i];
			payload << roomNameLens[i];
			payload.PutData((char*)(&roomNames[roomNamesIdx]), roomNameLens[i]);
			roomNamesIdx += roomNameLens[i];
			payload << headcounts[i];
			for (int j = 0; j < headcounts[i]; j++)
			{
				payload.PutData((char*)(&userNames[userNamesIdx]), sizeof(wchar_t) * dfNICK_MAX_LEN);
				userNamesIdx += sizeof(wchar_t) * dfNICK_MAX_LEN;
			}
		}
	}

	if (payload.GetDataSize() != size)
	{
		::printf("Create Payload Error, %d != %d: func %s, line %d\n",
			payload.GetDataSize(), size, __func__, __LINE__);
	}

	st_PACKET_HEADER header;
	BYTE checksum = GetChecksum(df_RES_ROOM_LIST, &payload);
	CreatePacket_HEADER(header, checksum, df_RES_ROOM_LIST, size);
	buffer->PutData((char*)&header, dfHEADER_SIZE);
	buffer->PutData(payload.GetReadPtr(), size);

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}


int CreatePacket_RES_ROOM_CREATE(SerializePacket* buffer, BYTE result, int roomID, short roomNameLen, wchar_t* roomName)
{
	::wprintf(L"(Debug) roomID-%d, roomNameLen-%d, roomName-%s\n", 
		roomID, roomNameLen, roomName);

	int size;
	SerializePacket payload;
	if(result == df_RESULT_ROOM_CREATE_OK)
	{
		size = sizeof(result) + sizeof(roomID) + sizeof(roomNameLen) + roomNameLen;
		
		payload << result;
		payload << roomID;
		payload << roomNameLen;
		payload.PutData((char*)roomName, roomNameLen);

	}
	else
	{
		size = sizeof(result);
		payload << result;
	}

	if (payload.GetDataSize() != size)
	{
		::printf("Create Payload Error, %d != %d: func %s, line %d\n",
			payload.GetDataSize(), size, __func__, __LINE__);
	}

	st_PACKET_HEADER header;
	BYTE checksum = GetChecksum(df_RES_ROOM_CREATE, &payload);
	CreatePacket_HEADER(header, checksum, df_RES_ROOM_CREATE, size);
	buffer->PutData((char*)&header, dfHEADER_SIZE);
	buffer->PutData(payload.GetReadPtr(), size);

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}


int CreatePacket_RES_ROOM_ENTER(SerializePacket* buffer, BYTE result, int roomID, short roomNameLen, wchar_t* roomName, BYTE headcount, wchar_t* userNames, int* userNums)
{
	int size;
	SerializePacket payload;

	if (result == df_RESULT_ROOM_ENTER_OK)
	{
		size = sizeof(result) + sizeof(roomID) + sizeof(roomNameLen) + roomNameLen
			+ sizeof(headcount) + headcount * (sizeof(wchar_t) * dfNICK_MAX_LEN + sizeof(int));

		payload << result;
		payload << roomID;
		payload << roomNameLen;
		payload.PutData((char*)roomName, roomNameLen);
		payload << headcount;

		for (int i = 0; i < headcount; i++)
		{
			payload.PutData((char*)(&userNames[i * dfNICK_MAX_LEN]), sizeof(wchar_t) * dfNICK_MAX_LEN);
			payload.PutData((char*)(&userNums[i]), sizeof(int));
		}
	}
	else
	{
		size = sizeof(result);
		payload << result;
	}

	if (payload.GetDataSize() != size)
	{
		::printf("Create Payload Error, %d != %d: func %s, line %d\n",
			payload.GetDataSize(), size, __func__, __LINE__);
	}

	st_PACKET_HEADER header;
	BYTE checksum = GetChecksum(df_RES_ROOM_ENTER, &payload);
	CreatePacket_HEADER(header, checksum, df_RES_ROOM_ENTER, size);
	buffer->PutData((char*)&header, dfHEADER_SIZE);
	buffer->PutData(payload.GetReadPtr(), size);

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}


int CreatePacket_RES_CHAT(SerializePacket* buffer, int userID, short msgSize, wchar_t* msg)
{
	
	int size = sizeof(userID) + sizeof(msgSize) + msgSize;
	SerializePacket payload;

	payload << userID;
	payload << msgSize;
	payload.PutData((char*)msg, msgSize);

	if (payload.GetDataSize() != size)
	{
		::printf("Create Payload Error, %d != %d: func %s, line %d\n",
			payload.GetDataSize(), size, __func__, __LINE__);
	}

	st_PACKET_HEADER header;
	BYTE checksum = GetChecksum(df_RES_CHAT, &payload);
	CreatePacket_HEADER(header, checksum, df_RES_CHAT, size);
	buffer->PutData((char*)&header, dfHEADER_SIZE);
	buffer->PutData(payload.GetReadPtr(), size);

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}

int CreatePacket_RES_ROOM_LEAVE(SerializePacket* buffer, int userID)
{
	int size = sizeof(userID);
	SerializePacket payload;

	payload << userID;

	if (payload.GetDataSize() != size)
	{
		::printf("Create Payload Error, %d != %d: func %s, line %d\n",
			payload.GetDataSize(), size, __func__, __LINE__);
	}

	st_PACKET_HEADER header;
	BYTE checksum = GetChecksum(df_RES_LOGIN, &payload);
	CreatePacket_HEADER(header, checksum, df_RES_ROOM_LEAVE, size);
	buffer->PutData((char*)&header, dfHEADER_SIZE);
	buffer->PutData(payload.GetReadPtr(), size);

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}


int CreatePacket_RES_ROOM_DELETE(SerializePacket* buffer, int roomID)
{
	int size = sizeof(roomID);
	SerializePacket payload;
	
	payload << roomID;

	if (payload.GetDataSize() != size)
	{
		::printf("Create Payload Error, %d != %d: func %s, line %d\n",
			payload.GetDataSize(), size, __func__, __LINE__);
	}

	st_PACKET_HEADER header;
	BYTE checksum = GetChecksum(df_RES_ROOM_DELETE, &payload);
	CreatePacket_HEADER(header, checksum, df_RES_ROOM_DELETE, size);
	buffer->PutData((char*)&header, dfHEADER_SIZE);
	buffer->PutData(payload.GetReadPtr(), size);

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}


int CreatePacket_RES_USER_ENTER(SerializePacket* buffer, wchar_t* userName, int userID)
{
	int size = sizeof(wchar_t) * dfNICK_MAX_LEN + sizeof(userID);
	SerializePacket payload;

	payload.PutData((char*)userName, sizeof(wchar_t) * dfNICK_MAX_LEN);
	payload << userID;

	if (payload.GetDataSize() != size)
	{
		::printf("Create Payload Error, %d != %d: func %s, line %d\n",
			payload.GetDataSize(), size, __func__, __LINE__);
	}

	st_PACKET_HEADER header;
	BYTE checksum = GetChecksum(df_RES_USER_ENTER, &payload);
	CreatePacket_HEADER(header, checksum, df_RES_USER_ENTER, size);
	buffer->PutData((char*)&header, dfHEADER_SIZE);
	buffer->PutData(payload.GetReadPtr(), size);

	if (buffer->GetDataSize() != size + dfHEADER_SIZE)
	{
		::printf("Create Packet Error, %d != %d: func %s, line %d\n",
			buffer->GetDataSize(), size + dfHEADER_SIZE, __func__, __LINE__);
	}

	return buffer->GetDataSize();
}
