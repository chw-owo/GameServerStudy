#include "PlayerManager.h"
#include "PacketDefine.h"
#include "NetworkManager.h"
#include "CreatePacket.h"

#include <windows.h>
#pragma comment(lib, "winmm.lib")

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
	srand((uint16)time(NULL));
	_ID = rand();
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
	stPACKET_SC_CREATE_MY_CHARACTER packetCreateMy;
	Create_PACKET_SC_CREATE_MY_CHARACTER(packetCreateMy,
		player->GetID(), player->GetDirection(), player->GetX(), player->GetY(), player->GetHp());
	EnqueueUnicast(dfPACKET_SC_CREATE_MY_CHARACTER, (char*)&packetCreateMy, 
		sizeof(stPACKET_SC_CREATE_MY_CHARACTER), player);

	// Send <Create New Player Message> to All Player
	stPACKET_SC_CREATE_OTHER_CHARACTER packetCreateOther;
	Create_PACKET_SC_CREATE_OTHER_CHARACTER(packetCreateOther,
		player->GetID(), player->GetDirection(), player->GetX(), player->GetY(), player->GetHp());
	EnqueueBroadcast(dfPACKET_SC_CREATE_OTHER_CHARACTER, (char*)&packetCreateOther, 
		sizeof(stPACKET_SC_CREATE_OTHER_CHARACTER), player);

	// Send <Create All Players Message> to New Player
	stPACKET_SC_CREATE_OTHER_CHARACTER packetCreateAll;
	for (CList<Player*>::iterator i = _PlayerList.begin(); i != _PlayerList.end(); i++)
	{
		if ((*i)->GetID() != _ID)
		{
			Create_PACKET_SC_CREATE_OTHER_CHARACTER(packetCreateAll,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY(), (*i)->GetHp());
			EnqueueUnicast(dfPACKET_SC_CREATE_OTHER_CHARACTER, (char*)&packetCreateAll,
				sizeof(stPACKET_SC_CREATE_OTHER_CHARACTER), player);
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

			stPACKET_SC_DELETE_CHARACTER packetDelete;
			Create_PACKET_SC_DELETE_CHARACTER(packetDelete, deletedPlayer->GetID());
			EnqueueBroadcast(dfPACKET_SC_DELETE_CHARACTER, (char*)&packetDelete,
				sizeof(stPACKET_SC_DELETE_CHARACTER));

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
			stPACKET_SC_MOVE_START packetSCMoveStart;
			Create_PACKET_SC_MOVE_START(packetSCMoveStart,
				(*i)->GetID(), (*i)->GetMoveDirection(), (*i)->GetX(), (*i)->GetY());
			EnqueueBroadcast(dfPACKET_SC_MOVE_START, (char*)&packetSCMoveStart,
				sizeof(stPACKET_SC_MOVE_START), (*i));
			(*i)->ResetPacketMoveStart();
		}

		if ((*i)->GetPacketMoveStop())
		{
			stPACKET_SC_MOVE_STOP packetSCMoveStop;
			Create_PACKET_SC_MOVE_STOP(packetSCMoveStop,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			EnqueueBroadcast(dfPACKET_SC_MOVE_STOP, (char*)&packetSCMoveStop,
				sizeof(stPACKET_SC_MOVE_STOP), (*i));
			(*i)->ResetPacketMoveStop();
		}

		if ((*i)->GetPacketAttack1())
		{
			stPACKET_SC_ATTACK1 packetSCAttack1;
			Create_PACKET_SC_ATTACK1(packetSCAttack1,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			CheckAttacknEnqueueDamagePacket(dfATTACK_TYPE_ATTACK1, 
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			EnqueueBroadcast(dfPACKET_SC_ATTACK1, (char*)&packetSCAttack1,
				sizeof(stPACKET_SC_ATTACK1), (*i));
			(*i)->ResetPacketAttack1();
		}

		if ((*i)->GetPacketAttack2())
		{
			stPACKET_SC_ATTACK2 packetSCAttack2;
			Create_PACKET_SC_ATTACK2(packetSCAttack2,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			CheckAttacknEnqueueDamagePacket(dfATTACK_TYPE_ATTACK2,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			EnqueueBroadcast(dfPACKET_SC_ATTACK2, (char*)&packetSCAttack2,
				sizeof(stPACKET_SC_ATTACK2), (*i));
			(*i)->ResetPacketAttack2();
		}

		if ((*i)->GetPacketAttack3())
		{
			stPACKET_SC_ATTACK3 packetSCAttack3;
			Create_PACKET_SC_ATTACK3(packetSCAttack3,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			CheckAttacknEnqueueDamagePacket(dfATTACK_TYPE_ATTACK3,
				(*i)->GetID(), (*i)->GetDirection(), (*i)->GetX(), (*i)->GetY());
			EnqueueBroadcast(dfPACKET_SC_ATTACK3, (char*)&packetSCAttack3,
				sizeof(stPACKET_SC_ATTACK3), (*i));
			(*i)->ResetPacketAttack3();
		}
	}
}

void PlayerManager::Terminate()
{
	DestroyAllPlayer();
}

void PlayerManager::EnqueueUnicast(uint8 msgType, char* msg, int size, Player* pPlayer)
{
	stPACKET_HEADER header;
	header.Code = 0x89;
	header.Size = size;
	header.Type = msgType;
	
	if(_pNetworkManager == nullptr)
		_pNetworkManager = NetworkManager::GetInstance();

	_pNetworkManager->EnqueueUnicast((char*)&header, HEADER_LEN, pPlayer->GetSession());
	_pNetworkManager->EnqueueUnicast(msg, size, pPlayer->GetSession());
}

void PlayerManager::EnqueueBroadcast(uint8 msgType, char* msg, int size, Player* pExpPlayer)
{
	stPACKET_HEADER header;
	header.Code = 0x89;
	header.Size = size;
	header.Type = msgType;

	if (_pNetworkManager == nullptr)
		_pNetworkManager = NetworkManager::GetInstance();

	if (pExpPlayer != nullptr)
	{
		_pNetworkManager->EnqueueBroadcast((char*)&header, HEADER_LEN, pExpPlayer->GetSession());
		_pNetworkManager->EnqueueBroadcast(msg, size, pExpPlayer->GetSession());
	}
	else
	{
		_pNetworkManager->EnqueueBroadcast((char*)&header, HEADER_LEN);
		_pNetworkManager->EnqueueBroadcast(msg, size);
	}
}

void PlayerManager::CheckAttacknEnqueueDamagePacket(uint8 attackType, uint32 ID, uint8 direction, uint16 x, uint16 y)
{
	uint16 rangeX;
	uint16 rangeY;
	uint16 damage;

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
		uint16 minX = x - rangeX;
		uint16 minY = y - rangeY;
		uint16 maxY = y + rangeY;
		for (CList<Player*>::iterator i = _PlayerList.begin(); i != _PlayerList.end(); i++)
		{
			if ((*i)->GetX() >= minX && (*i)->GetX() <= x &&
				(*i)->GetY() >= minY && (*i)->GetY() <= maxY && ID != (*i)->GetID())
			{
				(*i)->Attacked(damage);
				stPACKET_SC_DAMAGE packetSCDamage;
				Create_PACKET_SC_DAMAGE(packetSCDamage, ID, (*i)->GetID(), (*i)->GetHp());
				EnqueueBroadcast(dfPACKET_SC_DAMAGE, (char*)&packetSCDamage,
					sizeof(stPACKET_SC_DAMAGE));
			}
		}
	}
		
	else if (direction == dfPACKET_MOVE_DIR_RR)
	{
		uint16 maxX = x + rangeX;
		uint16 minY = y - rangeY;
		uint16 maxY = y + rangeY;

		for (CList<Player*>::iterator i = _PlayerList.begin(); i != _PlayerList.end(); i++)
		{
			if ((*i)->GetX() >= x && (*i)->GetX() <= maxX &&
				(*i)->GetY() >= minY && (*i)->GetY() <= maxY && ID != (*i)->GetID())
			{
				(*i)->Attacked(damage);
				stPACKET_SC_DAMAGE packetSCDamage;
				Create_PACKET_SC_DAMAGE(packetSCDamage, ID, (*i)->GetID(), (*i)->GetHp());
				EnqueueBroadcast(dfPACKET_SC_DAMAGE, (char*)&packetSCDamage,
					sizeof(stPACKET_SC_DAMAGE));
			}
		}
	}
}