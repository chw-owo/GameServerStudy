#pragma once
class Session;
class IStub
{
public:
	virtual void CS_MoveStart(int ID, char direction, short x, short y) = 0;
	virtual void CS_MoveStop(int ID, char direction, short x, short y) = 0;

	virtual void CS_Attack1(int ID, char direction, short x, short y) = 0;
	virtual void CS_Attack2(int ID, char direction, short x, short y) = 0;
	virtual void CS_Attack3(int ID, char direction, short x, short y) = 0;

public:
	void Handle_CS_MoveStart(Session* pSession);
	void Handle_CS_MoveStop(Session* pSession);

	void Handle_CS_Attack1(Session* pSession);
	void Handle_CS_Attack2(Session* pSession);
	void Handle_CS_Attack3(Session* pSession);

public:
	void ProcessReceivedMessage(Session* pSession);

};

