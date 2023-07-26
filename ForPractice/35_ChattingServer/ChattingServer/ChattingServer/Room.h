#pragma once
#include <wchar.h>
#include <vector>
#include <unordered_map>
using namespace std;
#define dfROOM_MAX_LEN 256
#define dfROOM_MAX_CNT 8
#define dfROOM_MAX_USER 8

class User;
class Room
{
public:
	Room(int ID, short titleLen, wchar_t* title) : _ID(ID)
	{
		_title = new wchar_t[titleLen];
		wcscpy_s(_title, titleLen, title);
	}

	~Room() 
	{
		delete[] _title;
		// TO-DO delete _useArray;
	}

public:
	int _ID;
	wchar_t* _title = nullptr;
	vector<User*> _userArray;
};

extern unordered_map<int, Room*> _roomIDMap;
//extern unordered_map<wchar_t*, Room*> _roomNameMap;