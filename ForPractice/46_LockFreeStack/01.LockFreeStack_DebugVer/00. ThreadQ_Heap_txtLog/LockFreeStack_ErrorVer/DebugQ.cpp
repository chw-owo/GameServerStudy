#include "DebugQ.h"

system_clock::time_point g_Start = system_clock::now();

void DebugQ::PushData(int line, __int64 ns)
{
	AcquireSRWLockExclusive(&_lock);
	_queue[_num % dfQUEUE_MAX] = DebugData(_num, line, ns);
	_num++;
	ReleaseSRWLockExclusive(&_lock);
}

void DebugQ::PrintData()
{
	AcquireSRWLockShared(&_lock);
	for (int i = 0; i < dfQUEUE_MAX; i++)
	{
		::printf("%d, %d, %d\n",
			_queue[((_num + i) % dfQUEUE_MAX)]._idx,
			_queue[((_num + i) % dfQUEUE_MAX)]._line,
			_queue[((_num + i) % dfQUEUE_MAX)]._ns);
	}
	ReleaseSRWLockShared(&_lock);
}

void DebugQManager::SaveDebugLog(int now)
{
	if (_terminate) return;
	_terminate = true;

	AcquireSRWLockExclusive(&_lock);

	memset(_stackLogData, dfOUTPUT_SIZE, '\0');
	char buffer[dfBUFFER_SIZE] = { '\0' };

	vector<DebugQ*>::iterator Qiter = _debugQs.begin();
	for (; Qiter != _debugQs.end(); Qiter++)
	{
		sprintf_s(buffer, dfBUFFER_SIZE, "\nThread ID: %d\n", (*Qiter)->_threadID);
		strcat_s(_stackLogData, dfOUTPUT_SIZE, buffer);

		DebugQ* Q = (*Qiter);

		AcquireSRWLockShared(&Q->_lock);
		int num = Q->_num;
		for (int j = 0; j < dfQUEUE_MAX; j++)
		{
			if (Q->_queue[((num + j) % dfQUEUE_MAX)]._ns <= now)
			{
				memset(buffer, dfBUFFER_SIZE, '\0');
				sprintf_s(buffer, dfBUFFER_SIZE, "%d, %d, %lld\n",
					Q->_queue[((num + j) % dfQUEUE_MAX)]._idx,
					Q->_queue[((num + j) % dfQUEUE_MAX)]._line,
					Q->_queue[((num + j) % dfQUEUE_MAX)]._ns);
				strcat_s(_stackLogData, dfOUTPUT_SIZE, buffer);
			}
		}
		ReleaseSRWLockShared(&Q->_lock);
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

	for (Qiter = _debugQs.begin(); Qiter != _debugQs.end(); Qiter++)
	{
		DebugQ* Q = (*Qiter);

		AcquireSRWLockShared(&Q->_lock);
		for (int j = 0; j < dfQUEUE_MAX; j++)
		{
			if (Q->_queue[((Q->_num + j) % dfQUEUE_MAX)]._ns <= now) 
			{
				LogData* pLogData = new LogData(Q->_threadID, Q->_queue[((Q->_num + j) % dfQUEUE_MAX)]);
				_logDataSet.insert(*pLogData);
			}
		}
		ReleaseSRWLockShared(&Q->_lock);
	}

	memset(_timelineData, dfOUTPUT_SIZE, '\0');
	multiset<LogData>::iterator dataIter = _logDataSet.begin();
	for (;  dataIter != _logDataSet.end();  dataIter++)
	{
		LogData pData = (*dataIter);
		memset(buffer, dfBUFFER_SIZE, '\0');
		sprintf_s(buffer, dfBUFFER_SIZE, "%d, %d, %lld\n",
			pData._threadID, pData._line, pData._ns);
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

