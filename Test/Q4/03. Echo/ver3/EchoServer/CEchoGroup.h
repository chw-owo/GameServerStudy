#pragma once
#include "CNetGroup.h"
#include "CRecvNetPacket.h"
#include "CSystemLog.h"
#include "CommonProtocol.h"
#include "CEchoUser.h"

#include <unordered_map>
using namespace std;

class CServer;
class CNetServer;
class CEchoGroup : public CNetGroup
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

	void OnRecv(unsigned __int64 sessionID, CRecvNetPacket* packet);
	void OnSend(unsigned __int64 sessionID);
	void OnError(int errorCode, wchar_t* errorMsg);
	void OnDebug(int debugCode, wchar_t* debugMsg);

	// for Echo Group
private:
	void ReqSendUnicast(CNetPacket* packet, unsigned __int64 sessionID);
	inline void HandlePacket_ECHO(CRecvNetPacket* packet, CEchoUser* user);
	inline void GetCSPacket_REQ_ECHO(CRecvNetPacket* packet, __int64& accountNo, LONGLONG& sendTick);
	inline void SetSCPacket_RES_ECHO(CNetPacket* packet, __int64 accountNo, LONGLONG sendTick);

private:
	unordered_map<unsigned __int64, CEchoUser*> _usersMap;
	CTlsPool<CEchoUser>* _userPool;

public:
	__int64 GetUserCount() { return _usersMap.size(); }
};

