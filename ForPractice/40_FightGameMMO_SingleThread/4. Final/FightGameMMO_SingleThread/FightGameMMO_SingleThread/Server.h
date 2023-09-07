#pragma once
#include "Protocol.h"
#include "ObjectPool.h"
#include "RingBuffer.h"
#include "SerializePacket.h"

#include "Profiler.h"
#include "SystemLog.h"
#include "ProcessCpuUsage.h"
#include "ProcessorCpuUsage.h"

#include <vector>
#include <wchar.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32")
using namespace std;

class CServer
{
public:
	CServer();
	~CServer();
	
public:
	void NetworkUpdate();
	void ContentUpdate();

#define dfMONITOR_MAX 1024
	inline void MonitorUpdate()
	{
		if (GetTickCount64() - _oldTick > 1000)
		{
			int connected = _sessionIDs - _usableCnt;
			_syncTotal += _syncCnt;

			SYSTEMTIME stTime;
			GetLocalTime(&stTime);

			WCHAR text[dfMONITOR_MAX];
			swprintf_s(text, dfMONITOR_MAX, 
				L"[%s %02d:%02d:%02d]----------------------------------\n\nConnected Session: %d\nTotal Sync: %d\nSync/1sec: %d\nAccept/1sec: %d\nDisconnect/1sec: %d\n - Dead: %d\n - Timeout: %d\n - Connect End: %d\n\n", 
				_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond, connected, _syncTotal, _syncCnt, _acceptCnt, _disconnectMonitorCnt, _deadCnt, _timeoutCnt, _connectEndCnt);
			::wprintf(L"%s", text);

			FILE* file;
			errno_t openRet = _wfopen_s(&file, L"MonitorLog.txt", L"a+");
			if (openRet != 0)
			{
				LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
					L"%s[%d]: Fail to open %s : %d\n",
					_T(__FUNCTION__), __LINE__, L"MonitorLog.txt", openRet);

				::wprintf(L"%s[%d]: Fail to open %s : %d\n",
					_T(__FUNCTION__), __LINE__, L"MonitorLog.txt", openRet);
			}

			if (file != nullptr)
			{
				fwprintf(file, text);
				fclose(file);
			}
			else
			{
				LOG(L"FightGame", CSystemLog::ERROR_LEVEL,
					L"%s[%d]: Fileptr is nullptr %s\n",
					_T(__FUNCTION__), __LINE__, L"MonitorLog.txt");

				::wprintf(L"%s[%d]: Fileptr is nullptr %s\n",
					_T(__FUNCTION__), __LINE__, L"MonitorLog.txt");
			}

			/*
			PRO_PRINT();		
			if (_checkPointsIdx < dfMONITOR_CHECKPOINT &&
				_checkPoints[_checkPointsIdx] < connected)
			{
				WCHAR titleBuf[32] = { L"\0", };
				wsprintf(titleBuf, L"ProfileBasic/%d.txt", _checkPoints[_checkPointsIdx]);
				PRO_SAVE(titleBuf);
				_checkPointsIdx++;
			}
			PRO_RESET();
			*/

			_syncCnt = 0;
			_acceptCnt = 0;
			_disconnectMonitorCnt = 0;
			_deadCnt = 0;
			_timeoutCnt = 0;
			_connectEndCnt = 0;

			_oldTick += 1000;
		}
	}


// About Network ==================================================
private:
	class Session
	{
	public:
		inline Session() {}
		inline void Initialize(int ID);
		
	public:
		bool _alive = true;
		int _ID;
		SOCKET _socket;
		SOCKADDR_IN _addr;
		CRingBuffer _recvRBuffer;
		CRingBuffer _sendRBuffer;
		CSerializePacket _recvSPacket;
		CSerializePacket _sendSPacket;
		DWORD _lastRecvTime;
	};

private:
	inline void SelectProc(int rStartIdx, int rCount, int wStartIdx, int wCount);
	inline void AcceptProc();
	inline void RecvProc(Session* pSession);
	inline void SendProc(Session* pSession);

private:
	class Sector;
	inline void EnqueueUnicast(char* msg, int size, Session* pSession);
	inline void EnqueueOneSector(char* msg, int size, Sector* sector, Session* pExpSession = nullptr);
	inline void EnqueueAroundSector(char* msg, int size, Sector* centerSector, Session* pExpSession = nullptr);

private:
	inline void SetSessionDead(Session* pSession, bool connectEnd = false);
	inline void DisconnectDeadSession();

private:
	FD_SET _rset;
	FD_SET _wset;
	timeval _time;
	int _addrlen;

	SOCKET _listensock = INVALID_SOCKET;
	Session* _Sessions[dfSESSION_MAX];  // Use Session ID for Index
	Session* _rSessions[dfSESSION_MAX];
	Session* _wSessions[dfSESSION_MAX];

	int _sessionIDs = 0;
	int _usableCnt = 0;
	int _usableSessionID[dfSESSION_MAX];
	int _disconnectCnt = 0;
	int _disconnectSessionIDs[dfSESSION_MAX];
	
// About Content ====================================================
private:
	class Player;
	class Sector
	{
	public:
		inline void InitializeSector(short xIndex, short yIndex);

	public:
		short _xIndex;
		short _yIndex;
		short _xPosMin;
		short _yPosMin;
		short _xPosMax;
		short _yPosMax;
		vector<Player*> _players;

	public:
		Sector* _llNew[dfVERT_SECTOR_NUM];
		Sector* _luNew[dfDIAG_SECTOR_NUM];
		Sector* _uuNew[dfVERT_SECTOR_NUM];
		Sector* _ruNew[dfDIAG_SECTOR_NUM];
		Sector* _rrNew[dfVERT_SECTOR_NUM];
		Sector* _rdNew[dfDIAG_SECTOR_NUM];
		Sector* _ddNew[dfVERT_SECTOR_NUM];
		Sector* _ldNew[dfDIAG_SECTOR_NUM];

		Sector* _llOld[dfVERT_SECTOR_NUM];
		Sector* _luOld[dfDIAG_SECTOR_NUM];
		Sector* _uuOld[dfVERT_SECTOR_NUM];
		Sector* _ruOld[dfDIAG_SECTOR_NUM];
		Sector* _rrOld[dfVERT_SECTOR_NUM];
		Sector* _rdOld[dfDIAG_SECTOR_NUM];
		Sector* _ddOld[dfVERT_SECTOR_NUM];
		Sector* _ldOld[dfDIAG_SECTOR_NUM];

	public:
		Sector* _around[dfAROUND_SECTOR_NUM];
		Sector** _new[dfMOVE_DIR_MAX] =
		{
			_llNew, _luNew, _uuNew, _ruNew, 
			_rrNew, _rdNew, _ddNew, _ldNew
		};
		
		Sector** _old[dfMOVE_DIR_MAX] =
		{
			_llOld, _luOld, _uuOld, _ruOld,
			_rrOld, _rdOld, _ddOld, _ldOld
		};
	};

private:
	class Player
	{
	public:
		inline Player(Session* pSession, int ID);
		inline Player();

	public:
		int _ID;
		bool _move;
		char _hp;
		short _x;
		short _y;
		BYTE _direction;
		BYTE _moveDirection;
		Session* _pSession;
		Sector* _pSector;
	};

private:
	inline bool SkipForFixedFrame();

private:
	inline void CreatePlayer(Session* pSession);
	inline void UpdatePlayerMove(Player* pPlayer);
	inline bool CheckMovable(short x, short y);

private:
	inline void UpdateSector(Player* pPlayer, short Direction);
	inline void SetSector(Player* pPlayer);
	inline void SetSectorsAroundInfo();

private:
	int _playerID = 0;
	Player*_Players[dfSESSION_MAX]; // Use Session ID for Index
	Sector _Sectors[dfSECTOR_CNT_Y][dfSECTOR_CNT_X];
	int _sectorCnt[dfMOVE_DIR_MAX] =
				{ dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM, 
				  dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM, 
				  dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM, 
				  dfVERT_SECTOR_NUM, dfDIAG_SECTOR_NUM };

private:
	CObjectPool<Session>* _pSessionPool;
	CObjectPool<Player>* _pPlayerPool;

// About Monitor ===============================================================
private:
	DWORD _oldTick;
	int _syncCnt = 0;
	int _acceptCnt = 0;
	int _deadCnt = 0;
	int _timeoutCnt = 0;
	int _connectEndCnt = 0;
	int _disconnectMonitorCnt = 0;
	int _syncTotal = 0;

	int _checkPointsIdx = 0;
	int _checkPoints[dfMONITOR_CHECKPOINT]
		= { 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000 };

	CProcessCpuUsage ProcessCPUTime;
	CProcessorCpuUsage ProcessorCPUTime;

// About Packet ====================================================
private:
	// Handle CS Packet
	inline bool HandleCSPackets(Player* pPlayer, BYTE type);
	inline bool HandleCSPacket_MOVE_START(Player* pPlayer);
	inline bool HandleCSPacket_MOVE_STOP(Player* pPlayer);
	inline bool HandleCSPacket_ATTACK1(Player* pPlayer);
	inline bool HandleCSPacket_ATTACK2(Player* pPlayer);
	inline bool HandleCSPacket_ATTACK3(Player* pPlayer);
	inline bool HandleCSPacket_ECHO(Player* pPlayer);

	// Get Data from CS Packet
	inline bool GetCSPacket_MOVE_START(CSerializePacket* pPacket, CRingBuffer* recvRBuffer, BYTE& moveDirection, short& x, short& y);
	inline bool GetCSPacket_MOVE_STOP(CSerializePacket* pPacket, CRingBuffer* recvRBuffer, BYTE& direction, short& x, short& y);
	inline bool GetCSPacket_ATTACK1(CSerializePacket* pPacket, CRingBuffer* recvRBuffer, BYTE& direction, short& x, short& y);
	inline bool GetCSPacket_ATTACK2(CSerializePacket* pPacket, CRingBuffer* recvRBuffer, BYTE& direction, short& x, short& y);
	inline bool GetCSPacket_ATTACK3(CSerializePacket* pPacket, CRingBuffer* recvRBuffer, BYTE& direction, short& x, short& y);
	inline bool GetCSPacket_ECHO(CSerializePacket* pPacket, CRingBuffer* recvRBuffer, int& time);

	// Set Game Data from Packet Data
	inline void SetPlayerMoveStart(Player* pPlayer, BYTE& moveDirection, short& x, short& y);
	inline void SetPlayerMoveStop(Player* pPlayer, BYTE& direction, short& x, short& y);
	inline void SetPlayerAttack1(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y);
	inline void SetPlayerAttack2(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y);
	inline void SetPlayerAttack3(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y);
	
	// Set Data on SC Packet
	inline void SetSCPacket_HEADER(CSerializePacket* pPacket, BYTE size, BYTE type);
	inline int SetSCPacket_CREATE_MY_CHAR(CSerializePacket* pPacket, int ID, BYTE direction, short x, short y, BYTE hp);
	inline int SetSCPacket_CREATE_OTHER_CHAR(CSerializePacket* pPacket, int ID, BYTE direction, short x, short y, BYTE hp);
	inline int SetSCPacket_DELETE_CHAR(CSerializePacket* pPacket, int ID);
	inline int SetSCPacket_MOVE_START(CSerializePacket* pPacket, int ID, BYTE moveDirection, short x, short y);
	inline int SetSCPacket_MOVE_STOP(CSerializePacket* pPacket, int ID, BYTE direction, short x, short y);
	inline int SetSCPacket_ATTACK1(CSerializePacket* pPacket, int ID, BYTE direction, short x, short y);
	inline int SetSCPacket_ATTACK2(CSerializePacket* pPacket, int ID, BYTE direction, short x, short y);
	inline int SetSCPacket_ATTACK3(CSerializePacket* pPacket, int ID, BYTE direction, short x, short y);
	inline int SetSCPacket_DAMAGE(CSerializePacket* pPacket, int attackID, int damageID, BYTE damageHP);
	inline int SetSCPacket_SYNC(CSerializePacket* pPacket, int ID, short x, short y);
	inline int SetSCPacket_ECHO(CSerializePacket* pPacket, int time);

};

