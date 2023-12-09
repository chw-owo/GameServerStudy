#include "CServer.h"

CServer::CServer()
{
}

CServer::~CServer()
{
}

bool CServer::Initialize()
{
	// Set Network Library
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	int cpuCount = (int)si.dwNumberOfProcessors;
	int networkThreadCnt = 0;

	::wprintf(L"CPU total: %d\n", cpuCount);
	::wprintf(L"Network Thread Count (Except Accept Thread) (recommend under %d): ", cpuCount - 1);
	::scanf_s("%d", &networkThreadCnt);

	if (!NetworkInitialize(dfSERVER_IP, dfECHO_PORT, networkThreadCnt, false))
	{
		Terminate();
		return false;
	}

	// Create Group
	_pEchoGroup = new CEchoGroup(this);
	RegisterGroup(_pEchoGroup);
	_pLoginGroup = new CLoginGroup(this);
	RegisterGroup(_pLoginGroup);
	
	// Initialize Compelete
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Main Server Initialize\n");
	::wprintf(L"Main Server Initialize\n");

    return true;
}

bool CServer::OnConnectRequest(WCHAR addr[dfADDRESS_LEN])
{
    return true;
}

void CServer::OnAcceptClient(unsigned __int64 sessionID, WCHAR addr[dfADDRESS_LEN])
{
	::printf("%llx (%d): %s\n", sessionID, GetCurrentThreadId(), __func__);
	MoveGroup(sessionID, _pLoginGroup);
}

void CServer::OnReleaseClient(unsigned __int64 sessionID)
{
}

void CServer::OnRecv(unsigned __int64 sessionID, CPacket* packet)
{
}

void CServer::OnSend(unsigned __int64 sessionID, int sendSize)
{
}


void CServer::Terminate()
{
	RemoveGroup(_pEchoGroup);
	NetworkTerminate();
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Server Terminate.\n");
	::wprintf(L"Server Terminate.\n");
}

void CServer::OnInitialize()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Library Initialize\n");
	::wprintf(L"Network Library Initialize\n");
}

void CServer::OnTerminate()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Library Terminate\n");
	::wprintf(L"Network Library Terminate\n");
}

void CServer::OnThreadTerminate(wchar_t* threadName)
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s Terminate\n", threadName);
	::wprintf(L"Network Library: %s Terminate\n", threadName);
}

void CServer::OnError(int errorCode, wchar_t* errorMsg)
{
	LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s (%d)\n", errorMsg, errorCode);
	::wprintf(L"%s (%d)\n", errorMsg, errorCode);
}

void CServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
	LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s (%d)\n", debugMsg, debugCode);
	::wprintf(L"%s (%d)\n", debugMsg, debugCode);
}
