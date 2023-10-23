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

// TO-DO: 동적 TLS를 사용하도록 수정 (지금은 고치다 말아서 동작하지 않는 코드!!)

/*=============================================================

<How to use>

1.	PRO_SAVE, PRO_SAVE_ADDUP 호출 시 확장자 이름은 포함하지 않는다.

2.	멀티 스레드 사용 시 #define MULTI_THREAD,
	싱글 스레드 사용 시 #define SINGLE_THREAD

3.	microsec 단위 출력 시 #define USE_MS_UNIT,
	nanosec 단위 출력 시 #define USE_NS_UNIT

==============================================================*/

//#define PROFILE
#ifdef PROFILE

#define MULTI_THREAD
//#define SINGLE_THREAD
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
	long			_lFlag = none;							  // 프로파일 사용 여부 및 상태		  
	wstring			_szName;								  // 프로파일 샘플 이름
	LARGE_INTEGER	_lStartTime;							  // 프로파일 샘플 실행 시간

	double			_dTotalTime = 0;						  // 전체 사용시간 카운터 Time
	double			_dMin[MINMAX_CNT] = { DBL_MAX, DBL_MAX }; // 최소 사용시간 카운터 Time
	double			_dMax[MINMAX_CNT] = { 0, 0 };			  // 최대 사용시간 카운터 Time
	__int64			_iCall = 0;								  // 누적 호출 횟수
};

class _PROFILE_RESULT_FOR_ADDUP
{
public:
	double			_dTotalTime = 0;						  // 전체 사용시간 카운터 Time
	double			_dMin[MINMAX_CNT] = { 0, 0 }; // 최소 사용시간 카운터 Time
	double			_dMax[MINMAX_CNT] = { 0, 0 };			  // 최대 사용시간 카운터 Time
	__int64			_iCall = 0;								  // 누적 호출 횟수
};

class CProfilerManager;
class CProfiler
{
	friend CProfilerManager;

public:
	void ProfileBegin(wstring _szName);
	void ProfileEnd(wstring _szName);

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
	void ProfileReset(void);
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
#define PRO_BEGIN(TagName)				pProfiler->ProfileBegin(TagName)
#define PRO_END(TagName)				pProfiler->ProfileEnd(TagName)
#define PRO_SAVE(FileName)				g_pManager->SaveResult(FileName)
#define PRO_SAVE_ADDUP(FileName)		g_pManager->SaveResultAddup(FileName)
#define PRO_PRINT()						g_pManager->PrintResult()
#define PRO_PRINT_ADDUP()				g_pManager->PrintResultAddup()
#define PRO_RESET()						g_pManager->ProfileReset()

#else

#define PRO_SET(profiler, threadID)		
#define PRO_BEGIN(TagName)				
#define PRO_END(TagName)				
#define PRO_SAVE(FileName)				
#define PRO_SAVE_ADDUP(FileName)		
#define PRO_PRINT()						
#define PRO_PRINT_ADDUP()				
#define PRO_RESET()						

#endif





