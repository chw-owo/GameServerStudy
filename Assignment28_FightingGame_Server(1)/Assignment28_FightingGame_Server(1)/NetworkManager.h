#pragma once
#include "RingBuffer.h"
#include "Typedef.h"
#include "List.h"

#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>

class Player;
class PlayerManager;

class Session;
class NetworkManager;

class Session
{
	friend NetworkManager;
	friend Player;

private:
	bool _disconnect;
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
	bool Initialize();
	bool Update();
	void Terminate();

	void AcceptProc();
	void RecvProc(Session* session);
	void SendProc(Session* session);
	void DisconnectDeadSessions();

	void EnqueueUnicast(char* msg, int size, Session* pSession);
	void EnqueueBroadcast(char* msg, int size, Session* pExpSession = nullptr);

private:
	static NetworkManager _networkMgr;
	FD_SET _rset;
	FD_SET _wset;
	SOCKET _listensock;
	CList<Session*> _sessionList;

private:
	PlayerManager* _pPlayerManager = nullptr;
};
