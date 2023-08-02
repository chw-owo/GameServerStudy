#pragma once
#include "Protocol.h"
#include "RingBuffer.h"
#include "SerializePacket.h"

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
	void Control();
	void Monitor();

private:
	static Server _server;

// About Network ==================================================
private:
	class Session
	{
	public:
		Session(int ID) { _ID = ID; }

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
	int _sessionID = 0;
	SOCKET _listensock = INVALID_SOCKET;
	Session* _sessionArray[FD_SETSIZE];
	timeval _time;
	vector<Session*> _allSessions;
	vector<Session*> _acceptedSessions;

// About Content ====================================================
private:
	class Player;
	class Sector
	{
	public:
		void InitializeSector(short xIndex, short yIndex);

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
			srand(ID);
			_x = rand() % dfRANGE_MOVE_RIGHT;
			_y = rand() % dfRANGE_MOVE_BOTTOM;
		}

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
	Sector _sectors[dfSECTOR_CNT_Y][dfSECTOR_CNT_X];
	unordered_map<int, Player*> _SessionIDPlayerMap;

private:
	bool SkipForFixedFrame();
	bool CheckMovable(short x, short y);
	void UpdatePlayerMove(Player* pPlayer);

private:
	void SetSector(Player* pPlayer);
	void GetAroundSector(Sector* centerSector, Sector* aroundSector);
	void UpdateSector(Player* pPlayer, Sector* inSector, Sector* outSector, 
		int inSectorNum, int outSectorNum, Sector* newSector);
	
private:
	void CreatePlayer(Session* pSession);
	void SetPlayerDead(Player* pPlayer);
	void DestroyDeadPlayers();

// About Packet ====================================================
private:
	// Handle CS Packet
	bool HandleCSPackets(Player* pPlayer, BYTE type);
	bool HandleCSPacket_MOVE_START(Player* pPlayer);
	bool HandleCSPacket_MOVE_STOP(Player* pPlayer);
	bool HandleCSPacket_ATTACK1(Player* pPlayer);
	bool HandleCSPacket_ATTACK2(Player* pPlayer);
	bool HandleCSPacket_ATTACK3(Player* pPlayer);
	bool HandleCSPacket_ECHO(Player* pPlayer);

	// Get Data from CS Packet
	bool GetCSPacket_MOVE_START(RingBuffer* recvBuffer, BYTE& moveDirection, short& x, short& y);
	bool GetCSPacket_MOVE_STOP(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y);
	bool GetCSPacket_ATTACK1(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y);
	bool GetCSPacket_ATTACK2(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y);
	bool GetCSPacket_ATTACK3(RingBuffer* recvBuffer, BYTE& direction, short& x, short& y);
	bool GetCSPacket_ECHO(RingBuffer* recvBuffer, int& time);

	// Set Game Data from Packet Data
	void SetPlayerMoveStart(Player* pPlayer, BYTE& moveDirection, short& x, short& y);
	void SetPlayerMoveStop(Player* pPlayer, BYTE& direction, short& x, short& y);
	void SetPlayerAttack1(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y);
	void SetPlayerAttack2(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y);
	void SetPlayerAttack3(Player* pPlayer, Player*& pDamagedPlayer, BYTE& direction, short& x, short& y);
	
	// Set Data on SC Packet
	void SetSCPacket_HEADER(SerializePacket* buffer, BYTE size, BYTE type);
	int SetSCPacket_CREATE_MY_CHAR(SerializePacket* buffer, int ID, BYTE direction, short x, short y, BYTE hp);
	int SetSCPacket_CREATE_OTHER_CHAR(SerializePacket* buffer, int ID, BYTE direction, short x, short y, BYTE hp);
	int SetSCPacket_DELETE_CHAR(SerializePacket* buffer, int ID);
	int SetSCPacket_MOVE_START(SerializePacket* buffer, int ID, BYTE moveDirection, short x, short y);
	int SetSCPacket_MOVE_STOP(SerializePacket* buffer, int ID, BYTE direction, short x, short y);
	int SetSCPacket_ATTACK1(SerializePacket* buffer, int ID, BYTE direction, short x, short y);
	int SetSCPacket_ATTACK2(SerializePacket* buffer, int ID, BYTE direction, short x, short y);
	int SetSCPacket_ATTACK3(SerializePacket* buffer, int ID, BYTE direction, short x, short y);
	int SetSCPacket_DAMAGE(SerializePacket* buffer, int attackID, int damageID, BYTE damageHP);
	int SetSCPacket_SYNC(SerializePacket* buffer, int ID, short x, short y);
	int SetSCPacket_ECHO(SerializePacket* buffer, int time);


};

