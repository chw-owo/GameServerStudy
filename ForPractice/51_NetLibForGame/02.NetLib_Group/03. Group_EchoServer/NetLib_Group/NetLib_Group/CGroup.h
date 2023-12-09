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
	virtual void OnReleaseClient(unsigned __int64 sessionID) = 0;
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
	void RemoveLeaveSessions();
	void RemoveReleaseSessions();
	void RemoveAllSessions();

private:
	bool _alive = true;
	DWORD _fps;
	CNetServer* _pNet;

private:
	vector<CSession*> _sessions;
	CLockFreeQueue<CSession*> _enterSessions;
	CLockFreeQueue<CSession*> _leaveSessions;
	CLockFreeQueue<CSession*> _releaseSessions;
};

