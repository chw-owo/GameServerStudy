#include <unordered_map>
#include <stdio.h>
#include "User.h"
#include "Session.h"
#include "User.h"
#include "Room.h"
#include "SerializePacket.h"

using namespace std;
vector<User*> _userArray;

void User::Handle_REQ_LOGIN()
{
	// TO-DO: Check Invalid User
	wchar_t nullname[dfNICK_MAX_LEN] = { '/0', };
	if (wcscmp(_name, nullname) != 0)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetUserDead();
		return;
	}

	// Get Data from Packet
	SerializePacket buffer;
	int size = sizeof(wchar_t) * dfNICK_MAX_LEN;
	int dequeueRet = _pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetUserDead();
		return;
	}

	buffer.MoveWritePos(size);
	buffer.GetData((char*)_name, size);
}

void User::Handle_REQ_ROOM_LIST()
{
	// TO-DO: Check Invalid User

}

void User::Handle_REQ_ROOM_CREATE()
{
	// TO-DO: Check Invalid User

	// Get Data from Packet
	SerializePacket buffer;
	short titleLen = 10; // TO-DO: 임시값. 수정 필요.
	wchar_t* title = new wchar_t[titleLen];

	int size = sizeof(titleLen) + titleLen * sizeof(wchar_t);
	int dequeueRet = _pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetUserDead();
		return;
	}

	buffer.MoveWritePos(size);
	buffer >> titleLen;
	buffer.GetData((char*)title, titleLen * sizeof(wchar_t));

	// Create Room
	Room* pRoom = new Room(_roomID, title);
	_roomMap.insert({ _roomID, pRoom });
	_roomID++;
	delete[] title;

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
		SetUserDead();
		return;
	}

	buffer.MoveWritePos(size);
	buffer >> roomID;

	// Enter Room
	unordered_map<int, Room*>::iterator iter = _roomMap.find(roomID);
	Room* pRoom = iter->second;
	pRoom->_userArray.push_back(this);
	_roomID = roomID;

}

void User::Handle_REQ_CHAT()
{
	// TO-DO: Check Invalid User

	// Get Data from Packet
	SerializePacket buffer;
	short msgLen = 10; // TO-DO: 임시값. 수정 필요.
	wchar_t* msg = new wchar_t[msgLen];

	int size = sizeof(msgLen) + msgLen * sizeof(wchar_t);
	int dequeueRet = _pSession->_recvBuf.Dequeue(buffer.GetWritePtr(), size);
	if (dequeueRet != size)
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetUserDead();
		return;
	}

	buffer.MoveWritePos(size);
	buffer >> msgLen;
	buffer.GetData((char*)msg, msgLen * sizeof(wchar_t));

	// Broadcast msg except this


}

void User::Handle_REQ_ROOM_LEAVE()
{
	// TO-DO: Check Invalid User

	// Leave Room
	unordered_map<int, Room*>::iterator roomIter = _roomMap.find(_roomID);
	Room* pRoom = roomIter->second;
	vector<User*>::iterator userIter = find(pRoom->_userArray.begin(), pRoom->_userArray.end(), this);

	if (userIter == pRoom->_userArray.end())
	{
		printf("Error! Func %s Line %d\n", __func__, __LINE__);
		SetUserDead();
		return;
	}

	pRoom->_userArray.erase(userIter);
	_roomID = -1;

}
