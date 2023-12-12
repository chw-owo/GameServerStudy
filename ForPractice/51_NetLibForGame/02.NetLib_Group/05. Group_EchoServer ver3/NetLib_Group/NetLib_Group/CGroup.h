#pragma once
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include "CPacket.h"
#include "CSession.h"
#include <vector>
#include <Windows.h>
using namespace std;


class CNetServer;
class CGroup
{
	friend CNetServer;

	// Call Everywhere
public:
	void Setting(CNetServer* pNet, DWORD fps)
	{
		_pNet = pNet;
		_fps = fps;	
	}

public:
	bool GetAlive() { return _alive; }
	void SetDead() 
	{ 
		_alive = false; 
		InterlockedIncrement(&_signal);
	}

	// Called in CGroup's Child Class
protected:
	bool Disconnect(unsigned __int64 sessionID);
	bool SendPacket(unsigned __int64 sessionID, CPacket* packet, bool disconnect = false);
	void MoveGroup(unsigned __int64 sessionID, CGroup* pGroup);

protected:
	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void Terminate() = 0;

protected:
	virtual void OnUpdate() = 0;
	virtual void OnInitialize() = 0;
	virtual void OnTerminate() = 0;

protected:
	virtual void OnEnterGroup(unsigned __int64 sessionID) = 0;
	virtual void OnLeaveGroup(unsigned __int64 sessionID) = 0;

protected:
	virtual void OnRecv(unsigned __int64 sessionID, CPacket* packet) = 0;
	virtual void OnSend(unsigned __int64 sessionID) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;
	virtual void OnDebug(int debugCode, wchar_t* debugMsg) = 0;

	// Called in CGroup
private:
	static unsigned int WINAPI UpdateThread(void* arg);

private:
	bool SkipForFixedFrame();
	void NetworkUpdate();
	void RemoveAllSessions();

private:
	bool _alive = true;
	DWORD _fps = 0;
	CNetServer* _pNet;
	long _signal = 0;
	long _undesired = 0;

private:
	vector<unsigned __int64> _sessions;
	CLockFreeQueue<unsigned __int64> _enterSessions;
	CLockFreeQueue<unsigned __int64> _OnSendQ;

	// Monitor
public:
	inline void UpdateMonitorData()
	{
		_enterTPS = InterlockedExchange(&_enterCnt, 0);
		_leaveTPS = InterlockedExchange(&_leaveCnt, 0);
		_recvTPS = InterlockedExchange(&_recvCnt, 0);
		_sendTPS = InterlockedExchange(&_sendCnt, 0);

		_enterTotal += _enterTPS;
		_leaveTotal += _leaveTPS;
		_recvTotal += _recvTPS;
		_sendTotal += _sendTPS;
	}

	__int64 GetSessionCount() { return _sessions.size(); }

	inline unsigned long long GetEnterTotal() { return _enterTotal; }
	inline unsigned long long GetLeaveTotal() { return _leaveTotal; }
	inline unsigned long long GetRecvTotal() { return _recvTotal; }
	inline unsigned long long GetSendTotal() { return _sendTotal; }

	inline long GetEnterTPS() { return _enterTPS; }
	inline long GetLeaveTPS() { return _leaveTPS; }
	inline long GetRecvTPS() { return _recvTPS; }
	inline long GetSendTPS() { return _sendTPS; }

private:
	unsigned long long _enterTotal = 0;
	unsigned long long _leaveTotal = 0;
	unsigned long long _recvTotal = 0;
	unsigned long long _sendTotal = 0;

	long _enterTPS = 0;
	long _leaveTPS = 0;
	long _recvTPS = 0;
	long _sendTPS = 0;

	volatile long _enterCnt = 0;
	volatile long _leaveCnt = 0;
	volatile long _recvCnt = 0;
	volatile long _sendCnt = 0;
};

