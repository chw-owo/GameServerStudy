#pragma once
#include "Protocol.h"
#include <wchar.h>

#define dfSTATE_ALIVE			0b00000001
#define dfSTATE_LOGIN			0b00000010
#define dfSTATE_ROOM_LIST		0b00000100
#define dfSTATE_ROOM_CREATE		0b00001000
#define dfSTATE_ROOM_ENTER		0b00010000
#define dfSTATE_CHAT			0b00100000
#define dfSTATE_ROOM_LEAVE		0b01000000

class Session;
class User
{
public:
	User(int ID, Session* pSession) : _ID(ID), _pSession(pSession) {}
	~User();
	void DequeueRecvBuf();

private:
	void Handle_REQ_LOGIN();
	void Handle_REQ_ROOM_LIST();
	void Handle_REQ_ROOM_CREATE();
	void Handle_REQ_ROOM_ENTER();
	void Handle_REQ_CHAT();
	void Handle_REQ_ROOM_LEAVE();

private:
	void SetStateDead()			{ _state ^= dfSTATE_ALIVE; }
	void SetStateLogin()		{ _state |= dfSTATE_LOGIN; }
	void SetStateRoomList()		{ _state |= dfSTATE_ROOM_LIST; }
	void SetStateRoomCreate()	{ _state |= dfSTATE_ROOM_CREATE; }
	void SetStateRoomEnter()	{ _state |= dfSTATE_ROOM_ENTER; }
	void SetStateChat()			{ _state |= dfSTATE_CHAT; }
	void SetStateRoomLeave()	{ _state |= dfSTATE_ROOM_LEAVE; }

public:
	bool GetStateAlive()		{ return (_state & dfSTATE_ALIVE); }
	bool GetStateLogin()		{ return ( _state & dfSTATE_LOGIN); }
	bool GetStateRoomList()		{ return ( _state & dfSTATE_ROOM_LIST); }
	bool GetStateRoomCreate()	{ return ( _state & dfSTATE_ROOM_CREATE); }
	bool GetStateRoomEnter()	{ return ( _state & dfSTATE_ROOM_ENTER); }
	bool GetStateChat()			{ return ( _state & dfSTATE_CHAT); }
	bool GetStateRoomLeave()	{ return ( _state & dfSTATE_ROOM_LEAVE); }

public:
	void ResetStateLogin()		{ _state ^= dfSTATE_LOGIN; }
	void ResetStateRoomList()	{ _state ^= dfSTATE_ROOM_LIST; }
	void ResetStateRoomCreate() { _state ^= dfSTATE_ROOM_CREATE; }
	void ResetStateRoomEnter()	{ _state ^= dfSTATE_ROOM_ENTER; }
	void ResetStateChat()		{ _state ^= dfSTATE_CHAT; }
	void ResetStateRoomLeave()	{ _state ^= dfSTATE_ROOM_LEAVE; }

public:
	char _state = 0;
	int _ID;
	int _roomID = -1;
	Session* _pSession;	
	wchar_t _name[dfNICK_MAX_LEN] = { '/0', };
};
