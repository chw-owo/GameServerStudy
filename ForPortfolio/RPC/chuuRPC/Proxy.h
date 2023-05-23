#pragma once
#include "Common.h"

class Session;
class NetworkManager;
class Proxy
{
private:
	Proxy();
	~Proxy();

public:
	static Proxy* GetInstance();

public:
	void SC_CreateMyCharacter(Session* pSession, int ID, char direction, short x, short y, char hp);
	void SC_CreateOtherCharacter(TARGET target, Session* pSession, int ID, char direction, short x, short y, char hp);
	void SC_DeleteCharacter(int ID);

	void SC_MoveStart(Session* pSession, int ID, char direction, short x, short y);
	void SC_MoveStop(Session* pSession, int ID, char direction, short x, short y);

	void SC_Attack1(int ID, char direction, short x, short y);
	void SC_Attack2(int ID, char direction, short x, short y);
	void SC_Attack3(int ID, char direction, short x, short y);

	void SC_Damage(int attackID, int damageID, char damageHp);

private:
	static Proxy _proxy;
	NetworkManager* _net;
};