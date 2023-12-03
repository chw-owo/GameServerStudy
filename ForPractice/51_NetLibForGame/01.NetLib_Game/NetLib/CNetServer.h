#pragma once
#include "Config.h"
#include "ErrorCode.h"
#include <ws2tcpip.h>
#include <process.h>
#include <unordered_map>
#pragma comment(lib, "ws2_32.lib")

class CPacket;
class CSession;
template<typename T>
class CLockFreeStack {};

class CGroup;
class CNetServer
{
	friend CGroup;

	// Use in Content Class ================================================

protected:
	CNetServer() {};
	~CNetServer() {};
	bool NetworkInitialize(const wchar_t* IP, short port, int numOfThreads, bool nagle);
	bool NetworkTerminate();

protected:
	bool RegisterGroup(CGroup* pGroup);
	bool RemoveGroup(CGroup* pGroup);
	bool MoveSession(CGroup* pGroup, __int64 sessionID);
	bool MoveSession(CGroup* pGroup, CSession* pSession);

private:
	unordered_map<CGroup*, HANDLE> _GroupThreads;

protected:
	bool Disconnect(__int64 sessionID);
	bool SendPacket(__int64 sessionID, CPacket* packet);
	bool Disconnect(CSession* pSession);
	bool SendPacket(CSession* pSession, CPacket* packet);

protected:
	virtual void OnInitialize() = 0;
	virtual void OnTerminate() = 0;
	virtual void OnThreadTerminate(wchar_t* threadName) = 0;

protected:
	virtual bool OnConnectRequest() = 0;
	virtual void OnAcceptClient(__int64 sessionID) = 0;
	virtual void OnRegisterClient(__int64 sessionID) = 0;
	virtual void OnReleaseClient(__int64 sessionID) = 0;
	virtual void OnRecv(__int64 sessionID, CPacket* packet) = 0;
	virtual void OnSend(__int64 sessionID, int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;
	virtual void OnDebug(int debugCode, wchar_t* debugMsg) = 0;

	// For Monitoring ==========================================================

protected:
	inline void UpdateMonitorData();
	inline int GetAcceptTotal() { return _acceptTotal; }
	inline int GetDisconnectTotal() { return _disconnectTotal; }
	inline long GetSessionCount() { return _sessionCnt; }

	inline int GetRecvMsgTPS() { return _recvMsgTPS; }
	inline int GetSendMsgTPS() { return _sendMsgTPS; }
	inline int GetAcceptTPS() { return _acceptTPS; }
	inline int GetDisconnectTPS() { return _disconnectTPS; }

	// Use in Network Library =================================================

private:
	static unsigned int WINAPI AcceptThread(void* arg);
	static unsigned int WINAPI NetworkThread(void* arg);

private:
	bool ReleaseSession(__int64 sessionID);
	bool HandleRecvCP(CSession* pSession, int recvBytes);
	bool HandleSendCP(CSession* pSession, int sendBytes);
	bool RecvPost(CSession* pSession);
	bool SendPost(CSession* pSession);

private:
	bool IncrementUseCount(CSession* pSession, bool check = true);
	void DecrementUseCount(CSession* pSession);

	// ==========================================================================

private:
	wchar_t _IP[10];
	short _port;
	bool _nagle;
	int _numOfThreads;

private:
	SOCKET _listenSock;
	volatile long _networkAlive = 0;

private:
	HANDLE _acceptThread;
	HANDLE* _networkThreads;
	HANDLE _hNetworkCP;

private:
	CSession* _sessions[dfSESSION_MAX] = { nullptr, };
	CLockFreeStack<long> _emptyIdx;
	volatile long _sessionCnt = 0;
	volatile __int64 _sessionID = 0;
	unsigned __int64 _indexMask = 0;
	unsigned __int64 _idMask = 0;

private:
	int _acceptTotal = 0;
	int _disconnectTotal = 0;
	int _TryReleaseTotal = 0;

	int _acceptTPS = 0;
	int _disconnectTPS = 0;
	int _recvMsgTPS = 0;
	int _sendMsgTPS = 0;

	volatile long _acceptCnt = 0;
	volatile long _disconnectCnt = 0;
	volatile long _recvMsgCnt = 0;
	volatile long _sendMsgCnt = 0;
};