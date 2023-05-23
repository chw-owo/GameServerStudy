#include "GameServer.h"

#include "Network.h"
#include "PlayerManager.h"
#include "Player.h"

GameServer::GameServer()
{

}

GameServer::~GameServer()
{
}

GameServer* GameServer::GetInstance()
{
	static GameServer _gameServer;
	return &_gameServer;
}

void GameServer::Initialize()
{
	// Network Manager Initialize
	_pNetworkManager = NetworkManager::GetInstance();
	_pNetworkManager->AttachStub(this);
	_pNetworkManager->Initialize();

	// Player Manager Initialize
	_pPlayerManager = PlayerManager::GetInstance();
}

void GameServer::Update()
{
	_pNetworkManager->Update();
	_pPlayerManager->Update();
}

void GameServer::Terminate()
{
	_pNetworkManager->Terminate();
	_pPlayerManager->Terminate();
}

void GameServer::CS_MoveStart(int ID, char direction, short x, short y)
{
	Player* pPlayer = _pPlayerManager->FindPlayer(ID);
	pPlayer->MoveStart(direction, x, y);
}

void GameServer::CS_MoveStop(int ID, char direction, short x, short y)
{
	Player* pPlayer = _pPlayerManager->FindPlayer(ID);
	pPlayer->MoveStop(direction, x, y);
}

void GameServer::CS_Attack1(int ID, char direction, short x, short y)
{
	Player* pPlayer = _pPlayerManager->FindPlayer(ID);
	pPlayer->Attack1(direction, x, y);
}

void GameServer::CS_Attack2(int ID, char direction, short x, short y)
{
	Player* pPlayer = _pPlayerManager->FindPlayer(ID);
	pPlayer->Attack2(direction, x, y);
}

void GameServer::CS_Attack3(int ID, char direction, short x, short y)
{
	Player* pPlayer = _pPlayerManager->FindPlayer(ID);
	pPlayer->Attack3(direction, x, y);
}

