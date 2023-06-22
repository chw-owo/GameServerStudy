#pragma once
#include "Protocol.h"
#include <wchar.h>
#include <vector>
using namespace std;
#define dfROOM_MAX_LEN 256

class User
{
public:
	int _ID;
	int _roomID;
	wchar_t _name[dfNICK_MAX_LEN];
};

class Room
{
public:
	int _ID;
	wchar_t _title[dfROOM_MAX_LEN];
	vector<User*> _userArray;
};

class NetworkManager;
class ContentManager
{
private:
	ContentManager();
	~ContentManager();

public:
	static ContentManager* GetInstance();
	void Update();

private:
	static ContentManager _contentManager;
	NetworkManager* _pNetworkManager = nullptr;

private:
	int _userID = 0;
	int _roomID = 0;
	vector<User*> _userArray;
	vector<Room*> _roomArray;
};

