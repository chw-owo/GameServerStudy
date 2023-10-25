#pragma once
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#include <iostream>
#include <string>
#include <float.h>
#include <vector>
#include <unordered_map>
#include <synchapi.h>
using namespace std;

/*=============================================================

<How to use>

1.	PRO_SAVE, PRO_SAVE_ADDUP ȣ�� �� Ȯ���� �̸��� �������� �ʴ´�.

2. microsec ���� ��� �� #define USE_MS_UNIT,
	nanosec ���� ��� �� #define USE_NS_UNIT

3. read �õ� �� write ���� ��� �ش� pf�� ����� �ݿ����� �ʰ� �Ѿ��
   write �õ� �� read ���� ��� �ش� ���ʴ� �������� �ʰ� �Ѿ��

==============================================================*/

#define PROFILE

#ifdef PROFILE

#define USE_MS_UNIT
//#define USE_NS_UNIT

#define NAME_LEN 64
#define FILE_NAME_LEN 128
#define PROFILE_CNT 32
#define MINMAX_CNT 2
#define BUFFER_SIZE 256
#define OUTPUT_SIZE BUFFER_SIZE * PROFILE_CNT
#define MS_PER_SEC 1000000
#define NS_PER_SEC 1000

class _PROFILE_RESULT
{
public:
	_PROFILE_RESULT()
	{
		InitializeSRWLock(&_lock);
	}

public:
	long			_useFlag = 0;								// �������� ��� ���� �� ����		  
	LARGE_INTEGER	_lStartTime;								// �������� ���� ���� �ð�

	wstring			_szName;									// �������� ���� �̸�
	double			_dTotalTime = 0;							// ��ü ���ð� ī���� Time
	double			_dMin[MINMAX_CNT] = { DBL_MAX, DBL_MAX };	// �ּ� ���ð� ī���� Time
	double			_dMax[MINMAX_CNT] = { 0, 0 };				// �ִ� ���ð� ī���� Time
	__int64			_iCall = 0;									// ���� ȣ�� Ƚ��

	SRWLOCK			_lock;
};

class _PROFILE_RESULT_FOR_ADDUP
{
public:
	double			_dTotalTime = 0;							// ��ü ���ð� ī���� Time
	double			_dMin[MINMAX_CNT] = { 0, 0 };				// �ּ� ���ð� ī���� Time
	double			_dMax[MINMAX_CNT] = { 0, 0 };				// �ִ� ���ð� ī���� Time
	__int64			_iCall = 0;									// ���� ȣ�� Ƚ��
};

class CProfilerManager;
class CProfiler
{
	friend CProfilerManager;

private:
	CProfiler()
	{
		_threadID = GetCurrentThreadId();
		_freq.QuadPart = 0;
	}

public:
	void ProfileBegin(wstring _szName);
	void ProfileEnd(wstring _szName);
	void ProfileReset(void);

private:
	DWORD _threadID;
	LARGE_INTEGER _freq;
	_PROFILE_RESULT _profileResults[PROFILE_CNT];
	int _IDSupplier = 0;
};

class CProfilerManager
{
private:
	CProfilerManager();
	~CProfilerManager();

public:
	static CProfilerManager* GetInstance();

public:
	void ProfileBegin(wstring _szName);
	void ProfileEnd(wstring _szName);
	void ProfileReset(void);

public:
	void SetProfiler();
	void PrintResult(void);
	void SaveResult(const wchar_t* szFileName);
	void PrintResultAddup(void);
	void SaveResultAddup(const wchar_t* szFileName);

private:
	int _tlsIdx;
	static CProfilerManager _manager;
	vector<CProfiler*> _profilers;

public:
	unordered_map<wstring, _PROFILE_RESULT_FOR_ADDUP*> _resultAddupMap;
};

extern CProfilerManager* g_pManager;

#define PRO_SET()						g_pManager->SetProfiler()
#define PRO_SAVE(FileName)				g_pManager->SaveResult(FileName)
#define PRO_PRINT()						g_pManager->PrintResult()
#define PRO_SAVE_ADDUP(FileName)		g_pManager->SaveResultAddup(FileName)
#define PRO_PRINT_ADDUP()				g_pManager->PrintResultAddup()
#define PRO_BEGIN(TagName)				g_pManager->ProfileBegin(TagName)
#define PRO_END(TagName)				g_pManager->ProfileEnd(TagName)
#define PRO_RESET()						g_pManager->ProfileReset()

#else


#define PRO_SET()		
#define PRO_SAVE(FileName)				
#define PRO_PRINT()						
#define PRO_SAVE_ADDUP(FileName)		
#define PRO_PRINT_ADDUP()				
#define PRO_BEGIN(TagName)				
#define PRO_END(TagName)				
#define PRO_RESET()	


#endif
