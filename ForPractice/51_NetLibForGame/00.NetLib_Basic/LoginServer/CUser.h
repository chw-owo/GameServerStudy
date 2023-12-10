#pragma once
#include <synchapi.h>
#pragma comment(lib, "Synchronization.lib")

class CUser
{
public:
	CUser() {};
	CUser(unsigned __int64 sessionID) : _sessionID(sessionID) 
	{
		InitializeSRWLock(&_lock);
	};

public:
	SRWLOCK _lock;
	unsigned __int64 _sessionID;
};