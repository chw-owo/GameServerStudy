// <Without Profiler>

#include "CObjectPoolTester.h"
#include <process.h>
#include <stdio.h>
#pragma comment(lib, "winmm.lib")
#define MS_PER_SEC	1000000

CObjectPoolTester::CObjectPoolTester()
{
	QueryPerformanceFrequency(&_freq);
}

void CObjectPoolTester::Test()
{
	// Set Test Config
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	::printf("Thread Count (max: %d): ", (int)si.dwNumberOfProcessors);
	::scanf_s("%d", &_threadCnt);
	::printf("Test Count: ");
	::scanf_s("%d", &_testCnt);

	_newDeleteThreads = new HANDLE[_threadCnt];
	_basicLockThreads = new HANDLE[_threadCnt];
	_lockFreeThreads = new HANDLE[_threadCnt];
	_minLockThreads = new HANDLE[_threadCnt];

	for (int i = 0; i < SIZENUM; i++)
	{
		_records[i]._NewDeleteTimes = new double[_threadCnt];
		_records[i]._BasicLockPoolTimes = new double[_threadCnt];
		_records[i]._LockFreePoolTimes = new double[_threadCnt];
		_records[i]._MinLockPoolTimes = new double[_threadCnt];
	}

	// Test Thread
	::printf("\nTest Start\n");
	timeBeginPeriod(1);

	NewDeleteTest();
	BasicLockPoolTest();
	LockFreePoolTest();
	MinLockPoolTest();

	timeEndPeriod(1);
	::printf("Test End\n\n");

	// Print Out Result
	PrintOutResult();

	for (int i = 0; i < SIZENUM; i++)
	{
		delete[] _records[i]._NewDeleteTimes;
		delete[] _records[i]._BasicLockPoolTimes; // Error
		delete[] _records[i]._LockFreePoolTimes;
		delete[] _records[i]._MinLockPoolTimes;
	}

	delete[] _newDeleteThreads;
	delete[] _basicLockThreads;
	delete[] _lockFreeThreads;
	delete[] _minLockThreads;
}

void CObjectPoolTester::TestLoop()
{
	// Set Test Config
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	::printf("Thread Count (max: %d): ", (int)si.dwNumberOfProcessors);
	::scanf_s("%d", &_threadCnt);
	::printf("Test Count: ");
	::scanf_s("%d", &_testCnt);

	for (;;)
	{
		if (GetAsyncKeyState(VK_SPACE)) break;

		_NewDeleteThreadIdx = -1;
		_BasicLockPoolThreadIdx = -1;
		_LockFreePoolThreadIdx = -1;
		_MinLockPoolThreadIdx = -1;

		_newDeleteThreads = new HANDLE[_threadCnt];
		_basicLockThreads = new HANDLE[_threadCnt];
		_lockFreeThreads = new HANDLE[_threadCnt];
		_minLockThreads = new HANDLE[_threadCnt];

		for (int i = 0; i < SIZENUM; i++)
		{
			_records[i]._NewDeleteTimes = new double[_threadCnt];
			_records[i]._BasicLockPoolTimes = new double[_threadCnt];
			_records[i]._LockFreePoolTimes = new double[_threadCnt];
			_records[i]._MinLockPoolTimes = new double[_threadCnt];
		}

		// Test Thread
		::printf("\nTest Start\n");
		timeBeginPeriod(1);

		NewDeleteTest();
		BasicLockPoolTest();
		LockFreePoolTest();
		MinLockPoolTest();

		timeEndPeriod(1);
		::printf("Test End\n\n");

		// Print Out Result
		PrintOutResult();

		for (int i = 0; i < SIZENUM; i++)
		{
			delete[] _records[i]._NewDeleteTimes;
			delete[] _records[i]._BasicLockPoolTimes; // Error
			delete[] _records[i]._LockFreePoolTimes;
			delete[] _records[i]._MinLockPoolTimes;
		}

		delete[] _newDeleteThreads;
		delete[] _basicLockThreads;
		delete[] _lockFreeThreads;
		delete[] _minLockThreads;
	}
}

CObjectPoolTester::~CObjectPoolTester()
{

}

void CObjectPoolTester::NewDeleteTest()
{
	::printf("New Delete Test Start!");
	for (int i = 0; i < _threadCnt; i++)
	{
		_newDeleteThreads[i] = (HANDLE)_beginthreadex(NULL, 0, NewDeleteThread, this, 0, nullptr);
		if (_newDeleteThreads[i] == NULL)
		{
			::printf(" Error! %s(%d)\n", __func__, __LINE__);
			__debugbreak();
		}
	}
	WaitForMultipleObjects(_threadCnt, _newDeleteThreads, true, INFINITE);
	::printf(" -> New Delete Test End!\n");
}

void CObjectPoolTester::BasicLockPoolTest()
{
	_pBasicLockPool1 = new CObjectPool_BasicLock<Data1>(_testCnt * _threadCnt, true);
	_pBasicLockPool2 = new CObjectPool_BasicLock<Data2>(_testCnt * _threadCnt, true);
	_pBasicLockPool3 = new CObjectPool_BasicLock<Data3>(_testCnt * _threadCnt, true);
	_pBasicLockPool4 = new CObjectPool_BasicLock<Data4>(_testCnt * _threadCnt, true);
	_pBasicLockPool5 = new CObjectPool_BasicLock<Data5>(_testCnt * _threadCnt, true);

	::printf("BasicLockPool Test Start!");
	for (int i = 0; i < _threadCnt; i++)
	{
		_basicLockThreads[i] = (HANDLE)_beginthreadex(NULL, 0, BasicLockPoolThread, this, 0, nullptr);
		if (_basicLockThreads[i] == NULL)
		{
			::printf(" Error! %s(%d)\n", __func__, __LINE__);
			__debugbreak();
		}
	}
	WaitForMultipleObjects(_threadCnt, _basicLockThreads, true, INFINITE);
	::printf(" -> BasicLockPool Test End!\n");

	delete _pBasicLockPool1;
	delete _pBasicLockPool2;
	delete _pBasicLockPool3;
	delete _pBasicLockPool4;
	delete _pBasicLockPool5;
}

void CObjectPoolTester::LockFreePoolTest()
{
	_pLockFreePool1 = new CLockFreePool<Data1>(_testCnt * _threadCnt, true);
	_pLockFreePool2 = new CLockFreePool<Data2>(_testCnt * _threadCnt, true);
	_pLockFreePool3 = new CLockFreePool<Data3>(_testCnt * _threadCnt, true);
	_pLockFreePool4 = new CLockFreePool<Data4>(_testCnt * _threadCnt, true);
	_pLockFreePool5 = new CLockFreePool<Data5>(_testCnt * _threadCnt, true);

	::printf("LockFreePool Test Start!");
	for (int i = 0; i < _threadCnt; i++)
	{
		_lockFreeThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreePoolThread, this, 0, nullptr);
		if (_lockFreeThreads[i] == NULL)
		{
			::printf(" Error! %s(%d)\n", __func__, __LINE__);
			__debugbreak();
		}
	}
	WaitForMultipleObjects(_threadCnt, _lockFreeThreads, true, INFINITE);
	::printf(" -> LockFreePool Test End!\n");

	delete _pLockFreePool1;
	delete _pLockFreePool2;
	delete _pLockFreePool3;
	delete _pLockFreePool4;
	delete _pLockFreePool5;
}

void CObjectPoolTester::MinLockPoolTest()
{
	_pMinLockPool1 = new CObjectPool_MinLock<Data1>(_testCnt * _threadCnt, true);
	_pMinLockPool2 = new CObjectPool_MinLock<Data2>(_testCnt * _threadCnt, true);
	_pMinLockPool3 = new CObjectPool_MinLock<Data3>(_testCnt * _threadCnt, true);
	_pMinLockPool4 = new CObjectPool_MinLock<Data4>(_testCnt * _threadCnt, true);
	_pMinLockPool5 = new CObjectPool_MinLock<Data5>(_testCnt * _threadCnt, true);

	::printf("MinLockPool Test Start!");
	for (int i = 0; i < _threadCnt; i++)
	{
		_minLockThreads[i] = (HANDLE)_beginthreadex(NULL, 0, MinLockPoolThread, this, 0, nullptr);
		if (_minLockThreads[i] == NULL)
		{
			::printf(" Error! %s(%d)\n", __func__, __LINE__);
			__debugbreak();
		}
	}
	WaitForMultipleObjects(_threadCnt, _minLockThreads, true, INFINITE);
	::printf(" -> MinLockPool Test End!\n");

	delete _pMinLockPool1;
	delete _pMinLockPool2;
	delete _pMinLockPool3;
	delete _pMinLockPool4;
	delete _pMinLockPool5;
}

void CObjectPoolTester::PrintOutResult()
{
	for (int i = 0; i < SIZENUM; i++)
	{
		for (int j = 0; j < _threadCnt; j++)
		{
			_records[i]._NewDeleteAvg += _records[i]._NewDeleteTimes[j];
			_records[i]._BasicLockPoolAvg += _records[i]._BasicLockPoolTimes[j];
			_records[i]._LockFreePoolAvg += _records[i]._LockFreePoolTimes[j];
			_records[i]._MinLockPoolAvg += _records[i]._MinLockPoolTimes[j];
		}
		_records[i]._NewDeleteAvg = _records[i]._NewDeleteAvg / _threadCnt;
		_records[i]._BasicLockPoolAvg = _records[i]._BasicLockPoolAvg / _threadCnt;
		_records[i]._LockFreePoolAvg = _records[i]._LockFreePoolAvg / _threadCnt;
		_records[i]._MinLockPoolAvg = _records[i]._MinLockPoolAvg / _threadCnt;
	}

	::printf("<Result>\n");

	int idx = 0;
	::printf("\n1. %d bytes test\n", SIZE1);
	::printf("New-Delete: %.2lfms\n", _records[idx]._NewDeleteAvg);
	::printf("BasicLockPool: %.2lfms\n", _records[idx]._BasicLockPoolAvg);
	::printf("LockFreePool: %.2lfms\n", _records[idx]._LockFreePoolAvg);
	::printf("MinLockPool: %.2lfms\n", _records[idx]._MinLockPoolAvg);
	idx++;

	::printf("\n2. %d bytes test\n", SIZE2);
	::printf("New-Delete: %.2lfms\n", _records[idx]._NewDeleteAvg);
	::printf("BasicLockPool: %.2lfms\n", _records[idx]._BasicLockPoolAvg);
	::printf("LockFreePool: %.2lfms\n", _records[idx]._LockFreePoolAvg);
	::printf("MinLockPool: %.2lfms\n", _records[idx]._MinLockPoolAvg);
	idx++;

	::printf("\n3. %d bytes test\n", SIZE3);
	::printf("New-Delete: %.2lfms\n", _records[idx]._NewDeleteAvg);
	::printf("BasicLockPool: %.2lfms\n", _records[idx]._BasicLockPoolAvg);
	::printf("LockFreePool: %.2lfms\n", _records[idx]._LockFreePoolAvg);
	::printf("MinLockPool: %.2lfms\n", _records[idx]._MinLockPoolAvg);
	idx++;

	::printf("\n4. %d bytes test\n", SIZE4);
	::printf("New-Delete: %.2lfms\n", _records[idx]._NewDeleteAvg);
	::printf("BasicLockPool: %.2lfms\n", _records[idx]._BasicLockPoolAvg);
	::printf("LockFreePool: %.2lfms\n", _records[idx]._LockFreePoolAvg);
	::printf("MinLockPool: %.2lfms\n", _records[idx]._MinLockPoolAvg);
	idx++;

	::printf("\n5. %d bytes test\n", SIZE5);
	::printf("New-Delete: %.2lfms\n", _records[idx]._NewDeleteAvg);
	::printf("BasicLockPool: %.2lfms\n", _records[idx]._BasicLockPoolAvg);
	::printf("LockFreePool: %.2lfms\n", _records[idx]._LockFreePoolAvg);
	::printf("MinLockPool: %.2lfms\n", _records[idx]._MinLockPoolAvg);
}

unsigned __stdcall CObjectPoolTester::NewDeleteThread(void* arg)
{
	CObjectPoolTester* tester = (CObjectPoolTester*)arg;

	int sizeIdx = 0;
	LARGE_INTEGER NewDeleteStart, NewDeleteEnd;
	int threadIdx = InterlockedIncrement(&tester->_NewDeleteThreadIdx);

	double sum;
	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&NewDeleteStart);
		Data1* data = new Data1;
		delete data;
		QueryPerformanceCounter(&NewDeleteEnd);
		sum += (NewDeleteEnd.QuadPart - NewDeleteStart.QuadPart) / (double)tester->_freq.QuadPart;
	}

	tester->_records[sizeIdx]._NewDeleteTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&NewDeleteStart);
		Data2* data = new Data2;
		delete data;
		QueryPerformanceCounter(&NewDeleteEnd);
		sum += (NewDeleteEnd.QuadPart - NewDeleteStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._NewDeleteTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&NewDeleteStart);
		Data3* data = new Data3;
		delete data;
		QueryPerformanceCounter(&NewDeleteEnd);
		sum += (NewDeleteEnd.QuadPart - NewDeleteStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._NewDeleteTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&NewDeleteStart);
		Data4* data = new Data4;
		delete data;
		QueryPerformanceCounter(&NewDeleteEnd);
		sum += (NewDeleteEnd.QuadPart - NewDeleteStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._NewDeleteTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&NewDeleteStart);
		Data5* data = new Data5;
		delete data;
		QueryPerformanceCounter(&NewDeleteEnd);
		sum += (NewDeleteEnd.QuadPart - NewDeleteStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._NewDeleteTimes[threadIdx] = sum * MS_PER_SEC;

	return 0;
}

unsigned __stdcall CObjectPoolTester::BasicLockPoolThread(void* arg)
{
	CObjectPoolTester* tester = (CObjectPoolTester*)arg;

	int sizeIdx = 0;
	LARGE_INTEGER BasicLockPoolStart, BasicLockPoolEnd;
	int threadIdx = InterlockedIncrement(&tester->_BasicLockPoolThreadIdx);

	CObjectPool_BasicLock<Data1>* pPool1 = tester->_pBasicLockPool1;
	CObjectPool_BasicLock<Data2>* pPool2 = tester->_pBasicLockPool2;
	CObjectPool_BasicLock<Data3>* pPool3 = tester->_pBasicLockPool3;
	CObjectPool_BasicLock<Data4>* pPool4 = tester->_pBasicLockPool4;
	CObjectPool_BasicLock<Data5>* pPool5 = tester->_pBasicLockPool5;

	double sum;
	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&BasicLockPoolStart);
		Data1* data1 = pPool1->Alloc();
		pPool1->Free(data1);
		QueryPerformanceCounter(&BasicLockPoolEnd);
		sum += (BasicLockPoolEnd.QuadPart - BasicLockPoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._BasicLockPoolTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&BasicLockPoolStart);
		Data2* data2 = pPool2->Alloc();
		pPool2->Free(data2);
		QueryPerformanceCounter(&BasicLockPoolEnd);
		sum += (BasicLockPoolEnd.QuadPart - BasicLockPoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._BasicLockPoolTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&BasicLockPoolStart);
		Data3* data3 = pPool3->Alloc();
		pPool3->Free(data3);
		QueryPerformanceCounter(&BasicLockPoolEnd);
		sum += (BasicLockPoolEnd.QuadPart - BasicLockPoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._BasicLockPoolTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&BasicLockPoolStart);
		Data4* data4 = pPool4->Alloc();
		pPool4->Free(data4);
		QueryPerformanceCounter(&BasicLockPoolEnd);
		sum += (BasicLockPoolEnd.QuadPart - BasicLockPoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._BasicLockPoolTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&BasicLockPoolStart);
		Data5* data5 = pPool5->Alloc();
		pPool5->Free(data5);
		QueryPerformanceCounter(&BasicLockPoolEnd);
		sum += (BasicLockPoolEnd.QuadPart - BasicLockPoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._BasicLockPoolTimes[threadIdx] = sum * MS_PER_SEC;

	return 0;
}

unsigned __stdcall CObjectPoolTester::LockFreePoolThread(void* arg)
{
	CObjectPoolTester* tester = (CObjectPoolTester*)arg;

	int sizeIdx = 0;
	LARGE_INTEGER LockFreePoolStart, LockFreePoolEnd;
	int threadIdx = InterlockedIncrement(&tester->_LockFreePoolThreadIdx);

	CLockFreePool<Data1>* pPool1 = tester->_pLockFreePool1;
	CLockFreePool<Data2>* pPool2 = tester->_pLockFreePool2;
	CLockFreePool<Data3>* pPool3 = tester->_pLockFreePool3;
	CLockFreePool<Data4>* pPool4 = tester->_pLockFreePool4;
	CLockFreePool<Data5>* pPool5 = tester->_pLockFreePool5;

	double sum;
	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&LockFreePoolStart);
		Data1* data1 = pPool1->Alloc();
		pPool1->Free(data1);
		QueryPerformanceCounter(&LockFreePoolEnd);
		sum += (LockFreePoolEnd.QuadPart - LockFreePoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._LockFreePoolTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&LockFreePoolStart);
		Data2* data2 = pPool2->Alloc();
		pPool2->Free(data2);
		QueryPerformanceCounter(&LockFreePoolEnd);
		sum += (LockFreePoolEnd.QuadPart - LockFreePoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._LockFreePoolTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&LockFreePoolStart);
		Data3* data3 = pPool3->Alloc();
		pPool3->Free(data3);
		QueryPerformanceCounter(&LockFreePoolEnd);
		sum += (LockFreePoolEnd.QuadPart - LockFreePoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._LockFreePoolTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&LockFreePoolStart);
		Data4* data4 = pPool4->Alloc();
		pPool4->Free(data4);
		QueryPerformanceCounter(&LockFreePoolEnd);
		sum += (LockFreePoolEnd.QuadPart - LockFreePoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._LockFreePoolTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&LockFreePoolStart);
		Data5* data5 = pPool5->Alloc();
		pPool5->Free(data5);
		QueryPerformanceCounter(&LockFreePoolEnd);
		sum += (LockFreePoolEnd.QuadPart - LockFreePoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._LockFreePoolTimes[threadIdx] = sum * MS_PER_SEC;

	return 0;
}

unsigned __stdcall CObjectPoolTester::MinLockPoolThread(void* arg)
{
	CObjectPoolTester* tester = (CObjectPoolTester*)arg;

	int sizeIdx = 0;
	LARGE_INTEGER MinLockPoolStart, MinLockPoolEnd;
	int threadIdx = InterlockedIncrement(&tester->_MinLockPoolThreadIdx);

	CObjectPool_MinLock<Data1>* pPool1 = tester->_pMinLockPool1;
	CObjectPool_MinLock<Data2>* pPool2 = tester->_pMinLockPool2;
	CObjectPool_MinLock<Data3>* pPool3 = tester->_pMinLockPool3;
	CObjectPool_MinLock<Data4>* pPool4 = tester->_pMinLockPool4;
	CObjectPool_MinLock<Data5>* pPool5 = tester->_pMinLockPool5;

	double sum;
	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&MinLockPoolStart);
		Data1* data1 = pPool1->Alloc();
		pPool1->Free(data1);
		QueryPerformanceCounter(&MinLockPoolEnd);
		sum += (MinLockPoolEnd.QuadPart - MinLockPoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._MinLockPoolTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&MinLockPoolStart);
		Data2* data2 = pPool2->Alloc();
		pPool2->Free(data2);
		QueryPerformanceCounter(&MinLockPoolEnd);
		sum += (MinLockPoolEnd.QuadPart - MinLockPoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._MinLockPoolTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&MinLockPoolStart);
		Data3* data3 = pPool3->Alloc();
		pPool3->Free(data3);
		QueryPerformanceCounter(&MinLockPoolEnd);
		sum += (MinLockPoolEnd.QuadPart - MinLockPoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._MinLockPoolTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&MinLockPoolStart);
		Data4* data4 = pPool4->Alloc();
		pPool4->Free(data4);
		QueryPerformanceCounter(&MinLockPoolEnd);
		sum += (MinLockPoolEnd.QuadPart - MinLockPoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._MinLockPoolTimes[threadIdx] = sum * MS_PER_SEC;
	sizeIdx++;

	sum = 0;
	for (int i = 0; i < tester->_testCnt; i++)
	{
		QueryPerformanceCounter(&MinLockPoolStart);
		Data5* data5 = pPool5->Alloc();
		pPool5->Free(data5);
		QueryPerformanceCounter(&MinLockPoolEnd);
		sum += (MinLockPoolEnd.QuadPart - MinLockPoolStart.QuadPart) / (double)tester->_freq.QuadPart;
	}
	tester->_records[sizeIdx]._MinLockPoolTimes[threadIdx] = sum * MS_PER_SEC;

	return 0;
}

/*
// <Use Profiler>

CObjectPoolTester::CObjectPoolTester()
{
	// QueryPerformanceFrequency(&_freq);
}

void CObjectPoolTester::Test()
{
	// Set Test Config
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	::printf("Thread Count (max: %d): ", (int)si.dwNumberOfProcessors);
	::scanf_s("%d", &_threadCnt);
	::printf("Test Count: ");
	::scanf_s("%d", &_testCnt);

	_newDeleteThreads = new HANDLE[_threadCnt];
	_basicLockThreads = new HANDLE[_threadCnt];
	_lockFreeThreads = new HANDLE[_threadCnt];
	_minLockThreads = new HANDLE[_threadCnt];

	// Test Thread
	::printf("\nTest Start\n");
	timeBeginPeriod(1);

	NewDeleteTest();
	BasicLockPoolTest();
	LockFreePoolTest();
	MinLockPoolTest();

	timeEndPeriod(1);
	::printf("Test End\n\n");

	// Print Out Result
	PRO_PRINT();
	::printf("\n\n=============================================================\n\n\n");
	PRO_PRINT_ADDUP();

	delete[] _newDeleteThreads;
	delete[] _basicLockThreads;
	delete[] _lockFreeThreads;
	delete[] _minLockThreads;
}

void CObjectPoolTester::TestLoop()
{
	// Set Test Config
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	::printf("Thread Count (max: %d): ", (int)si.dwNumberOfProcessors);
	::scanf_s("%d", &_threadCnt);
	::printf("Test Count: ");
	::scanf_s("%d", &_testCnt);

	for (;;)
	{
		if (GetAsyncKeyState(VK_SPACE)) break;

		_newDeleteThreads = new HANDLE[_threadCnt];
		_basicLockThreads = new HANDLE[_threadCnt];
		_lockFreeThreads = new HANDLE[_threadCnt];
		_minLockThreads = new HANDLE[_threadCnt];

		// Test Thread
		::printf("\nTest Start\n");
		timeBeginPeriod(1);

		NewDeleteTest();
		BasicLockPoolTest();
		LockFreePoolTest();
		MinLockPoolTest();

		timeEndPeriod(1);
		::printf("Test End\n\n");

		// Print Out Result
		PRO_PRINT();
		::printf("\n\n=============================================================\n\n\n");
		PRO_PRINT_ADDUP();

		delete[] _newDeleteThreads;
		delete[] _basicLockThreads;
		delete[] _lockFreeThreads;
		delete[] _minLockThreads;
	}
}

CObjectPoolTester::~CObjectPoolTester()
{

}

void CObjectPoolTester::NewDeleteTest()
{
	::printf("New Delete Test Start!");
	for (int i = 0; i < _threadCnt; i++)
	{
		_newDeleteThreads[i] = (HANDLE)_beginthreadex(NULL, 0, NewDeleteThread, this, 0, nullptr);
		if (_newDeleteThreads[i] == NULL)
		{
			::printf(" Error! %s(%d)\n", __func__, __LINE__);
			__debugbreak();
		}
	}
	WaitForMultipleObjects(_threadCnt, _newDeleteThreads, true, INFINITE);
	::printf(" -> New Delete Test End!\n");
}

void CObjectPoolTester::BasicLockPoolTest()
{
	_pBasicLockPool1 = new CObjectPool_BasicLock<Data1>(_testCnt * _threadCnt, true);
	_pBasicLockPool2 = new CObjectPool_BasicLock<Data2>(_testCnt * _threadCnt, true);
	_pBasicLockPool3 = new CObjectPool_BasicLock<Data3>(_testCnt * _threadCnt, true);
	_pBasicLockPool4 = new CObjectPool_BasicLock<Data4>(_testCnt * _threadCnt, true);
	_pBasicLockPool5 = new CObjectPool_BasicLock<Data5>(_testCnt * _threadCnt, true);

	::printf("BasicLockPool Test Start!");
	for (int i = 0; i < _threadCnt; i++)
	{
		_basicLockThreads[i] = (HANDLE)_beginthreadex(NULL, 0, BasicLockPoolThread, this, 0, nullptr);
		if (_basicLockThreads[i] == NULL)
		{
			::printf(" Error! %s(%d)\n", __func__, __LINE__);
			__debugbreak();
		}
	}
	WaitForMultipleObjects(_threadCnt, _basicLockThreads, true, INFINITE);
	::printf(" -> BasicLockPool Test End!\n");

	delete _pBasicLockPool1;
	delete _pBasicLockPool2;
	delete _pBasicLockPool3;
	delete _pBasicLockPool4;
	delete _pBasicLockPool5;
}

void CObjectPoolTester::LockFreePoolTest()
{
	_pLockFreePool1 = new CLockFreePool<Data1>(_testCnt * _threadCnt, true);
	_pLockFreePool2 = new CLockFreePool<Data2>(_testCnt * _threadCnt, true);
	_pLockFreePool3 = new CLockFreePool<Data3>(_testCnt * _threadCnt, true);
	_pLockFreePool4 = new CLockFreePool<Data4>(_testCnt * _threadCnt, true);
	_pLockFreePool5 = new CLockFreePool<Data5>(_testCnt * _threadCnt, true);

	::printf("LockFreePool Test Start!");
	for (int i = 0; i < _threadCnt; i++)
	{
		_lockFreeThreads[i] = (HANDLE)_beginthreadex(NULL, 0, LockFreePoolThread, this, 0, nullptr);
		if (_lockFreeThreads[i] == NULL)
		{
			::printf(" Error! %s(%d)\n", __func__, __LINE__);
			__debugbreak();
		}
	}
	WaitForMultipleObjects(_threadCnt, _lockFreeThreads, true, INFINITE);
	::printf(" -> LockFreePool Test End!\n");

	delete _pLockFreePool1;
	delete _pLockFreePool2;
	delete _pLockFreePool3;
	delete _pLockFreePool4;
	delete _pLockFreePool5;
}

void CObjectPoolTester::MinLockPoolTest()
{
	_pMinLockPool1 = new CObjectPool_MinLock<Data1>(_testCnt * _threadCnt, true);
	_pMinLockPool2 = new CObjectPool_MinLock<Data2>(_testCnt * _threadCnt, true);
	_pMinLockPool3 = new CObjectPool_MinLock<Data3>(_testCnt * _threadCnt, true);
	_pMinLockPool4 = new CObjectPool_MinLock<Data4>(_testCnt * _threadCnt, true);
	_pMinLockPool5 = new CObjectPool_MinLock<Data5>(_testCnt * _threadCnt, true);

	::printf("MinLockPool Test Start!");
	for (int i = 0; i < _threadCnt; i++)
	{
		_minLockThreads[i] = (HANDLE)_beginthreadex(NULL, 0, MinLockPoolThread, this, 0, nullptr);
		if (_minLockThreads[i] == NULL)
		{
			::printf(" Error! %s(%d)\n", __func__, __LINE__);
			__debugbreak();
		}
	}
	WaitForMultipleObjects(_threadCnt, _minLockThreads, true, INFINITE);
	::printf(" -> MinLockPool Test End!\n");

	delete _pMinLockPool1;
	delete _pMinLockPool2;
	delete _pMinLockPool3;
	delete _pMinLockPool4;
	delete _pMinLockPool5;
}

unsigned __stdcall CObjectPoolTester::NewDeleteThread(void* arg)
{
	CObjectPoolTester* tester = (CObjectPoolTester*)arg;
	PRO_SET();

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"New-delete: 4byte");
		Data1* data = new Data1;
		delete data;
		PRO_END(L"New-delete: 4byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"New-delete: 16byte");
		Data2* data = new Data2;
		delete data;
		PRO_END(L"New-delete: 16byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"New-delete: 64byte");
		Data3* data = new Data3;
		delete data;
		PRO_END(L"New-delete: 64byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"New-delete: 256byte");
		Data4* data = new Data4;
		delete data;
		PRO_END(L"New-delete: 256byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"New-delete: 512byte");
		Data5* data = new Data5;
		delete data;
		PRO_END(L"New-delete: 512byte");
	}

	return 0;
}

unsigned __stdcall CObjectPoolTester::BasicLockPoolThread(void* arg)
{
	CObjectPoolTester* tester = (CObjectPoolTester*)arg;
	PRO_SET();

	CObjectPool_BasicLock<Data1>* pPool1 = tester->_pBasicLockPool1;
	CObjectPool_BasicLock<Data2>* pPool2 = tester->_pBasicLockPool2;
	CObjectPool_BasicLock<Data3>* pPool3 = tester->_pBasicLockPool3;
	CObjectPool_BasicLock<Data4>* pPool4 = tester->_pBasicLockPool4;
	CObjectPool_BasicLock<Data5>* pPool5 = tester->_pBasicLockPool5;

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Basic lock: 4byte");
		Data1* data1 = pPool1->Alloc();
		pPool1->Free(data1);
		PRO_END(L"Basic lock: 4byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Basic lock: 16byte");
		Data2* data2 = pPool2->Alloc();
		pPool2->Free(data2);
		PRO_END(L"Basic lock: 16byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Basic lock: 64byte");
		Data3* data3 = pPool3->Alloc();
		pPool3->Free(data3);
		PRO_END(L"Basic lock: 64byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Basic lock: 256byte");
		Data4* data4 = pPool4->Alloc();
		pPool4->Free(data4);
		PRO_END(L"Basic lock: 256byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Basic lock: 128byte");
		Data5* data5 = pPool5->Alloc();
		pPool5->Free(data5);
		PRO_END(L"Basic lock: 128byte");
	}

	return 0;
}

unsigned __stdcall CObjectPoolTester::LockFreePoolThread(void* arg)
{
	CObjectPoolTester* tester = (CObjectPoolTester*)arg;
	PRO_SET();

	CLockFreePool<Data1>* pPool1 = tester->_pLockFreePool1;
	CLockFreePool<Data2>* pPool2 = tester->_pLockFreePool2;
	CLockFreePool<Data3>* pPool3 = tester->_pLockFreePool3;
	CLockFreePool<Data4>* pPool4 = tester->_pLockFreePool4;
	CLockFreePool<Data5>* pPool5 = tester->_pLockFreePool5;

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Lock free: 4byte");
		Data1* data1 = pPool1->Alloc();
		pPool1->Free(data1);
		PRO_END(L"Lock free: 4byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Lock free: 16byte");
		Data2* data2 = pPool2->Alloc();
		pPool2->Free(data2);
		PRO_END(L"Lock free: 16byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Lock free: 64byte");
		Data3* data3 = pPool3->Alloc();
		pPool3->Free(data3);
		PRO_END(L"Lock free: 64byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Lock free: 256byte");
		Data4* data4 = pPool4->Alloc();
		pPool4->Free(data4);
		PRO_END(L"Lock free: 256byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Lock free: 512byte");
		Data5* data5 = pPool5->Alloc();
		pPool5->Free(data5);
		PRO_END(L"Lock free: 512byte");
	}

	return 0;
}

unsigned __stdcall CObjectPoolTester::MinLockPoolThread(void* arg)
{
	CObjectPoolTester* tester = (CObjectPoolTester*)arg;
	PRO_SET();

	CObjectPool_MinLock<Data1>* pPool1 = tester->_pMinLockPool1;
	CObjectPool_MinLock<Data2>* pPool2 = tester->_pMinLockPool2;
	CObjectPool_MinLock<Data3>* pPool3 = tester->_pMinLockPool3;
	CObjectPool_MinLock<Data4>* pPool4 = tester->_pMinLockPool4;
	CObjectPool_MinLock<Data5>* pPool5 = tester->_pMinLockPool5;

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Minimum lock: 4byte");
		Data1* data1 = pPool1->Alloc();
		pPool1->Free(data1);
		PRO_END(L"Minimum lock: 4byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Minimum lock: 16byte");
		Data2* data2 = pPool2->Alloc();
		pPool2->Free(data2);
		PRO_END(L"Minimum lock: 16byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Minimum lock: 64byte");
		Data3* data3 = pPool3->Alloc();
		pPool3->Free(data3);
		PRO_END(L"Minimum lock: 64byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Minimum lock: 256byte");
		Data4* data4 = pPool4->Alloc();
		pPool4->Free(data4);
		PRO_END(L"Minimum lock: 256byte");
	}

	for (int i = 0; i < tester->_testCnt; i++)
	{
		PRO_BEGIN(L"Minimum lock: 512byte");
		Data5* data5 = pPool5->Alloc();
		pPool5->Free(data5);
		PRO_END(L"Minimum lock: 512byte");
	}

	return 0;
}
*/