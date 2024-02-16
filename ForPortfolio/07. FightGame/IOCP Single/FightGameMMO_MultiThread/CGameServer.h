#pragma once
#include "CLanServer.h"
#include "CObjectPool.h"
#include "CSector.h"
#include "CPlayer.h"
#include "CProcessCpu.h"
#include "CProcessorCpu.h"
#include "Protocol.h"
#include "CLockFreeQueue.h"

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

private:
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
	void ReqSendUnicast(CLanSendPacket* packet, __int64 sessionID);
	void ReqSendOneSector(CLanSendPacket* packet, CSector* sector, CPlayer* pExpPlayer = nullptr);
	void ReqSendAroundSector(CLanSendPacket* packet, CSector* centerSector, CPlayer* pExpPlayer = nullptr);

private:
	void GetDataFromPacket();
	bool SkipForFixedFrame();
	void LogicUpdate();

private:
	void CreatePlayer(__int64 sessionID);
	void SetPlayerDead(CPlayer* pPlayer);
	void DeleteDeadPlayer(CPlayer* pPlayer);

	bool CheckMovable(short x, short y);
	void UpdatePlayerMove(CPlayer* pPlayer);
	void SetSector(CPlayer* pPlayer);
	void UpdateSector(CPlayer* pPlayer, short direction);
	void SetSectorsAroundInfo();

private:
	// About Packet ====================================================
private:
	// Handle System Job
	inline void Handle_ACCEPT(__int64 sessionID);
	inline void Handle_RELEASE(__int64 sessionID);

	// Handle CS Packet
	inline bool HandleCSPacket_MOVE_START(CLanMsg* recvPacket, CPlayer* pPlayer);
	inline bool HandleCSPacket_MOVE_STOP(CLanMsg* recvPacket, CPlayer* pPlayer);
	inline bool HandleCSPacket_ATTACK1(CLanMsg* recvPacket, CPlayer* pPlayer);
	inline bool HandleCSPacket_ATTACK2(CLanMsg* recvPacket, CPlayer* pPlayer);
	inline bool HandleCSPacket_ATTACK3(CLanMsg* recvPacket, CPlayer* pPlayer);
	inline bool HandleCSPacket_ECHO(CLanMsg* recvPacket, CPlayer* pPlayer);

	// Get Data from CS Packet
	inline void GetCSPacket_MOVE_START(CLanMsg* pPacket, unsigned char& moveDirection, short& x, short& y);
	inline void GetCSPacket_MOVE_STOP(CLanMsg* pPacket, unsigned char& direction, short& x, short& y);
	inline void GetCSPacket_ATTACK1(CLanMsg* pPacket, unsigned char& direction, short& x, short& y);
	inline void GetCSPacket_ATTACK2(CLanMsg* pPacket, unsigned char& direction, short& x, short& y);
	inline void GetCSPacket_ATTACK3(CLanMsg* pPacket, unsigned char& direction, short& x, short& y);
	inline void GetCSPacket_ECHO(CLanMsg* pPacket, int& time);

	// Set Game Data from Packet Data
	inline void SetPlayerMoveStart(CPlayer* pPlayer, unsigned char& moveDirection, short& x, short& y);
	inline void SetPlayerMoveStop(CPlayer* pPlayer, unsigned char& direction, short& x, short& y);
	inline void SetPlayerAttack1(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y);
	inline void SetPlayerAttack2(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y);
	inline void SetPlayerAttack3(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y);

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
	HANDLE _updateThread;
	HANDLE _monitorThread;
	CLockFreeQueue<CLanJob*>* _jobQueue;

private:
	CObjectPool<CPlayer>* _pPlayerPool;
	CTlsPool<CLanJob>* _pJobPool;
	CSector _sectors[dfSECTOR_CNT_Y][dfSECTOR_CNT_X];
	int _sectorCnt[dfMOVE_DIR_MAX] =
					{ dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM,
					  dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM,
					  dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM,
					  dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM };

	int _playerID = 0;
	int _timeGap = 1000 / dfFPS;

private:
	unordered_map<__int64, CPlayer*> _playerSessionID;
	CPlayer* _players[dfPLAYER_MAX] = { nullptr, };
	int _usableIDs[dfPLAYER_MAX];
	int _usableIDCnt = 0;

private:
	// For Monitor
	int _logicFPS = 0;
	int _syncCnt = 0;
	int _deadCnt = 0;
	int _timeoutCnt = 0;
	int _connectEndCnt = 0;
	int _checkPointsIdx = 0;
	int _checkPoints[dfMONITOR_CHECKPOINT] 
		= { 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000 };

	CProcessCpu _processCPUTime;
	CProcessorCpu _processorCPUTime;
};

