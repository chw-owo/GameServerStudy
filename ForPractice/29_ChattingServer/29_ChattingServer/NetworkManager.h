#pragma once
#pragma comment(lib, "ws2_32")
#include <ws2tcpip.h>
#include <vector>
using namespace std;

class Session;
class ContentManager;
class NetworkManager
{
private:
	NetworkManager();
	~NetworkManager();

public:
	static NetworkManager* GetInstance();
	void Update();

private:
	void SelectProc(FD_SET rset, FD_SET wset, int max);
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
	SOCKET _listensock;
	vector<Session*> _allSessions;
	Session* _sessionArray[FD_SETSIZE]; 

private:
	int _sessionID = 0;

private:
	ContentManager* _pContentManager = nullptr;
};
