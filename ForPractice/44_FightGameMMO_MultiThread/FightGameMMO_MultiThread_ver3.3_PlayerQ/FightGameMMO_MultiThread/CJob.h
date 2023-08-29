#pragma once
#include "CPacket.h"

enum class JOB_TYPE
{
	SYSTEM,
	CONTENT
};

enum class SYS_TYPE
{
	ACCEPT,
	RELEASE
};

class CJob
{
public:
	CJob() {}
	CJob(JOB_TYPE type, SYS_TYPE sysType, __int64 sessionID, CPacket* packet)
		: _type(type), _sysType(sysType), _sessionID(sessionID), _packet(packet) {}

public:
	JOB_TYPE _type;
	SYS_TYPE _sysType;
	__int64 _sessionID;
	CPacket* _packet;
};