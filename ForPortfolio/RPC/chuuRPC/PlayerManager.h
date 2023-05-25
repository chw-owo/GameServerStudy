#pragma once
#include "List.h"
#include "Player.h"
#include "SerializePacket.h"
#include "MemoryPool.h"

#define FPS 50

class IStub;
class Proxy;
class NetworkManager;

class PlayerManager
{
private:
	PlayerManager();
	~PlayerManager();

public:
	static PlayerManager* GetInstance();
	
public:
	Player* FindPlayer(int sessionID);
	void Update();
	void Terminate();

private:
	void CreateNewPlayers();
	void DestroyDeadPlayers();

	Player* CreatePlayer(Session* session);
	void DestroyPlayer(Player* player);
	void DestroyAllPlayer();

private:
	bool SkipForFixedFrame();

private:
	Player* GetAttackedPlayer(char attackType, int ID, char direction, short x, short y);

private:
	int _ID = 0;

	IStub* _stub;
	Proxy* _proxy;
	NetworkManager* _net;

	static PlayerManager _playerMgr;
	CMemoryPoolT<Player> _PlayerPool;
	CList<Player*> _playerList;
};


