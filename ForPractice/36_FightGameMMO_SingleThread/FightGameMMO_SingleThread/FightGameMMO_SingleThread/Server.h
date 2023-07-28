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
	void Update();
	static Server* GetInstance();

private:
	static Server _server;

// About Network ==================================================
private:
	class Session
	{
	public:
		Session(int ID) { _ID = ID; }

	public:
		void SetSessionAlive() { _alive = true; }
		void SetSessionDead() { _alive = false; }
		bool GetSessionAlive() { return _alive; }

	private:
		bool _alive = false;
		int _ID;

	public:
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
	void EnqueueUnicast(char* msg, int size, Session* pSession);
	void EnqueueBroadcast(char* msg, int size, Session* pExpSession = nullptr);
	void EnqueueBroadcastInSector(char* msg, int size, Session* pExpSession = nullptr);

private:
	void DisconnectDeadSession();

private:
	int _sessionID = 0;
	SOCKET _listensock;
	Session* _sessionArray[FD_SETSIZE];
	timeval _time;
	vector<Session*> _allSessions;

// About Content ====================================================
private:
	class Player
	{
	public:
		Player(Session* pSession, int ID)
			: _pSession(pSession), _alive(false), _ID(ID), _direction(dfPACKET_MOVE_DIR_LL),
			_moveDirection(dfPACKET_MOVE_DIR_LL), _hp(dfMAX_HP)
		{
			srand(ID);
			_x = rand() % dfRANGE_MOVE_RIGHT;
			_y = rand() % dfRANGE_MOVE_BOTTOM;
		}

	public:
		void SetPlayerAlive() { _alive = true; }
		void SetPlayerDead() { _alive = false; }
		bool GetPlayerAlive() { return _alive; }

	public:
		Session* _pSession;

	public:
		bool _alive;
		int _ID;
		BYTE _direction;
		BYTE _moveDirection;
		unsigned short _x;
		unsigned short _y;
		BYTE _hp;

	};

#define dfDEFAULT_PLAYERS_PER_SECTOR 100

	class Sector
	{
	public:
		Sector()
		{
			_playerArray.reserve(dfDEFAULT_PLAYERS_PER_SECTOR);
		}

	public:
		vector<Player*> _playerArray;
	};

private:
	void CreatePlayer(Session* pSession);
	void DeletePlayer(Player* pPlayer);

private:
	int _assignID = 0;
	unordered_map<Session*, Player*> _SessionPlayerMap;

// About Packet ====================================================
private:
	// Handle CS Packet
	bool HandleCSPackets(Player* pPlayer, WORD type);
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

