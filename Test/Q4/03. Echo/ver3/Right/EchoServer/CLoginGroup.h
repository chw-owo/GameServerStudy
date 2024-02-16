#pragma once
#include "CNetGroup.h"
#include "CNetSendPacket.h"
#include "CSystemLog.h"
#include "CommonProtocol.h"
#include "CLoginUser.h"

#include <unordered_map>
using namespace std;

class CServer;
class CNetServer;
class CEchoGroup;
class CLoginGroup : public CNetGroup
{
	// for Group Function
public:
	CLoginGroup(CServer* pNet);

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

	void OnRecv(unsigned __int64 sessionID, CNetMsg* packet);
	void OnSend(unsigned __int64 sessionID);
	void OnError(int errorCode, wchar_t* errorMsg);
	void OnDebug(int debugCode, wchar_t* debugMsg);

	// for Echo Group
private:
	void ReqSendUnicast(CNetSendPacket* packet, unsigned __int64 sessionID);
	inline void HandlePacket_LOGIN(CNetMsg* packet, CLoginUser* user);
	inline void GetCSPacket_REQ_LOGIN(CNetMsg* packet, __int64& accountNo, char sessionKey[dfSESSIONKEY_LEN], int& version);
	inline void SetSCPacket_RES_LOGIN(CNetSendPacket* packet, BYTE status, __int64 accountNo);
	BYTE LoginCheck(CLoginUser* user);

private:
	unordered_map<unsigned __int64, CLoginUser*> _usersMap;
	CEchoGroup* _pEchoGroup;
	CTlsPool<CLoginUser>* _userPool;

public:
	__int64 GetUserCount() { return _usersMap.size(); }
};

