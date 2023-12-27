#include "CNetMonitorServer.h"
#include "MonitorProtocol.h"
#include "CSystemLog.h"
#include <tchar.h>
#include <time.h>

#define __SAVELOG__

bool CNetMonitorServer::Initialize()
{
	// Set Network Library

	int totalThreadCnt = 0;
	int runningThreadCnt = 0;
	::wprintf(L"<Net Server>\n");
	::wprintf(L"Total Network Thread Count: ");
	::scanf_s("%d", &totalThreadCnt);
	::wprintf(L"Running Network Thread Count: ");
	::scanf_s("%d", &runningThreadCnt);

	if (!NetworkInitialize(dfSERVER_IP, dfNETSERVER_PORT, totalThreadCnt, runningThreadCnt, false, false))
	{
		Terminate();
		return false;
	}

	_jobThread = (HANDLE)_beginthreadex(NULL, 0, JobThread, this, 0, nullptr);
	if (_jobThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Job Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Job Thread Error\n", _T(__FUNCTION__), __LINE__);
	}

	_sendThread = (HANDLE)_beginthreadex(NULL, 0, SendThread, this, 0, nullptr);
	if (_sendThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Send Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Send Thread Error\n", _T(__FUNCTION__), __LINE__);
	}

	_logThread = (HANDLE)_beginthreadex(NULL, 0, SaveThread, this, 0, nullptr);
	if (_logThread == NULL)
	{
		LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"%s[%d]: Begin Send Thread Error\n", _T(__FUNCTION__), __LINE__);
		::wprintf(L"%s[%d]: Begin Send Thread Error\n", _T(__FUNCTION__), __LINE__);
	}

	// Initialize Compelete
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Net: Server Initialize\n");
	::wprintf(L"Net: Server Initialize\n\n");
}

void CNetMonitorServer::Terminate()
{
	// TO-DO
}

void CNetMonitorServer::OnAcceptClient(unsigned __int64 sessionID)
{
	CNetJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::ACCEPT, sessionID, nullptr);
	EnqueueJob(sessionID, job);

	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
}

void CNetMonitorServer::OnReleaseClient(unsigned __int64 sessionID)
{
	CNetJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::SYSTEM, SYS_TYPE::RELEASE, sessionID, nullptr);
	EnqueueJob(sessionID, job);

	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
}

void CNetMonitorServer::OnRecv(unsigned __int64 sessionID, CRecvNetPacket* packet)
{
	CNetJob* job = _pJobPool->Alloc();
	job->Setting(JOB_TYPE::CONTENT, SYS_TYPE::NONE, sessionID, packet);
	EnqueueJob(sessionID, job);

	InterlockedIncrement(&_signal);
	WakeByAddressSingle(&_signal);
}

unsigned int __stdcall CNetMonitorServer::JobThread(void* arg)
{
	CNetMonitorServer* pServer = (CNetMonitorServer*)arg;
	CLockFreeQueue<CNetJob*>** pJobQueues = pServer->_pJobQueues;
	long undesired = 0;

	while (pServer->_serverAlive)
	{
		WaitOnAddress(&pServer->_signal, &undesired, sizeof(long), INFINITE);

		for (int i = 0; i < dfJOB_QUEUE_CNT; i++)
		{
			while (pJobQueues[i]->GetUseSize() > 0)
			{
				CNetJob* job = pJobQueues[i]->Dequeue();
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

void CNetMonitorServer::HandleAccept(unsigned __int64 sessionID)
{
	_sessions.push_back(sessionID);
}

void CNetMonitorServer::HandleRelease(unsigned __int64 sessionID)
{
	vector<unsigned __int64>::iterator it = _sessions.begin();
	for (; it < _sessions.end(); it++)
	{
		if (sessionID == *it)
		{
			_sessions.erase(it);
			break;
		}
	}
}

void CNetMonitorServer::HandleRecv(unsigned __int64 sessionID, CRecvNetPacket* packet)
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
			LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"%s[%d] NetPacket Error\n", _T(__FUNCTION__), __LINE__);
			::wprintf(L"%s[%d] NetPacket Error\n", _T(__FUNCTION__), __LINE__);
		}
	}
	CRecvNetPacket::Free(packet);
}

unsigned int __stdcall CNetMonitorServer::SaveThread(void* arg)
{
	CNetMonitorServer* pServer = (CNetMonitorServer*)arg;
	pServer->_saveOldTick = timeGetTime();

	pServer->InitializeDB();
	while (pServer->_serverAlive)
	{
		pServer->SleepForSave();
		pServer->SaveDatasToDB();
	}
	return 0;
}

void CNetMonitorServer::SleepForSave()
{
	if ((timeGetTime() - _saveOldTick) < _saveTimeGap)
		Sleep(_saveTimeGap - (timeGetTime() - _saveOldTick));
	_saveOldTick += _saveTimeGap;
}

void CNetMonitorServer::InitializeDB()
{
	mysql_init(&_conn);
	_connection = mysql_real_connect(&_conn,
		"127.0.0.1", "root", "password", "logdb", 3306, (char*)NULL, 0);
	if (_connection == NULL)
	{
		LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Mysql connection error : %s", mysql_error(&_conn));
		::printf("Mysql connection error : %s", mysql_error(&_conn));
	}
}

void CNetMonitorServer::SaveDatasToDB()
{
	SYSTEMTIME stTime;
	GetLocalTime(&stTime);

	// Login
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, dfLOGIN_NO, &g_monitor._login.SERVER_RUN);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, dfLOGIN_NO, &g_monitor._login.SERVER_CPU);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, dfLOGIN_NO, &g_monitor._login.SERVER_MEM);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_LOGIN_SESSION, dfLOGIN_NO, &g_monitor._login.SESSION);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, dfLOGIN_NO, &g_monitor._login.AUTH_TPS);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, dfLOGIN_NO, &g_monitor._login.PACKET_POOL);

	// Game
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, dfGAME_NO, &g_monitor._game.SERVER_RUN);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, dfGAME_NO, &g_monitor._game.SERVER_CPU);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, dfGAME_NO, &g_monitor._game.SERVER_MEM);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_SESSION, dfGAME_NO, &g_monitor._game.SESSION);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER, dfGAME_NO, &g_monitor._game.AUTH_PLAYER);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER, dfGAME_NO, &g_monitor._game.GAME_PLAYER);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS, dfGAME_NO, &g_monitor._game.ACCEPT_TPS);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS, dfGAME_NO, &g_monitor._game.PACKET_RECV_TPS);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS, dfGAME_NO, &g_monitor._game.PACKET_SEND_TPS);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS, dfGAME_NO, &g_monitor._game.DB_WRITE_TPS);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG, dfGAME_NO, &g_monitor._game.DB_WRITE_MSG);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS, dfGAME_NO, &g_monitor._game.AUTH_THREAD_FPS);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS, dfGAME_NO, &g_monitor._game.GAME_THREAD_FPS);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_GAME_PACKET_POOL, dfGAME_NO, &g_monitor._game.PACKET_POOL);

	// Chat
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, dfCHAT_NO, &g_monitor._chat.SERVER_RUN);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, dfCHAT_NO, &g_monitor._chat.SERVER_CPU);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, dfCHAT_NO, &g_monitor._chat.SERVER_MEM);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_CHAT_SESSION, dfCHAT_NO, &g_monitor._chat.SESSION);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_CHAT_PLAYER, dfCHAT_NO, &g_monitor._chat.PLAYER);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, dfCHAT_NO, &g_monitor._chat.UPDATE_TPS);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, dfCHAT_NO, &g_monitor._chat.PACKET_POOL);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, dfCHAT_NO, &g_monitor._chat.UPDATEMSG_POOL);

	// Server
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, dfSERVER_NO, &g_monitor._server.CPU_TOTAL);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, dfSERVER_NO, &g_monitor._server.NONPAGED_MEMORY);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, dfSERVER_NO, &g_monitor._server.NETWORK_RECV);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, dfSERVER_NO, &g_monitor._server.NETWORK_SEND);
	SaveDataToDB(stTime, dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, dfSERVER_NO, &g_monitor._server.AVAILABLE_MEMORY);

	::printf("[%d-%02d-%02d %02d:%02d:%02d] Save Log To DB Success\n",
		stTime.wYear, stTime.wMonth, stTime.wDay, stTime.wHour, stTime.wMinute, stTime.wSecond);
}

void CNetMonitorServer::SaveDataToDB(SYSTEMTIME stTime, BYTE type, BYTE serverNo, CData* data)
{
	long sum = InterlockedExchange(&data->_sum, 0);
	long call = InterlockedExchange(&data->_call, 0);
	long min = InterlockedExchange(&data->_min, LONG_MAX);
	long max = InterlockedExchange(&data->_max, LONG_MIN);

	char time[dfTIME_LEN] = { 0, };
	sprintf_s(time, "%d-%02d-%02d %02d:%02d:%02d",
		stTime.wYear, stTime.wMonth, stTime.wDay,
		stTime.wHour, stTime.wMinute, stTime.wSecond);

	char table[dfTABLE_LEN] = { 0, };
	sprintf_s(table, "monitorlog_%d%d", stTime.wYear, stTime.wMonth);

	char query[dfQUERY_MAX] = { 0, };
	if (call == 0)
	{
		sprintf_s(query, dfQUERY_MAX,
			"INSERT INTO %s (logtime, serverno, type, avr, min, max) VALUES (\"%s\", % d, % d, % d, % d, % d)",
			table, time, serverNo, type, 0, 0, 0);
	}
	else
	{
		sprintf_s(query, dfQUERY_MAX,
			"INSERT INTO %s (logtime, serverno, type, avr, min, max) VALUES (\"%s\", %d, %d, %d, %d, %d)",
			table, time, serverNo, type, sum / call, min, max);
	}

	int query_stat = mysql_query(_connection, query);
	if (query_stat != 0)
	{
		if (mysql_errno(_connection) == 1146)
		{
			char createQuery[dfQUERY_MAX] = { 0, };

			sprintf_s(createQuery, "CREATE TABLE %s LIKE monitorlog", table);
			mysql_query(_connection, createQuery);
			mysql_query(_connection, query);
		}
		else
		{
			LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Mysql query error : %s\n", mysql_error(&_conn));
			::printf("Mysql query error : %s\n", mysql_error(&_conn));
		}
	}
}

unsigned int __stdcall CNetMonitorServer::SendThread(void* arg)
{
	CNetMonitorServer* pServer = (CNetMonitorServer*)arg;
	CLockFreeQueue<CNetJob*>** pJobQueues = pServer->_pJobQueues;
	pServer->_sendOldTick = timeGetTime();

	while (pServer->_serverAlive)
	{
		pServer->SleepForSend();
		vector<unsigned __int64>::iterator it = pServer->_sessions.begin();
		for (; it < pServer->_sessions.end(); it++)
		{
			pServer->SendDatasToClient((*it));
		}
	}
	return 0;
}


void CNetMonitorServer::SleepForSend()
{
	if ((timeGetTime() - _sendOldTick) < _sendTimeGap)
		Sleep(_sendTimeGap - (timeGetTime() - _sendOldTick));
	_sendOldTick += _sendTimeGap;
}

void CNetMonitorServer::ReqSendUnicast(unsigned __int64 sessionID, CNetPacket* packet)
{
	packet->AddUsageCount(1);
	if (!SendPacket(sessionID, packet))
	{
		CNetPacket::Free(packet);
	}
}

void CNetMonitorServer::SendDatasToClient(unsigned __int64 sessionID)
{
	// Login
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN , dfLOGIN_NO, &g_monitor._login.SERVER_RUN);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU , dfLOGIN_NO, &g_monitor._login.SERVER_CPU);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM , dfLOGIN_NO, &g_monitor._login.SERVER_MEM);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_LOGIN_SESSION , dfLOGIN_NO, &g_monitor._login.SESSION);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS , dfLOGIN_NO, &g_monitor._login.AUTH_TPS);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL , dfLOGIN_NO, &g_monitor._login.PACKET_POOL);
	
	// Game
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_SERVER_RUN , dfGAME_NO, &g_monitor._game.SERVER_RUN);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_SERVER_CPU , dfGAME_NO, &g_monitor._game.SERVER_CPU);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_SERVER_MEM , dfGAME_NO, &g_monitor._game.SERVER_MEM);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_SESSION , dfGAME_NO, &g_monitor._game.SESSION);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER , dfGAME_NO, &g_monitor._game.AUTH_PLAYER);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER , dfGAME_NO, &g_monitor._game.GAME_PLAYER);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS , dfGAME_NO, &g_monitor._game.ACCEPT_TPS);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS , dfGAME_NO, &g_monitor._game.PACKET_RECV_TPS);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS , dfGAME_NO, &g_monitor._game.PACKET_SEND_TPS);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS, dfGAME_NO, &g_monitor._game.DB_WRITE_TPS);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG , dfGAME_NO, &g_monitor._game.DB_WRITE_MSG);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS , dfGAME_NO, &g_monitor._game.AUTH_THREAD_FPS);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS , dfGAME_NO, &g_monitor._game.GAME_THREAD_FPS);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_GAME_PACKET_POOL , dfGAME_NO, &g_monitor._game.PACKET_POOL);
	
	// Chat
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN , dfCHAT_NO, &g_monitor._chat.SERVER_RUN);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU , dfCHAT_NO, &g_monitor._chat.SERVER_CPU);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM , dfCHAT_NO, &g_monitor._chat.SERVER_MEM);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_CHAT_SESSION , dfCHAT_NO, &g_monitor._chat.SESSION);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_CHAT_PLAYER , dfCHAT_NO, &g_monitor._chat.PLAYER);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS , dfCHAT_NO, &g_monitor._chat.UPDATE_TPS);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL , dfCHAT_NO, &g_monitor._chat.PACKET_POOL);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL , dfCHAT_NO, &g_monitor._chat.UPDATEMSG_POOL);
	
	// Server
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, dfSERVER_NO, &g_monitor._server.CPU_TOTAL);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY , dfSERVER_NO, &g_monitor._server.NONPAGED_MEMORY);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV , dfSERVER_NO, &g_monitor._server.NETWORK_RECV);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND , dfSERVER_NO, &g_monitor._server.NETWORK_SEND);
	SendDataToClient(sessionID, dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY , dfSERVER_NO, &g_monitor._server.AVAILABLE_MEMORY);
}

void CNetMonitorServer::SendDataToClient(unsigned __int64 sessionID, BYTE type, BYTE serverNo, CData* data)
{
	long val = InterlockedExchange(&data->_val, -1);
	long timestamp = InterlockedExchange(&data->_timestamp, -1);
	if (val == -1 || timestamp == -1) return;

	InterlockedAdd(&data->_sum, val);
	InterlockedIncrement(&data->_call);
	if (data->_min > val) data->_min = val;
	if (data->_max < val) data->_max = val;

	CNetPacket* packet = CNetPacket::Alloc();
	packet->Clear();
	*packet << (WORD) en_PACKET_SC_MONITOR_TOOL_DATA_UPDATE;
	*packet << serverNo;
	*packet << type;
	*packet << val;
	*packet << timestamp;

	// printf("%d, %d\n", val, timestamp);
	ReqSendUnicast(sessionID, packet);
}

void CNetMonitorServer::OnInitialize()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Net: Network Library Initialize\n");
	::wprintf(L"Net: Network Library Initialize\n");
}

void CNetMonitorServer::OnTerminate()
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Net: Network Library Terminate\n");
	::wprintf(L"Net: Network Library Terminate\n");
}

void CNetMonitorServer::OnThreadTerminate(wchar_t* threadName)
{
	LOG(L"FightGame", CSystemLog::SYSTEM_LEVEL, L"Net: %s Terminate\n", threadName);
	::wprintf(L"Net: %s Terminate\n", threadName);
}

void CNetMonitorServer::OnError(int errorCode, wchar_t* errorMsg)
{
	LOG(L"FightGame", CSystemLog::ERROR_LEVEL, L"Net: %s (%d)\n", errorMsg, errorCode);
	::wprintf(L"Net: %s (%d)\n", errorMsg, errorCode);
}

void CNetMonitorServer::OnDebug(int debugCode, wchar_t* debugMsg)
{
	LOG(L"FightGame", CSystemLog::DEBUG_LEVEL, L"Net: %s (%d)\n", debugMsg, debugCode);
	::wprintf(L"Net: %s (%d)\n", debugMsg, debugCode);
}

bool CNetMonitorServer::OnConnectRequest()
{
	return false;
}

void CNetMonitorServer::OnSend(unsigned __int64 sessionID, int sendSize)
{
}

void CNetMonitorServer::Handle_REQ_LOGIN(unsigned __int64 sessionID, CRecvNetPacket* packet)
{
	char LoginSessionKey[32];
	packet->GetPayloadData(LoginSessionKey, sizeof(LoginSessionKey));

	BYTE status = dfMONITOR_TOOL_LOGIN_OK;
	if (strcmp(LoginSessionKey, dfSESSION_KEY) != 0)
	{
		status = dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY;
	}

	/*
	if (status == dfMONITOR_TOOL_LOGIN_OK)
	{
		_sessions.push_back(sessionID);
	}
	*/

	CNetPacket* sendPacket = CNetPacket::Alloc();
	sendPacket->Clear();
	*sendPacket << (WORD)en_PACKET_SC_MONITOR_TOOL_RES_LOGIN;
	*sendPacket << status;
	ReqSendUnicast(sessionID, sendPacket);
}

