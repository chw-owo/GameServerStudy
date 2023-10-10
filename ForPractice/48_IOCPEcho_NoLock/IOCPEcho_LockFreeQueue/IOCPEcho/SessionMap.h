#pragma once
#include "Session.h"

class SessionMap
{
private:
	SessionMap() { }
	~SessionMap() {	}
	static SessionMap _sessionMap;
	
public:
	static SessionMap* GetInstance()
	{
		static SessionMap _sessionMap;
		return  &_sessionMap;
	}

public:
	
};