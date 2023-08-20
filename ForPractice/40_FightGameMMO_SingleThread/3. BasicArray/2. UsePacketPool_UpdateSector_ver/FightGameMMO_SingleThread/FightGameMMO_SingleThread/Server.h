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
private:
	Server();
	~Server();

public:
	static Server* GetInstance();

public:
	void NetworkUpdate();
	void ContentUpdate();

private:
	static Server _server;

// Monitor and Control ==================================================
private:
	DWORD _oldTick;

	int _syncCnt = 0;
	int _acceptCnt = 0;
	int _disconnectCnt = 0;
	int _deadCnt = 0;
	int _timeoutCnt = 0;
	int _ConnResetCnt = 0;

#define checkPointsMax 11
	int _checkPointsIdx = 0;
	int _checkPoints[checkPointsMax]
		= { 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000 };

public:
#define dfPRO_TITLE_LEN 64
	inline void Monitor()
	{
		if (timeGetTime() - _oldTick > 1000)
		{
			int connected = dfSESSION_MAX - _emptySessionID.size();

			printf("[%s %s]\n\n", __DATE__, __TIME__);
			printf("Connected Session: %d\n", connected);
			printf("Sync/1sec: %d\n", _syncCnt);
			printf("Accept/1sec: %d\n", _acceptCnt);
			printf("Disconnect/1sec: %d\n", _disconnectCnt);
			printf(" - Dead: %d\n", _deadCnt);
			printf(" - Timeout: %d\n", _timeoutCnt);
			printf(" - ConnReset(10054): %d\n", _ConnResetCnt); 

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
			_disconnectCnt = 0;
			_deadCnt = 0;
			_timeoutCnt = 0;
			_ConnResetCnt = 0;

			_oldTick += 1000;
		}
	}

	inline void Control() {}

// About Network ==================================================
private:
	class Session
	{
	public:
		Session(int ID) { _ID = ID; }
		Session() 
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
		DWORD _lastRecvTime;
	};

private:
	void SelectProc(int rStartIdx, int rCount, int wStartIdx, int wCount);
	void AcceptProc();
	void RecvProc(Session* pSession);
	void SendProc(Session* pSession);

private:
	class Sector;
	void EnqueueUnicast(char* msg, int size, Session* pSession);
	void EnqueueOneSector(char* msg, int size, Sector* sector, Session* pExpSession = nullptr);
	void EnqueueAroundSector(char* msg, int size, Sector* centerSector, Session* pExpSession = nullptr);

private:
	void SetSessionDead(Session* pSession);
	void DisconnectDeadSession();

private:
	timeval _time;
	SOCKET _listensock = INVALID_SOCKET;
	CObjectPool<Session>* _pSessionPool;
	Session* _SessionMap[dfSESSION_MAX];

	Session* _rSessions[dfSESSION_MAX];
	Session* _wSessions[dfSESSION_MAX];
	vector<int> _disconnectedSessionIDs;
	vector<int> _emptySessionID;
	
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
		Player(Session* pSession, int ID)
			: _pSession(pSession), _pSector(nullptr), _ID(ID), _hp(dfMAX_HP), 
			_move(false), _direction(dfPACKET_MOVE_DIR_LL), _moveDirection(dfPACKET_MOVE_DIR_LL)
		{
			_x = rand() % dfRANGE_MOVE_RIGHT;
			_y = rand() % dfRANGE_MOVE_BOTTOM;
		}

		Player() 
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
	int _playerID = 0;
	CObjectPool<Player>* _pPlayerPool;
	CObjectPool<SerializePacket>* _pSPacketPool;
	Player* _PlayerMap[dfSESSION_MAX];
	Sector _sectors[dfSECTOR_CNT_Y][dfSECTOR_CNT_X];
	int _sectorCnt[8] = { 3, 5, 3, 5, 3, 5, 3, 5 };

private:
	void CreatePlayer(Session* pSession);

private:
	void UpdatePlayerMove(Player* pPlayer);
	inline bool SkipForFixedFrame();
	inline bool CheckMovable(short x, short y);

private:
	void UpdateSector(Player* pPlayer, short Direction);
	inline void SetSectorsAroundInfo();
	inline void SetSector(Player* pPlayer);
	

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
	inline bool GetCSPacket_MOVE_START(RingBuffer* recvBuffer, BYTE& moveDirection, short& x, short& y);
	inline bool GetCSPacket_MOVE_STOP(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y);
	inline bool GetCSPacket_ATTACK1(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y);
	inline bool GetCSPacket_ATTACK2(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y);
	inline bool GetCSPacket_ATTACK3(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y);
	inline bool GetCSPacket_ECHO(RingBuffer* recvBuffer, int& time);

	// Set Game Data from Packet Data
	inline void SetPlayerMoveStart(Player* pPlayer, BYTE& moveDirection, short& x, short& y);
	inline void SetPlayerMoveStop(Player* pPlayer, BYTE& direction, short& x, short& y);
	inline void SetPlayerAttack1(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y);
	inline void SetPlayerAttack2(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y);
	inline void SetPlayerAttack3(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y);
	
	// Set Data on SC Packet
	inline void SetSCPacket_HEADER(SerializePacket* buffer, BYTE size, BYTE type);
	inline int SetSCPacket_CREATE_MY_CHAR(SerializePacket* buffer, int ID, BYTE direction, short x, short y, BYTE hp);
	inline int SetSCPacket_CREATE_OTHER_CHAR(SerializePacket* buffer, int ID, BYTE direction, short x, short y, BYTE hp);
	inline int SetSCPacket_DELETE_CHAR(SerializePacket* buffer, int ID);
	inline int SetSCPacket_MOVE_START(SerializePacket* buffer, int ID, BYTE moveDirection, short x, short y);
	inline int SetSCPacket_MOVE_STOP(SerializePacket* buffer, int ID, BYTE direction, short x, short y);
	inline int SetSCPacket_ATTACK1(SerializePacket* buffer, int ID, BYTE direction, short x, short y);
	inline int SetSCPacket_ATTACK2(SerializePacket* buffer, int ID, BYTE direction, short x, short y);
	inline int SetSCPacket_ATTACK3(SerializePacket* buffer, int ID, BYTE direction, short x, short y);
	inline int SetSCPacket_DAMAGE(SerializePacket* buffer, int attackID, int damageID, BYTE damageHP);
	inline int SetSCPacket_SYNC(SerializePacket* buffer, int ID, short x, short y);
	inline int SetSCPacket_ECHO(SerializePacket* buffer, int time);

};

