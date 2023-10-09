#pragma once
#include "Session.h"
#include <unordered_map>
using namespace std;

class SessionMap
{
private:
	SessionMap() { InitializeCriticalSection(&_cs); }
	~SessionMap() {	}
	static SessionMap _sessionMap;
	
public:
	static SessionMap* GetInstance()
	{
		static SessionMap _sessionMap;
		return  &_sessionMap;
	}

public:
	unordered_map<__int64, Session*> _map;
	CRITICAL_SECTION _cs;
};