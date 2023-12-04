#include "CEchoServer.h"
#include "CSystemLog.h"

CEchoServer::CEchoServer()
{

}

void CEchoServer::Initialize()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int threadCnt = (int)si.dwNumberOfProcessors;

	if (!NetworkInitialize(dfSERVER_IP, dfSERVER_PORT, threadCnt, false))
		Terminate();

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"EchoServer Initialize\n");
	::wprintf(L"EchoServer Initialize\n\n");
}

void CEchoServer::Terminate()
{
	NetworkTerminate();
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"EchoServer Terminate.\n");
	::wprintf(L"EchoServer Terminate.\n");
}

bool CEchoServer::OnConnectRequest()
{
	return true;
}

void CEchoServer::OnAcceptClient(unsigned __int64 sessionID)
{
}

void CEchoServer::OnRecv(unsigned __int64 sessionID, CPacket* packet)
{
	try
	{
		__int64 echo;
		*packet >> echo;

		// ::printf("%llu\n", echo);

		CPacket* sendPacket = CPacket::Alloc();
		sendPacket->Clear();
		sendPacket->AddUsageCount(1);
		*sendPacket << echo;
		if (!SendPacket(sessionID, sendPacket))
		{
			// ::printf("Fail to Send Packet\n");
			CPacket::Free(packet);
		}
	}
	catch (int packetError)
	{
		if (packetError == ERR_PACKET)
		{
			::printf("packet error!\n");
			Disconnect(sessionID);
		}
	}
}

void CEchoServer::OnSend(unsigned __int64 sessionID, int sendSize)
{

}

void CEchoServer::OnReleaseClient(unsigned __int64 sessionID)
{

}

void CEchoServer::OnInitialize()
{
}

void CEchoServer::OnTerminate()
{
}

void CEchoServer::OnThreadTerminate(wchar_t* threadName)
{
}

void CEchoServer::OnError(int errorCode, wchar_t* errorMsg)
{
}

void CEchoServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
}




