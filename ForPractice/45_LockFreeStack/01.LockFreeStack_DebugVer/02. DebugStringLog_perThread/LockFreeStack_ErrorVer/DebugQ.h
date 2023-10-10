#pragma once

#include <chrono>
#include <iostream>
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <unordered_map>
using namespace std;
using namespace chrono;

class DebugQ;
class DebugQManager;
extern DebugQManager _debugQManager;
extern system_clock::time_point g_Start;

#define dfQUEUE_MAX 10
#define dfTHREAD_MAX 16
#define dfPRINT_BUFFER_SIZE 64
#define dfLOG_BUFFER_SIZE 64
#define dfOUTPUT_SIZE (dfQUEUE_MAX * dfTHREAD_MAX * dfLOG_BUFFER_SIZE)
#define dfFILENAME_MAX 128

class DebugData
{
	friend DebugQ;
	friend DebugQManager;

private:
	DebugData() : _cnt(-1), _line(-1), _ns(-1), _exchange(-1), _comperand(-1) {}
	DebugData(int cnt, int line, __int64 ns, __int64 exchange, __int64 comperand)
		: _cnt(cnt), _line(line), _ns(ns), _exchange(exchange), _comperand(comperand) {}

private:
	int _cnt;
	int _line;
	__int64 _ns;
	__int64 _exchange;
	__int64 _comperand;
};

class LogData
{
	friend DebugQManager;

private:
	LogData() : _threadID(-1), _line(-1), _ns(INT64_MAX), _exchange(-1), _comperand(-1) {}
	void Initialize(int threadID, int line, __int64 ns, __int64 exchange, __int64 comperand)
	{
		_threadID = threadID; 
		_line = line; 
		_ns = ns;
		_exchange = exchange;
		_comperand = comperand;
	}

public:
	bool operator < (const LogData& other) const {
		return _ns < other._ns;
	}

private:
	int _threadID;
	int _line;
	__int64 _ns;
	__int64 _exchange;
	__int64 _comperand;
};

class DebugQ
{
	friend DebugQManager;

private:
	DebugQ()
	{
		InitializeSRWLock(&_lock);
	}

	void Initialize(int threadID) 
	{
		_threadID = threadID;
	}

private:
	int _num = 0;
	int _threadID = -1;
	SRWLOCK _lock;
	DebugData _queue[dfQUEUE_MAX];
	
private:
	void EnqueueLog(int line, __int64 ns, __int64 exchange, __int64 comperand);
};

class DebugQManager
{
public:
	DebugQManager()
	{
		InitializeSRWLock(&_lock);
	}

public:
	void SetDebugQ();
	inline void PrintLog(int line, __int64 exchange, __int64 comperand)
	{
		char buffer[dfPRINT_BUFFER_SIZE] = { '\0' };
		sprintf_s(buffer, dfPRINT_BUFFER_SIZE, "%d(%d): %llx->%llx\n", 
			GetCurrentThreadId(), line, comperand, exchange);
		OutputDebugStringA(buffer);
	}

	void EnqueueLog(int line, __int64 ns, __int64 exchange, __int64 comperand);
	void SaveLog(int now); // Only Called in CrashDump

private:
	bool _stop = false;
	int _IdxManager = 0;

private:
	SRWLOCK _lock;	
	DebugQ _debugQArray[dfTHREAD_MAX];
	unordered_map<DWORD, int> _threadMap;
	
private: // Used in SaveLog
	LogData _logDataArray[(dfTHREAD_MAX * dfQUEUE_MAX)];
	char _stackLogData[dfOUTPUT_SIZE] = { '\0' };
	char _timelineData[dfOUTPUT_SIZE] = { '\0' };
};

