#pragma once
#include <windows.h>
#include <unordered_map>
#include <string>
using namespace std;
#define PROFILE

#define NAME_LEN 64
#define THREAD_MAX 32
#define MS_PER_SEC 1000
#define PROFILE_MAX 32
#define BUFFER_SIZE 256
#define OUTPUT_SIZE BUFFER_SIZE * PROFILE_MAX

struct ProfileData
{
	long			_flag = 0;
	string			_szName;
	LARGE_INTEGER	_lStartTime;
	__int64			_iTotalTime = 0;
	__int64			_iCall = 0;
};

struct ProfileResult
{
	double			_dTotalTimes = 0;
	__int64			_iCalls = 0;
};

class ProfilerMgr;
class Profiler
{
	friend ProfilerMgr;

private:
	void ProfileBegin(string szName);
	void ProfileEnd(string szName);

private:
	ProfileData _datas[PROFILE_MAX];
};

class ProfilerMgr
{
public:
	ProfilerMgr();
	~ProfilerMgr();

public:
	Profiler* GetProfiler();
	void ProfileBegin(string szName);
	void ProfileEnd(string szName);
	void ProfilePrint(const WCHAR* szFileName);

private:
	Profiler* _profilers[THREAD_MAX] = { nullptr, };
	long _profilerIdx = -1;
	unordered_map<string, ProfileResult*> _results;

public:
	long _tlsIdx;
};

extern ProfilerMgr* g_profilerMgr;

#ifdef PROFILE
#define PRO_BEGIN(TagName)	g_profilerMgr->ProfileBegin(TagName)
#define PRO_END(TagName)	g_profilerMgr->ProfileEnd(TagName)
#define PRO_OUT(FileName)	g_profilerMgr->ProfilePrint(FileName)
#define PRO_RESET()			g_profilerMgr->ProfileReset()
#else
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#define PRO_OUT(TagName)	
#define PRO_RESET()			
#endif


