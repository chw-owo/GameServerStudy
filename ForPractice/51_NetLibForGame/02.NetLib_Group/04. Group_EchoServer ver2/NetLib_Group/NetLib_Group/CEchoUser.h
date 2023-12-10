#pragma once
class CEchoUser
{
public:
	CEchoUser() {};
	CEchoUser(unsigned __int64 sessionID) : _sessionID(sessionID) {};

public:
	unsigned __int64 _sessionID;
};