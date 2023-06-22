#pragma once

#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <vector>
//#include <unordered_map>
#include "SerializePacket.h"
#include "RingBuffer.h"
using namespace std;

class Session
{
public:
	Session(int ID) { _ID = ID; }

public:
	void SetSessionAlive() { _alive = true; }
	void SetSessionDead() { _alive = false; }
	bool GetSessionAlive() { return _alive; }

private:
	bool _alive = true;
	int _ID;

public:
	SOCKET _socket;
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
	void Update();

private:
	void AcceptProc();
	void RecvProc(Session* pSession);
	void SendProc(Session* pSession);
	void DisconnectSession();

private:
	void EnqueueUnicast(char* msg, int size, Session* pSession);
	void EnqueueBroadcast(char* msg, int size, Session* pExpSession = nullptr);	

private:
	static NetworkManager _networkManager;

private:
	FD_SET _rset;
	FD_SET _wset;
	SOCKET _listensock;
	vector<Session*> _sessionArray;
	int _sessionID = 0;
	//unordered_map<int, Session> _sessionMap;
};
