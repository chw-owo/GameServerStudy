#pragma once
#include <vector>
#include <unordered_map>
#include <stdio.h>
using namespace std;

#include "Session.h"
#include "User.h"
#include "Room.h"

#define IP L"0.0.0.0"
#define dfDEFAULT_SESSIONS_MAX 1000

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

	
	// About Network	
private:
	void SelectProc(FD_SET rset, FD_SET wset, int max);
	void AcceptProc();
	void RecvProc(Session* pSession);
	void SendProc(Session* pSession);
	void DisconnectDeadSession();

private:
	void EnqueueUnicast(char* msg, int size, Session* pSession);
	void EnqueueBroadcast(char* msg, int size, Session* pExpSession = nullptr);

private:
	int _sessionID = 0;
	SOCKET _listensock;
	vector<Session*> _allSessions;
	Session* _sessionArray[FD_SETSIZE];

	// About Content
private:
	void CreateUser(Session* pSession);
	void DestroyDeadUser();
	void CreateRoom();
	void DestroyRoom(Room* pRoom);

private:
	int _userID = 0;
	int _roomID = 0;
	unordered_map<Session*, User*> _SessionUserMap;
};

