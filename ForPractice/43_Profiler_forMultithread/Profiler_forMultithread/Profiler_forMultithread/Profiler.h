#pragma once
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
#include <windows.h>
#include <iostream>
#include <float.h>
#include <vector>
using namespace std;

#define PROFILE
#define USE_STLS
//#define USE_DTLS
//#define NO_USE_TLS

#ifdef PROFILE

#define USE_MS_UNIT
//#define USE_NS_UNIT

#define THREAD_MAX 10
#define NAME_LEN 64
#define PROFILE_CNT 32
#define MINMAX_CNT 2
#define BUFFER_SIZE 256
#define OUTPUT_SIZE BUFFER_SIZE * PROFILE_CNT
#define MS_PER_SEC 1000000
#define NS_PER_SEC 1000

enum ProfilerState
{
	none = 0,
	begin_in,
	begin_out,
	end_in,
	end_out
};

struct _PROFILE_RESULT
{
	long			_lFlag = none;							  // 프로파일의 사용 여부
	WCHAR			_szName[NAME_LEN] = { '/0', };			  // 프로파일 샘플 이름
	LARGE_INTEGER	_lStartTime;							  // 프로파일 샘플 실행 시간

	double			_dTotalTime = 0;						  // 전체 사용시간 카운터 Time
	double			_dMin[MINMAX_CNT] = { DBL_MAX, DBL_MAX }; // 최소 사용시간 카운터 Time
	double			_dMax[MINMAX_CNT] = { 0, 0 };			  // 최대 사용시간 카운터 Time
	__int64			_iCall = 0;								  // 누적 호출 횟수
};

class CProfiler
{
public:
	LARGE_INTEGER _freq;
	_PROFILE_RESULT _profileResults[PROFILE_CNT];

public:
	void ProfileBegin(const WCHAR* _szName);
	void ProfileEnd(const WCHAR* _szName);
	void ProfileReset(void);
};

class CProfilerManager
{
private:
	CProfilerManager();
	~CProfilerManager();

public:
	static CProfilerManager* GetInstance();

public:
	void InitializeProfiler();
	void PrintDataOnConsole(void);
	void ProfileDataOutText(const WCHAR* szFileName);

private:
	void PauseAllProfiler();
	void RestartAllProfiler();

private:
	static CProfilerManager _manager;
	vector<CProfiler*> _profilers;
};

CProfilerManager* g_pManager = CProfilerManager::GetInstance();
#define PRO_INIT()				g_pManager->InitializeProfiler()
#define PRO_FILE_OUT(FileName)	g_pManager ->ProfileDataOutText(FileName)
#define PRO_PRINT_CONSOLE()		g_pManager ->PrintDataOnConsole()

#ifdef USE_STLS
__declspec (thread) CProfiler*  pSTLSProfiler;
#define PRO_BEGIN(TagName)		pSTLSProfiler->ProfileBegin(TagName)
#define PRO_END(TagName)		pSTLSProfiler->ProfileEnd(TagName)
#define PRO_RESET()				pSTLSProfiler->ProfileReset()
#endif

#ifdef USE_DTLS
#define PRO_BEGIN(TagName)		pProfiler->ProfileBegin(TagName)
#define PRO_END(TagName)		pProfiler->ProfileEnd(TagName)
#define PRO_RESET()				pProfiler->ProfileReset()
#endif

#ifdef NO_USE_TLS
#define PRO_BEGIN(TagName)		pProfiler->ProfileBegin(TagName)
#define PRO_END(TagName)		pProfiler->ProfileEnd(TagName)
#define PRO_RESET()				pProfiler->ProfileReset()
#endif

#else
#define PRO_INIT()	
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#define PRO_RESET()
#define PRO_FILE_OUT(TagName)	
#define PRO_PRO_PRINT_CONSOLE()		
#endif





