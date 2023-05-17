#pragma once

#include "List.h"
#include "Typedef.h"
#include "RingBuffer.h"

#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>

class Player;
class PlayerManager;

class Session;
class NetworkManager;

class Session
{
	friend NetworkManager;
	friend PlayerManager;
	friend Player;

private:
	void SetSessionAlive() { _alive = true; }
	void SetSessionDead() { _alive = false; }
	bool GetSessionAlive() { return _alive; }

private:
	bool _alive;
	SOCKET _sock;
	RingBuffer _recvBuf;
	RingBuffer _sendBuf;
};

class NetworkManager
{
private:
	NetworkManager();
	~NetworkManager();

public:
	static NetworkManager* GetInstance();

public:
	void Initialize();
	void Update();
	void Terminate();

public: 
	void EnqueueUnicast(char* msg, int size, Session* pSession);
	void EnqueueBroadcast(char* msg, int size, Session* pExpSession = nullptr);

private:
	void AcceptProc();
	void RecvProc(Session* session);
	void SendProc(Session* session);
	void DisconnectDeadSessions();

private:
	static NetworkManager _networkMgr;
	FD_SET _rset;
	FD_SET _wset;
	SOCKET _listensock;
	CList<Session*> _sessionList;

private:
	PlayerManager* _pPlayerManager = nullptr;
};
