#pragma once
#include "CSession.h"
#include "CSessionMap.h"
#include "CObjectPool.h"
#include "CLockFreeQueue.h"
#include "CProfiler.h"
#include "Protocol.h"

#include <ws2tcpip.h>
#include <process.h>
#include <unordered_map>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#ifdef USE_STLS
extern __declspec (thread) CProfiler* pSTLSProfiler;
#endif

class CLanServer
{
	// Called in Content
protected:
	bool NetworkStart(const wchar_t* IP, short port,
		int numOfWorkerThreads, bool nagle, int sessionMax);
public:
	void NetworkTerminate();

protected:
	bool Disconnect(__int64 sessionID);
	bool SendPacket(__int64 sessionID, CPacket* packet);

protected:
	virtual bool OnConnectRequest() = 0;
	virtual void OnAcceptClient(__int64 sessionID) = 0;
	virtual void OnReleaseClient(__int64 sessionID) = 0;
	virtual void OnRecv(__int64 sessionID, CPacket* packet) = 0;
	virtual void OnSend(__int64 sessionID, int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;

	virtual void ForDebug(__int64 sessionID) = 0;

protected:
	void UpdateMonitorData();
	inline int GetAcceptTotal() { return _acceptTotal; }
	inline int GetDisconnectTotal() { return _disconnectTotal; }
	inline long GetSessionCount() { return _sessionMap->_map.size(); }

	inline int GetAcceptTPS() { return _acceptTPS; }
	inline int GetDisconnectTPS() { return _disconnectTPS; }
	inline int GetRecvMsgTPS() { return _recvMsgTPS; }
	inline int GetSendMsgTPS() { return _sendMsgTPS; }
	inline int GetClientDisconnTPS() { return _clientDisconnTPS; }
	inline int GetServerDisconnTPS() { return _serverDisconnTPS; }

protected:
	CObjectPool<CPacket>* _pPacketPool;

	// Called in Network Library
private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);
	void ReleaseSession(CLanServer* pLanServer, __int64 sessionID);

private:
	void HandleRecvCP(__int64 sessionID, int recvBytes);
	void HandleSendCP(__int64 sessionID, int sendBytes);
	void RecvPost(CSession* pSession);
	void SendPost(CSession* pSession);

private:
	wchar_t _IP[10];
	short _port;
	bool _nagle;
	int _sessionMax;
	int _numOfWorkerThreads;

private:
	bool _alive = false;
	__int64 _IDGenerator = 0;
	SOCKET _listenSock;
	CSessionMap* _sessionMap;
	//CObjectPool<CSession>* _pSessionPool;

private:
	int _acceptTotal = 0;
	int _disconnectTotal = 0;

	int _acceptTPS = 0;
	int _disconnectTPS = 0;
	int _recvMsgTPS = 0;
	int _sendMsgTPS = 0;
	int  _clientDisconnTPS = 0;
	int  _serverDisconnTPS = 0;

	int _acceptCnt = 0;
	int _disconnectCnt = 0;
	int _recvMsgCnt = 0;
	int _sendMsgCnt = 0;
	int _clientDisconnCnt = 0;
	int _serverDisconnCnt = 0;

private:
	HANDLE _acceptThread;
	HANDLE* _networkThreads;
	HANDLE _hNetworkCP;
};
