#include "PlayerManager.h"
#include "PacketDefine.h"
#include "NetworkManager.h"
#include "CreatePacket.h"

#include <windows.h>
#pragma comment(lib, "winmm.lib")


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
		player->_ID, player->_direction, player->_x, player->_y, player->_hp);
	EnqueueUnicast(dfPACKET_SC_CREATE_MY_CHARACTER, (char*)&packetCreateMy, 
		sizeof(stPACKET_SC_CREATE_MY_CHARACTER), player);

	// Send <Create New Player Message> to All Player
	stPACKET_SC_CREATE_OTHER_CHARACTER packetCreateOther;
	Create_PACKET_SC_CREATE_OTHER_CHARACTER(packetCreateOther,
		player->_ID, player->_direction, player->_x, player->_y, player->_hp);
	EnqueueBroadcast(dfPACKET_SC_CREATE_OTHER_CHARACTER, (char*)&packetCreateOther, 
		sizeof(stPACKET_SC_CREATE_OTHER_CHARACTER), player);

	// Send <Create All Players Message> to New Player
	stPACKET_SC_CREATE_OTHER_CHARACTER packetCreateAll;
	for (CList<Player*>::iterator i = _PlayerList.begin(); i != _PlayerList.end(); i++)
	{
		if ((*i)->_ID != _ID)
		{
			Create_PACKET_SC_CREATE_OTHER_CHARACTER(packetCreateAll,
				(*i)->_ID, (*i)->_direction, (*i)->_x, (*i)->_y, (*i)->_hp);
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
		delete(player);
	}
}

void PlayerManager::Update()
{
	timeBeginPeriod(1);
	static DWORD oldTick = timeGetTime();
	if ((timeGetTime() - oldTick) < (1000 / FPS))
		return;
	oldTick += (1000 / FPS);

	for (CList<Player*>::iterator i = _PlayerList.begin(); i != _PlayerList.end();)
	{
		// Destroy Dead Player
		if (!(*i)->GetStateAlive())
		{
			i = _PlayerList.erase(i);
			delete((*i));
			continue;
		}

		// Recv and Update
		(*i)->DequeueRecvBuf();
		(*i)->Update();

		// Collision Check
		/*
		for (CList<Player*>::iterator j = _PlayerList.begin(); j != _PlayerList.end(); ++j)
		{
			if ((*i)->GetStateAlive() && !(*j)->GetStateAlive() &&
				(*i)->GetX() == (*j)->GetX() &&
				(*i)->GetY() == (*j)->GetY())
			{
				(*i)->OnCollision((*j));
				(*j)->OnCollision((*i));
			}
		}
		*/

		// Broadcast Move Packet
		if ((*i)->GetPacketMoveStart())
		{
			stPACKET_SC_MOVE_START packetSCMoveStart;
			Create_PACKET_SC_MOVE_START(packetSCMoveStart,
				(*i)->_ID, (*i)->_moveDirection, (*i)->_x, (*i)->_y);
			(*i)->ResetPacketMoveStart();
		}

		if ((*i)->GetPacketMoveStop())
		{
			stPACKET_SC_MOVE_START packetSCMoveStart;
			Create_PACKET_SC_MOVE_START(packetSCMoveStart,
				(*i)->_ID, (*i)->_direction, (*i)->_x, (*i)->_y);
			(*i)->ResetPacketMoveStop();
		}

		i++;
	}
}

void PlayerManager::Terminate()
{

}

void PlayerManager::EnqueueUnicast(uint8 msgType, char* msg, int size, Player* pPlayer)
{
	stPACKET_HEADER header;
	header.Code = 0x89;
	header.Size = size;
	header.Type = msgType;
	
	if(_pNetworkManager == nullptr)
		_pNetworkManager = NetworkManager::GetInstance();

	_pNetworkManager->EnqueueUnicast((char*)&header, HEADER_LEN, pPlayer->_pSession);
	_pNetworkManager->EnqueueUnicast(msg, size, pPlayer->_pSession);
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
		_pNetworkManager->EnqueueBroadcast((char*)&header, HEADER_LEN, pExpPlayer->_pSession);
		_pNetworkManager->EnqueueBroadcast(msg, size, pExpPlayer->_pSession);
	}
	else
	{
		_pNetworkManager->EnqueueBroadcast((char*)&header, HEADER_LEN);
		_pNetworkManager->EnqueueBroadcast(msg, size);
	}
}