#pragma once

#include "List.h"
#include "Player.h"
#include "Proxy.h"
#include "Stub.h"
#include "SerializePacket.h"

#define FPS 50

class Proxy;
class PlayerManager
{
private:
	PlayerManager();
	~PlayerManager();

public:
	static PlayerManager* GetInstance();

public:
	Player* CreatePlayer(Session* session);
	Player* FindPlayer(int sessionID);

public:
	void Update();
	void Terminate();

private:
	void DestroyPlayer(Player* player);
	void DestroyAllPlayer();
	void DestroyDeadPlayers();

private:
	bool SkipForFixedFrame();

private:
	Player* GetAttackedPlayer(char attackType, int ID, char direction, short x, short y);

private:
	int _ID = 0;
	Proxy* _proxy;
	IStub* _stub;
	static PlayerManager _playerMgr;
	CList<Player*> _playerList;
};


