#pragma once
#include <vector>
#include "CommonProtocol.h"
using namespace std;

class CPlayer;
class CSector
{
public: 
	CSector()
	{
		InitializeSRWLock(&_lock);
	}

public:
	int _x; // For Debug
	int _y; // For Debug

	vector<CSector*> _around;
	vector<CPlayer*> _players;
	SRWLOCK _lock;
};