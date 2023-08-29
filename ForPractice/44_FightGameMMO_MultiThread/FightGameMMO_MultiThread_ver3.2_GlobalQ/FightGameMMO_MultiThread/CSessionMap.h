#pragma once
#include "CSession.h"
#include <unordered_map>
using namespace std;

class CSessionMap
{
private:
	CSessionMap()
	{
		InitializeSRWLock(&_lock);
	}

	~CSessionMap()
	{

	}
	static CSessionMap _sessionMap;

public: 
	static CSessionMap* GetInstance()
	{
		static  CSessionMap _sessionMap;
		return  &_sessionMap;
	}

public:
	unordered_map<__int64, CSession*> _map;
	SRWLOCK _lock;
};