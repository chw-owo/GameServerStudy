#pragma once
#include "Utils.h"
// TO-DO: ��� �޾Ƽ� ������ �� �ִ� ���·� ����

class CLanMsg;
class CLanJob
{
public:
	CLanJob() : _type(JOB_TYPE::NONE), _sysType(SYS_TYPE::NONE), _sessionID(-1), _packet(nullptr) {}

	inline void Setting(JOB_TYPE type, SYS_TYPE sysType, unsigned __int64 sessionID, CLanMsg* packet)
	{
		_type = type;
		_sysType = sysType;
		_sessionID = sessionID;
		_packet = packet;
	}

public:
	JOB_TYPE _type;
	SYS_TYPE _sysType;
	unsigned __int64 _sessionID;
	CLanMsg* _packet;
};