#pragma once
#include "CNetServer.h"
#include <vector>
using namespace std;

class CSession;
class CNetServer;
template<typename T>
class CLockFreeQueue {};

class CGroup
{
	friend CNetServer;
public:
	void Setting(CNetServer* pNet, _beginthreadex_proc_type function)
	{
		_pNet = pNet;
		_function = function;
	}
	bool GetAlive() { return _alive; }

private:
	void Terminate() { _alive = false; };
	bool _alive = true;



protected:
	virtual void OnInitialize() = 0;
	virtual void OnTerminate() = 0;

protected:
	virtual void OnRegisterClient(CSession* pSession) = 0;
	virtual void OnReleaseClient(CSession* pSession) = 0;
	virtual void OnRecv(CSession* pSession, CPacket* packet) = 0;
	virtual void OnSend(CSession* pSession, int sendSize) = 0;

protected:
	bool Disconnect(CSession* pSession)
	{
		return _pNet->Disconnect(pSession);
	}

	bool SendPacket(CSession* pSession, CPacket* packet)
	{
		return _pNet->SendPacket(pSession, packet);
	}
	
	bool MoveSession(CGroup* pGroup, CSession* pSession)
	{
		return _pNet->MoveSession(pGroup, pSession);
	}

private:
	CLockFreeQueue<CSession*> _inSessions;
	CLockFreeQueue<CSession*> _outSessions;
	vector<CSession*> _groupSessions;
	_beginthreadex_proc_type _function;
	CNetServer* _pNet;
};

