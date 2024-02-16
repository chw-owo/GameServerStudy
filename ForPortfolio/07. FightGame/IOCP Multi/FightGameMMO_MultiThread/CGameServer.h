#pragma once
#include "CLanServer.h"
#include "CTlsPool.h"
#include "CSector.h"
#include "CPlayer.h"
#include "CProcessCpu.h"
#include "CProcessorCpu.h"
#include "Protocol.h"
#include <queue>
#include <unordered_map>
using namespace std;

class CGameServer : public CLanServer
{
public:
	CGameServer();
	~CGameServer();
	void Initialize();

private:
	void OnInitialize();
	void OnTerminate();
	void OnThreadTerminate(wchar_t* threadName);

	bool OnConnectRequest(WCHAR* addr);
	void OnAcceptClient(unsigned __int64 sessionID, WCHAR* addr);
	void OnReleaseClient(unsigned __int64 sessionID);
	void OnRecv(unsigned __int64 sessionID, CLanMsg* packet);
	void OnSend(unsigned __int64 sessionID, int sendSize);
	void OnError(int errorCode, wchar_t* errorMsg);
	void OnDebug(int debugCode, wchar_t* debugMsg);

private:
	static unsigned int WINAPI UpdateThread(void* arg);
	static unsigned int WINAPI MonitorThread(void* arg);

private:
	inline void ReqSendUnicast(CLanSendPacket* packet, unsigned __int64 sessionID);
	inline void ReqSendOneSector(CLanSendPacket* packet, CSector* sector, CPlayer* pExpPlayer = nullptr);
	inline void ReqSendAroundSector(CLanSendPacket* packet, CSector* centerSector, CPlayer* pExpPlayer = nullptr);

private:
	inline void SleepForFixedFrame(DWORD& oldTick);
	inline void LogicUpdate(int threadNum);

private:
	inline bool CheckMovable(short x, short y);
	inline void UpdatePlayerMove(CPlayer* pPlayer);
	inline void SetPlayerDead(CPlayer* pPlayer, bool timeout);

	inline void UpdateSector(CPlayer* pPlayer, short direction);
	inline void SetSectorsAroundInfo();

private:
	// About Packet ====================================================
private:
	// Handle System Job
	inline void HandleAccept(unsigned __int64 sessionID);
	inline void HandleRelease(unsigned __int64 sessionID);
	inline void HandleRecv(unsigned __int64 sessionID, CLanMsg* packet);

	// Handle CS Packet
	inline void HandleCSPacket_MOVE_START(CLanMsg* recvPacket, CPlayer* pPlayer);
	inline void HandleCSPacket_MOVE_STOP(CLanMsg* recvPacket, CPlayer* pPlayer);
	inline void HandleCSPacket_ATTACK1(CLanMsg* recvPacket, CPlayer* pPlayer);
	inline void HandleCSPacket_ATTACK2(CLanMsg* recvPacket, CPlayer* pPlayer);
	inline void HandleCSPacket_ATTACK3(CLanMsg* recvPacket, CPlayer* pPlayer);
	inline void HandleCSPacket_ECHO(CLanMsg* recvPacket, CPlayer* pPlayer);

	// Get Data from CS Packet
	inline void GetCSPacket_MOVE_START(CLanMsg* pPacket, unsigned char& moveDirection, short& x, short& y);
	inline void GetCSPacket_MOVE_STOP(CLanMsg* pPacket, unsigned char& direction, short& x, short& y);
	inline void GetCSPacket_ATTACK1(CLanMsg* pPacket, unsigned char& direction, short& x, short& y);
	inline void GetCSPacket_ATTACK2(CLanMsg* pPacket, unsigned char& direction, short& x, short& y);
	inline void GetCSPacket_ATTACK3(CLanMsg* pPacket, unsigned char& direction, short& x, short& y);
	inline void GetCSPacket_ECHO(CLanMsg* pPacket, int& time);

	// Set Game Data from Packet Data
	inline bool SetPlayerMoveStart(CPlayer* pPlayer, unsigned char& moveDirection, short& x, short& y);
	inline bool SetPlayerMoveStop(CPlayer* pPlayer, unsigned char& direction, short& x, short& y);
	inline bool SetPlayerAttack1(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y);
	inline bool SetPlayerAttack2(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y);
	inline bool SetPlayerAttack3(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y);

	// Set Data on SC Packet
	inline int SetSCPacket_CREATE_MY_CHAR(CLanSendPacket* pPacket, int ID, unsigned char direction, short x, short y, unsigned char hp);
	inline int SetSCPacket_CREATE_OTHER_CHAR(CLanSendPacket* pPacket, int ID, unsigned char direction, short x, short y, unsigned char hp);
	inline int SetSCPacket_DELETE_CHAR(CLanSendPacket* pPacket, int ID);
	inline int SetSCPacket_MOVE_START(CLanSendPacket* pPacket, int ID, unsigned char moveDirection, short x, short y);
	inline int SetSCPacket_MOVE_STOP(CLanSendPacket* pPacket, int ID, unsigned char direction, short x, short y);
	inline int SetSCPacket_ATTACK1(CLanSendPacket* pPacket, int ID, unsigned char direction, short x, short y);
	inline int SetSCPacket_ATTACK2(CLanSendPacket* pPacket, int ID, unsigned char direction, short x, short y);
	inline int SetSCPacket_ATTACK3(CLanSendPacket* pPacket, int ID, unsigned char direction, short x, short y);
	inline int SetSCPacket_DAMAGE(CLanSendPacket* pPacket, int attackID, int damageID, unsigned char damageHP);
	inline int SetSCPacket_SYNC(CLanSendPacket* pPacket, int ID, short x, short y);
	inline int SetSCPacket_ECHO(CLanSendPacket* pPacket, int time);

private:
	class ThreadArg
	{
	public:
		ThreadArg(CGameServer* pServer, int num)
			:_pServer(pServer), _num(num) {};
	public:
		CGameServer* _pServer;
		int _num;
	};

private:
	HANDLE* _logicThreads;
	HANDLE _monitorThread;

private:
	CSector _sectors[dfSECTOR_CNT_Y][dfSECTOR_CNT_X];
	int _sectorCnt[dfMOVE_DIR_MAX] =
	{ dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM,
	  dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM,
	  dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM,
	  dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM };

private:
	CPlayer* _playersArray[dfLOGIC_THREAD_NUM][dfLOGIC_PLAYER_NUM];

	unordered_map<unsigned __int64, CPlayer*> _playersMap;
	SRWLOCK _playersMapLock;
	long long _playerIDGenerator = 0;
	CLockFreeStack<int> _usablePlayerIdx;

private:
	DWORD _timeGap = 1000 / dfFPS;

private:
	// For Monitor
	volatile long _logicFPS = 0;
	volatile long _syncCnt = 0;
	volatile long _deadCnt = 0;
	volatile long _timeoutCnt = 0;
	volatile long _connectEndCnt = 0;

	CProcessCpu _processCPUTime;
	CProcessorCpu _processorCPUTime;
};
