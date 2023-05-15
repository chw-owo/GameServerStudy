#pragma once
#include "Player.h"
#include "Typedef.h"
#include "List.h"
#include <iostream>

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
	void EnqueueUnicast(uint8 msgType, char* msg, int size, Player* pPlayer);
	void EnqueueBroadcast(uint8 msgType, char* msg, int size, Player* pExpPlayer = nullptr);

private:
	void CheckAttacknEnqueueDamagePacket(uint8 attackType, 
		uint32 ID, uint8 direction, uint16 x, uint16 y);

private:
	uint32 _ID = 0;
	static PlayerManager _PlayerMgr;
	CList<Player*> _PlayerList;

private:
	NetworkManager* _pNetworkManager = nullptr;

};


