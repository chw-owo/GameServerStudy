#pragma once

#include "List.h"
#include "RingBuffer.h"

#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <stdio.h>

class IStub;
class Proxy;
class NetworkManager;

// ID = code(2byte, 0x0089) + IP(4byte) + port(2byte);
class Session
{
	friend IStub;
	friend Proxy;
	friend NetworkManager;

public:
	int GetSessionID() { return _ID; }
	bool GetSessionAlive() { return _alive; }
	void SetSessionDead() { _alive = false; }

private:
	void SetSessionAlive() { _alive = true; }

private:
	bool _alive;
	unsigned __int64 _ID; 
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
	int GetNewSessionCnt() { return _newSessionList.size();  }
	CList<Session*>* GetNewSessionList() { return &_newSessionList; }
	void SetNewSessionsToUsed() 
	{ 
		while (!_newSessionList.empty())
		{
			Session* pSession = _newSessionList.pop_back();
			_sessionList.push_back(pSession);
		}
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
	CList<Session*> _newSessionList;
};