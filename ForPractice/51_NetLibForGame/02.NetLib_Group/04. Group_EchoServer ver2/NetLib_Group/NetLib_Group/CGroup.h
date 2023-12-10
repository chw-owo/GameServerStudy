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
	void SetDead() { _alive = false; }
	bool GetAlive() { return _alive; }

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
	virtual void OnSend(unsigned __int64 sessionID, int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;
	virtual void OnDebug(int debugCode, wchar_t* debugMsg) = 0;

	// Called in CGroup
private:
	static unsigned int WINAPI UpdateThread(void* arg);

private:
	bool SkipForFixedFrame();
	void NetworkUpdate();

private:
	void AcceptEnterSessions();
	void RemoveAllSessions();

private:
	bool _alive = true;
	DWORD _fps;
	CNetServer* _pNet;

private:
	vector<unsigned __int64> _sessions;
	CLockFreeQueue<unsigned __int64> _enterSessions;

	// Monitor
private:
	int _sendTPS = 0;
	int _recvTPS = 0;
	int _enterTPS = 0;
	int _leaveTPS = 0;

	long _sendCnt = 0;
	long _recvCnt = 0;
	long _enterCnt = 0;
	long _leaveCnt = 0;

public:
	void UpdateMonitorData()
	{
		_sendTPS = _sendCnt;
		_recvTPS = _recvCnt;
		_enterTPS = _enterCnt;
		_leaveTPS = _leaveCnt;

		_sendCnt = 0;
		_recvCnt = 0;
		_enterCnt = 0;
		_leaveCnt = 0;
	}

	int GetSendTPS() { return _sendTPS; }
	int GetRecvTPS() { return _recvTPS; }
	int GetEnterTPS() { return _enterTPS; }
	int GetLeaveTPS() { return _leaveTPS; }

public:
	__int64 GetSessionCount() { return _sessions.size(); }
};

