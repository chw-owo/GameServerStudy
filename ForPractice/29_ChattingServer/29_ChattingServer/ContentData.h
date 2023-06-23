#pragma once
#include "User.h"
#include "Room.h"
#include <vector>
#include <unordered_map>
using namespace std;

struct ContentData
{
	int _userID = 0;
	int _roomID = 0;
	vector<User*> _userArray;
	unordered_map<int, Room*> _roomMap;
};

extern ContentData g_ContentData;