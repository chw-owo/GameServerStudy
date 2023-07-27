#include "ContentManager.h"
#include "NetworkManager.h"
#include "SerializePacket.h"
#include "Protocol.h"
#include "Session.h"

ContentManager::ContentManager()
{
}

ContentManager::~ContentManager()
{
}

ContentManager* ContentManager::GetInstance()
{
	static ContentManager _contentMgr;
	return &_contentMgr;
}

void ContentManager::Update()
{
	vector<User*>::iterator i = g_ContentData._userArray.begin();
	for (; i != g_ContentData._userArray.end(); i++)
	{
		SerializePacket packet;
		(*i)->DequeueRecvBuf();
	}
}

void ContentManager::CreateUser(Session* pSession)
{
	User* pUser = new User(g_ContentData._userID, pSession);
	g_ContentData._userArray.push_back(pUser);
	g_ContentData._userID++;
}

void ContentManager::CreateRoom()
{
}

void ContentManager::DestroyRoom(Room* pRoom)
{
}

void ContentManager::DestroyDeadUser()
{
	vector<User*>::iterator i = g_ContentData._userArray.begin();
	for (; i != g_ContentData._userArray.end();)
	{
		if (!(*i)->GetStateAlive())
		{
			(*i)->_pSession->SetSessionDead();
			User* pUser = *i;
			i = g_ContentData._userArray.erase(i);
			delete(pUser);
		}
		else
		{
			i++;
		}
	}
}
