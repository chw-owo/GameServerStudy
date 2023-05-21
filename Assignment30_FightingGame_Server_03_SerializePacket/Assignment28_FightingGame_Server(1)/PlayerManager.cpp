#include "PlayerManager.h"
#include "PacketDefine.h"
#include "NetworkManager.h"
#include "CreateSCPacket.h"

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

}

PlayerManager::~PlayerManager()
{

}

PlayerManager* PlayerManager::GetInstance()
{
	static PlayerManager _PlayerMgr;
	return &_PlayerMgr;
}

Player* PlayerManager::CreatePlayer(Session* pSession)
{
	Player* player = new Player(pSession, _ID);
	_PlayerList.push_back(player);

	// Send <Allocate ID Message> to New Player
	int CreateMyCharSize = Create_Packet_SC_CREATE_MY_CHARACTER(&_packetBuffer,
		player->GetID(), player->GetDirection(), player->GetX(), player->GetY(), player->GetHp());
	EnqueueUnicast(CreateMyCharSize, player);

	// Send <Create New Player Message> to All Player
	int CreateOtherCharSize = Create_Packet_SC_CREATE_OTHER_CHARACTER(&_packetBuffer,
		player->GetID(), player->GetDirection(), player->GetX(), player->GetY(), player->GetHp());
	EnqueueBroadcast(CreateOtherCharSize, player);

	// Send <Create All Players Message> to New Player
	for (CList<Player*>::iterator i = _PlayerList.begin(); i != _PlayerList.end(); i++)
	{
		if ((*i)->GetID() != _ID)
		{
			int CreateOtherCharAllSize = Create_Packet_SC_CREATE_OTHER_CHARACTER(&_packetBuffer,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY(), (*i)->GetHp());
			EnqueueUnicast(CreateOtherCharAllSize, player);
		}
	}

	_ID++;
	return player;
}

void PlayerManager::DestroyPlayer(Player* player)
{
	_PlayerList.remove(player);
	delete(player);
}

void PlayerManager::DestroyAllPlayer()
{
	Player* player;
	while (!_PlayerList.empty())
	{
		player = _PlayerList.pop_back();
		//player->GetSession()->SetSessionDead();
		delete(player);
	}
}

void PlayerManager::DestroyDeadPlayers()
{
	for (CList<Player*>::iterator i = _PlayerList.begin(); i != _PlayerList.end();)
	{
		// Destroy Dead Player
		if (!(*i)->GetStateAlive())
		{
			Player* deletedPlayer = (*i);

			_packetBuffer.Clear();

			int DeleteCharSize = Create_Packet_SC_DELETE_CHARACTER(&_packetBuffer, deletedPlayer->GetID());
			EnqueueBroadcast(DeleteCharSize);

			if (_packetBuffer.GetReadPtr() == _packetBuffer.GetWritePtr())
				printf("Good~~ Func %s, Line %d\n", __func__, __LINE__);
			else
				printf("No!!! Func %s, Line %d\n", __func__, __LINE__);

			deletedPlayer->GetSession()->SetSessionDead();
			i = _PlayerList.erase(i);
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

	for (CList<Player*>::iterator i = _PlayerList.begin(); i != _PlayerList.end(); i++)
	{
		// Recv and Update
		(*i)->DequeueRecvBuf();
		(*i)->Update();

		// Broadcast SC Packets
		if ((*i)->GetPacketMoveStart())
		{
			_packetBuffer.Clear();

			int MoveStartSize = Create_Packet_SC_MOVE_START(&_packetBuffer,
				(*i)->GetID(), (*i)->GetMoveDirection(), (*i)->GetX(), (*i)->GetY());
			EnqueueBroadcast(MoveStartSize, (*i));
			(*i)->ResetPacketMoveStart();

			if (_packetBuffer.GetReadPtr() == _packetBuffer.GetWritePtr())
				printf("Good~~ Func %s, Line %d\n", __func__, __LINE__);
			else
				printf("No!!! Func %s, Line %d\n", __func__, __LINE__);
		}

		if ((*i)->GetPacketMoveStop())
		{
			_packetBuffer.Clear();

			int MoveStopSize = Create_Packet_SC_MOVE_STOP(&_packetBuffer,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			EnqueueBroadcast(MoveStopSize, (*i));
			(*i)->ResetPacketMoveStop();

			if (_packetBuffer.GetReadPtr() == _packetBuffer.GetWritePtr())
				printf("Good~~ Func %s, Line %d\n", __func__, __LINE__);
			else
				printf("No!!! Func %s, Line %d\n", __func__, __LINE__);
		}

		if ((*i)->GetPacketAttack1())
		{
			_packetBuffer.Clear();

			int Attack1Size = Create_Packet_SC_ATTACK1(&_packetBuffer,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			EnqueueBroadcast(Attack1Size, (*i));
			(*i)->ResetPacketAttack1();

			if (_packetBuffer.GetReadPtr() == _packetBuffer.GetWritePtr())
				printf("Good~~ Func %s, Line %d\n", __func__, __LINE__);
			else
				printf("No!!! Func %s, Line %d\n", __func__, __LINE__);

			CheckAttacknEnqueueDamagePacket(dfATTACK_TYPE_ATTACK1,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
		}

		if ((*i)->GetPacketAttack2())
		{
			_packetBuffer.Clear();

			int Attack2Size = Create_Packet_SC_ATTACK2(&_packetBuffer,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			EnqueueBroadcast(Attack2Size, (*i));
			(*i)->ResetPacketAttack2();

			if (_packetBuffer.GetReadPtr() == _packetBuffer.GetWritePtr())
				printf("Good~~ Func %s, Line %d\n", __func__, __LINE__);
			else
				printf("No!!! Func %s, Line %d\n", __func__, __LINE__);

			CheckAttacknEnqueueDamagePacket(dfATTACK_TYPE_ATTACK2,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
		}

		if ((*i)->GetPacketAttack3())
		{
			_packetBuffer.Clear();

			int Attack3Size = Create_Packet_SC_ATTACK3(&_packetBuffer,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			EnqueueBroadcast(Attack3Size, (*i));
			(*i)->ResetPacketAttack3();

			if (_packetBuffer.GetReadPtr() == _packetBuffer.GetWritePtr())
				printf("Good~~ Func %s, Line %d\n", __func__, __LINE__);
			else
				printf("No!!! Func %s, Line %d\n", __func__, __LINE__);

			CheckAttacknEnqueueDamagePacket(dfATTACK_TYPE_ATTACK3,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
		}
	}
}

void PlayerManager::Terminate()
{
	DestroyAllPlayer();
}

void PlayerManager::EnqueueUnicast(int size, Player* pPlayer)
{
	if(_pNetworkManager == nullptr)
		_pNetworkManager = NetworkManager::GetInstance();

	_pNetworkManager->EnqueueUnicast(_packetBuffer.GetReadPtr(), size, pPlayer->GetSession());
	_packetBuffer.MoveReadPos(size);
}

void PlayerManager::EnqueueBroadcast(int size, Player* pExpPlayer)
{

	if (_pNetworkManager == nullptr)
		_pNetworkManager = NetworkManager::GetInstance();

	if (pExpPlayer != nullptr)
	{
		_pNetworkManager->EnqueueBroadcast(_packetBuffer.GetReadPtr(), size, pExpPlayer->GetSession());
		_packetBuffer.MoveReadPos(size);
	}
	else
	{
		_pNetworkManager->EnqueueBroadcast(_packetBuffer.GetReadPtr(), size);
		_packetBuffer.MoveReadPos(size);
	}
}

void PlayerManager::CheckAttacknEnqueueDamagePacket(char attackType, int ID, char direction, short x, short y)
{
	short rangeX;
	short rangeY;
	short damage;

	switch (attackType)
	{
	case dfATTACK_TYPE_ATTACK1:
		rangeX = dfATTACK1_RANGE_X;
		rangeY = dfATTACK1_RANGE_Y;
		damage = dfATTACK1_DAMAGE;
		break;

	case dfATTACK_TYPE_ATTACK2:
		rangeX = dfATTACK2_RANGE_X;
		rangeY = dfATTACK2_RANGE_Y;
		damage = dfATTACK2_DAMAGE;
		break;

	case dfATTACK_TYPE_ATTACK3:
		rangeX = dfATTACK3_RANGE_X;
		rangeY = dfATTACK3_RANGE_Y;
		damage = dfATTACK3_DAMAGE;
		break;

	default:
		return;
	}

	if (direction == dfPACKET_MOVE_DIR_LL)
	{
		short minX = x - rangeX;
		short minY = y - rangeY;
		short maxY = y + rangeY;
		for (CList<Player*>::iterator i = _PlayerList.begin(); i != _PlayerList.end(); i++)
		{
			if ((*i)->GetX() >= minX && (*i)->GetX() <= x &&
				(*i)->GetY() >= minY && (*i)->GetY() <= maxY && ID != (*i)->GetID())
			{
				_packetBuffer.Clear();

				(*i)->Attacked(damage);
				int DamageSize = Create_Packet_SC_DAMAGE(&_packetBuffer, ID, (*i)->GetID(), (*i)->GetHp());
				EnqueueBroadcast(DamageSize);

				if (_packetBuffer.GetReadPtr() == _packetBuffer.GetWritePtr())
					printf("Good~~ Func %s, Line %d\n", __func__, __LINE__);
				else
					printf("No!!! Func %s, Line %d\n", __func__, __LINE__);
			}
		}
	}
		
	else if (direction == dfPACKET_MOVE_DIR_RR)
	{
		short maxX = x + rangeX;
		short minY = y - rangeY;
		short maxY = y + rangeY;

		for (CList<Player*>::iterator i = _PlayerList.begin(); i != _PlayerList.end(); i++)
		{
			if ((*i)->GetX() >= x && (*i)->GetX() <= maxX &&
				(*i)->GetY() >= minY && (*i)->GetY() <= maxY && ID != (*i)->GetID())
			{
				_packetBuffer.Clear();

				(*i)->Attacked(damage);
				int DamageSize = Create_Packet_SC_DAMAGE(&_packetBuffer, ID, (*i)->GetID(), (*i)->GetHp());
				EnqueueBroadcast(DamageSize);

				if (_packetBuffer.GetReadPtr() == _packetBuffer.GetWritePtr())
					printf("Good~~ Func %s, Line %d\n", __func__, __LINE__);
				else
					printf("No!!! Func %s, Line %d\n", __func__, __LINE__);
			}
		}
	}
}