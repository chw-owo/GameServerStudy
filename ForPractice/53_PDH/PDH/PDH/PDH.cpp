#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <strsafe.h>
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
void GetEthernetData();

int main(void)
{
    // BrowsePdhCount();
	// GetPdhData();
    GetEthernetData();
	return 0;
}

int data[100000];

#define df_PDH_ETHERNET_MAX 8
//--------------------------------------------------------------
// 이더넷 하나에 대한 Send,Recv PDH 쿼리 정보.
//--------------------------------------------------------------
struct st_ETHERNET
{
    bool _bUse;
    WCHAR _szName[128];
    PDH_HCOUNTER _pdh_Counter_Network_RecvBytes;
    PDH_HCOUNTER _pdh_Counter_Network_SendBytes;
};
st_ETHERNET _EthernetStruct[df_PDH_ETHERNET_MAX];
PDH_HQUERY _netQuery;

void GetEthernetData()
{
    int iCnt = 0;
    bool bErr = false;
    WCHAR* szCur = NULL;
    WCHAR* szCounters = NULL;
    WCHAR* szInterfaces = NULL;
    DWORD dwCounterSize = 0, dwInterfaceSize = 0;
    WCHAR szQuery[1024] = { 0, };
    
    PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);
    szCounters = new WCHAR[dwCounterSize];
    szInterfaces = new WCHAR[dwInterfaceSize];
   
    if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0) != ERROR_SUCCESS)
    {
        delete[] szCounters;
        delete[] szInterfaces;
        __debugbreak();
        return;
    }
    iCnt = 0;
    szCur = szInterfaces;
    PdhOpenQuery(NULL, NULL, &_netQuery);

    for (; *szCur != L'\0' && iCnt < df_PDH_ETHERNET_MAX; szCur += wcslen(szCur) + 1, iCnt++)
    {
        _EthernetStruct[iCnt]._bUse = true;
        _EthernetStruct[iCnt]._szName[0] = L'\0';
        wcscpy_s(_EthernetStruct[iCnt]._szName, szCur);
             
        szQuery[0] = L'\0';
        StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur);
        ::wprintf(L"recv: %s\n", szQuery);
        PdhAddCounter(_netQuery, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes);
       
        szQuery[0] = L'\0';
        StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur);
        ::wprintf(L"send: %s\n", szQuery);
        PdhAddCounter(_netQuery, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes);
    }
    
    while (1)
    {
        float recv = 0;
        float send = 0;

        for (iCnt = 0; iCnt < df_PDH_ETHERNET_MAX; iCnt++)
        {
            if (_EthernetStruct[iCnt]._bUse)
            {
                PdhCollectQueryData(_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes);
                PDH_FMT_COUNTERVALUE counterVal1;
                long status = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes, PDH_FMT_DOUBLE, NULL, &counterVal1);
                ::wprintf(L"recv: %f\n", counterVal1.doubleValue);
                if (status == 0) recv += counterVal1.doubleValue;

                PdhCollectQueryData(_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes);
                PDH_FMT_COUNTERVALUE counterVal2;
                status = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes, PDH_FMT_DOUBLE, NULL, &counterVal2);
                ::wprintf(L"send: %f\n", counterVal2.doubleValue);
                if (status == 0) send += counterVal2.doubleValue;
            }
        }

        //::printf("recv: %f\n", recv);
        //::printf("send: %f\n", send);

        Sleep(1000);
    }
}

void GetPdhData()
{
	PDH_HQUERY cpuQuery;
    PDH_HCOUNTER cpuTotal;
	PdhOpenQuery(NULL, NULL, &cpuQuery);
	PdhAddCounter(cpuQuery, L"\\Process(PDH)\\Private Bytes", NULL, &cpuTotal);

	while (true)
	{
		Sleep(1000);

		PdhCollectQueryData(cpuQuery);
		PDH_FMT_COUNTERVALUE counterVal;
		PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);

		wprintf(L"Private Bytes : %f\n", counterVal.doubleValue);
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