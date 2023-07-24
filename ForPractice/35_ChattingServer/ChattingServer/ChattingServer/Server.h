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
	timeval _time;

	// About Content
private:
	void CreateUser(Session* pSession);
	void DestroyDeadUser();
	void CreateRoom(wchar_t* title);
	void EnterRoom(User* user, int roomID);
	void LeaveRoom(User* user);

private:
	void Handle_RecvPacket(User* user, WORD type);
	LOGIN_RESULT Handle_REQ_LOGIN(User* user);
	RLIST_RESULT Handle_REQ_ROOM_LIST(User* user);
	RCREATE_RESULT Handle_REQ_ROOM_CREATE(User* user, wchar_t* title);
	RENTER_RESULT Handle_REQ_ROOM_ENTER(User* user);
	CHAT_RESULT Handle_REQ_CHAT(User* user);
	RLEAVE_RESULT Handle_REQ_ROOM_LEAVE(User* user);

private:
	int _userID = 0;
	int _roomID = 0;
	unordered_map<Session*, User*> _SessionUserMap;
};

