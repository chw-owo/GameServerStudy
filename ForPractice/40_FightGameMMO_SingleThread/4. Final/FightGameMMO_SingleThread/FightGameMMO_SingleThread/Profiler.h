#pragma once

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>

#define PROFILE
#define dfPRO_TITLE_LEN 64

#ifdef PROFILE
#define PRO_BEGIN(tagname)		ProfileBegin(tagname)
#define PRO_END(tagname)		ProfileEnd(tagname)
#define PRO_RESET()				ProfileReset()
#define PRO_SAVE(filename)		ProfileSaveResult(filename)
#define PRO_PRINT()				ProfilePrintResult()
#else
#define PRO_BEGIN(tagname)
#define PRO_END(tagname)
#define PRO_RESET()	
#define PRO_SAVE(tagname)	
#define PRO_PRO_PRINT()			
#endif

struct _PROFILE_RESULT;
void ProfileBegin(const WCHAR* name);
void ProfileEnd(const WCHAR* name);
void ProfileSaveResult(const WCHAR* filename);
void ProfileReset(void);
void ProfilePrintResult(void);
