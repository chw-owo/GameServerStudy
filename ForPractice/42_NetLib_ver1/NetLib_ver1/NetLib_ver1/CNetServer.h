#pragma once
#include "CPacket.h"
class CNetServer
{
protected:
	CNetServer();
	~CNetServer();

private:
	bool Start();
	void Stop();
	int GetSessionCount();

	bool Disconnect(__int64 sessionID);
	bool SendPacket(__int64 sessionId, CPacket* packet);

protected:
	virtual bool OnConnectRequest() = 0;
	virtual void OnAccept() = 0;
	virtual void OnLeave(__int64 sessionID) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;

	virtual void OnRecv(__int64 sessionID, CPacket* packet) = 0;
	virtual void OnSend(__int64 sessionID, int sendSize) = 0;

protected:
	int getAcceptTPS() { return _acceptTPS; }
	int getRecvMsgTPS() { return _recvMsgTPS; }
	int getSendMsgTPS() { return _sendMsgTPS; }

private:
	int _acceptTPS = 0;
	int _recvMsgTPS = 0;
	int _sendMsgTPS = 0;
};

