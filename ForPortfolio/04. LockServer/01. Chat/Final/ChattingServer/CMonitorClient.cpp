#include "CMonitorClient.h"
#include "CChattingServer.h"
#include <tchar.h>
#include <time.h>

#define __CONSOLE_MONITOR

bool CMonitorClient::Initialize(CChattingServer* pChattingServer)
{
	// Set Network Library
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
	CChattingServer* pServer = pMonitor->_pChattingServer;

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
		/*
		while (pMonitor->_serverAlive && !pMonitor->_connected)
		{
			pMonitor->_connected = pMonitor->NetworkInitialize(dfMONIOTOR_IP, dfMONIOTOR_PORT, totalThreadCnt, runningThreadCnt, false, true);
			Sleep(500);
		}
		if (!pMonitor->_serverAlive) break;

		CLanSendPacket* packet = CLanSendPacket::Alloc();
		*packet << (WORD)en_PACKET_SS_MONITOR_LOGIN;
		*packet << (int)0;
		pMonitor->ReqSendUnicast(packet);
		*/

		// while (pMonitor->_serverAlive && pMonitor->_connected)
		{
			pMonitor->SleepForFixedFrame();

			// Get Data ==============================================================================

			SYSTEMTIME stTime;
			GetLocalTime(&stTime);
			int now = (int)time(NULL);

			pServer->UpdateMonitorData();
			pServer->_mm->UpdateProcessCPUTime();
			pServer->_mm->UpdateTotalCPUTime();

			long processOn = pServer->_mm->GetProcessOnOff();
			float processCPU = pServer->_mm->GetProcessCPUTime();
			long processMem = pServer->_mm->GetProcessMemory();

			long sessions = pServer->GetSessionCount();
			long players = pServer->GetPlayerCount();
			long updateTPS = pServer->GetUpdateTPS();
			long packets = pServer->GetPacketPoolSize();
			long jobs = pServer->GetJobPoolSize();

			long acceptTotal = pServer->GetAcceptTotal();
			long disconnectTotal = pServer->GetDisconnectTotal();
			long acceptTPS = pServer->GetAcceptTPS();
			long recvTPS = pServer->GetRecvTPS();
			long sendTPS = pServer->GetSendTPS();

			float totalCPU = pServer->_mm->GetTotalCPUTime();
			long nonpaged = pServer->_mm->GetTotalNonpaged();
			long totalRecv = pServer->_mm->GetTotalRecv();
			long totalSend = pServer->_mm->GetTotalSend();
			long totalUsableMem = pServer->_mm->GetTotalUsableMemory();

			// Send Data ==============================================================================

			/*
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_SERVER_RUN, processOn, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_SERVER_CPU, processCPU, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_SERVER_MEM, processMem, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_SESSION, sessions, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_PLAYER, players, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_UPDATE_TPS, updateTPS, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_PACKET_POOL, packets, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, jobs, now);

			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, totalCPU, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, nonpaged, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, totalRecv, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, totalSend, now);
			pMonitor->SetDataToPacket(en_MONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, totalUsableMem, now);
			*/

			// Console Monitor ===========================================================================

			WCHAR text[dfMONITOR_TEXT_LEN];
			swprintf_s(text, dfMONITOR_TEXT_LEN,

				L"\n\n[%s %02d:%02d:%02d]\n\n"

				L"Chat CPU: %f\n"
				L"Chat Mem: %d\n\n"

				L"Chat Session: %d\n"
				L"Chat Player: %d\n"
				L"Chat Update TPS: %d\n"
				L"Chat Packet Pool: %d\n"
				L"Chat Job Pool: %d\n\n"

				L"Chat Accept Total: %d\n"
				L"Chat Disconnect Total: %d\n"
				L"Chat Accept TPS: %d\n"
				L"Chat Recv TPS: %d\n"
				L"Chat Send TPS: %d\n\n"

				L"Total CPU: %f\n"
				L"Total Nonpaged: %d\n"
				L"Total Recv: %d\n"
				L"Total Send: %d\n"
				L"Total Usable Mem: %d\n\n"

				L"==================================================",

				_T(__DATE__), stTime.wHour, stTime.wMinute, stTime.wSecond,

				processCPU, processMem,
				sessions, players, updateTPS, packets, jobs,
				acceptTotal, disconnectTotal, acceptTPS, recvTPS, sendTPS,
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
	CLanSendPacket* packet = CLanSendPacket::Alloc();
	*packet << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
	*packet << type;
	*packet << val;
	*packet << time;
	ReqSendUnicast(packet);
}

void CMonitorClient::ReqSendUnicast(CLanSendPacket* packet)
{
	packet->AddUsageCount(1);
	if (!SendPacket(packet))
	{
		CLanSendPacket::Free(packet);
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

void CMonitorClient::OnRecv(CLanMsg* packet)
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
