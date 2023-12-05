#include <winsock2.h>
#include <process.h>
#include <synchapi.h>
#include "mysql.h"
#include "errmsg.h"

#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "libmysql.lib")
#pragma comment(lib, "Synchronization.lib")

#include "QueryMsg.h"
#include "CLockFreeQueue.h"

void SingleTest();
void MultiTest();
unsigned int __stdcall UpdateThread(void* arg);
unsigned int __stdcall DBWriterThread(void* arg);

bool g_fin = false;
long g_msgCnt = 0; // TO-DO
CLockFreeQueue<QueryMsg_SetData*>* g_Queue;

int main()
{
	g_Queue = new CLockFreeQueue<QueryMsg_SetData*>;
	MultiTest();
	delete g_Queue;
	return 0;
}

void SingleTest()
{
	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	int query_stat;

	// DB 연결
	mysql_init(&conn);
	connection = mysql_real_connect(&conn, "127.0.0.1", "root", "password", "testdb", 3306, (char*)NULL, 0);
	if (connection == NULL)
	{
		::printf("Mysql connection error : %s", mysql_error(&conn));
		return;
	}

	// Select 쿼리
	char query[dfQUERY_MAX] = "SELECT * FROM account";
	query_stat = mysql_query(connection, query);
	if (query_stat != 0)
	{
		printf("Mysql query error : %s", mysql_error(&conn));
		return;
	}

	// 결과출력
	sql_result = mysql_store_result(connection);
	while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
	{
		printf("%2s %2s %s\n", sql_row[0], sql_row[1], sql_row[2]);
	}

	mysql_free_result(sql_result);
	mysql_close(connection);
}
void MultiTest()
{
	// DB 연결
	mysql_init(&conn);
	connection = mysql_real_connect(&conn, "127.0.0.1", "root", "password", "testdb", 3306, (char*)NULL, 0);
	if (connection == NULL)
	{
		::printf("Mysql connection error : %s", mysql_error(&conn));
		return;
	}

	HANDLE updateThread0 = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, (void*)0, 0, nullptr);
	if (updateThread0 == NULL)
	{
		::wprintf(L"Begin Update Thread 0 Error\n");
		return;
	}

	HANDLE updateThread1 = (HANDLE)_beginthreadex(NULL, 0, UpdateThread, (void*)1, 0, nullptr);
	if (updateThread1 == NULL)
	{
		::wprintf(L"Begin Update Thread 1 Error\n");
		return;
	}

	HANDLE dbWriterThread = (HANDLE)_beginthreadex(NULL, 0, DBWriterThread, nullptr, 0, nullptr);
	if (dbWriterThread == NULL)
	{
		::wprintf(L"Begin DBWriter Thread Error\n");
		return;
	}

	WaitForSingleObject(updateThread0, INFINITE);
	WaitForSingleObject(updateThread1, INFINITE);

	g_fin = true;
	WaitForSingleObject(dbWriterThread, INFINITE);

	mysql_close(connection);
}

#define dfTEST_MAX 50
unsigned int __stdcall UpdateThread(void* arg)
{
	int idx = (int)arg;
	int start = (dfTEST_MAX * idx);
	int end = (dfTEST_MAX * (idx + 1));

	for (int i = start; i < end; i++)
	{
		QueryMsg_SetData* msg = new QueryMsg_SetData(i);
		g_Queue->Enqueue(msg);
		InterlockedIncrement(&g_msgCnt);
		WakeByAddressSingle(&g_msgCnt);
	}
	return 0;
}

unsigned int __stdcall DBWriterThread(void* arg)
{
	long undesired = 0;
	while(!g_fin)
	{
		WaitOnAddress(&g_msgCnt, &undesired, sizeof(long), INFINITE);
		while (g_Queue->GetUseSize() > 0)
		{
			QueryMsg_SetData* msg = g_Queue->Dequeue();
			if (msg == nullptr) break;
			msg->Execute();
			delete msg;
			InterlockedDecrement(&g_msgCnt);
		}
	}

	return 0;
}
