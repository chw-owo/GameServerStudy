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
	void Terminate() { _alive = false; }
	bool GetAlive() { return _alive; }

	// Called in CGroup's Child Class
protected:
	bool Disconnect(CSession* pSession);
	bool SendPacket(CSession* pSession, CPacket* packet, bool disconnect = false);
	void MoveGroup(CSession* pSession, CGroup* pGroup);

protected:
	virtual void Update() = 0;
	virtual void OnUpdate() = 0;

protected:
	virtual void OnInitialize() = 0;
	virtual void OnTerminate() = 0;

protected:
	virtual void OnEnterGroup(CSession* pSession) = 0;
	virtual void OnLeaveGroup(CSession* pSession) = 0;

protected:
	virtual void OnReleaseClient(CSession* pSession) = 0;
	virtual void OnRecv(CSession* pSession, CPacket* packet) = 0;
	virtual void OnSend(CSession* pSession, int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;
	virtual void OnDebug(int debugCode, wchar_t* debugMsg) = 0;

	// Called in CGroup
private:
	void NetworkUpdate();
	void AcceptEnterSessions();
	void RemoveLeaveSessions();
	bool SkipForFixedFrame();
	static unsigned int WINAPI UpdateThread(void* arg);

private:
	void HandleRelease(CSession* pSession);
	bool HandleRecv(CSession* pSession, int recvBytes);
	bool HandleSend(CSession* pSession, int sendBytes);
	bool RecvPost(CSession* pSession);
	bool SendPost(CSession* pSession);

private:
	bool _alive;
	DWORD _fps;
	CNetServer* _pNet;

private:
	vector<CSession*> _sessions;
	CLockFreeQueue<CSession*> _enterSessions;
	CLockFreeQueue<CSession*> _leaveSessions;
};


