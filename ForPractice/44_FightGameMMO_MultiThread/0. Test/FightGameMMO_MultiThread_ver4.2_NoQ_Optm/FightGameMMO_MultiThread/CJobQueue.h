#pragma once
#include "CJob.h"

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#include <queue>
using namespace std;

/*
class CJobQueue
{
public:
	CJobQueue()
	{
		InitializeSRWLock(&_lock);
	}
	~CJobQueue(){}

public:
	queue<CJob*> _queue;
	SRWLOCK _lock;
};
*/