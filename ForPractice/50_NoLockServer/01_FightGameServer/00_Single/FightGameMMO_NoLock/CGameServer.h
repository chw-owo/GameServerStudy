#pragma once
#include "CLockFreePool.h"
#include "CLockFreeQueue.h"
#include "CObjectPool.h"
#include "CLanServer.h"
#include "CSector.h"
#include "CPlayer.h"
#include "CJob.h"
#include "Protocol.h"
#include "ErrorCode.h"

#include <unordered_map>
using namespace std;

class CGameServer : public CLanServer
{
public:
	CGameServer() {};
	~CGameServer() { Terminate(); };

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
	static unsigned int WINAPI UpdateThread(void* arg);
	static unsigned int WINAPI MonitorThread(void* arg);

private:
	void HandleAccept(__int64 sessionID);
	void HandleRelease(__int64 sessionID);
	void HandleRecv(__int64 sessionID, CPacket* packet);

private:
	void ReqSendUnicast(CPacket* packet, __int64 sessionID);
	void ReqSendSectors(CPacket* packet, CSector** sector, int sectorCnt, CPlayer* pExpPlayer = nullptr);
	void ReqSendAroundSector(CPacket* packet, CSector* centerSector, CPlayer* pExpPlayer = nullptr);

private:
	bool SkipForFixedFrame(DWORD time);
	void LogicUpdate();
	void HandleNetwork();

private:
	bool CheckMovable(short x, short y);
	void UpdatePlayerMove(CPlayer* pPlayer);
	void SetPlayerDead(CPlayer* pPlayer);
	void UpdateSector(CPlayer* pPlayer, short direction);
	void SetSectorsAroundInfo();

private:
	// About Packet ====================================================
	
	// Handle CS Packet
	inline void HandleCSPacket_MOVE_START(CPacket* recvPacket, CPlayer* pPlayer);
	inline void HandleCSPacket_MOVE_STOP(CPacket* recvPacket, CPlayer* pPlayer);
	inline void HandleCSPacket_ATTACK1(CPacket* recvPacket, CPlayer* pPlayer);
	inline void HandleCSPacket_ATTACK2(CPacket* recvPacket, CPlayer* pPlayer);
	inline void HandleCSPacket_ATTACK3(CPacket* recvPacket, CPlayer* pPlayer);
	inline void HandleCSPacket_ECHO(CPacket* recvPacket, CPlayer* pPlayer);

	// Get Data from CS Packet
	inline void GetCSPacket_MOVE_START(CPacket* pPacket, unsigned char& moveDirection, short& x, short& y);
	inline void GetCSPacket_MOVE_STOP(CPacket* pPacket, unsigned char& direction, short& x, short& y);
	inline void GetCSPacket_ATTACK1(CPacket* pPacket, unsigned char& direction, short& x, short& y);
	inline void GetCSPacket_ATTACK2(CPacket* pPacket, unsigned char& direction, short& x, short& y);
	inline void GetCSPacket_ATTACK3(CPacket* pPacket, unsigned char& direction, short& x, short& y);
	inline void GetCSPacket_ECHO(CPacket* pPacket, int& time);

	// Set Game Data from Packet Data
	inline bool SetPlayerMoveStart(CPlayer* pPlayer, unsigned char& moveDirection, short& x, short& y);
	inline bool SetPlayerMoveStop(CPlayer* pPlayer, unsigned char& direction, short& x, short& y);
	inline bool SetPlayerAttack1(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y);
	inline bool SetPlayerAttack2(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y);
	inline bool SetPlayerAttack3(CPlayer* pPlayer, CPlayer*& pDamagedPlayer, unsigned char& direction, short& x, short& y);

	// Set Data on SC Packet
	inline int SetSCPacket_CREATE_MY_CHAR(CPacket* pPacket, int ID, unsigned char direction, short x, short y, unsigned char hp);
	inline int SetSCPacket_CREATE_OTHER_CHAR(CPacket* pPacket, int ID, unsigned char direction, short x, short y, unsigned char hp);
	inline int SetSCPacket_DELETE_CHAR(CPacket* pPacket, int ID);
	inline int SetSCPacket_MOVE_START(CPacket* pPacket, int ID, unsigned char moveDirection, short x, short y);
	inline int SetSCPacket_MOVE_STOP(CPacket* pPacket, int ID, unsigned char direction, short x, short y);
	inline int SetSCPacket_ATTACK1(CPacket* pPacket, int ID, unsigned char direction, short x, short y);
	inline int SetSCPacket_ATTACK2(CPacket* pPacket, int ID, unsigned char direction, short x, short y);
	inline int SetSCPacket_ATTACK3(CPacket* pPacket, int ID, unsigned char direction, short x, short y);
	inline int SetSCPacket_DAMAGE(CPacket* pPacket, int attackID, int damageID, unsigned char damageHP);
	inline int SetSCPacket_SYNC(CPacket* pPacket, int ID, short x, short y);
	inline int SetSCPacket_ECHO(CPacket* pPacket, int time);

private:
	bool _serverAlive = true;
	HANDLE _updateThread;
	HANDLE _monitorThread;

private:
	CSector _sectors[dfSECTOR_CNT_Y][dfSECTOR_CNT_X];
	int _sectorCnt[dfMOVE_DIR_MAX] =
	{ dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM,
	  dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM,
	  dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM,
	  dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM };

private:
	__int64 _playerIDGenerator = 0;
	unordered_map<__int64, CPlayer*> _playersMap;
	CObjectPool<CPlayer>* _playerPool;

private:
	// For Update Thread
	DWORD _oldTick;
	int _timeGap = 1000 / dfFPS;
	CLockFreeQueue<CJob*>* _pJobQueue;
	CLockFreePool<CJob>* _pJobPool;

private:
	// For Monitor Thread
	volatile long _logicFPS = 0;
	volatile long _syncCnt = 0;
	volatile long _timeoutCnt = 0;
};