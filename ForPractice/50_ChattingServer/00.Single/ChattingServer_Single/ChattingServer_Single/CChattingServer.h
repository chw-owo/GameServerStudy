#pragma once
#include "CNetServer.h"
#include "CObjectPool.h"
#include "CLockFreePool.h"
#include "CLockFreeQueue.h"

#include "CSector.h"
#include "CPlayer.h"
#include "CJob.h"

#include "Protocol.h"
#include "ErrorCode.h"

#include <unordered_set>
#include <unordered_map>
using namespace std;

class CChattingServer : public CNetServer
{
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

private:
	bool OnConnectRequest();
	void OnAcceptClient(__int64 sessionID);
	void OnReleaseClient(__int64 sessionID);
	void OnRecv(__int64 sessionID, CPacket* packet);
	void OnSend(__int64 sessionID, int sendSize);

private:
	void HandleAccept(__int64 sessionID);
	void HandleRelease(__int64 sessionID);
	void HandleRecv(__int64 sessionID, CPacket* packet);

private:
	static unsigned int WINAPI UpdateThread(void* arg);
	static unsigned int WINAPI MonitorThread(void* arg);

private:
	void ReqSendUnicast(CPacket* packet, __int64 sessionID);
	void ReqSendAroundSector(CPacket* packet, CSector* centerSector, CPlayer* pExpPlayer = nullptr);
	
private:
	inline void HandleCSPacket_REQ_LOGIN(CPacket* CSpacket, CPlayer* player);
	inline void HandleCSPacket_REQ_SECTOR_MOVE(CPacket* CSpacket, CPlayer* player);
	inline void HandleCSPacket_REQ_MESSAGE(CPacket* CSpacket, CPlayer* player);
	inline void HandleCSPacket_REQ_HEARTBEAT(CPlayer* player);

private:
	inline void GetCSPacket_REQ_LOGIN(CPacket* packet, __int64& accountNo, wchar_t*& ID, wchar_t*& nickname, char*& sessionKey);
	inline void GetCSPacket_REQ_SECTOR_MOVE(CPacket* packet, __int64& accountNo, WORD& sectorX, WORD& sectorY);
	inline void GetCSPacket_REQ_MESSAGE(CPacket* packet, __int64& accountNo, WORD& messageLen, wchar_t*& message);
	
private:
	inline void SetSCPacket_RES_LOGIN(CPacket* packet, BYTE status, __int64 accountNo);
	inline void SetSCPacket_RES_SECTOR_MOVE(CPacket* packet, __int64 accountNo, WORD sectorX, WORD sectorY);
	inline void SetSCPacket_RES_MESSAGE(CPacket* packet, __int64 accountNo, wchar_t* ID, wchar_t* nickname, WORD messageLen, wchar_t* message);

private:
	bool _serverAlive = true;
	HANDLE _updateThread;
	HANDLE _monitorThread;

private:
	CSector _sectors[dfSECTOR_CNT_Y][dfSECTOR_CNT_X];

private:
	__int64 _playerIDGenerator = 0;
	unordered_set<CPlayer*> _players;
	unordered_map<__int64, CPlayer*> _playersMap;
	CObjectPool<CPlayer>* _pPlayerPool;

private:
	CLockFreeQueue<CJob*>* _pJobQueue;
	CLockFreePool<CJob>* _pJobPool;

private: // For Monitor
	long _totalAccept = 0;
	long _totalDisconnect = 0;
};

