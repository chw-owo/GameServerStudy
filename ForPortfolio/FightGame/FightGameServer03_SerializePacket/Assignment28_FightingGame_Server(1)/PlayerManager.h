#pragma once

#include "List.h"
 
#include "Player.h"
#include "SerializePacket.h"

#define FPS 50
class NetworkManager;
class PlayerManager
{
private:
	PlayerManager();
	~PlayerManager();

public:
	static PlayerManager* GetInstance();

public:
	Player* CreatePlayer(Session* session);

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
	void EnqueueUnicast(int size, Player* pPlayer);
	void EnqueueBroadcast(int size, Player* pExpPlayer = nullptr);

private:
	void CheckAttacknEnqueueDamagePacket(char attackType, 
		int ID, char direction, short x, short y);

private:
	int _ID = 0;
	static PlayerManager _PlayerMgr;
	CList<Player*> _PlayerList;
	SerializePacket _packetBuffer;

private:
	NetworkManager* _pNetworkManager = nullptr;

};


