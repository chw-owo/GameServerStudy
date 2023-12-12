#pragma once
#include "CGroup.h"
#include "CPacket.h"
#include "CSystemLog.h"
#include "CommonProtocol.h"
#include "CLoginUser.h"

#include <unordered_map>
using namespace std;

class CServer;
class CNetServer;
class CEchoGroup;
class CLoginGroup : public CGroup
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

	void OnRecv(unsigned __int64 sessionID, CPacket* packet);
	void OnSend(unsigned __int64 sessionID);
	void OnError(int errorCode, wchar_t* errorMsg);
	void OnDebug(int debugCode, wchar_t* debugMsg);

	// for Echo Group
private:
	void ReqSendUnicast(CPacket* packet, unsigned __int64 sessionID);
	inline void HandlePacket_LOGIN(CPacket* packet, CLoginUser* user);
	inline void GetCSPacket_REQ_LOGIN(CPacket* packet, __int64& accountNo, char*& sessionKey, int& version);
	inline void SetSCPacket_RES_LOGIN(CPacket* packet, BYTE status, __int64 accountNo);
	BYTE LoginCheck(CLoginUser* user);

private:
	unordered_map<unsigned __int64, CLoginUser*> _usersMap;
	CEchoGroup* _pEchoGroup;
	CTlsPool<CLoginUser>* _userPool;

public:
	__int64 GetUserCount() { return _usersMap.size(); }
};

