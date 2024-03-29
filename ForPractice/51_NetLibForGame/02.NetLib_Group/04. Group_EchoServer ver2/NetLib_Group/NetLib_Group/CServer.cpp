#include "CServer.h"
#include <tchar.h>

#define __MONITOR

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

#ifdef __MONITOR
	// Monitor
	_MonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, (void*)this, 0, nullptr);
	if (_MonitorThread == NULL)
	{
		int err = WSAGetLastError();
		LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s[%d]: Create Group Thread Error, %d", _T(__FUNCTION__), __LINE__, err);
		::wprintf(L"%s[%d]: Create Group Thread Error, %d", _T(__FUNCTION__), __LINE__, err);
		return false;
	}

#endif
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
	// ::printf("%016llx (%d): %s\n", sessionID, GetCurrentThreadId(), __func__);
	MoveGroup(sessionID, _pLoginGroup);
}

void CServer::OnReleaseClient(unsigned __int64 sessionID)
{
	// ::printf("%016llx (%d): Main::%s\n", sessionID, GetCurrentThreadId(), __func__);
}

void CServer::OnRecv(unsigned __int64 sessionID, CPacket* packet)
{
	// ::printf("%016llx (%d): Main::%s\n", sessionID, GetCurrentThreadId(), __func__);
}

void CServer::OnSend(unsigned __int64 sessionID, int sendSize)
{
	// ::printf("%016llx (%d): Main::%s\n", sessionID, GetCurrentThreadId(), __func__);
}


void CServer::Terminate()
{
	RemoveGroup(_pLoginGroup);
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


unsigned int __stdcall CServer::MonitorThread(void* arg)
{
	CServer* pServer = (CServer*)arg;
	CLoginGroup* pLogin = pServer->_pLoginGroup;
	CEchoGroup* pEcho = pServer->_pEchoGroup;

	CreateDirectory(L"MonitorLog", NULL);

	while (1)
	{
		SYSTEMTIME stTime;
		GetLocalTime(&stTime);
		WCHAR text[dfMONITOR_TEXT_LEN];

		pServer->UpdateMonitorData();
		pLogin->UpdateMonitorData();
		pEcho->UpdateMonitorData();

		swprintf_s(text, dfMONITOR_TEXT_LEN,

			L"[%s %02d:%02d:%02d]\n\n"

			"<Network Library> ---------------------------\n\n"

			L"Session Count : %lld\n"
			L"Total Accept : %d\n"
			L"Total Disconnect : %d\n\n"

			L"Accept / 1sec: % d\n"
			L"Disconnect / 1sec: % d\n"
			L"Recv / 1sec: % d\n"
			L"Send / 1sec: % d\n\n"

			"<Login Server> ---------------------------\n\n"

			L"Session Count : %lld\n"
			L"User Count : %lld\n"
			L"Enter / 1sec: % d\n"
			L"Leave / 1sec: % d\n"
			L"Recv / 1sec: % d\n"
			L"Send / 1sec: % d\n\n"

			L"<Echo Server> ------------------------\n\n"

			L"Session Count : %lld\n"
			L"User Count : %lld\n"
			L"Enter / 1sec: % d\n"
			L"Leave / 1sec: % d\n"
			L"Recv / 1sec: % d\n"
			L"Send / 1sec: % d\n\n"

			L"============================================\n\n",

			_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond,

			pServer->GetSessionCount(),
			pServer->GetAcceptTotal(), pServer->GetDisconnectTotal(),

			pServer->GetAcceptTPS(), pServer->GetDisconnectTPS(),
			pServer->GetRecvTPS(), pServer->GetSendTPS(),

			pLogin->GetSessionCount(), pLogin->GetUserCount(),
			pLogin->GetEnterTPS(), pLogin->GetLeaveTPS(),
			pLogin->GetRecvTPS(), pLogin->GetSendTPS(),

			pEcho->GetSessionCount(), pEcho->GetUserCount(),
			pEcho->GetEnterTPS(), pEcho->GetLeaveTPS(),
			pEcho->GetRecvTPS(), pEcho->GetSendTPS());

		::wprintf(L"%s", text);

		FILE* file;
		errno_t openRet = _wfopen_s(&file, L"MonitorLog/MonitorLog.txt", L"a+");
		if (openRet != 0)
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Fail to open %s : %d\n", _T(__FUNCTION__), __LINE__, L"MonitorLog/MonitorLog.txt", openRet);
			::wprintf(L"%s[%d]: Fail to open %s : %d\n", _T(__FUNCTION__), __LINE__, L"MonitorLog/MonitorLog.txt", openRet);
		}
		if (file != nullptr)
		{
			fwprintf(file, text);
			fclose(file);
		}
		else
		{
			LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Fileptr is nullptr %s\n", _T(__FUNCTION__), __LINE__, L"MonitorLog/MonitorLog.txt");
			::wprintf(L"%s[%d]: Fileptr is nullptr %s\n", _T(__FUNCTION__), __LINE__, L"MonitorLog/MonitorLog.txt");
		}

		Sleep(1000);
	}

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Monitor Thread (%d) Terminate\n", GetCurrentThreadId());
	::wprintf(L"Monitor Thread (%d) Terminate\n", GetCurrentThreadId());
	return 0;

	/*
::printf(
	"Echo SendTPS: %d\n"
	"Echo RecvTPS: %d\n"
	"Echo EnterTPS: %d\n"
	"Echo LeaveTPS: %d\n"
	"Echo ReleaseTPS: %d\n\n",
	_sendTPS,
	_recvTPS,
	_enterTPS,
	_leaveTPS,
	_releaseTPS
);
*/

/*
::printf(
	"Login SendTPS: %d\n"
	"Login RecvTPS: %d\n"
	"Login EnterTPS: %d\n"
	"Login LeaveTPS: %d\n"
	"Login ReleaseTPS: %d\n\n",
	_sendTPS,
	_recvTPS,
	_enterTPS,
	_leaveTPS,
	_releaseTPS
);
*/
	return 0;
}
