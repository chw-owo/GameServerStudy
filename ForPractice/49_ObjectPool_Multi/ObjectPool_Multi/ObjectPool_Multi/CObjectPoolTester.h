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

private:
	void NewDeleteTest();
	void BasicLockPoolTest();
	void LockFreePoolTest();
	void MinLockPoolTest();
	void PrintOutResult();

private:
	static unsigned WINAPI NewDeleteThread(void* arg);
	static unsigned WINAPI BasicLockPoolThread(void* arg);
	static unsigned WINAPI LockFreePoolThread(void* arg);
	static unsigned WINAPI MinLockPoolThread(void* arg);

private:
	struct Data1 { int data; };
	struct Data2 { int data[BLOCK2]; };
	struct Data3 { int data[BLOCK3]; };
	struct Data4 { int data[BLOCK4]; };
	struct Data5 { int data[BLOCK5]; };

private:
	int _threadCnt = 0;
	int _testCnt = 0;

private:
	volatile long _NewDeleteThreadIdx = -1;
	volatile long _BasicLockPoolThreadIdx = -1;
	volatile long _LockFreePoolThreadIdx = -1;
	volatile long _MinLockPoolThreadIdx = -1;

private:
	double _freq = 0;
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

};