#pragma once
#include "CGroup.h"
class CDungeonServer : public CGroup
{	
public: 
	CDungeonServer(CNetServer* pNet) 
	{
		Setting(pNet, UpdateThread);
	};
	~CDungeonServer() {};

private:
	void OnInitialize();
	void OnTerminate();
	static unsigned int WINAPI UpdateThread(void* arg);

private:
	void OnRegisterClient(CSession* pSession);
	void OnReleaseClient(CSession* pSession);
	void OnRecv(CSession* pSession, CPacket* packet);
	void OnSend(CSession* pSession, int sendSize);
};

