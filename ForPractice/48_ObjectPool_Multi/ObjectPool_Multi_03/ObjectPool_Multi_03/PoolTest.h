#pragma once
#define BLOCK 8

#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <queue>
#include <set>

#include "CObjectPool.h"
#include "CLockPool.h"
#include "CLockFreePool.h"
#include "CTlsObjectPool.h"
#include "CTlsObjectPool_Upgrade.h"
using namespace std;

#define POOL_CNT 5
#define THREAD_CNT 4
#define TEST_CNT 10000
#define TOTAL_CNT 40000
#define LOOP_CNT 10
#define MS_PER_SEC 1000000

long g_profileComplete = 0;
HANDLE g_beginThreadComplete = nullptr;
HANDLE g_printComplete = nullptr;
struct Data { char data[BLOCK]; };

int newCalls[POOL_CNT][THREAD_CNT];
int deleteCalls[POOL_CNT][THREAD_CNT];
double newTimes[POOL_CNT][THREAD_CNT];
double deleteTimes[POOL_CNT][THREAD_CNT];
set<unsigned __int64> address[POOL_CNT][THREAD_CNT];

void SetTest();
void SingleNewDelete();
void SingleBasicLockPool();
void SingleLockFreePool();
void SingleTlsLockPool();
void SingleTlsLockPoolUpgrade();

void SetMultiTest();
void MultiNewDelete();
void MultiBasicLockPool();
void MultiLockFreePool();
void MultiTlsLockPool();
void MultiTlsLockPoolUpgrade();

void ProfileTlsLockPool();
void ProfileTlsLockPoolUpgrade();

inline unsigned __int64 GetAddress(Data* p);
unsigned __stdcall NewDeleteThread(void* arg);
unsigned __stdcall BasicLockPoolThread(void* arg);
unsigned __stdcall LockFreePoolThread(void* arg);
unsigned __stdcall TlsLockPoolThread(void* arg);
unsigned __stdcall TlsLockPoolUpgradeThread(void* arg);

struct argForThread
{
    argForThread(size_t pool, int idx) 
        : _pool(pool), _idx(idx) {};
    size_t _pool;
    int _idx;
};

