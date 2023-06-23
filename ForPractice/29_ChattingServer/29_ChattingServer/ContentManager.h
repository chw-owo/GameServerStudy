#pragma once
#include "ContentData.h"

class Session;
class NetworkManager;
class ContentManager
{
private:
	ContentManager();
	~ContentManager();

public:
	static ContentManager* GetInstance();
	void Update();

public:
	void CreateUser(Session* pSession);
	void CreateRoom();
	void DestroyRoom(Room* pRoom);
	void DestroyDeadUser();

private:
	static ContentManager _contentManager;
	NetworkManager* _pNetworkManager = nullptr;
};

