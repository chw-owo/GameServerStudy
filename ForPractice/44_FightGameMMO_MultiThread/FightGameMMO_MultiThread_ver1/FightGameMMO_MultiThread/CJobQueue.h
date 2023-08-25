#pragma once
#include "CJob.h"

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#include <queue>
using namespace std;

class CJobQueue
{
private:
	CJobQueue()
	{
		InitializeSRWLock(&_lock);
	}

	~CJobQueue()
	{

	}

	static CJobQueue _jobQueue;

public:
	static CJobQueue* GetInstance()
	{
		static  CJobQueue _jobQueue;
		return  &_jobQueue;
	}

public:
	
	queue<CJob*> _queue;
	SRWLOCK _lock;
};
