#pragma once

class CPacket;
class CJob
{
public:
	CJob() : _sessionID(-1), _packet(nullptr){}

	void Setting(__int64 sessionID, CPacket* packet)
	{
		_sessionID = sessionID; 
		_packet = packet;
	}

public:
	__int64 _sessionID;
	CPacket* _packet;
};