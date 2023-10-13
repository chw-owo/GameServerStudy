#pragma once
#include "CSession.h"
#include "CLockFreePool.h"
#include "Protocol.h"

#include <ws2tcpip.h>
#include <process.h>
#include <unordered_map>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

class CNetServer
{
protected:
	bool NetworkStart(const wchar_t* IP, short port, int numOfThreads, bool nagle, int sessionMax);
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

protected:
	void UpdateMonitorData();

	inline int GetAcceptTotal() { return _acceptTotal; }
	inline int GetDisconnectTotal() { return _disconnectTotal; }
	inline long GetSessionCount() { return _sessionCnt; }

	inline int GetAcceptTPS() { return _acceptTPS; }
	inline int GetDisconnectTPS() { return _disconnectTPS; }
	inline int GetRecvMsgTPS() { return _recvMsgTPS; }
	inline int GetSendMsgTPS() { return _sendMsgTPS; }
	inline int GetClientDisconnTPS() { return _clientDisconnTPS; }
	inline int GetServerDisconnTPS() { return _serverDisconnTPS; }

protected:
	CLockFreePool<CPacket>* _pPacketPool;

	// Called in Network Library
private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);
	void ReleaseSession(CNetServer* pLanServer, __int64 sessionID);

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
	int _numOfThreads;

private:
	SOCKET _listenSock;
	bool _serverAlive = false;

private:
	HANDLE _acceptThread;
	HANDLE* _networkThreads;
	HANDLE _hNetworkCP;

private:
	CSession* _sessions[dfSESSION_MAX] = { nullptr, };
	__int64 _sessionID = 0;
	int _sessionCnt = 0;

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
};
