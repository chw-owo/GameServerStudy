#pragma once
#include <wchar.h>
#include <vector>
#include "Protocol.h"
#include "Session.h"

using namespace std;

class User
{
public:
	User(int ID, Session* pSession) : _ID(ID), _pSession(pSession) {}
	~User() {}

public:
	void SetUserAlive() { _alive = true; }
	void SetUserDead() { _alive = false; }
	bool GetUserAlive() { return _alive; }

public:
	void Handle_REQ_LOGIN();
	void Handle_REQ_ROOM_LIST();
	void Handle_REQ_ROOM_CREATE();
	void Handle_REQ_ROOM_ENTER();
	void Handle_REQ_CHAT();
	void Handle_REQ_ROOM_LEAVE();

public:
	bool _alive = true;
	int _ID;

public:
	int _roomID = -1;
	Session* _pSession;
	wchar_t _name[dfNICK_MAX_LEN] = { '/0', };
};

extern vector<User*> _userArray;