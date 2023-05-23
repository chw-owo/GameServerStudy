#pragma once

#include "List.h"
#include "RingBuffer.h"

#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <stdio.h>

class Proxy;
class IStub;
class NetworkManager;
class PlayerManager;

extern int gSessionID; // TO-DO
class Session
{
	friend IStub;
	friend NetworkManager;
	friend PlayerManager; // TO-DO... 없애야 함

public:
	Session() 
	{
		_ID = gSessionID;
		gSessionID++;
	}
	~Session() { }

public:
	int GetSessionID() { return _ID; }

private:
	void SetSessionAlive() { _alive = true; }
	void SetSessionDead() { _alive = false; }
	bool GetSessionAlive() { return _alive; }

private:
	bool _alive;
	int _ID;
	SOCKET _sock;
	RingBuffer _recvBuf;
	RingBuffer _sendBuf;
};


class NetworkManager
{
	friend Proxy;

private:
	NetworkManager();
	~NetworkManager();

public:
	static NetworkManager* GetInstance();
	void AttachStub(IStub* stub) 
	{ 
		_pStub = stub; 
		printf("Attach Stub Success!\n");
	}

public:
	void Initialize();
	void Update();
	void Terminate();

private:
	void EnqueueUnicast(char* msg, int size, Session* pSession);
	void EnqueueMulticast(char* msg, int size, Session* pSessionList, int SessionNum);
	void EnqueueBroadcast(char* msg, int size, Session* pExpSession = nullptr);

private:
	void AcceptProc();
	void RecvProc(Session* session);
	void SendProc(Session* session);
	void DisconnectDeadSessions();

private:
	static NetworkManager _networkMgr;
	IStub* _pStub = nullptr;

	FD_SET _rset;
	FD_SET _wset;
	SOCKET _listensock;
	CList<Session*> _sessionList;
	
private:
	PlayerManager* _pPlayerManager = nullptr; // TO-DO: 분리해야 됨
};
