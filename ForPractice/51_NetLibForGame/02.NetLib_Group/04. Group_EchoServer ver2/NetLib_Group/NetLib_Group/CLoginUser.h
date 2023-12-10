#pragma once
class CLoginUser
{
public:
	CLoginUser() {};
	CLoginUser(unsigned __int64 sessionID) : _sessionID(sessionID) {};

public:
	unsigned __int64 _sessionID;
};