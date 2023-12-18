#include "CMonitorClient.h"
#include "CChattingServer.h"
#include <tchar.h>

/*
	<����͸� �׸�>

	en_MONITOR_DATA_TYPE_CHAT_SERVER_RUN = 30,			// ä�ü��� ChatServer ���� ���� ON / OFF
	en_MONITOR_DATA_TYPE_CHAT_SERVER_CPU = 31,			// ä�ü��� ChatServer CPU ����
	en_MONITOR_DATA_TYPE_CHAT_SERVER_MEM = 32,			// ä�ü��� ChatServer �޸� ��� MByte
	en_MONITOR_DATA_TYPE_CHAT_SESSION = 33,				// ä�ü��� ���� �� (Ŀ�ؼ� ��)
	en_MONITOR_DATA_TYPE_CHAT_PLAYER = 34,				// ä�ü��� �������� ����� �� (���� ������)
	en_MONITOR_DATA_TYPE_CHAT_UPDATE_TPS = 35,			// ä�ü��� UPDATE ������ �ʴ� ó�� Ƚ��
	en_MONITOR_DATA_TYPE_CHAT_PACKET_POOL = 36,			// ä�ü��� ��ŶǮ ��뷮
	en_MONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL = 37,		// ä�ü��� UPDATE MSG Ǯ ��뷮

	en_MONITOR_DATA_TYPE_MONITOR_CPU_TOTAL = 40,		// ������ǻ�� CPU ��ü ����
	en_MONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY = 41,	// ������ǻ�� �������� �޸� MByte
	en_MONITOR_DATA_TYPE_MONITOR_NETWORK_RECV = 42,		// ������ǻ�� ��Ʈ��ũ ���ŷ� KByte
	en_MONITOR_DATA_TYPE_MONITOR_NETWORK_SEND = 43,		// ������ǻ�� ��Ʈ��ũ �۽ŷ� KByte
	en_MONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY = 44,	// ������ǻ�� ��밡�� �޸�

	<�� �� ����͸� �α�>

	- Update TPS, msgQ size
	- Accept Total, Disconnect Total
	- Accept TPS, Disconnect TPS, Recv TPS, Send TPS

*/

bool CMonitorClient::Initialize(CChattingServer* pChattingServer)
{
	_pChattingServer = pChattingServer;
	_monitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, nullptr);
	if (_monitorThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
}

unsigned int __stdcall CMonitorClient::MonitorThread(void* arg)
{
	CMonitorClient* pMonitor = (CMonitorClient*)arg;
	CChattingServer* pServer = pMonitor->_pChattingServer;

	CreateDirectory(L"MonitorLog", NULL);

	for (;;)
	{
		Sleep(1000);

		SYSTEMTIME stTime;
		GetLocalTime(&stTime);
		WCHAR text[dfMONITOR_TEXT_LEN];

		pServer->UpdateMonitorData();

		swprintf_s(text, dfMONITOR_TEXT_LEN,

			L"\n\n[%s %02d:%02d:%02d]\n\n"

			L"<Total Server> -----------------------------------\n\n"

			L"CPU Total : %d\n"
			L"Recv (KB) : %d\n"
			L"Send (KB) : %d\n"
			L"Usable Memory (MB) : %d\n"
			L"Nonpaged Memory (MB) : %d\n\n"

			L"<Chatting Server> --------------------------------\n\n"

			L"CPU : %d\n"
			L"ON/OFF : %d\n"
			L"Private Memory (MB) : %d\n\n"

			L"<Chatting Server - Network> ----------------------\n\n"

			L"Session Count : %d\n"
			L"Total Accept : %d\n"
			L"Total Disconnect : %d\n"
			L"Accept / 1sec: %d\n"
			L"Disconnect / 1sec: %d\n"
			L"Recv / 1sec: % d\n"
			L"Send / 1sec: % d\n\n"

			L"<Chatting Server - Content> ----------------------\n\n"

			L"Player Count : %d\n"
			L"Update / 1sec : %d\n"
			L"Job Q Size : %d\n"
			L"Job Pool Size : %d\n"
			L"Packet Pool Size : %d\n\n"

			L"==================================================",

			_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond,

			pServer->GetCPUTotal(),
			pServer->GetRecvTotal(), pServer->GetSendTotal(),
			pServer->GetUsableMemoryTotal(), pServer->GetNonpagedTotal(),

			pServer->GetProcessCPU(), pServer->GetProcessOnOff(),
			pServer->GetProcessMemory(),
			
			pServer->GetSessionCount(),
			pServer->GetAcceptTotal(), pServer->GetDisconnectTotal(),
			pServer->GetAcceptTPS(), pServer->GetDisconnectTPS(),
			pServer->GetRecvTPS(), pServer->GetSendTPS(), 
			
			pServer->GetPlayerCount(), pServer->GetUpdateTPS(),
			pServer->GetJobQSize(), pServer->GetJobPoolSize(), 
			pServer->GetPacketPoolSize());

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
	}

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Monitor Thread (%d) Terminate\n", GetCurrentThreadId());
	::wprintf(L"Monitor Thread (%d) Terminate\n", GetCurrentThreadId());
	return 0;
}

void CMonitorClient::Terminate()
{
}

void CMonitorClient::OnInitialize()
{
}

void CMonitorClient::OnTerminate()
{
}

void CMonitorClient::OnThreadTerminate(wchar_t* threadName)
{
}

void CMonitorClient::OnEnterServer()
{
}

void CMonitorClient::OnLeaveServer()
{
}

void CMonitorClient::OnRecv(CPacket* packet)
{
}

void CMonitorClient::OnSend(int sendSize)
{
}

void CMonitorClient::OnError(int errorCode, wchar_t* errorMsg)
{
}

void CMonitorClient::OnDebug(int debugCode, wchar_t* debugMsg)
{
}
