#pragma once
#include "CNetServer.h"
#include "CTlsPool.h"
#include "CLockFreeQueue.h"

#include "CSector.h"
#include "CPlayer.h"
#include "CNetJob.h"

#include "CommonProtocol.h"
#include "ErrorCode.h"

#include <unordered_map>
using namespace std;

#include <synchapi.h>
#pragma comment(lib, "Synchronization.lib")

class CMonitorClient;
class CChattingServer : public CNetServer
{
	friend CMonitorClient;

public:
	CChattingServer() {};
	~CChattingServer() { Terminate(); };

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
	inline void HandleTimeout();
	inline void HandleRecv(CPlayer* pPlayer, CNetMsg* packet);

private:
	static unsigned int WINAPI UpdateThread(void* arg);
	static unsigned int WINAPI TimeoutThread(void* arg);

private:
	inline void ReqSendUnicast(CNetSendPacket* packet, __int64 sessionID);
	inline void ReqSendAroundSector(CNetSendPacket* packet, CSector* centerSector, CPlayer* pExpPlayer = nullptr);

private:
	inline void HandleCSPacket_REQ_LOGIN(CNetMsg* CSpacket, CPlayer* player);
	inline void HandleCSPacket_REQ_SECTOR_MOVE(CNetMsg* CSpacket, CPlayer* player);
	inline void HandleCSPacket_REQ_MESSAGE(CNetMsg* CSpacket, CPlayer* player);
	inline void HandleCSPacket_REQ_HEARTBEAT(CPlayer* player);

private:
	inline void GetCSPacket_REQ_LOGIN(CNetMsg* packet, __int64& accountNo, wchar_t ID[dfID_LEN], wchar_t nickname[dfNICKNAME_LEN], char sessionKey[dfSESSIONKEY_LEN]);
	inline void GetCSPacket_REQ_SECTOR_MOVE(CNetMsg* packet, __int64& accountNo, WORD& sectorX, WORD& sectorY);
	inline void GetCSPacket_REQ_MESSAGE(CNetMsg* packet, __int64& accountNo, WORD& messageLen, wchar_t message[dfMSG_MAX]);

private:
	inline void SetSCNetPacket_RES_LOGIN(CNetSendPacket* packet, BYTE status, __int64 accountNo);
	inline void SetSCNetPacket_RES_SECTOR_MOVE(CNetSendPacket* packet, __int64 accountNo, WORD sectorX, WORD sectorY);
	inline void SetSCNetPacket_RES_MESSAGE(CNetSendPacket* packet, __int64 accountNo, wchar_t* ID, wchar_t* nickname, WORD messageLen, wchar_t* message);

private: // For Monitor

	inline long GetPlayerCount()
	{
		return _playersMap.size();
	}

	inline long GetJobPoolSize()
	{
		return _pJobPool->GetNodeCount();
	}

	inline long GetPacketPoolSize()
	{
		return CNetSendPacket::_pool.GetNodeCount() + CNetRecvPacket::_pool.GetNodeCount() - dfSESSION_MAX;
	}

	inline long GetJobQSize()
	{
		return _jobQSize;
	}

	inline long GetUpdateTPS()
	{
		long updateTPS = InterlockedExchange(&_updateCnt, 0);
		return updateTPS;
	}

private:
	bool _serverAlive = true;
	HANDLE _timeoutThread;

private:
	CSector _sectors[dfSECTOR_CNT_Y][dfSECTOR_CNT_X];

private:
	__int64 _playerIDGenerator = 0;
	CPlayer** _players;
	unordered_map<unsigned __int64, CPlayer*> _playersMap;
	CLockFreeStack<int> _usablePlayerIdx;
	SRWLOCK _playersLock;

private:
	CTlsPool<CNetJob>* _pJobPool;
	long* _signals = 0;
	long _updateCnt = 0;

private: // For Multithread
	int _playerPerThread = 0;
	HANDLE* _updateThreads;
	long _jobQSize;
};

class ThreadArg
{
public:
	ThreadArg(CChattingServer* pServer, int threadIdx)
		:_pServer(pServer), _threadIdx(threadIdx) {};
public:
	CChattingServer* _pServer;
	int _threadIdx;
};
