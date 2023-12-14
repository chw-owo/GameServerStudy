#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <pdh.h>
#include <pdhmsg.h>
#pragma comment(lib,"Pdh.lib")

/*

* cpu total 사용률
* cpu process 사용률
* 프로세스 유저할당 메모리
* 프로세스 논페이지 메모리
* 사용가능 메모리
* 논페이지드 메모리
* 네트워크 사용량

*/

CONST ULONG SAMPLE_INTERVAL_MS = 1000;
void GetPdhData();
void BrowsePdhCount();


int main(void)
{
    // BrowsePdhCount();
	GetPdhData();
	return 0;
}

void GetPdhData()
{
	PDH_HQUERY cpuQuery;
	PdhOpenQuery(NULL, NULL, &cpuQuery);

	PDH_HCOUNTER cpuTotal;
	PdhAddCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
	PdhCollectQueryData(cpuQuery);

	while (true)
	{
		Sleep(1000);

		PdhCollectQueryData(cpuQuery);

		PDH_FMT_COUNTERVALUE counterVal;
		PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);

		wprintf(L"CPU Usage : %f%%\n", counterVal.doubleValue);
	}
}

void BrowsePdhCount()
{
    PDH_STATUS Status;
    HQUERY Query = NULL;
    HCOUNTER Counter;
    PDH_FMT_COUNTERVALUE DisplayValue;
    DWORD CounterType;
    SYSTEMTIME SampleTime;
    PDH_BROWSE_DLG_CONFIG BrowseDlgData;
    WCHAR CounterPathBuffer[PDH_MAX_COUNTER_PATH] = { 0, };

    Status = PdhOpenQuery(NULL, NULL, &Query);
    if (Status != ERROR_SUCCESS)
    {
        wprintf(L"\nPdhOpenQuery failed with status 0x%x.", Status);
        goto Cleanup;
    }

    ZeroMemory(&CounterPathBuffer, sizeof(CounterPathBuffer));
    ZeroMemory(&BrowseDlgData, sizeof(PDH_BROWSE_DLG_CONFIG));

    BrowseDlgData.bIncludeInstanceIndex = FALSE;
    BrowseDlgData.bSingleCounterPerAdd = FALSE;
    BrowseDlgData.bSingleCounterPerDialog = TRUE;
    BrowseDlgData.bLocalCountersOnly = FALSE;
    BrowseDlgData.bWildCardInstances = TRUE;
    BrowseDlgData.bHideDetailBox = TRUE;
    BrowseDlgData.bInitializePath = FALSE;
    BrowseDlgData.bDisableMachineSelection = FALSE;
    BrowseDlgData.bIncludeCostlyObjects = FALSE;
    BrowseDlgData.bShowObjectBrowser = FALSE;
    BrowseDlgData.hWndOwner = NULL;
    BrowseDlgData.szReturnPathBuffer = CounterPathBuffer;
    BrowseDlgData.cchReturnPathLength = PDH_MAX_COUNTER_PATH;
    BrowseDlgData.pCallBack = NULL;
    BrowseDlgData.dwCallBackArg = 0;
    BrowseDlgData.CallBackStatus = ERROR_SUCCESS;
    BrowseDlgData.dwDefaultDetailLevel = PERF_DETAIL_WIZARD;

    Status = PdhBrowseCounters(&BrowseDlgData);

    if (Status != ERROR_SUCCESS)
    {
        if (Status == PDH_DIALOG_CANCELLED)
        {
            wprintf(L"\nDialog canceled by user.");
        }
        else
        {
            wprintf(L"\nPdhBrowseCounters failed with status 0x%x.", Status);
        }
        goto Cleanup;
    }
    else if (wcslen(CounterPathBuffer) == 0)
    {
        wprintf(L"\nUser did not select any counter.");
        goto Cleanup;
    }
    else
    {
        wprintf(L"\nCounter selected: %s\n", CounterPathBuffer);
    }

    Status = PdhAddCounter(Query, CounterPathBuffer, 0, &Counter);
    if (Status != ERROR_SUCCESS)
    {
        wprintf(L"\nPdhAddCounter failed with status 0x%x.", Status);
        goto Cleanup;
    }

    Status = PdhCollectQueryData(Query);
    if (Status != ERROR_SUCCESS)
    {
        wprintf(L"\nPdhCollectQueryData failed with 0x%x.\n", Status);
        goto Cleanup;
    }

    while (!_kbhit())
    {
        Sleep(SAMPLE_INTERVAL_MS);

        GetLocalTime(&SampleTime);

        Status = PdhCollectQueryData(Query);
        if (Status != ERROR_SUCCESS)
        {
            wprintf(L"\nPdhCollectQueryData failed with status 0x%x.", Status);
        }

        wprintf(L"\n\"%2.2d/%2.2d/%4.4d %2.2d:%2.2d:%2.2d.%3.3d\"",
            SampleTime.wMonth,
            SampleTime.wDay,
            SampleTime.wYear,
            SampleTime.wHour,
            SampleTime.wMinute,
            SampleTime.wSecond,
            SampleTime.wMilliseconds);

        Status = PdhGetFormattedCounterValue(Counter, PDH_FMT_DOUBLE, &CounterType, &DisplayValue);
        if (Status != ERROR_SUCCESS)
        {
            wprintf(L"\nPdhGetFormattedCounterValue failed with status 0x%x.", Status);
            goto Cleanup;
        }

        wprintf(L",\"%.20g\"", DisplayValue.doubleValue);
    }

Cleanup:
    if (Query)
    {
        PdhCloseQuery(Query);
    }
}