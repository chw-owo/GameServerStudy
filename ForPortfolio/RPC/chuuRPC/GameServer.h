#pragma once
#include "Stub.h"

class PlayerManager;
class NetworkManager;
class GameServer : public IStub
{
private:
	GameServer();
	~GameServer();

public:
	static GameServer* GetInstance();

public:
	void Initialize();
	void Update();
	void Terminate();

private:
	void CS_MoveStart(int ID, char direction, short x, short y);
	void CS_MoveStop(int ID, char direction, short x, short y);

	void CS_Attack1(int ID, char direction, short x, short y);
	void CS_Attack2(int ID, char direction, short x, short y);
	void CS_Attack3(int ID, char direction, short x, short y);

private:
	static GameServer _gameServer;

private:
	PlayerManager* _pPlayerManager = nullptr;
	NetworkManager* _pNetworkManager = nullptr;
};

