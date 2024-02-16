#pragma once

#include "CommonProtocol.h"
#include "ErrorCode.h"
#include "CSystemLog.h"
#include "CNetServer.h"
#include "CObjectPool.h"
#include "CRedis.h"
#include "CUser.h"

#include <unordered_map>
#include <unordered_set>
using namespace std;

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include "mysql.h"
#pragma comment (lib, "libmysql.lib")

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
	bool OnConnectRequest(WCHAR* addr);
	void OnAcceptClient(unsigned __int64 sessionID, WCHAR* addr);
	void OnReleaseClient(unsigned __int64 sessionID);
	void OnRecv(unsigned __int64 sessionID, CNetMsg* packet);
	void OnSend(unsigned __int64 sessionID, int sendSize);

private:
	static unsigned int WINAPI TimeoutThread(void* arg);
	void ReqSendUnicast(CNetSendPacket* packet, unsigned __int64 sessionID);
	bool InitDBConnect(long& idx);

private:
	BYTE CheckAccountNoValid(__int64 accountNo);
	void GetIPforClient(CUser* user, WCHAR* gameIP, WCHAR* chatIP);
	bool GetDataFromMysql(CUser* user);
	void SetDataToRedis(CUser* user);
	void SetUserData(CUser* user);

private:
	inline void HandleCSPacket_REQ_LOGIN(CNetMsg* CSpacket, CUser* user);
	inline void GetCSPacket_REQ_LOGIN(CNetMsg* packet, __int64& accountNo, char*& sessionKey);
	inline void SetSCPacket_RES_LOGIN(
		CNetSendPacket* packet, __int64 accountNo, BYTE status, WCHAR*& ID, WCHAR*& nickname,
		WCHAR*& gameIP, unsigned short gamePort, WCHAR*& chatIP, unsigned short chatPort);

public:
	long GetAuthTPS() 
	{ 
		long authTPS = InterlockedExchange(&_authTPS, 0);
		return authTPS; 
	}

	long GetPacketPoolSize() 
	{ 
		return CNetSendPacket::_pool.GetNodeCount() + CNetRecvPacket::_pool.GetNodeCount();
	}

private:
	bool _serverAlive = true;
	HANDLE _timeoutThread;

private:
	SRWLOCK _setLock;
	SRWLOCK _mapLock;
	unordered_map<unsigned __int64, CUser*> _usersMap;
	unordered_set<__int64> _accountNos;
	CObjectPool<CUser>* _pUserPool;

private:
	struct cmp
	{
		bool operator()(const WCHAR* a, const WCHAR* b) const
		{
			return wcscmp(a, b);
		}
	};

	map<WCHAR*, WCHAR*, cmp> _LanIPs;
	WCHAR* _WanIPs;

private:
	long _tlsKey;
	long _connCnt = 0;
	SRWLOCK _connLock;
	MYSQL _conn[dfTHREAD_MAX] = { 0, };
	MYSQL* _connection[dfTHREAD_MAX] = { 0, };
	cpp_redis::client* _redis;

private:
	volatile long _authTPS = 0;

};