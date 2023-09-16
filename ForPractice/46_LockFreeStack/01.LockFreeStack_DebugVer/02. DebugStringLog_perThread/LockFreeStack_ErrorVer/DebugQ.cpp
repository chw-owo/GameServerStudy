#include "DebugQ.h"
#include <algorithm>

DebugQManager _debugQManager;
system_clock::time_point g_Start = system_clock::now();

void DebugQ::EnqueueLog(int line, __int64 ns, __int64 exchange, __int64 comperand)
{
	AcquireSRWLockExclusive(&_lock);
	_queue[(_num % dfQUEUE_MAX)]._cnt = _num;
	_queue[(_num % dfQUEUE_MAX)]._line = line;
	_queue[(_num % dfQUEUE_MAX)]._ns = ns;
	_queue[(_num % dfQUEUE_MAX)]._exchange = exchange;
	_queue[(_num % dfQUEUE_MAX)]._comperand = comperand;
	_num++;
	ReleaseSRWLockExclusive(&_lock);
}

void DebugQManager::SetDebugQ()
{
	if (_stop) return;

	AcquireSRWLockExclusive(&_lock);
	_threadMap.insert(make_pair(GetCurrentThreadId(), _IdxManager));
	_debugQArray[_IdxManager].Initialize(GetCurrentThreadId());
	_IdxManager++;
	ReleaseSRWLockExclusive(&_lock);
}

void DebugQManager::EnqueueLog(int line, __int64 ns, __int64 exchange, __int64 comperand)
{
	if (_stop) return;

	AcquireSRWLockShared(&_lock);
	unordered_map<DWORD, int>::iterator iter = _threadMap.find(GetCurrentThreadId());
	if (iter != _threadMap.end())
	{
		ReleaseSRWLockShared(&_lock);
		int idx = iter->second;
		_debugQArray[idx].EnqueueLog(line, ns, exchange, comperand);
	}
	else
	{
		ReleaseSRWLockShared(&_lock);
		::printf("No such data: %d\n", GetCurrentThreadId());
	}
}

void DebugQManager::SaveLog(int now)
{
	if (_stop) return;
	_stop = true;
	AcquireSRWLockExclusive(&_lock);

	memset(_stackLogData, dfOUTPUT_SIZE, '\0');
	char buffer[dfLOG_BUFFER_SIZE] = { '\0' };

	for (int i = 0; i < _threadMap.size(); i++)
	{
		DebugQ Q = _debugQArray[i];

		AcquireSRWLockShared(&Q._lock);
		sprintf_s(buffer, dfLOG_BUFFER_SIZE, "\nThread ID: %d\n", Q._threadID);
		strcat_s(_stackLogData, dfOUTPUT_SIZE, buffer);
		
		int num = Q._num;
		for (int j = 0; j < dfQUEUE_MAX; j++)
		{
			if (Q._queue[((num + j) % dfQUEUE_MAX)]._ns <= now)
			{
				memset(buffer, dfLOG_BUFFER_SIZE, '\0');
				sprintf_s(buffer, dfLOG_BUFFER_SIZE, "%d, %d, %lld, %llx, %llx\n",
					Q._queue[((num + j) % dfQUEUE_MAX)]._cnt,
					Q._queue[((num + j) % dfQUEUE_MAX)]._line,
					Q._queue[((num + j) % dfQUEUE_MAX)]._ns,
					Q._queue[((num + j) % dfQUEUE_MAX)]._exchange,
					Q._queue[((num + j) % dfQUEUE_MAX)]._comperand);
				strcat_s(_stackLogData, dfOUTPUT_SIZE, buffer);
			}
		}
		ReleaseSRWLockShared(&Q._lock);
	}

	SYSTEMTIME stTime;
	GetLocalTime(&stTime);

	WCHAR stackLog[dfFILENAME_MAX] = { '\0', };
	swprintf_s(stackLog, dfFILENAME_MAX, L"StackLog_%d%02d%02d_%02d.%02d.%02d.txt",
		stTime.wYear, stTime.wMonth, stTime.wDay,
		stTime.wHour, stTime.wMinute, stTime.wSecond);

	FILE* stackLogFile;
	errno_t stackLogOpenRet = _wfopen_s(&stackLogFile, stackLog, L"wb");
	if (stackLogOpenRet != 0)
		::wprintf(L"Fail to open %s : %d\n", stackLog, stackLogOpenRet);
	::fwrite(_stackLogData, strlen(_stackLogData), 1, stackLogFile);
	::fclose(stackLogFile);

	int logDataIdx = 0;
	for (int i = 0; i < _threadMap.size(); i++)
	{
		DebugQ Q = _debugQArray[i];
		AcquireSRWLockShared(&Q._lock);
		int num = Q._num;
		for (int j = 0; j < dfQUEUE_MAX; j++)
		{
			if (Q._queue[((num + j) % dfQUEUE_MAX)]._ns <= now)
			{
				_logDataArray[logDataIdx].Initialize(Q._threadID,
					Q._queue[((num + j) % dfQUEUE_MAX)]._line,
					Q._queue[((num + j) % dfQUEUE_MAX)]._ns,
					Q._queue[((num + j) % dfQUEUE_MAX)]._exchange,
					Q._queue[((num + j) % dfQUEUE_MAX)]._comperand);
				logDataIdx++;
			}
		}
		ReleaseSRWLockShared(&Q._lock);
	}

	sort(_logDataArray, _logDataArray + logDataIdx);

	memset(_timelineData, dfOUTPUT_SIZE, '\0');
	for (int i = 0; i < logDataIdx; i++)
	{
		LogData logData = _logDataArray[i];
		memset(buffer, dfLOG_BUFFER_SIZE, '\0');
		sprintf_s(buffer, dfLOG_BUFFER_SIZE, "%d, %d, %lld, %llx, %llx\n",
			logData._threadID, logData._line, logData._ns, logData._exchange, logData._comperand);
		strcat_s(_timelineData, dfOUTPUT_SIZE, buffer);
	}

	WCHAR timelineFilename[dfFILENAME_MAX] = { '\0', };
	swprintf_s(timelineFilename, dfFILENAME_MAX, L"StackLogTimeline_%d%02d%02d_%02d.%02d.%02d.txt",
		stTime.wYear, stTime.wMonth, stTime.wDay,
		stTime.wHour, stTime.wMinute, stTime.wSecond);

	FILE* timelineFile;
	errno_t timelineRet = _wfopen_s(&timelineFile, timelineFilename, L"wb");
	if (timelineRet != 0)
		::wprintf(L"Fail to open %s : %d\n", timelineFilename, timelineRet);
	::fwrite(_timelineData, strlen(_timelineData), 1, timelineFile);
	::fclose(timelineFile);

	::printf("Save Stack Log Success!\n");

	ReleaseSRWLockExclusive(&_lock);
}
