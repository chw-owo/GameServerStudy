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

	void OnRecv(unsigned __int64 sessionID, CPacket* packet);
	void OnSend(unsigned __int64 sessionID);
	void OnError(int errorCode, wchar_t* errorMsg);
	void OnDebug(int debugCode, wchar_t* debugMsg);

	// for Echo Group
private:
	void ReqSendUnicast(CPacket* packet, unsigned __int64 sessionID);
	inline void HandlePacket_ECHO(CPacket* packet, CEchoUser* user);
	inline void GetCSPacket_REQ_ECHO(CPacket* packet, __int64& accountNo, LONGLONG& sendTick);
	inline void SetSCPacket_RES_ECHO(CPacket* packet, __int64 accountNo, LONGLONG sendTick);

private:
	unordered_map<unsigned __int64, CEchoUser*> _usersMap;
	CTlsPool<CEchoUser>* _userPool;

public:
	__int64 GetUserCount() { return _usersMap.size(); }
};

