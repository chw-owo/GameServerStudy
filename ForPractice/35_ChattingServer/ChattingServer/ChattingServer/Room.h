#pragma once
#include <wchar.h>
#include <vector>
#include <unordered_map>
using namespace std;
#define dfROOM_MAX_LEN 256

class User;
class Room
{
public:
	Room(int ID, wchar_t* title) : _ID(ID)
	{
		wcscpy_s(_title, dfROOM_MAX_LEN, title);
	}

	~Room() {}

public:
	int _ID;
	wchar_t* _title;
	vector<User*> _userArray;
};

extern unordered_map<int, Room*> _roomMap;
