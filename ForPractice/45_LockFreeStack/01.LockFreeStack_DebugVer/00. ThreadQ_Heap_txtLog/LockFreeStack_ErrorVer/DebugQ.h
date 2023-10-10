#pragma once

#include <chrono>
#include <iostream>
#include <windows.h>
#include <process.h>
#include <stdio.h>

#include <set>
#include <vector>
#include <unordered_map>
using namespace std;
using namespace chrono;

extern system_clock::time_point g_Start;

#define dfQUEUE_MAX 10
#define dfOUTPUT_SIZE 9182
#define dfBUFFER_SIZE 128
#define dfFILENAME_MAX 128

class DebugData
{
public:
	DebugData() : _idx(-1), _line(-1), _ns(-1) {}
	DebugData(int idx, int line, __int64 ns)
		: _idx(idx), _line(line), _ns(ns) {}

public:
	int _idx;
	int _line;
	__int64 _ns;
};

class LogData
{
public:
	LogData() : _threadID(-1), _idx(-1), _line(-1), _ns(-1) {}
	LogData(int threadID, DebugData debugData)
		: _threadID(threadID), _idx(debugData._idx), _line(debugData._line), _ns(debugData._ns) {}

public:
	bool operator == (const LogData& other) const {
		return _ns == other._ns;
	}
	bool operator != (const LogData& other) const {
		return _ns != other._ns;
	}
	bool operator < (const LogData& other) const {
		return _ns < other._ns;
	}
	bool operator > (const LogData& other) const {
		return _ns > other._ns;
	}

public:
	int _threadID;
	int _idx;
	int _line;
	__int64 _ns;
};

class DebugQManager;
class DebugQ
{
	friend DebugQManager;

private:
	DebugQ(int threadID) : _threadID(threadID)
	{
		InitializeSRWLock(&_lock);
	}

private:
	DebugData _queue[dfQUEUE_MAX];
	int _num = 0;
	int _threadID;
	SRWLOCK _lock;

private:
	void PushData(int line, __int64 ns);
	void PrintData();
};

class DebugQManager
{
private:
	DebugQManager()
	{
		InitializeSRWLock(&_lock);
	}
	~DebugQManager() {}
	static DebugQManager _manager;

public:
	static DebugQManager* GetInstance()
	{
		static DebugQManager _manager;
		return  &_manager;
	}

public:
	void SaveDebugLog(int now); // Only Called in CrashDump

public:
	void SetDebugQ()
	{
		if (_terminate) return;

		AcquireSRWLockExclusive(&_lock);
		_threadMap.insert(make_pair(GetCurrentThreadId(), _IdxManager));
		DebugQ* debugQ = new DebugQ(GetCurrentThreadId());
		_debugQs.push_back(debugQ);
		_IdxManager++;
		ReleaseSRWLockExclusive(&_lock);
	}

	inline void PushData(int line, __int64 ns)
	{
		if (_terminate) return;

		AcquireSRWLockShared(&_lock);
		unordered_map<DWORD, int>::iterator iter = _threadMap.find(GetCurrentThreadId());
		if(iter != _threadMap.end())
		{
			ReleaseSRWLockShared(&_lock);
			int idx = iter->second;
			_debugQs[idx]->PushData(line, ns);
		}
		else
		{
			ReleaseSRWLockShared(&_lock);
			::printf("No such data: %d\n", GetCurrentThreadId());
		}
	}
	inline void PrintData()
	{
		if (_terminate) return;

		AcquireSRWLockShared(&_lock);
		unordered_map<DWORD, int>::iterator iter = _threadMap.find(GetCurrentThreadId());
		if (iter != _threadMap.end())
		{
			ReleaseSRWLockShared(&_lock);
			int idx = iter->second;
			_debugQs[idx]->PrintData();
		}
		else
		{
			ReleaseSRWLockShared(&_lock);
			::printf("No such data: %d\n", GetCurrentThreadId());
		}
	}

private:
	bool _terminate = false; 
	SRWLOCK _lock;
	vector<DebugQ*> _debugQs;
	unordered_map<DWORD, int> _threadMap;
	int _IdxManager = 0;

private:
	multiset<LogData> _logDataSet;
	char _stackLogData[dfOUTPUT_SIZE] = { '\0' };
	char _timelineData[dfOUTPUT_SIZE] = { '\0' };
};
