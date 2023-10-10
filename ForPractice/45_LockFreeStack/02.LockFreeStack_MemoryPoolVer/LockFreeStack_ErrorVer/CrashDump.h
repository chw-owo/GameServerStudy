#pragma once

#ifndef __CRASH_DUMP__
#define __CRASH_DUMP__

#include <stdio.h>
#include <crtdbg.h>
#include <windows.h>
#include <minidumpapiset.h>

#pragma comment(lib, "Dbghelp.lib")

class CCrashDump
{
public:
	CCrashDump()
	{
		_dumpCount = 0;
		_invalid_parameter_handler oldHandler, newHandler;
		newHandler = customInvalidParameterHandler;
		oldHandler = _set_invalid_parameter_handler(newHandler);

		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);
		_CrtSetReportHook(_custom_Report_hook);
		_set_purecall_handler(customPureCallHandler);

		SetHandlerDump();
	}

	static LONG WINAPI CustomExceptionFilter(
		__in PEXCEPTION_POINTERS pExceptionPointer)
	{
		long dumpCount = InterlockedIncrement(&_dumpCount);
		if (dumpCount != 1) return -1;

		SYSTEMTIME stTime;
		WCHAR filename[MAX_PATH];
		GetLocalTime(&stTime);
		wsprintf(filename, L"Dump_%d%02d%02d_%02d.%02d.%02d_%d.dmp",
			stTime.wYear, stTime.wMonth, stTime.wDay,
			stTime.wHour, stTime.wMinute, stTime.wSecond, dumpCount);
		wprintf(L"\n\n\n Crash Error!!! %d.%d.%d/%d:%d:%d\n",
			stTime.wYear, stTime.wMonth, stTime.wDay,
			stTime.wHour, stTime.wMinute, stTime.wSecond);
		wprintf(L"Now Save Dump File...\n");

		HANDLE hDumpFile = ::CreateFile(
			filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (hDumpFile != INVALID_HANDLE_VALUE)
		{
			_MINIDUMP_EXCEPTION_INFORMATION MinidumpExceptionInformation;
			MinidumpExceptionInformation.ThreadId = GetCurrentThreadId();
			MinidumpExceptionInformation.ExceptionPointers = pExceptionPointer;
			MinidumpExceptionInformation.ClientPointers = TRUE;

			MiniDumpWriteDump(
				GetCurrentProcess(), GetCurrentProcessId(), hDumpFile,
				MiniDumpWithFullMemory, &MinidumpExceptionInformation, NULL, NULL);
			CloseHandle(hDumpFile);
			wprintf(L"CrashDump Save Finish!\n");
		}

		printf("Hey~~");
		for (;;);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void Crash(void)
	{
		__debugbreak();
	}

	static void SetHandlerDump()
	{
		SetUnhandledExceptionFilter(CustomExceptionFilter);
	}

	static void customInvalidParameterHandler(
		const wchar_t* expression, const wchar_t* function,
		const wchar_t* file, unsigned int line, uintptr_t pReserverd)
	{
		Crash();
	}

	static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue)
	{
		Crash();
		return true;
	}

	static void customPureCallHandler(void)
	{
		Crash();
	}

	static long _dumpCount;
};

long CCrashDump::_dumpCount = 0;

#endif __CRASH_DUMP__