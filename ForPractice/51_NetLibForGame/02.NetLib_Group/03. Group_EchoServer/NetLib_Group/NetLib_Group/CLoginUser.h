#pragma once
class CLoginUser
{
public:
	CLoginUser(unsigned __int64 sessionID) : _sessionID(sessionID) {};

public:
	unsigned __int64 _sessionID;
};