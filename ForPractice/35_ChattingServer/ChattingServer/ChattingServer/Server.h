#pragma once
#include <vector>
#include <unordered_map>
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
	void DestroyDeadUser();

private:
	bool Handle_RecvPacket(Session* pSession, User* pUser, WORD type);
	bool Handle_REQ_LOGIN(Session* pSession, User* pUser, wchar_t*& name);
	bool Handle_REQ_ROOM_LIST(User* pUser);
	bool Handle_REQ_ROOM_CREATE(User* pUser, short& titleLen, wchar_t*& title);
	bool Handle_REQ_ROOM_ENTER(User* pUser, int& roomID);
	bool Handle_REQ_CHAT(User* pUser, short& msgLen, wchar_t*& msg);
	bool Handle_REQ_ROOM_LEAVE(User* pUser);

private:
	BYTE CreateUser(Session* pSession, User*& pUser, wchar_t* name);
	BYTE CreateRoom(short& titleLen, wchar_t* title, int& roomID);
	BYTE EnterRoom(User* pUser, int roomID, Room*& pRoom);
	bool LeaveRoom(User* pUser);

private:
	int _userID = 0;
	int _roomID = 0;
	unordered_map<Session*, User*> _SessionUserMap;
};

