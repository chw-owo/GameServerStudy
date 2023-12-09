#pragma once
#include "CSystemLog.h"
#include "CommonProtocol.h"
#include "ErrorCode.h"

#include "CNetServer.h"
#include "CTlsPool.h"
#include "CUser.h"

#include <unordered_map>
using namespace std;

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

using namespace std;

class CMonitorServer;
class CLoginServer : public CNetServer
{
	friend CMonitorServer;

public:
	CLoginServer() {}
	~CLoginServer() { Terminate(); }

public:
	bool Initialize();
	void Terminate();

private:
	void OnInitialize();
	void OnTerminate();
	void OnThreadTerminate(wchar_t* threadName);
	void OnError(int errorCode, wchar_t* errorMsg);
	void OnDebug(int debugCode, wchar_t* debugMsg);

private:
	bool OnConnectRequest(WCHAR addr[dfADDRESS_LEN]);
	void OnAcceptClient(unsigned __int64 sessionID, WCHAR addr[dfADDRESS_LEN]);
	void OnReleaseClient(unsigned __int64 sessionID);
	void OnRecv(unsigned __int64 sessionID, CPacket* packet);
	void OnSend(unsigned __int64 sessionID, int sendSize);

private:
	void ReqSendUnicast(CPacket* packet, unsigned __int64 sessionID);

private:
	BYTE CheckAccountNoValid(__int64 accountNo);
	inline void HandlePacket_LOGIN(CPacket* packet, CUser* user);
	inline void GetCSPacket_REQ_LOGIN(CPacket* packet, __int64& accountNo, char*& sessionKey, int& version);
	inline void SetSCPacket_RES_LOGIN(CPacket* packet, BYTE status, __int64 accountNo);
	inline void HandlePacket_ECHO(CPacket* packet, CUser* user);
	inline void GetCSPacket_REQ_ECHO(CPacket* packet, __int64& accountNo, LONGLONG& sendTick);
	inline void SetSCPacket_RES_ECHO(CPacket* packet, __int64 accountNo, LONGLONG sendTick);

private:
	bool _serverAlive = true;
	HANDLE _timeoutThread;

private:
	SRWLOCK _mapLock;
	unordered_map<unsigned __int64, CUser*> _usersMap;
	CTlsPool<CUser>* _pUserPool;
};