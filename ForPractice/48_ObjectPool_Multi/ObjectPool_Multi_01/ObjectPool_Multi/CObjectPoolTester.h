#pragma once
#include "CObjectPool_BasicLock.h"
#include "CLockFreePool.h"
#include "CObjectPool_MinLock.h"
#include <windows.h>

#define SIZENUM	5

#define SIZE1	4
#define SIZE2	16
#define SIZE3	64
#define SIZE4	256
#define SIZE5	512

#define BLOCK1	1
#define BLOCK2	4
#define BLOCK3	16
#define BLOCK4	64
#define BLOCK5	128

class CObjectPoolTester
{
public:
	CObjectPoolTester();
	~CObjectPoolTester();
	void Test();
	void TestLoop();

private:
	void NewDeleteTest();
	void BasicLockPoolTest();
	void LockFreePoolTest();
	void MinLockPoolTest();
	void PrintOutResult();

private:
	static unsigned WINAPI NewDelete_TestThread(void* arg);
	static unsigned WINAPI BasicLockPool_TestThread(void* arg);
	static unsigned WINAPI LockFreePool_TestThread(void* arg);
	static unsigned WINAPI MinLockPool_TestThread(void* arg);

public:
	void ProfileSingleNewDelete();
	void ProfileNewDelete();
	void ProfileBasicPool();
	void ProfileMinLockPool();
	void ProfileLockFreePool();

private:
	static unsigned WINAPI NewDelete2_ProfileThread(void* arg);
	static unsigned WINAPI BasicLockPool2_ProfileThread(void* arg);
	static unsigned WINAPI LockFreePool2_ProfileThread(void* arg);
	static unsigned WINAPI MinLockPool2_ProfileThread(void* arg);

	static unsigned WINAPI NewDelete4_ProfileThread(void* arg);
	static unsigned WINAPI BasicLockPool4_ProfileThread(void* arg);
	static unsigned WINAPI LockFreePool4_ProfileThread(void* arg);
	static unsigned WINAPI MinLockPool4_ProfileThread(void* arg);

private:
	struct Data1 { Data1() { data = 0; data++; }; int data; };
	struct Data2 
	{ 
		Data2() 
		{ 
			for(int i = 0; i < BLOCK2; i++) data[i] = 0; 
			for (int i = 0; i < BLOCK2; i++) data[i]++;
		}; 	
		int data[BLOCK2]; 
	};

	struct Data3
	{
		Data3()
		{
			for (int i = 0; i < BLOCK3; i++) data[i] = 0;
			for (int i = 0; i < BLOCK3; i++) data[i]++;
		};
		int data[BLOCK3];
	};

	struct Data4 
	{
		Data4()
		{
			for (int i = 0; i < BLOCK4; i++) data[i] = 0;
			for (int i = 0; i < BLOCK4; i++) data[i]++;
		};
		int data[BLOCK4];
	};

	struct Data5
	{
		Data5()
		{
			for (int i = 0; i < BLOCK5; i++) data[i] = 0;
			for (int i = 0; i < BLOCK5; i++) data[i]++;
		};
		int data[BLOCK5];
	};

private:
	int _threadCnt = 0;
	int _testCnt = 0;

private:	
	volatile long _NewDelete_TestThreadIdx = -1;
	volatile long _BasicLockPool_TestThreadIdx = -1;
	volatile long _LockFreePool_TestThreadIdx = -1;
	volatile long _MinLockPool_TestThreadIdx = -1;

private:

	LARGE_INTEGER _freq;
	struct RecordTime
	{
		double* _NewDeleteTimes;
		double* _BasicLockPoolTimes;
		double* _LockFreePoolTimes;
		double* _MinLockPoolTimes;

		double _NewDeleteAvg = 0;
		double _BasicLockPoolAvg = 0;
		double _LockFreePoolAvg = 0;
		double _MinLockPoolAvg = 0;
	};	
	RecordTime _records[SIZENUM];

private:
	HANDLE* _newDeleteThreads = nullptr;
	HANDLE* _basicLockThreads = nullptr;
	HANDLE* _lockFreeThreads = nullptr;
	HANDLE* _minLockThreads = nullptr;

private:
	CObjectPool_BasicLock<Data1>* _pBasicLockPool1 = nullptr;
	CObjectPool_BasicLock<Data2>* _pBasicLockPool2 = nullptr;
	CObjectPool_BasicLock<Data3>* _pBasicLockPool3 = nullptr;
	CObjectPool_BasicLock<Data4>* _pBasicLockPool4 = nullptr;
	CObjectPool_BasicLock<Data5>* _pBasicLockPool5 = nullptr;

	CLockFreePool<Data1>* _pLockFreePool1 = nullptr;
	CLockFreePool<Data2>* _pLockFreePool2 = nullptr;
	CLockFreePool<Data3>* _pLockFreePool3 = nullptr;
	CLockFreePool<Data4>* _pLockFreePool4 = nullptr;
	CLockFreePool<Data5>* _pLockFreePool5 = nullptr;

	CObjectPool_MinLock<Data1>* _pMinLockPool1 = nullptr;
	CObjectPool_MinLock<Data2>* _pMinLockPool2 = nullptr;
	CObjectPool_MinLock<Data3>* _pMinLockPool3 = nullptr;
	CObjectPool_MinLock<Data4>* _pMinLockPool4 = nullptr;
	CObjectPool_MinLock<Data5>* _pMinLockPool5 = nullptr;

private:
	long _profileComplete = 0;
	long _printComplete = 0;
};