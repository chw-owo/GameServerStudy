#pragma once
#include <windows.h>

#define PROFILE

#ifdef PROFILE
#define PRO_BEGIN(TagName)	ProfileBegin(TagName)
#define PRO_END(TagName)	ProfileEnd(TagName)
#define PRO_OUT(FileName)	ProfileDataOutText(FileName)
#define PRO_RESET()			ProfileReset()
//#define PRO_DEBUG()			PrintDataForDebug()

#else
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#define PRO_OUT(TagName)	
#define PRO_RESET()			
#endif

struct _PROFILE_RESULT;
void ProfileBegin(const WCHAR* szName);
void ProfileEnd(const WCHAR* szName);
void ProfileDataOutText(const WCHAR* szFileName);
void ProfileReset(void);
//void PrintDataForDebug(void);
