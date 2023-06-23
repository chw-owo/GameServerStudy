#include "User.h"
#include "Session.h"
#include "ContentData.h"
#include "SerializePacket.h"
#include <stdio.h>
#include <algorithm>

User::~User()
{
}

void User::DequeueRecvBuf()
{
	int useSize = _pSession->_recvBuf.GetUseSize();
	while (useSize > 0)
	{
		if (useSize <= dfHEADER_SIZE)
			break;

		st_PACKET_HEADER header;
		int peekRet = _pSession->_recvBuf.Peek((char*)&header, dfHEADER_SIZE);
		if (peekRet != dfHEADER_SIZE)
		{
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			SetStateDead();
			return;
		}

		if ((char)header.byCode != (char)dfPACKET_CODE)
		{
			printf("Error! Wrong Header Code! %x - Func %s Line %d\n", header.byCode, __func__, __LINE__);
			SetStateDead();
			return;
		}

		if (useSize < dfHEADER_SIZE + header.wPayloadSize)
			break;

		int moveReadRet = _pSession->_recvBuf.MoveReadPos(dfHEADER_SIZE);
		if (moveReadRet != dfHEADER_SIZE)
		{
			printf("Error! Func %s Line %d\n", __func__, __LINE__);
			SetStateDead();
			return;
		}

		switch (header.wMsgType)
		{
		case df_REQ_LOGIN:
			Handle_REQ_LOGIN();
			break;

		case df_REQ_ROOM_LIST:
			Handle_REQ_ROOM_LIST();
			break;

		case df_REQ_ROOM_CREATE:
			Handle_REQ_ROOM_CREATE();
			break;

		case df_REQ_ROOM_ENTER:
			Handle_REQ_ROOM_ENTER();
			break;

		case df_REQ_CHAT:
			Handle_REQ_CHAT();
			break;

		case df_REQ_ROOM_LEAVE:
			Handle_REQ_ROOM_LEAVE();
			break;
		}

		useSize = _pSession->_recvBuf.GetUseSize();
	}
}

void User::Handle_REQ_LOGIN()
{
	// TO-DO: Check Invalid User
	wchar_t nullname[dfNICK_MAX_LEN] = { '/0',};
	if (wcscmp(_name, nullname) != 0)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}

	// Get Data from Packet
	SerializePacket buffer;
	int size = sizeof(wchar_t) * dfNICK_MAX_LEN;
	int dequeueRet = _pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}

	buffer.MoveWritePos(size);
	buffer.GetData((char*)_name, size);
	SetStateLogin();
}

void User::Handle_REQ_ROOM_LIST()
{
	// TO-DO: Check Invalid User
	SetStateRoomList();
}

void User::Handle_REQ_ROOM_CREATE()
{
	// TO-DO: Check Invalid User
	
	// Get Data from Packet
	SerializePacket buffer;
	short titleLen;
	wchar_t* title = new wchar_t[titleLen];

	int size = sizeof(titleLen) + titleLen * sizeof(wchar_t);
	int dequeueRet = _pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}

	buffer.MoveWritePos(size);
	buffer >> titleLen;
	buffer.GetData((char*)title, titleLen * sizeof(wchar_t));

	// Create Room
	Room* pRoom = new Room(g_ContentData._roomID, title);
	g_ContentData._roomMap.insert({ g_ContentData._roomID, pRoom });
	g_ContentData._roomID++;
	delete[] title;

	SetStateRoomCreate();
}

void User::Handle_REQ_ROOM_ENTER()
{
	// TO-DO: Check Invalid User

	// Get Data from Packet
	SerializePacket buffer;
	int roomID;

	int size = sizeof(roomID);
	int dequeueRet = _pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}

	buffer.MoveWritePos(size);
	buffer >> roomID;

	// Enter Room
	unordered_map<int, Room*>::iterator iter = g_ContentData._roomMap.find(roomID);
	Room* pRoom = iter->second;
	pRoom->_userArray.push_back(this);
	_roomID = roomID;

	SetStateRoomCreate();
}

void User::Handle_REQ_CHAT()
{
	// TO-DO: Check Invalid User
	
	// Get Data from Packet
	SerializePacket buffer;
	short msgLen;
	wchar_t* msg = new wchar_t[msgLen];

	int size = sizeof(msgLen) + msgLen * sizeof(wchar_t);
	int dequeueRet = _pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}

	buffer.MoveWritePos(size);
	buffer >> msgLen;
	buffer.GetData((char*)msg, msgLen * sizeof(wchar_t));
	
	// Broadcast msg except this

	SetStateChat();
}

void User::Handle_REQ_ROOM_LEAVE()
{
	// TO-DO: Check Invalid User
	
	// Leave Room
	unordered_map<int, Room*>::iterator roomIter = g_ContentData._roomMap.find(_roomID);
	Room* pRoom = roomIter->second;
	vector<User*>::iterator userIter = find(pRoom->_userArray.begin(), pRoom->_userArray.end(), this);
	
	if (userIter == pRoom->_userArray.end())
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetStateDead();
		return;
	}

	pRoom->_userArray.erase(userIter);
	_roomID = -1;

	SetStateRoomLeave();
}
