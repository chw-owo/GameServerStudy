#include "CMonitorServer.h"
#include "MonitorProtocol.h"
#include "CSystemLog.h"
#include <tchar.h>

bool CMonitorServer::Initialize()
{
	// Set Network Library
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	int cpuCount = (int)si.dwNumberOfProcessors;
	int totalThreadCnt = 0;
	int runningThreadCnt = 0;

	::wprintf(L"CPU total: %d\n", cpuCount);
	::wprintf(L"Total Network Thread Count: ");
	::scanf_s("%d", &totalThreadCnt);
	::wprintf(L"Running Network Thread Count: ");
	::scanf_s("%d", &runningThreadCnt);

	if (!NetworkInitialize(dfSERVER_IP, dfSERVER_PORT, totalThreadCnt, runningThreadCnt, false, true))
	{
		Terminate();
		return false;
	}

	_monitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, nullptr);
	if (_monitorThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		return false;
	}

	// Initialize Compelete
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Server Initialize\n");
	::wprintf(L"Server Initialize\n\n");
}

unsigned int __stdcall CMonitorServer::MonitorThread(void* arg)
{
	for (;;)
	{
		Sleep(1000);
	}
	return 0;
}

void CMonitorServer::Terminate()
{
}

void CMonitorServer::OnInitialize()
{
}

void CMonitorServer::OnTerminate()
{
}

void CMonitorServer::OnThreadTerminate(wchar_t* threadName)
{
}

void CMonitorServer::OnError(int errorCode, wchar_t* errorMsg)
{
}

void CMonitorServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
}

bool CMonitorServer::OnConnectRequest()
{
	return false;
}

void CMonitorServer::OnAcceptClient(unsigned __int64 sessionID)
{
}

void CMonitorServer::OnReleaseClient(unsigned __int64 sessionID)
{
}

void CMonitorServer::OnRecv(unsigned __int64 sessionID, CPacket* packet)
{
}

void CMonitorServer::OnSend(unsigned __int64 sessionID, int sendSize)
{
}
