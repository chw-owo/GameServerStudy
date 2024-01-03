#include "CMonitorClient.h"
#include "CChattingServer.h"
#include "CLoginServer.h"
#include <tchar.h>
#include <time.h>

#define __CONSOLE_MONITOR

bool CMonitorClient::Initialize(CChattingServer* pChattingServer, CLoginServer* pLoginServer)
{
	// Set Network Library
	_pLoginServer = pLoginServer;
	_pChattingServer = pChattingServer;

	_monitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, nullptr);
	if (_monitorThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Monitor Thread Error\n", _T(__FUNCTION__), __LINE__);
		return false;
	}

	return true;
}

void CMonitorClient::SleepForFixedFrame()
{
	if ((timeGetTime() - _oldTick) < _timeGap)
		Sleep(_timeGap - (timeGetTime() - _oldTick));
	_oldTick += _timeGap;
}

unsigned int __stdcall CMonitorClient::MonitorThread(void* arg)
{
	CMonitorClient* pMonitor = (CMonitorClient*)arg;
	CChattingServer* pChat = pMonitor->_pChattingServer;
	CLoginServer* pLogin = pMonitor->_pLoginServer;

	int totalThreadCnt = 0;
	int runningThreadCnt = 0;
	::wprintf(L"\n<Monitor Client>\n");
	::wprintf(L"Total Network Thread Count: ");
	::scanf_s("%d", &totalThreadCnt);
	::wprintf(L"Running Network Thread Count: ");
	::scanf_s("%d", &runningThreadCnt);

	CreateDirectory(L"MonitorLog", NULL);
	pMonitor->_oldTick = timeGetTime();

	while (pMonitor->_serverAlive)
	{
		while (pMonitor->_serverAlive && !pMonitor->_connected)
		{
			pMonitor->_connected = pMonitor->NetworkInitialize(dfMONIOTOR_IP, dfMONIOTOR_PORT, totalThreadCnt, runningThreadCnt, false, true);
			Sleep(500);
		}
		if (!pMonitor->_serverAlive) break;

		CLanPacket* packet = CLanPacket::Alloc();
		packet->Clear();
		*packet << (WORD)en_PACKET_SS_MONITOR_LOGIN;
		*packet << (int)0;
		pMonitor->ReqSendUnicast(packet);

		while (pMonitor->_serverAlive && pMonitor->_connected)
		{
			pMonitor->SleepForFixedFrame();

			// Get Data ==============================================================================

			SYSTEMTIME stTime;
			GetLocalTime(&stTime);
			int now = (int)time(NULL);
			pChat->_mm->UpdateTotalCPUTime();

			pLogin->UpdateMonitorData();
			pLogin->_mm->UpdateProcessCPUTime();
			pChat->UpdateMonitorData();
			pChat->_mm->UpdateProcessCPUTime();

			long loginOn = pLogin->_mm->GetProcessOnOff();
			float loginCPU = pLogin->_mm->GetProcessCPUTime();
			long loginMem = pLogin->_mm->GetProcessMemory();
			long loginSessions = pLogin->GetSessionCount();
			long loginAuthTPS = pLogin->GetAuthTPS();
			long loginPackets = pLogin->GetPacketPoolSize();

			long chatOn = pChat->_mm->GetProcessOnOff();
			float chatCPU = pChat->_mm->GetProcessCPUTime();
			long chatMem = pChat->_mm->GetProcessMemory();
			long chatSessions = pChat->GetSessionCount();
			long chatPlayers = pChat->GetPlayerCount();
			long chatUpdateTPS = pChat->GetUpdateTPS();
			long chatPackets = pChat->GetPacketPoolSize();
			long chatJobs = pChat->GetJobPoolSize();

			float totalCPU = pChat->_mm->GetTotalCPUTime();
			long nonpaged = pChat->_mm->GetTotalNonpaged();
			long totalRecv = pChat->_mm->GetTotalRecv();
			long totalSend = pChat->_mm->GetTotalSend();
			long totalUsableMem = pChat->_mm->GetTotalUsableMemory();

			// Send Data ==============================================================================

			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_LOGIN_SERVER_RUN, loginOn, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_LOGIN_SERVER_CPU, loginCPU, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_LOGIN_SERVER_MEM, loginMem, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_LOGIN_SESSION, loginSessions, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_LOGIN_AUTH_TPS, loginAuthTPS, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_LOGIN_PACKET_POOL, loginPackets, now);

			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_SERVER_RUN, chatOn, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_SERVER_CPU, chatCPU, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_SERVER_MEM, chatMem, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_SESSION, chatSessions, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_PLAYER, chatPlayers, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_UPDATE_TPS, chatUpdateTPS, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_PACKET_POOL, chatPackets, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, chatJobs, now);

			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, totalCPU, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, nonpaged, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, totalRecv, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, totalSend, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, totalUsableMem, now);

			// Console Monitor ===========================================================================

			WCHAR text[dfMONITOR_TEXT_LEN];
			swprintf_s(text, dfMONITOR_TEXT_LEN,

				L"\n\n[%s %02d:%02d:%02d]\n\n"

				L"Login CPU: %f\n"
				L"Login Mem: %d\n"
				L"Login Session: %d\n"
				L"Login Auth TPS: %d\n"
				L"Login Packet Pool: %d\n\n"

				L"Chat CPU: %f\n"
				L"Chat Mem: %d\n"
				L"Chat Session: %d\n"
				L"Chat Player: %d\n"
				L"Chat Auth TPS: %d\n"
				L"Chat Packet Pool: %d\n"
				L"Chat Job Pool: %d\n\n"

				L"Total CPU: %f\n"
				L"Total Nonpaged: %d\n"
				L"Total Recv: %d\n"
				L"Total Send: %d\n"
				L"Total Usable Mem: %d\n\n"

				L"==================================================",

				_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond,

				loginCPU, loginMem, loginSessions, loginAuthTPS, loginPackets,
				chatCPU, chatMem, chatSessions, chatPlayers, chatUpdateTPS, chatPackets, chatJobs,
				totalCPU, nonpaged, totalRecv, totalSend, totalUsableMem);

			::wprintf(L"%s", text);

			// File Log ==============================================================================
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
	}

	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Monitor Thread (%d) Terminate\n", GetCurrentThreadId());
	::wprintf(L"Monitor Thread (%d) Terminate\n", GetCurrentThreadId());

	return 0;
}


void CMonitorClient::SetDataToPacket(BYTE type, int val, int time)
{
	CLanPacket* packet = CLanPacket::Alloc();
	packet->Clear();
	*packet << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
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
	_connected = false;
}

void CMonitorClient::OnDebug(int debugCode, wchar_t* debugMsg)
{
	LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"Monitor: %s (%d)\n", debugMsg, debugCode);
	::wprintf(L"Monitor: %s (%d)\n", debugMsg, debugCode);
}
