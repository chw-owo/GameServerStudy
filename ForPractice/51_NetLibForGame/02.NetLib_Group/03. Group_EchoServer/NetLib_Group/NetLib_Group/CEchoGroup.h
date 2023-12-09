#pragma once
#include "CGroup.h"
#include "CPacket.h"
#include "CSystemLog.h"
#include "CommonProtocol.h"
#include "CEchoUser.h"

#include <unordered_map>
using namespace std;

class CServer;
class CNetServer;
class CEchoGroup : public CGroup
{
	// for Group Function
public:
	CEchoGroup(CServer* pNet);

private:
	 void Initialize();
	 void Update();
	 void Terminate();

private:
	 void OnUpdate();
	 void OnInitialize();
	 void OnTerminate();

	 void OnEnterGroup(unsigned __int64 sessionID);
	 void OnLeaveGroup(unsigned __int64 sessionID);

	 void OnReleaseClient(unsigned __int64 sessionID);
	 void OnRecv(unsigned __int64 sessionID, CPacket* packet);
	 void OnSend(unsigned __int64 sessionID, int sendSize);
	 void OnError(int errorCode, wchar_t* errorMsg);
	 void OnDebug(int debugCode, wchar_t* debugMsg);

	 // for Echo Group
private:
	inline void HandlePacket_ECHO(CPacket* packet, CEchoUser* user);
	inline void GetCSPacket_REQ_ECHO(CPacket* packet, __int64& accountNo, LONGLONG& sendTick);
	inline void SetSCPacket_RES_ECHO(CPacket* packet, __int64 accountNo, LONGLONG sendTick);
	
private:
	unordered_map<unsigned __int64, CEchoUser*> _usersMap;

	// Monitor
private:
	int _sendTPS = 0;
	int _recvTPS = 0;
	int _enterTPS = 0;
	int _leaveTPS = 0;
	int _releaseTPS = 0;
};

