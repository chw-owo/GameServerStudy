#pragma once
#include "SerializePacket.h"
#include "RingBuffer.h"
#include "ObjectPool.h"
#include "Profiler.h"
#include "Protocol.h"
#include "SystemLog.h"

#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <vector>
#include <wchar.h>
using namespace std;

class Server
{
public:
	Server();
	~Server();
	void NetworkUpdate();
	void ContentUpdate();

// Monitor ===============================================================
public:

#define checkPointsMax 11
#define dfPRO_TITLE_LEN 64
	inline void Monitor()
	{
		if (GetTickCount64() - _oldTick > 1000)
		{
			int connected = _sessionIDs - _usableCnt;

			printf("[%s %s]\n\n", __DATE__, __TIME__);
			printf("Connected Session: %d\n", connected);
			printf("Sync/1sec: %d\n", _syncCnt);
			printf("Accept/1sec: %d\n", _acceptCnt);
			printf("Disconnect/1sec: %d\n", _disconnectMonitorCnt);
			printf(" - Dead: %d\n", _deadCnt);
			printf(" - Timeout: %d\n", _timeoutCnt);
			printf(" - Connect End(10054): %d\n", _connectEndCnt); 

			PRO_PRINT_CONSOLE();

			if (_checkPointsIdx < checkPointsMax &&
				_checkPoints[_checkPointsIdx] < connected)
			{
				WCHAR titleBuf[32] = { L"\0", };
				wsprintf(titleBuf, L"ProfileBasic/%d.txt", _checkPoints[_checkPointsIdx]);
				PRO_FILE_OUT(titleBuf);
				_checkPointsIdx++;
			}

			PRO_RESET();

			_syncCnt = 0;
			_acceptCnt = 0;
			_disconnectMonitorCnt = 0;
			_deadCnt = 0;
			_timeoutCnt = 0;
			_connectEndCnt = 0;

			_oldTick += 1000;
		}
	}

private:
	DWORD _oldTick;
	int _syncCnt = 0;
	int _acceptCnt = 0;
	int _deadCnt = 0;
	int _timeoutCnt = 0;
	int _connectEndCnt = 0;
	int _disconnectMonitorCnt = 0;

	int _checkPointsIdx = 0;
	int _checkPoints[checkPointsMax]
		= { 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000 };

// About Network ==================================================
private:
	class Session
	{
	public:
		inline Session(int ID, SerializePacket* pRecvPacket, SerializePacket* pSendPacket)
		{ 
			_ID = ID; 
			_pRecvPacket = pRecvPacket;
			_pSendPacket = pSendPacket;
		}

		inline Session() 
		{ 
			printf("Error! Session() is called\n"); 
			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: Session() is called\n", _T(__FUNCTION__), __LINE__);
		}

	public:
		bool _alive = true;
		int _ID;
		SOCKET _socket;
		SOCKADDR_IN _addr;
		RingBuffer _recvBuf;
		RingBuffer _sendBuf;
		SerializePacket* _pRecvPacket;
		SerializePacket* _pSendPacket;
		DWORD _lastRecvTime;
	};

private:
	inline void SelectProc(FD_SET rset, FD_SET wset, int rStartIdx, int rCount, int wStartIdx, int wCount);
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
	timeval _time;
	SOCKET _listensock = INVALID_SOCKET;
	Session* _SessionMap[dfSESSION_MAX];
	Session* _rSessions[dfSESSION_MAX];
	Session* _wSessions[dfSESSION_MAX];

	int _sessionIDs = 0;
	int _usableCnt = 0;
	int _usableSessionID[dfSESSION_MAX];
	int _disconnectCnt = 0;
	int _disconnectedSessionIDs[dfSESSION_MAX];
	
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
		vector<Player*> _players;

	public:
		short _xPosMin;
		short _yPosMin;
		short _xPosMax;
		short _yPosMax;

	public:
		Sector* _llNew[3];
		Sector* _luNew[5];
		Sector* _uuNew[3];
		Sector* _ruNew[5];
		Sector* _rrNew[3];
		Sector* _rdNew[5];
		Sector* _ddNew[3];
		Sector* _ldNew[5];

		Sector* _llOld[3];
		Sector* _luOld[5];
		Sector* _uuOld[3];
		Sector* _ruOld[5];
		Sector* _rrOld[3];
		Sector* _rdOld[5];
		Sector* _ddOld[3];
		Sector* _ldOld[5];

	public:
		Sector* _around[9];
		Sector** _new[8] = 
		{
			_llNew, _luNew, _uuNew, _ruNew, 
			_rrNew, _rdNew, _ddNew, _ldNew
		};
		
		Sector** _old[8] =
		{
			_llOld, _luOld, _uuOld, _ruOld,
			_rrOld, _rdOld, _ddOld, _ldOld
		};
	};

private:
	class Player
	{
	public:
		inline Player(Session* pSession, int ID)
			: _pSession(pSession), _pSector(nullptr), _ID(ID), _hp(dfMAX_HP), 
			_move(false), _direction(dfPACKET_MOVE_DIR_LL), _moveDirection(dfPACKET_MOVE_DIR_LL)
		{
			_x = rand() % dfRANGE_MOVE_RIGHT;
			_y = rand() % dfRANGE_MOVE_BOTTOM;
		}

		inline Player() 
		{ 
			printf("Error! Player() is called\n"); 
			LOG(L"ERROR", SystemLog::ERROR_LEVEL,
				L"%s[%d]: Session() is called\n", _T(__FUNCTION__), __LINE__);
		}

	public:
		int _ID;
		bool _move;
		signed char _hp;
		Session* _pSession;
		Sector* _pSector;

		short _x;
		short _y;
		BYTE _direction;
		BYTE _moveDirection;
	};

private:
	inline bool SkipForFixedFrame();

private:
	inline void CreatePlayer(Session* pSession);
	inline void UpdatePlayerMove(Player* pPlayer);
	inline bool CheckMovable(short x, short y);

private:
	inline void UpdateSector(Player* pPlayer, short Direction);
	inline void SetSectorsAroundInfo();
	inline void SetSector(Player* pPlayer);
	
private:
	int _playerID = 0;
	Player* _PlayerMap[dfSESSION_MAX];
	Sector _sectors[dfSECTOR_CNT_Y][dfSECTOR_CNT_X];
	int _sectorCnt[8] = { 3, 5, 3, 5, 3, 5, 3, 5 };

private:
	CObjectPool<Session>* _pSessionPool;
	CObjectPool<Player>* _pPlayerPool;
	CObjectPool<SerializePacket>* _pSPacketPool;

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
	inline bool GetCSPacket_MOVE_START(SerializePacket* pPacket, RingBuffer* recvBuffer, BYTE& moveDirection, short& x, short& y);
	inline bool GetCSPacket_MOVE_STOP(SerializePacket* pPacket, RingBuffer* recvBuffer, BYTE& direction, short& x, short& y);
	inline bool GetCSPacket_ATTACK1(SerializePacket* pPacket, RingBuffer* recvBuffer, BYTE& direction, short& x, short& y);
	inline bool GetCSPacket_ATTACK2(SerializePacket* pPacket, RingBuffer* recvBuffer, BYTE& direction, short& x, short& y);
	inline bool GetCSPacket_ATTACK3(SerializePacket* pPacket, RingBuffer* recvBuffer, BYTE& direction, short& x, short& y);
	inline bool GetCSPacket_ECHO(SerializePacket* pPacket, RingBuffer* recvBuffer, int& time);

	// Set Game Data from Packet Data
	inline void SetPlayerMoveStart(Player* pPlayer, BYTE& moveDirection, short& x, short& y);
	inline void SetPlayerMoveStop(Player* pPlayer, BYTE& direction, short& x, short& y);
	inline void SetPlayerAttack1(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y);
	inline void SetPlayerAttack2(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y);
	inline void SetPlayerAttack3(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y);
	
	// Set Data on SC Packet
	inline void SetSCPacket_HEADER(SerializePacket* pPacket, BYTE size, BYTE type);
	inline int SetSCPacket_CREATE_MY_CHAR(SerializePacket* pPacket, int ID, BYTE direction, short x, short y, BYTE hp);
	inline int SetSCPacket_CREATE_OTHER_CHAR(SerializePacket* pPacket, int ID, BYTE direction, short x, short y, BYTE hp);
	inline int SetSCPacket_DELETE_CHAR(SerializePacket* pPacket, int ID);
	inline int SetSCPacket_MOVE_START(SerializePacket* pPacket, int ID, BYTE moveDirection, short x, short y);
	inline int SetSCPacket_MOVE_STOP(SerializePacket* pPacket, int ID, BYTE direction, short x, short y);
	inline int SetSCPacket_ATTACK1(SerializePacket* pPacket, int ID, BYTE direction, short x, short y);
	inline int SetSCPacket_ATTACK2(SerializePacket* pPacket, int ID, BYTE direction, short x, short y);
	inline int SetSCPacket_ATTACK3(SerializePacket* pPacket, int ID, BYTE direction, short x, short y);
	inline int SetSCPacket_DAMAGE(SerializePacket* pPacket, int attackID, int damageID, BYTE damageHP);
	inline int SetSCPacket_SYNC(SerializePacket* pPacket, int ID, short x, short y);
	inline int SetSCPacket_ECHO(SerializePacket* pPacket, int time);

};

