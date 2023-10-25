#pragma once

class CPacket;
enum class JOB_TYPE
{
	NONE,
	SYSTEM,
	CONTENT
};

enum class SYS_TYPE
{
	NONE,
	ACCEPT,
	RELEASE,
	TIMEOUT
};

class CJob
{
public:
	CJob() : _type(JOB_TYPE::NONE), _sysType(SYS_TYPE::NONE), _sessionID(-1), _packet(nullptr){}

	void Setting(JOB_TYPE type, SYS_TYPE sysType, __int64 sessionID, CPacket* packet)
	{
		_type = type; 
		_sysType = sysType;
		_sessionID = sessionID; 
		_packet = packet;
	}

public:
	JOB_TYPE _type;
	SYS_TYPE _sysType;
	__int64 _sessionID;
	CPacket* _packet;
};