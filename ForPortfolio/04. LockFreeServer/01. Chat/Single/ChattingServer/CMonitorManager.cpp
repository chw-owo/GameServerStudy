#include "CMonitorManager.h"
#include <strsafe.h>

CMonitorManager::CMonitorManager(bool monitorServer)
{
	_monitorServer = monitorServer;

	GetCPUData();
	GetMemoryData();
	GetEthernetData();
}

CMonitorManager::~CMonitorManager()
{
}

bool CMonitorManager::Initialize()
{
	return false;
}

void CMonitorManager::Terminate()
{
}

void CMonitorManager::GetCPUData()
{
	_hProcess = GetCurrentProcess();
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	_iNumOfProcessors = SystemInfo.dwNumberOfProcessors;

	_ftTotalCPU_LastIdle.QuadPart = 0;
	_ftTotalCPU_LastKernel.QuadPart = 0;
	_ftTotalCPU_LastUser.QuadPart = 0;

	_ftProcess_LastTime.QuadPart = 0;
	_ftProcess_LastKernel.QuadPart = 0;
	_ftProcess_LastUser.QuadPart = 0;
}

void CMonitorManager::GetMemoryData()
{
	PdhOpenQuery(NULL, NULL, &_processMemQuery);
	PdhOpenQuery(NULL, NULL, &_nonpagedMemQuery);
	PdhOpenQuery(NULL, NULL, &_usableMemQuery);
	PdhAddCounter(_processMemQuery, L"\\Process(ChattingServer)\\Private Bytes", NULL, &_processMemTotal);
	PdhAddCounter(_nonpagedMemQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &_nonpagedMemTotal);
	PdhAddCounter(_usableMemQuery, L"\\Memory\\Available MBytes", NULL, &_usableMemTotal);
}

void CMonitorManager::GetEthernetData()
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

	for (; *szCur != L'\0' && iCnt < df_PDH_ETHERNET_MAX; szCur += wcslen(szCur) + 1, iCnt++)
	{
		_EthernetStruct[iCnt]._bUse = true;
		_EthernetStruct[iCnt]._szName[0] = L'\0';
		wcscpy_s(_EthernetStruct[iCnt]._szName, szCur);

		szQuery[0] = L'\0';
		PdhOpenQuery(NULL, NULL, &_EthernetStruct[iCnt]._recvQuery);
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received\/sec", szCur);
		PdhAddCounter(_EthernetStruct[iCnt]._recvQuery, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes);

		szQuery[0] = L'\0';
		PdhOpenQuery(NULL, NULL, &_EthernetStruct[iCnt]._sendQuery);
		StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent\/sec", szCur);
		PdhAddCounter(_EthernetStruct[iCnt]._sendQuery, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes);
	}
}

void CMonitorManager::UpdateProcessCPUTime()
{
	ULARGE_INTEGER None;
	ULARGE_INTEGER Kernel;
	ULARGE_INTEGER User;
	ULARGE_INTEGER NowTime;

	GetSystemTimeAsFileTime((LPFILETIME)&NowTime);
	GetProcessTimes(_hProcess, (LPFILETIME)&None,
		(LPFILETIME)&None, (LPFILETIME)&Kernel, (LPFILETIME)&User);

	ULONGLONG TimeDiff = NowTime.QuadPart - _ftProcess_LastTime.QuadPart;
	ULONGLONG KernelDiff = Kernel.QuadPart - _ftProcess_LastKernel.QuadPart;
	ULONGLONG UserDiff = User.QuadPart - _ftProcess_LastUser.QuadPart;
	ULONGLONG Total = KernelDiff + UserDiff;

	_fProcessCPU = (float)(Total / (double)_iNumOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessKernelCPU = (float)(KernelDiff / (double)_iNumOfProcessors / (double)TimeDiff * 100.0f);
	_fProcessUserCPU = (float)(UserDiff / (double)_iNumOfProcessors / (double)TimeDiff * 100.0f);

	_ftProcess_LastTime = NowTime;
	_ftProcess_LastKernel = Kernel;
	_ftProcess_LastUser = User;
}

long CMonitorManager::GetProcessOnOff()
{
	// TO-DO
	return 1;
}

void CMonitorManager::UpdateTotalCPUTime()
{
	ULARGE_INTEGER Idle;
	ULARGE_INTEGER Kernel;
	ULARGE_INTEGER User;
	if (GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User) == false) return;

	ULONGLONG IdleDiff = Idle.QuadPart - _ftTotalCPU_LastIdle.QuadPart;
	ULONGLONG KernelDiff = Kernel.QuadPart - _ftTotalCPU_LastKernel.QuadPart;
	ULONGLONG UserDiff = User.QuadPart - _ftTotalCPU_LastUser.QuadPart;
	ULONGLONG Total = KernelDiff + UserDiff;

	if (Total == 0)
	{
		_fTotalCPU = 0.0f;
		_fTotalKernelCPU = 0.0f;
		_fTotalUserCPU = 0.0f;
	}
	else
	{
		_fTotalCPU = (float)((double)(Total - IdleDiff) / Total * 100.0f);
		_fTotalKernelCPU = (float)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
		_fTotalUserCPU = (float)((double)UserDiff / Total * 100.0f);
	}

	_ftTotalCPU_LastIdle = Idle;
	_ftTotalCPU_LastKernel = Kernel;
	_ftTotalCPU_LastUser = User;
}

long CMonitorManager::GetProcessMemory()
{
	PdhCollectQueryData(_processMemQuery);
	PDH_FMT_COUNTERVALUE counterVal;
	PdhGetFormattedCounterValue(_processMemTotal, PDH_FMT_LONG, NULL, &counterVal);
	return counterVal.longValue / 1000000;
}

long CMonitorManager::GetTotalNonpaged()
{
	PdhCollectQueryData(_nonpagedMemQuery);
	PDH_FMT_COUNTERVALUE counterVal;
	PdhGetFormattedCounterValue(_nonpagedMemTotal, PDH_FMT_LONG, NULL, &counterVal);
	return counterVal.longValue / 1000000;
}

long CMonitorManager::GetTotalUsableMemory()
{
	PdhCollectQueryData(_usableMemQuery);
	PDH_FMT_COUNTERVALUE counterVal;
	PdhGetFormattedCounterValue(_usableMemTotal, PDH_FMT_LONG, NULL, &counterVal);
	return counterVal.longValue;
}

long CMonitorManager::GetTotalRecv()
{
	long sum = 0;
	for (int iCnt = 0; iCnt < df_PDH_ETHERNET_MAX; iCnt++)
	{
		if (_EthernetStruct[iCnt]._bUse)
		{
			PDH_FMT_COUNTERVALUE counterVal;
			PdhCollectQueryData(_EthernetStruct[iCnt]._recvQuery);
			long status = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes, PDH_FMT_DOUBLE, NULL, &counterVal);
			if (status == 0) sum += counterVal.doubleValue;
		}
	}
	return sum;
}

long CMonitorManager::GetTotalSend()
{
	long sum = 0;
	for (int iCnt = 0; iCnt < df_PDH_ETHERNET_MAX; iCnt++)
	{
		if (_EthernetStruct[iCnt]._bUse)
		{
			PDH_FMT_COUNTERVALUE counterVal;
			PdhCollectQueryData(_EthernetStruct[iCnt]._sendQuery);
			long status = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes, PDH_FMT_DOUBLE, NULL, &counterVal);
			if (status == 0) sum += counterVal.doubleValue;
		}
	}
	return sum;
}
