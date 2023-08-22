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
#pragma comment(lib, "Synchronization.lib")
using namespace std;

/*=============================================================

<How to use>

1.	PRO_SAVE, PRO_SAVE_ADDUP ȣ�� �� Ȯ���� �̸��� �������� �ʴ´�.

2.	���� TLS ��� �� #define USE_STLS,
	���� TLS ��� �� #define USE_DTLS,
	�̱� ������ ���� #define SINGLE_THREAD

3.	microsec ���� ��� �� #define USE_MS_UNIT,
	nanosec ���� ��� �� #define USE_NS_UNIT

==============================================================*/

#define PROFILE
#define USE_STLS
//#define USE_DTLS
//#define SINGLE_THREAD

#ifdef PROFILE

#define USE_MS_UNIT
//#define USE_NS_UNIT

#define THREAD_MAX 10
#define NAME_LEN 64
#define FILE_NAME_LEN 128
#define PROFILE_CNT 32
#define MINMAX_CNT 2
#define BUFFER_SIZE 256
#define OUTPUT_SIZE BUFFER_SIZE * PROFILE_CNT
#define MS_PER_SEC 1000000
#define NS_PER_SEC 1000

enum ProfilerState
{
	none = 0,
	underway,
	complete,
	pause
};

class _PROFILE_RESULT
{
public:
	long			_lFlag = none;							  // �������� ��� ���� �� ����		  
	wstring			_szName;								  // �������� ���� �̸�
	LARGE_INTEGER	_lStartTime;							  // �������� ���� ���� �ð�

	double			_dTotalTime = 0;						  // ��ü ���ð� ī���� Time
	double			_dMin[MINMAX_CNT] = { DBL_MAX, DBL_MAX }; // �ּ� ���ð� ī���� Time
	double			_dMax[MINMAX_CNT] = { 0, 0 };			  // �ִ� ���ð� ī���� Time
	__int64			_iCall = 0;								  // ���� ȣ�� Ƚ��
};

class _PROFILE_RESULT_FOR_ADDUP
{
public:
	double			_dTotalTime = 0;						  // ��ü ���ð� ī���� Time
	double			_dMin[MINMAX_CNT] = { 0, 0 }; // �ּ� ���ð� ī���� Time
	double			_dMax[MINMAX_CNT] = { 0, 0 };			  // �ִ� ���ð� ī���� Time
	__int64			_iCall = 0;								  // ���� ȣ�� Ƚ��
};

class CProfilerManager;
class CProfiler
{
	friend CProfilerManager;

public:
	void ProfileBegin(wstring _szName);
	void ProfileEnd(wstring _szName);
	void ProfileReset(void);

private:
	LARGE_INTEGER _freq;
	_PROFILE_RESULT _profileResults[PROFILE_CNT];
	DWORD _threadID;
	int _IDSupplier = 0;
	long _pauseFlag = pause;
};

class CProfilerManager
{
private:
	CProfilerManager();
	~CProfilerManager();

public:
	static CProfilerManager* GetInstance();

public:
	void SetProfiler(CProfiler* pProfiler, DWORD threadID);
	void PrintResult(void);
	void SaveResult(const wchar_t* szFileName);
	void PrintResultAddup(void);
	void SaveResultAddup(const wchar_t* szFileName);

private:
	static CProfilerManager _manager;
	vector<CProfiler*> _profilers;
	long _underwayFlag = underway;

public:
	unordered_map<wstring, _PROFILE_RESULT_FOR_ADDUP*> _resultAddupMap;
};

extern CProfilerManager* g_pManager;

#define PRO_SET(profiler, threadID)		g_pManager->SetProfiler(profiler, threadID)
#define PRO_SAVE(FileName)				g_pManager->SaveResult(FileName)
#define PRO_PRINT()						g_pManager->PrintResult()
#define PRO_SAVE_ADDUP(FileName)		g_pManager->SaveResultAddup(FileName)
#define PRO_PRINT_ADDUP()				g_pManager->PrintResultAddup()

#ifdef USE_STLS
#define PRO_BEGIN(TagName)		pSTLSProfiler->ProfileBegin(TagName)
#define PRO_END(TagName)		pSTLSProfiler->ProfileEnd(TagName)
#define PRO_RESET()				pSTLSProfiler->ProfileReset()

#endif

#ifdef USE_DTLS
#define PRO_BEGIN(TagName)		pProfiler->ProfileBegin(TagName)
#define PRO_END(TagName)		pProfiler->ProfileEnd(TagName)
#define PRO_RESET()				pProfiler->ProfileReset()
#endif

#ifdef SINGLE_THREAD
#define PRO_BEGIN(TagName)		pProfiler->ProfileBegin(TagName)
#define PRO_END(TagName)		pProfiler->ProfileEnd(TagName)
#define PRO_RESET()				pProfiler->ProfileReset()
#endif

#else
#define PRO_INIT()	
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#define PRO_RESET()
#define PRO_SAVE(FileName)			
#define PRO_PRINT()					
#define PRO_SAVE_ADDUP(FileName)	
#define PRO_PRINT_ADDUP()			
#endif





