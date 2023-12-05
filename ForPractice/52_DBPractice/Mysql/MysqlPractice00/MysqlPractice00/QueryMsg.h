#pragma once
#include <stdio.h>
#define dfQUERY_MAX 50

#define dfUSERID_LEN	64
#define dfUSERPASS_LEN	128
#define dfUSERNICK_LEN	64

MYSQL conn;
MYSQL* connection = NULL;

class QueryMsg
{
public:
	virtual void Execute() = 0;
};

class QueryMsg_GetData : public QueryMsg
{
public:
	QueryMsg_GetData(int accountNo) : _accountNo(accountNo) {};

public:
	void Execute()
	{
		int query_stat;
		MYSQL_ROW sql_row;
		MYSQL_RES* sql_result;

		char query[dfQUERY_MAX] = { 0 };
		int ret = sprintf_s(query, dfQUERY_MAX,
			"SELECT * FROM account WHERE accountno = %d", _accountNo);

		query_stat = mysql_query(connection, query);
		if (query_stat != 0)
		{
			printf("Mysql query error : %s (%d)\n", mysql_error(&conn), ret);
			return;
		}

		sql_result = mysql_store_result(connection);
		while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
		{
			// Compare User Data
		}
		mysql_free_result(sql_result);
	}

public:
	int _accountNo;
	// User* user;
};
class QueryMsg_SetData : public QueryMsg
{
public:
	QueryMsg_SetData(int accountNo) : _accountNo(accountNo) {};

public:
	void Execute()
	{
		char query[dfQUERY_MAX];
		int ret = sprintf_s(query, dfQUERY_MAX, 
			"INSERT INTO account VALUES (%d, 'a', 'aa', 'aaa')", _accountNo);

		printf("%s\n", query);
		int query_stat = mysql_query(connection, query);
		if (query_stat != 0)
		{
			printf("Mysql query error : %s (%d)\n", mysql_error(&conn), ret);
			__debugbreak();
		}
	}

public:
	int _accountNo;
};