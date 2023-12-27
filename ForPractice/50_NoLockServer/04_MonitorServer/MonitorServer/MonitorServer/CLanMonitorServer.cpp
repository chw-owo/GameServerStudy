#include "CLanMonitorServer.h"
#include "MonitorProtocol.h"
#include "CSystemLog.h"
#include <tchar.h>

/*
1. Recv 테스트
2. DB 저장 스레드 생성
*/

bool CLanMonitorServer::Initialize()
{
	// Set Network Library

	int totalThreadCnt = 0;
	int runningThreadCnt = 0;
	::wprintf(L"<Lan Server>\n");
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
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Lan: Server Initialize\n");
	::wprintf(L"Lan: Server Initialize\n\n");
}

void CLanMonitorServer::Terminate()
{
	// TO-DO
}

unsigned int __stdcall CLanMonitorServer::MonitorThread(void* arg)
{
	CLanMonitorServer* pServer = (CLanMonitorServer*)arg;
	CLockFreeQueue<CLanJob*>** pJobQueues = pServer->_pJobQueues;
	long undesired = 0;

	while (pServer->_serverAlive)
	{
		WaitOnAddress(&pServer->_signal, &undesired, sizeof(long), INFINITE);

		for (int i = 0; i < dfJOB_QUEUE_CNT; i++)
		{
			while (pJobQueues[i]->GetUseSize() > 0)
			{
				CLanJob* job = pJobQueues[i]->Dequeue();
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

void CLanMonitorServer::OnInitialize()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Lan: Network Library Initialize\n");
	::wprintf(L"Lan: Network Library Initialize\n");
}

void CLanMonitorServer::OnTerminate()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Lan: Network Library Terminate\n");
	::wprintf(L"Lan: Network Library Terminate\n");
}

void CLanMonitorServer::OnThreadTerminate(wchar_t* threadName)
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Lan: %s Terminate\n", threadName);
	::wprintf(L"Lan: %s Terminate\n", threadName);
}

void CLanMonitorServer::OnError(int errorCode, wchar_t* errorMsg)
{
	LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"Lan: %s (%d)\n", errorMsg, errorCode);
	::wprintf(L"Lan: %s (%d)\n", errorMsg, errorCode);
}

void CLanMonitorServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
	LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"Lan: %s (%d)\n", debugMsg, debugCode);
	::wprintf(L"Lan: %s (%d)\n", debugMsg, debugCode);
}

bool CLanMonitorServer::OnConnectRequest()
{
	return false;
}

void CLanMonitorServer::OnAcceptClient(unsigned __int64 sessionID)
{
	CLanJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::ACCEPT, sessionID, nullptr);
	EnqueueJob(sessionID, job);

	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
}

void CLanMonitorServer::OnReleaseClient(unsigned __int64 sessionID)
{
	CLanJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::RELEASE, sessionID, nullptr);
	EnqueueJob(sessionID, job);

	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
}

void CLanMonitorServer::OnRecv(unsigned __int64 sessionID, CRecvLanPacket* packet)
{
	CLanJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::CONTENT, SYS_TYPE::NONE, sessionID, packet);
	EnqueueJob(sessionID, job);

	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
}

void CLanMonitorServer::OnSend(unsigned __int64 sessionID, int sendSize)
{
}

void CLanMonitorServer::HandleAccept(unsigned __int64 sessionID)
{
	// TO-DO
}

void CLanMonitorServer::HandleRelease(unsigned __int64 sessionID)
{
	// TO-DO
}

void CLanMonitorServer::HandleRecv(unsigned __int64 sessionID, CRecvLanPacket* packet)
{
	try
	{
		WORD msgType;
		*packet >> msgType;

		switch (msgType)
		{
		case en_PACKET_SS_MONITOR_LOGIN:
			Handle_LOGIN(sessionID, packet);
			break;

		case en_PACKET_SS_MONITOR_DATA_UPDATE:
			Handle_DATA_UPDATE(sessionID, packet);
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
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] LanPacket Error\n", _T(__FUNCTION__), __LINE__);
			::wprintf(L"%s[%d] LanPacket Error\n", _T(__FUNCTION__), __LINE__);
		}
	}
	CRecvLanPacket::Free(packet);
}

void CLanMonitorServer::Handle_LOGIN(unsigned __int64 sessionID, CRecvLanPacket* packet)
{
	int serverID;
	*packet >> serverID;
	auto ret = _serverID.insert(make_pair(sessionID, serverID));
	if (ret.second == false)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d] Already Login SessionID: %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
		::wprintf(L"%s[%d] Already Login SessionID: %lld\n", _T(__FUNCTION__), __LINE__, sessionID);
	}
}

void CLanMonitorServer::Handle_DATA_UPDATE(unsigned __int64 sessionID, CRecvLanPacket* packet)
{
	BYTE dataType;
	*packet >> dataType;

	switch (dataType)
	{

	// LOGIN ==========================================================

	case dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN:
		GetDataFromPacket(packet, &g_monitor._login.SERVER_RUN);
		break;

	case dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU:
		GetDataFromPacket(packet, &g_monitor._login.SERVER_CPU);
		break;

	case dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM:
		GetDataFromPacket(packet, &g_monitor._login.SERVER_MEM);
		break;

	case dfMONITOR_DATA_TYPE_LOGIN_SESSION:
		GetDataFromPacket(packet, &g_monitor._login.SESSION);
		break;

	case dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS:
		GetDataFromPacket(packet, &g_monitor._login.AUTH_TPS);
		break;

	case dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL:
		GetDataFromPacket(packet, &g_monitor._login.PACKET_POOL);
		break;

	// GAME ===========================================================

	case dfMONITOR_DATA_TYPE_GAME_SERVER_RUN:
		GetDataFromPacket(packet, &g_monitor._game.SERVER_RUN);
		break;

	case dfMONITOR_DATA_TYPE_GAME_SERVER_CPU:
		GetDataFromPacket(packet, &g_monitor._game.SERVER_CPU);
		break;

	case dfMONITOR_DATA_TYPE_GAME_SERVER_MEM:
		GetDataFromPacket(packet, &g_monitor._game.SERVER_MEM);
		break;

	case dfMONITOR_DATA_TYPE_GAME_SESSION:
		GetDataFromPacket(packet, &g_monitor._game.SESSION);
		break;

	case dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER:
		GetDataFromPacket(packet, &g_monitor._game.AUTH_PLAYER);
		break;

	case dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER:
		GetDataFromPacket(packet, &g_monitor._game.GAME_PLAYER);
		break;

	case dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS:
		GetDataFromPacket(packet, &g_monitor._game.ACCEPT_TPS);
		break;

	case dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS:
		GetDataFromPacket(packet, &g_monitor._game.PACKET_RECV_TPS);
		break;

	case dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS:
		GetDataFromPacket(packet, &g_monitor._game.PACKET_SEND_TPS);
		break;

	case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS:
		GetDataFromPacket(packet, &g_monitor._game.DB_WRITE_TPS);
		break;

	case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG:
		GetDataFromPacket(packet, &g_monitor._game.DB_WRITE_MSG);
		break;

	case dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS:
		GetDataFromPacket(packet, &g_monitor._game.AUTH_THREAD_FPS);
		break;

	case dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS:
		GetDataFromPacket(packet, &g_monitor._game.GAME_THREAD_FPS);
		break;

	case dfMONITOR_DATA_TYPE_GAME_PACKET_POOL:
		GetDataFromPacket(packet, &g_monitor._game.PACKET_POOL);
		break;

	// CHAT ===========================================================

	case dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN:
		GetDataFromPacket(packet, &g_monitor._chat.SERVER_RUN);
		break;

	case dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU:
		GetDataFromPacket(packet, &g_monitor._chat.SERVER_CPU);
		break;

	case dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM:
		GetDataFromPacket(packet, &g_monitor._chat.SERVER_MEM);
		break;

	case dfMONITOR_DATA_TYPE_CHAT_SESSION:
		GetDataFromPacket(packet, &g_monitor._chat.SESSION);
		break;

	case dfMONITOR_DATA_TYPE_CHAT_PLAYER:
		GetDataFromPacket(packet, &g_monitor._chat.PLAYER);
		break;

	case dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS:
		GetDataFromPacket(packet, &g_monitor._chat.UPDATE_TPS);
		break;

	case dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL:
		GetDataFromPacket(packet, &g_monitor._chat.PACKET_POOL);
		break;

	case dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL:
		GetDataFromPacket(packet, &g_monitor._chat.UPDATEMSG_POOL);
		break;

	// SERVER ===========================================================

	case dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL:
		GetDataFromPacket(packet, &g_monitor._server.CPU_TOTAL);
		break;

	case dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY:
		GetDataFromPacket(packet, &g_monitor._server.NONPAGED_MEMORY);
		break;

	case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV:
		GetDataFromPacket(packet, &g_monitor._server.NETWORK_RECV);
		break;

	case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND:
		GetDataFromPacket(packet, &g_monitor._server.NETWORK_SEND);
		break;

	case dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY:
		GetDataFromPacket(packet, &g_monitor._server.AVAILABLE_MEMORY);
		break;

	default:
		Disconnect(sessionID);
		LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] Undefined Data Type, %d\n", _T(__FUNCTION__), __LINE__, dataType);
		::wprintf(L"%s[%d] Undefined Data Type, %d\n", _T(__FUNCTION__), __LINE__, dataType);
		break;
	}
}

void CLanMonitorServer::GetDataFromPacket(CRecvLanPacket* packet, CData* data)
{
	long val, timestamp;
	*packet >> val;
	*packet >> timestamp;
	InterlockedExchange(&data->_val, val);
	InterlockedExchange(&data->_timestamp, timestamp);
}
