#pragma once
#include "SerializePacket.h"
#include "RingBuffer.h"
#include "ObjectPool.h"
#include "Profiler.h"
#include "Protocol.h"

#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <vector>
#include <unordered_map>
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
	inline void Monitor()
	{
		if (timeGetTime() - _oldTick > 1000)
		{
			printf("[%s %s]\n", __DATE__, __TIME__);

			//printf("Session Count: %llu\n", _allSessions.size());
			//printf("Player Count: %llu\n", _allPlayers.size());

			PRO_PRINT_CONSOLE();
			PRO_RESET();

			_oldTick += 1000;
		}
	}

	inline void Control()
	{

	}

private:
	static Server _server;

// About Monitor ==================================================
private:
	DWORD _oldTick;

// About Network ==================================================
private:
	class Session
	{
	public:
		Session(int ID) { _ID = ID; }
		Session() { printf("Error! Session() is called\n"); }

	public:
		bool _alive = false;
		int _ID;
		SOCKET _socket;
		SOCKADDR_IN _addr;
		RingBuffer _recvBuf;
		RingBuffer _sendBuf;
		DWORD _lastRecvTime;
	};

private:
	void SelectProc(FD_SET rset, FD_SET wset, int max);
	void AcceptProc();
	void RecvProc(Session* pSession);
	void SendProc(Session* pSession);

private:
	class Sector;
	void EnqueueUnicast(char* msg, int size, Session* pSession);
	void EnqueueOneSector(char* msg, int size, Sector* sector, Session* pExpSession = nullptr);
	void EnqueueAroundSector(char* msg, int size, Sector* centerSector, Session* pExpSession = nullptr);

private:
	void SetAcceptedSession();
	void SetSessionDead(Session* pSession);
	void DisconnectDeadSession();

private:
	timeval _time;
	int _sessionID = 0;
	SOCKET _listensock = INVALID_SOCKET;
	Session* _sessionArray[FD_SETSIZE];
	vector<Session*> _allSessions;
	vector<Session*> _acceptedSessions;
	CObjectPool<Session>* _pSessionPool;

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
	};

private:
	class Player
	{
	public:
		Player(Session* pSession, int ID)
			: _pSession(pSession), _sessionID(pSession->_ID), _connected(true),
			_ID(ID), _alive(true), _hp(dfMAX_HP), _move(false), _pSector(nullptr),
			_direction(dfPACKET_MOVE_DIR_LL), _moveDirection(dfPACKET_MOVE_DIR_LL)
		{
			_x = rand() % dfRANGE_MOVE_RIGHT;
			_y = rand() % dfRANGE_MOVE_BOTTOM;
		}

		Player() { printf("Error! Player() is called\n"); }

	public:
		Session* _pSession;
		int _sessionID;
		bool _connected;

	public:
		int _ID;
		bool _alive;
		bool _move;
		signed char _hp;
		Sector* _pSector;

		short _x;
		short _y;
		BYTE _direction;
		BYTE _moveDirection;
	};

private:
	int _playerID = 0;
	vector<Player*> _allPlayers;
	unordered_map<int, Player*> _SessionIDPlayerMap;
	CObjectPool<Player>* _pPlayerPool;
	Sector _sectors[dfSECTOR_CNT_Y][dfSECTOR_CNT_X];
	
private:
	void CreatePlayer(Session* pSession);
	void DestroyDeadPlayers();
	inline void SetPlayerDead(Player* pPlayer);

private:
	void UpdatePlayerMove(Player* pPlayer);
	inline bool SkipForFixedFrame();
	inline bool CheckMovable(short x, short y);

private:
	void UpdateSector(Player* pPlayer, Sector* inSector, Sector* outSector, 
		int inSectorNum, int outSectorNum, Sector* newSector);
	inline void SetSector(Player* pPlayer);
	inline void GetAroundSector(Sector* centerSector, Sector* aroundSector);
	

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

