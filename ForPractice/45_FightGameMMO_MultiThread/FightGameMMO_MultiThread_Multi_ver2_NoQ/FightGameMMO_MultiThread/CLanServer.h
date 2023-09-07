#pragma once
#include "CSession.h"
#include "CSessionMap.h"
#include "CObjectPool.h"
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
	inline int GetAcceptTPS() { return _acceptTPS; }
	inline int GetDisconnectTPS() { return _disconnectTPS; }
	inline int GetRecvMsgTPS() { return _recvMsgTPS; }
	inline int GetSendMsgTPS() { return _sendMsgTPS; }
	inline long GetSessionCount() { return _sessionMap->_map.size(); }

protected:
	CObjectPool<CPacket>* _pPacketPool;

	// Called in Network Library
private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);
	void ReleaseSession(__int64 sessionID);

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

	int _acceptCnt = 0;
	int _disconnectCnt = 0;
	int _recvMsgCnt = 0;
	int _sendMsgCnt = 0;

	// For Debug
protected:
	inline int Get10054TPS() { return _10054TPS; }

private:
	int _10054TPS = 0;
	int _10054Cnt = 0;

protected:
	int _NoSendIOCP = 0;

private:
	// For Debug
	class ThreadArg
	{
	public:
		ThreadArg(CLanServer* pServer, int num)
			:_pServer(pServer), _num(num) {};
	public:
		CLanServer* _pServer;
		int _num;
	};

protected:
	// For Debug
	bool* _activeNetworkThreads;
	int _activeThreadNum;
	int _numOfWorkerThreads;

private:
	HANDLE _acceptThread;
	HANDLE* _networkThreads;
	HANDLE _hNetworkCP;
};
