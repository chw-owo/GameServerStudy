#include "PlayerManager.h"
#include "PacketDefine.h"
#include "NetworkManager.h"

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
	Player* player = new Player(pSession, 
		_ID, dfPACKET_MOVE_DIR_LL, 20, 20, 100);
	_PlayerList.push_back(player);

	// Send <Allocate ID Message> to New Player
	stPACKET_SC_CREATE_MY_CHARACTER packetCreateMy;
	packetCreateMy.ID = player->_ID;
	packetCreateMy.Direction = player->_direction;
	packetCreateMy.X = player->_x;
	packetCreateMy.Y = player->_y;
	packetCreateMy.HP = player->_hp;
	EnqueueUnicast(dfPACKET_SC_CREATE_MY_CHARACTER, (char*)&packetCreateMy, 
		sizeof(stPACKET_SC_CREATE_MY_CHARACTER), player);

	// Send <Create New Player Message> to All Player
	stPACKET_SC_CREATE_OTHER_CHARACTER packetCreateOther;
	packetCreateOther.ID = player->_ID;
	packetCreateOther.Direction = player->_direction;
	packetCreateOther.X = player->_x;
	packetCreateOther.Y = player->_y;
	packetCreateOther.HP = player->_hp;
	EnqueueBroadcast(dfPACKET_SC_CREATE_OTHER_CHARACTER, (char*)&packetCreateOther, 
		sizeof(stPACKET_SC_CREATE_OTHER_CHARACTER), player);

	// Send <Create All Players Message> to New Player
	stPACKET_SC_CREATE_OTHER_CHARACTER packetCreateAll;
	for (CList<Player*>::iterator i = _PlayerList.begin(); i != _PlayerList.end(); i++)
	{
		if ((*i)->_ID != _ID)
		{
			packetCreateAll.ID = (*i)->_ID;
			packetCreateAll.Direction = (*i)->_direction;
			packetCreateAll.X = (*i)->_x;
			packetCreateAll.Y = (*i)->_y;
			packetCreateAll.HP = (*i)->_hp;
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
	for (CList<Player*>::iterator i = _PlayerList.begin(); i != _PlayerList.end(); ++i)
	{
		// Update
		(*i)->Update();

		// Collision Check
		for (CList<Player*>::iterator j = _PlayerList.begin(); j != _PlayerList.end(); ++j)
		{
			if (!(*i)->GetDead() && !(*j)->GetDead() &&
				(*i)->GetX() == (*j)->GetX() &&
				(*i)->GetY() == (*j)->GetY())
			{
				(*i)->OnCollision((*j));
				(*j)->OnCollision((*i));
			}
		}
	}
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
