#include "CMonitorClient.h"
#include "CChattingServer.h"
#include <tchar.h>
#include <time.h>

#define __CONSOLE_MONITOR

bool CMonitorClient::Initialize(CChattingServer* pChattingServer)
{
	// Set Network Library
	
	int totalThreadCnt = 0;
	int runningThreadCnt = 0;
	::wprintf(L"<Monitor Client>\n");
	::wprintf(L"Total Network Thread Count: ");
	::scanf_s("%d", &totalThreadCnt);
	::wprintf(L"Running Network Thread Count: ");
	::scanf_s("%d", &runningThreadCnt);

	if (!NetworkInitialize(dfMONIOTOR_IP, dfMONIOTOR_PORT, totalThreadCnt, runningThreadCnt, false, true))
	{
		Terminate();
		return false;
	}

	_pChattingServer = pChattingServer;

#ifdef __CONSOLE_MONITOR
	_printThread = (HANDLE)_beginthreadex(NULL, 0, PrintThread, this, 0, nullptr);
	if (_printThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Print Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Print Thread Error\n", _T(__FUNCTION__), __LINE__);
		return false;
	}
#endif

	_sendThread = (HANDLE)_beginthreadex(NULL, 0, SendThread, this, 0, nullptr);
	if (_sendThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Send Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Send Thread Error\n", _T(__FUNCTION__), __LINE__);
		return false;
	}

	// Initialize Compelete
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Monitor: Server Initialize\n");
	::wprintf(L"Monitor: Server Initialize\n\n");
}

unsigned int __stdcall CMonitorClient::PrintThread(void* arg)
{
	CMonitorClient* pMonitor = (CMonitorClient*)arg;
	CChattingServer* pServer = pMonitor->_pChattingServer;
	CreateDirectory(L"MonitorLog", NULL);

	while (pServer->_serverAlive)
	{
		Sleep(1000);

		pServer->_mm->UpdateProcessCPUTime();
		pServer->_mm->UpdateTotalCPUTime();

		SYSTEMTIME stTime;
		GetLocalTime(&stTime);
		WCHAR text[dfMONITOR_TEXT_LEN];

		pServer->UpdateMonitorData();

		swprintf_s(text, dfMONITOR_TEXT_LEN,

			L"\n\n[%s %02d:%02d:%02d]\n\n"

			L"<Chatting Server> --------------------------------\n\n"

			L"Process CPU : %f\n"
			L"Process User CPU : %f\n"
			L"Process Kernel CPU : %f\n\n"

			L"<Chatting Server - Network> ----------------------\n\n"

			L"Session Count : %d\n"
			L"Accept / 1sec: %d	|	Disconnect / 1sec: %d\n"
			L"Recv / 1sec: % d	|	Send / 1sec: % d\n\n"

			L"<Chatting Server - Content> ----------------------\n\n"

			L"Player Count : %d\n"
			L"Packet Pool Size : %d	|	Update / 1sec : %d\n"
			L"Job Q Size : %d		|	Job Pool Size : %d\n\n"

			L"==================================================",

			_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond,

			pServer->_mm->GetProcessCPUTime(),
			pServer->_mm->GetProcessUserCPUTime(),
			pServer->_mm->GetProcessKernelCPUTime(),
			
			pServer->GetSessionCount(),
			pServer->GetAcceptTPS(), pServer->GetDisconnectTPS(),
			pServer->GetRecvTPS(), pServer->GetSendTPS(), 
			
			pServer->GetPlayerCount(), 
			pServer->GetPacketPoolSize(), pServer->GetUpdateTPS(),
			pServer->GetJobQSize(), pServer->GetJobPoolSize()
			);

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

unsigned int __stdcall CMonitorClient::SendThread(void* arg)
{
	CMonitorClient* pMonitor = (CMonitorClient*)arg;
	CChattingServer* pServer = pMonitor->_pChattingServer;

	CLanPacket* packet = CLanPacket::Alloc();
	packet->Clear();
	*packet << (WORD)en_PACKET_SS_MONITOR_LOGIN;
	*packet << (int)0; 
	pMonitor->ReqSendUnicast(packet);

	while (pMonitor->_serverAlive)
	{
		Sleep(1000);

		int now = (int) time(NULL);

		// Chat
		pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_SERVER_RUN, pServer->_mm->GetProcessOnOff(), now);
		pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_SERVER_CPU, pServer->_mm->GetProcessCPUTime(), now);
		pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_SERVER_MEM, pServer->_mm->GetProcessMemory(), now);
		pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_SESSION, pServer->GetSessionCount(), now);
		pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_PLAYER, pServer->GetPlayerCount(), now);
		pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_UPDATE_TPS, pServer->GetUpdateTPS(), now);
		pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_PACKET_POOL, pServer->GetPacketPoolSize(), now);
		pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, pServer->GetJobQSize(), now);

		// Server
		pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, pServer->_mm->GetTotalCPUTime(), now);
		pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, pServer->_mm->GetTotalNonpaged(), now);
		pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, pServer->_mm->GetTotalRecv(), now);
		pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, pServer->_mm->GetTotalSend(), now);
		pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, pServer->_mm->GetTotalUsableMemory(), now);
	}
	return 0;
}

void CMonitorClient::SetDataToPacket(BYTE type, int val, int time)
{
	CLanPacket* packet = CLanPacket::Alloc();
	packet->Clear();
	*packet << (WORD) en_PACKET_SS_MONITOR_DATA_UPDATE;
	*packet << type;
	*packet << val;
	*packet << time;
	ReqSendUnicast(packet);
}

void CMonitorClient::ReqSendUnicast(CLanPacket* packet)
{
	packet->AddUsageCount(1);
	if (!SendPacket(packet))
	{
		CLanPacket::Free(packet);
	}
}

void CMonitorClient::Terminate()
{
	// TO-DO
}

void CMonitorClient::OnInitialize()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Monitor: Network Library Initialize\n");
	::wprintf(L"Monitor: Network Library Initialize\n");
}

void CMonitorClient::OnTerminate()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Monitor: Network Library Terminate\n");
	::wprintf(L"Monitor: Network Library Terminate\n");
}

void CMonitorClient::OnThreadTerminate(wchar_t* threadName)
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Monitor: %s Terminate\n", threadName);
	::wprintf(L"Monitor: %s Terminate\n", threadName);
}

void CMonitorClient::OnEnterServer()
{
}

void CMonitorClient::OnLeaveServer()
{
}

void CMonitorClient::OnRecv(CRecvLanPacket* packet)
{

}

void CMonitorClient::OnSend(int sendSize)
{
}

void CMonitorClient::OnError(int errorCode, wchar_t* errorMsg)
{
	LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"Monitor: %s (%d)\n", errorMsg, errorCode);
	::wprintf(L"Monitor: %s (%d)\n", errorMsg, errorCode);
}

void CMonitorClient::OnDebug(int debugCode, wchar_t* debugMsg)
{
	LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"Monitor: %s (%d)\n", debugMsg, debugCode);
	::wprintf(L"Monitor: %s (%d)\n", debugMsg, debugCode);
}
