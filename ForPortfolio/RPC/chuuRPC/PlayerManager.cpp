#include "PlayerManager.h"
#include "Network.h"
#include "GameServer.h"
#include "Proxy.h"
#include "Stub.h"

#pragma comment(lib, "winmm.lib")
#include <windows.h>
#include <iostream>

// ATTACK TYPE Define
#define dfATTACK_TYPE_ATTACK1	1
#define dfATTACK_TYPE_ATTACK2	2
#define dfATTACK_TYPE_ATTACK3	3

// ATTACK RANGE Define
#define dfATTACK1_RANGE_X		80
#define dfATTACK2_RANGE_X		90
#define dfATTACK3_RANGE_X		100
#define dfATTACK1_RANGE_Y		10
#define dfATTACK2_RANGE_Y		10
#define dfATTACK3_RANGE_Y		20

// ATTACK DAMAGE Define
#define dfATTACK1_DAMAGE		10
#define dfATTACK2_DAMAGE		20
#define dfATTACK3_DAMAGE		30

PlayerManager::PlayerManager()
{
	_proxy = Proxy::GetInstance();
	_stub = GameServer::GetInstance();
	_net = NetworkManager::GetInstance();
}

PlayerManager::~PlayerManager()
{

}

PlayerManager* PlayerManager::GetInstance()
{
	static PlayerManager _playerMgr;
	return &_playerMgr;
}

void PlayerManager::CreateNewPlayers()
{
	if (_net->GetNewSessionCnt() > 0)
	{
		CList<Session*>* pSessions = _net->GetNewSessionList();
		CList<Session*>::iterator i = pSessions->begin();

		for (; i != pSessions->end(); i++)
		{
			CreatePlayer(*i);
		}

		_net->SetNewSessionsToUsed();
	}
}

Player* PlayerManager::CreatePlayer(Session* pSession)
{
	Player* player = new Player(pSession, _ID);
	_playerList.push_back(player);

	_proxy->SC_CreateMyCharacter(player->GetSession(),
		player->GetID(), player->GetDirection(), player->GetX(), player->GetY(), player->GetHp());

	_proxy->SC_CreateOtherCharacter(BROAD_EXP, player->GetSession(),
		player->GetID(), player->GetDirection(), player->GetX(), player->GetY(), player->GetHp());

	// Send <Create All Players Message> to New Player
	for (CList<Player*>::iterator i = _playerList.begin(); i != _playerList.end(); i++)
	{
		if ((*i)->GetID() != _ID)
		{
			_proxy->SC_CreateOtherCharacter(UNI, player->GetSession(),
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY(), (*i)->GetHp());
		}
	}

	_ID++;
	return player;
}

Player* PlayerManager::FindPlayer(int sessionID)
{
	for (CList<Player*>::iterator i = _playerList.begin(); i != _playerList.end(); i++)
	{
		if (sessionID == (*i)->GetSession()->GetSessionID())
			return (*i);
	}
	return nullptr;
}

void PlayerManager::DestroyPlayer(Player* player)
{
	_playerList.remove(player);
	delete(player);
}

void PlayerManager::DestroyAllPlayer()
{
	Player* player;
	while (!_playerList.empty())
	{
		player = _playerList.pop_back();
		delete(player);
	}
}

void PlayerManager::DestroyDeadPlayers()
{
	for (CList<Player*>::iterator i = _playerList.begin(); i != _playerList.end();)
	{
		// Destroy Dead Player
		if (!(*i)->GetPlayerState_Alive())
		{
			Player* deletedPlayer = (*i);

			_proxy->SC_DeleteCharacter(deletedPlayer->GetID());
			deletedPlayer->GetSession()->SetSessionDead();
			i = _playerList.erase(i);
			delete(deletedPlayer);
		}
		else
			i++;
	}
}

bool PlayerManager::SkipForFixedFrame()
{
	timeBeginPeriod(1);
	static DWORD oldTick = timeGetTime();
	if ((timeGetTime() - oldTick) < (1000 / FPS))
		return true;
	oldTick += (1000 / FPS);
	return false;
}

void PlayerManager::Update()
{
	// Skip For Fixed Frame
	if (SkipForFixedFrame()) return;

	DestroyDeadPlayers();
	CreateNewPlayers();
		
	for (CList<Player*>::iterator i = _playerList.begin(); i != _playerList.end(); i++)
	{
		// Recv and Update
		Session* pSession = (*i)->GetSession();
		_stub->ProcessReceivedMessage(pSession);
		(*i)->Update();

		// Broadcast SC Packets
		if ((*i)->GetPacketState_MoveStart())
		{
			_proxy->SC_MoveStart(pSession, (*i)->GetID(), (*i)->GetMoveDirection(), (*i)->GetX(), (*i)->GetY());
			(*i)->ResetPacketState_MoveStart();
		}

		if ((*i)->GetPacketState_MoveStop())
		{
			_proxy->SC_MoveStop(pSession, (*i)->GetID(), (*i)->GetMoveDirection(), (*i)->GetX(), (*i)->GetY());
			(*i)->ResetPacketState_MoveStop();
		}

		if ((*i)->GetPacketState_Attack1())
		{
			_proxy->SC_Attack1((*i)->GetID(), (*i)->GetMoveDirection(), (*i)->GetX(), (*i)->GetY());
			Player* attacked1Player = GetAttackedPlayer(dfATTACK_TYPE_ATTACK1, 
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			if (attacked1Player != nullptr)
			{
				attacked1Player->Attacked(dfATTACK1_DAMAGE);
				_proxy->SC_Damage((*i)->GetID(), attacked1Player->GetID(), attacked1Player->GetHp());
			}
			(*i)->ResetPacketState_Attack1();
		}

		if ((*i)->GetPacketState_Attack2())
		{
			_proxy->SC_Attack2((*i)->GetID(), (*i)->GetMoveDirection(), (*i)->GetX(), (*i)->GetY());

			Player* attacked2Player = GetAttackedPlayer(dfATTACK_TYPE_ATTACK2, 
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			if (attacked2Player != nullptr)
			{
				attacked2Player->Attacked(dfATTACK2_DAMAGE);
				_proxy->SC_Damage((*i)->GetID(), attacked2Player->GetID(), attacked2Player->GetHp());
			}
			(*i)->ResetPacketState_Attack2();
		}

		if ((*i)->GetPacketState_Attack3())
		{
			_proxy->SC_Attack3((*i)->GetID(), (*i)->GetMoveDirection(), (*i)->GetX(), (*i)->GetY());
			Player* attacked3Player = GetAttackedPlayer(dfATTACK_TYPE_ATTACK3, 
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			if (attacked3Player != nullptr)
			{
				attacked3Player->Attacked(dfATTACK3_DAMAGE);
				_proxy->SC_Damage((*i)->GetID(), attacked3Player->GetID(), attacked3Player->GetHp());
			}
			(*i)->ResetPacketState_Attack3();
		}
	}
}

void PlayerManager::Terminate()
{
	DestroyAllPlayer();
}

Player* PlayerManager::GetAttackedPlayer(char attackType, int ID, char direction, short x, short y)
{
	short rangeX;
	short rangeY;

	switch (attackType)
	{
	case dfATTACK_TYPE_ATTACK1:
		rangeX = dfATTACK1_RANGE_X;
		rangeY = dfATTACK1_RANGE_Y;
		break;

	case dfATTACK_TYPE_ATTACK2:
		rangeX = dfATTACK2_RANGE_X;
		rangeY = dfATTACK2_RANGE_Y;
		break;

	case dfATTACK_TYPE_ATTACK3:
		rangeX = dfATTACK3_RANGE_X;
		rangeY = dfATTACK3_RANGE_Y;
		break;

	default:
		return nullptr;
	}

	if (direction == dfPACKET_MOVE_DIR_LL)
	{
		short minX = x - rangeX;
		short minY = y - rangeY;
		short maxY = y + rangeY;
		for (CList<Player*>::iterator i = _playerList.begin(); i != _playerList.end(); i++)
		{
			if ((*i)->GetX() >= minX && (*i)->GetX() <= x &&
				(*i)->GetY() >= minY && (*i)->GetY() <= maxY && ID != (*i)->GetID())
				return (*i);
		}
	}
	else if (direction == dfPACKET_MOVE_DIR_RR)
	{
		short maxX = x + rangeX;
		short minY = y - rangeY;
		short maxY = y + rangeY;

		for (CList<Player*>::iterator i = _playerList.begin(); i != _playerList.end(); i++)
		{
			if ((*i)->GetX() >= x && (*i)->GetX() <= maxX &&
				(*i)->GetY() >= minY && (*i)->GetY() <= maxY && ID != (*i)->GetID())
				return (*i);
		}
	}

	return nullptr;
}