#pragma once
#include "CNetServer.h"
#include "CMonitorData.h"
#include "mysql.h"
#pragma comment (lib, "libmysql.lib")

#include <vector>
using namespace std;

class CNetMonitorServer : public CNetServer
{
public:
	CNetMonitorServer() {};
	~CNetMonitorServer() { Terminate(); };

public:
	bool Initialize();
	void Terminate();

private:
	void OnInitialize();
	void OnTerminate();
	void OnThreadTerminate(wchar_t* threadName);
	void OnError(int errorCode, wchar_t* errorMsg);
	void OnDebug(int debugCode, wchar_t* debugMsg);

private:
	bool OnConnectRequest();
	void OnAcceptClient(unsigned __int64 sessionID);
	void OnReleaseClient(unsigned __int64 sessionID);
	void OnRecv(unsigned __int64 sessionID, CRecvNetPacket* packet);
	void OnSend(unsigned __int64 sessionID, int sendSize);

private:
	void HandleAccept(unsigned __int64 sessionID);
	void HandleRelease(unsigned __int64 sessionID);
	void HandleRecv(unsigned __int64 sessionID, CRecvNetPacket* packet);

private:
	static unsigned int WINAPI JobThread(void* arg);
	static unsigned int WINAPI SendThread(void* arg);
	static unsigned int WINAPI SaveThread(void* arg);

private: // Called in SendThread
	void SleepForSend();
	void Handle_REQ_LOGIN(unsigned __int64 sessionID, CRecvNetPacket* packet);
	void SendDatasToClient(unsigned __int64 sessionID);
	void SendDataToClient(unsigned __int64 sessionID, BYTE type, BYTE serverNo, CData* data);
	void ReqSendUnicast(unsigned __int64 sessionID, CNetPacket* packet);

private: // Called in SaveThread
	void SleepForSave();
	void InitializeDB();
	void SaveDatasToDB();
	void SaveDataToDB(SYSTEMTIME stTime, BYTE type, BYTE serverNo, CData* data);

private:
	HANDLE _jobThread;
	bool _serverAlive = true;
	long _signal = 0;

private:
	HANDLE _sendThread;
	DWORD _sendOldTick;
	long _sendTimeGap = 1000;
	vector<unsigned __int64> _sessions;

private:
#define dfTIME_LEN 32
#define dfTABLE_LEN 32
#define dfQUERY_MAX 512
	HANDLE _logThread;
	MYSQL _conn;
	MYSQL* _connection;
	DWORD _saveOldTick;
	long _saveTimeGap = 300000;

};

