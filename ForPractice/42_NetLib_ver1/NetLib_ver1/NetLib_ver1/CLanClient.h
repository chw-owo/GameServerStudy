#pragma once
#include "CPacket.h"
class CLanClient
{
private:
	bool Connect();
	bool Disconnect();
	bool SendPacket(CPacket* packet);

protected:
	virtual bool OnConnectRequest() = 0;
	virtual void OnLeaveServer() = 0;
	virtual void OnRecv(__int64 sessionID, CPacket* packet) = 0;
	virtual void OnSend(__int64 sessionID, int sendSize) = 0;
	virtual void OnError(int errorCode, wchar_t* errorMsg) = 0;
};

