#include "CNetMonitorServer.h"
#include "MonitorProtocol.h"
#include "CSystemLog.h"
#include <tchar.h>

bool CNetMonitorServer::Initialize()
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

	if (!NetworkInitialize(dfSERVER_IP, dfLANSERVER_PORT, totalThreadCnt, runningThreadCnt, false, false))
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

void CNetMonitorServer::Terminate()
{
	// TO-DO
}

unsigned int __stdcall CNetMonitorServer::MonitorThread(void* arg)
{
	CNetMonitorServer* pServer = (CNetMonitorServer*)arg;
	CLockFreeQueue<CJob*>** pJobQueues = pServer->_pJobQueues;
	long undesired = 0;

	while (pServer->_serverAlive)
	{
		WaitOnAddress(&pServer->_signal, &undesired, sizeof(long), INFINITE);

		for (int i = 0; i < dfJOB_QUEUE_CNT; i++)
		{
			while (pJobQueues[i]->GetUseSize() > 0)
			{
				CJob* job = pJobQueues[i]->Dequeue();
				if (job == nullptr) break;

				if (job->_type == JOB_TYPE::SYSTEM)
				{
					if (job->_sysType == SYS_TYPE::ACCEPT)
					{
						pServer->HandleAccept(job->_sessionID);
					}
					else if (job->_sysType == SYS_TYPE::RELEASE)
					{
						pServer->HandleRelease(job->_sessionID);
					}
					else if (job->_sysType == SYS_TYPE::TERMINATE)
					{
						break;
					}
				}
				else if (job->_type == JOB_TYPE::CONTENT)
				{
					pServer->HandleRecv(job->_sessionID, job->_packet);
				}

				pServer->_pJobPool->Free(job);
				InterlockedDecrement(&pServer->_signal);
			}
		}
	}
	return 0;
}

void CNetMonitorServer::OnInitialize()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Library Initialize\n");
	::wprintf(L"Network Library Initialize\n");
}

void CNetMonitorServer::OnTerminate()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Network Library Terminate\n");
	::wprintf(L"Network Library Terminate\n");
}

void CNetMonitorServer::OnThreadTerminate(wchar_t* threadName)
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"%s Terminate\n", threadName);
	::wprintf(L"%s Terminate\n", threadName);
}

void CNetMonitorServer::OnError(int errorCode, wchar_t* errorMsg)
{
	LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s (%d)\n", errorMsg, errorCode);
	::wprintf(L"%s (%d)\n", errorMsg, errorCode);
}

void CNetMonitorServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
	LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s (%d)\n", debugMsg, debugCode);
	::wprintf(L"%s (%d)\n", debugMsg, debugCode);
}

bool CNetMonitorServer::OnConnectRequest()
{
	return false;
}

void CNetMonitorServer::OnAcceptClient(unsigned __int64 sessionID)
{
	CJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::ACCEPT, sessionID, nullptr);
	EnqueueJob(sessionID, job);

	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
}

void CNetMonitorServer::OnReleaseClient(unsigned __int64 sessionID)
{
	CJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::RELEASE, sessionID, nullptr);
	EnqueueJob(sessionID, job);

	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
}

void CNetMonitorServer::OnRecv(unsigned __int64 sessionID, CRecvPacket* packet)
{
	CJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::CONTENT, SYS_TYPE::NONE, sessionID, packet);
	EnqueueJob(sessionID, job);

	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
}

void CNetMonitorServer::OnSend(unsigned __int64 sessionID, int sendSize)
{
}

void CNetMonitorServer::HandleAccept(unsigned __int64 sessionID)
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Accept: %016llx\n", sessionID);
	::wprintf(L"Accept: %016llx\n", sessionID);
}

void CNetMonitorServer::HandleRelease(unsigned __int64 sessionID)
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Release: %016llx\n", sessionID);
	::wprintf(L"Release: %016llx\n", sessionID);
}

void CNetMonitorServer::HandleRecv(unsigned __int64 sessionID, CRecvPacket* packet)
{
	try
	{
		WORD msgType;
		*packet >> msgType;

		switch (msgType)
		{
		case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
			Handle_REQ_LOGIN(sessionID, packet);
			break;

		default:
			Disconnect(sessionID);
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
			::wprintf(L"%s[%d] Undefined Message, %d\n", _T(__FUNCTION__), __LINE__, msgType);
			break;
		}
	}
	catch (int packetError)
	{
		if (packetError == ERR_PACKET)
		{
			Disconnect(sessionID);
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
			::wprintf(L"%s[%d] Packet Error\n", _T(__FUNCTION__), __LINE__);
		}
	}
	CRecvPacket::Free(packet);
}

void CNetMonitorServer::Handle_REQ_LOGIN(unsigned __int64 sessionID, CRecvPacket* packet)
{
	char LoginSessionKey[32];
	packet->GetPayloadData(LoginSessionKey, sizeof(LoginSessionKey));

	BYTE Status = 1;
	if (strcmp(LoginSessionKey, dfSESSION_KEY) == 0)
	{
		Status = 3;
	}
}
