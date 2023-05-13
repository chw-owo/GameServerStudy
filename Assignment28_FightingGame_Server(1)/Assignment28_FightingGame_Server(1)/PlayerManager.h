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
	void DestroyPlayer(Player* player);
	void DestroyAllPlayer();

public:
	void Update();
	void Terminate(); 

private:
	void EnqueueUnicast(uint8 msgType, char* msg, int size, Player* pPlayer);
	void EnqueueBroadcast(uint8 msgType, char* msg, int size, Player* pExpPlayer = nullptr);

private:
	uint32 _ID;
	static PlayerManager _PlayerMgr;
	CList<Player*> _PlayerList;

private:
	NetworkManager* _pNetworkManager = nullptr;

};


