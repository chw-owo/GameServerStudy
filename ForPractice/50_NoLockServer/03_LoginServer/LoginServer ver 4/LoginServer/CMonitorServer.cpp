#include "CMonitorServer.h"
#include <tchar.h>

CMonitorServer::CMonitorServer()
{

}

bool CMonitorServer::Initialize(CLoginServer* pLoginServer, CChattingServer* pChattingServer)
{
	_servers._login = pLoginServer;
	_servers._chatting = pChattingServer;

	// Set Monitor Thread
	_monitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, nullptr);
	if (_monitorThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
	return true;
}

unsigned int __stdcall CMonitorServer::MonitorThread(void* arg)
{
	CMonitorServer* pMonitor = (CMonitorServer*)arg;
	CChattingServer* pChat = pMonitor->_servers._chatting;
	CLoginServer* pLogin = pMonitor->_servers._login;
	CreateDirectory(L"MonitorLog", NULL);

	while (pMonitor->_serverAlive)
	{
		pLogin->UpdateMonitorData();
		pChat->UpdateMonitorData();

		SYSTEMTIME stTime;
		GetLocalTime(&stTime);

		WCHAR text[dfMONITOR_TEXT_LEN];

		swprintf_s(text, dfMONITOR_TEXT_LEN, 

			L"[%s %02d:%02d:%02d]\n\n"

			"<Login Server> ---------------------------\n\n"

			L"Connected Session : %d\n"
			L"User Count : %lld\n"
			L"Total Accept : %d\n"
			L"Total Disconnect : %d\n"

			L"Recv / 1sec: % d\n"
			L"Send / 1sec: % d\n"
			L"Accept / 1sec: % d\n"
			L"Disconnect / 1sec: % d\n\n"

			L"<Chatting Server> ------------------------\n\n"

			L"Connected Session : %d\n"
			L"Player Count : %lld\n"
			L"Total Accept : %d\n"
			L"Total Disconnect : %d\n"

			L"Recv / 1sec: % d\n"
			L"Send / 1sec: % d\n"
			L"Accept / 1sec: % d\n"
			L"Disconnect / 1sec: % d\n\n"

			L"============================================\n\n",

			_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond,		

			pLogin->GetSessionCount(), pLogin->_usersMap.size(),
			pLogin->GetAcceptTotal(), pLogin->GetDisconnectTotal(),
			pLogin->GetRecvMsgTPS(), pLogin->GetSendMsgTPS(),
			pLogin->GetAcceptTPS(), pLogin->GetDisconnectTPS(),

			pChat->GetSessionCount(), pChat->_playersMap.size(), 
			pChat->GetAcceptTotal(), pChat->GetDisconnectTotal(), 
			pChat->GetRecvMsgTPS(), pChat->GetSendMsgTPS(), 
			pChat->GetAcceptTPS(), pChat->GetDisconnectTPS());

		::wprintf(L"%s", text);

		pChat->_updateThreadWakeTPS = 0;
		pChat->_handlePacketTPS = 0;
		pChat->_jobQSize = 0;

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
}
