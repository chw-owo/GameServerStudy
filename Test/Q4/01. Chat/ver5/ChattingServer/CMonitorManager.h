#pragma once
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
class CNetServer;
class CLanServer;
class CLanClient;

class CMonitorManager
{
	friend CNetServer;
	friend CLanServer;
	friend CLanClient;

private:
	CMonitorManager(bool monitorTotal);
	~CMonitorManager();

private:
	bool Initialize();
	void Terminate();

public:
	void UpdateProcessCPUTime(); 
	float GetProcessCPUTime() { return _fProcessCPU; }
	float GetProcessUserCPUTime() { return _fProcessUserCPU; }
	float GetProcessKernelCPUTime() { return _fProcessKernelCPU; }

public:
	long GetProcessOnOff();
	long GetProcessMemory();

public:
	void UpdateTotalCPUTime();
	float GetTotalCPUTime() { return _fTotalCPU; }
	float GetTotalUserCPUTime() { return _fTotalUserCPU; }
	float GetTotalKernelCPUTime() { return _fTotalKernelCPU; }

public:
	long GetTotalNonpaged();
	long GetTotalUsableMemory();

public:
	long GetTotalRecv();
	long GetTotalSend();
	
private:
	HANDLE _hProcess;
	int _iNumOfProcessors;

	float _fTotalCPU = 0.0f;
	float _fTotalUserCPU = 0.0f;
	float _fTotalKernelCPU = 0.0f;

	ULARGE_INTEGER _ftTotalCPU_LastIdle;
	ULARGE_INTEGER _ftTotalCPU_LastKernel;
	ULARGE_INTEGER _ftTotalCPU_LastUser;

	float _fProcessCPU = 0.0f;
	float _fProcessUserCPU = 0.0f;
	float _fProcessKernelCPU = 0.0f;

	ULARGE_INTEGER _ftProcess_LastTime;
	ULARGE_INTEGER _ftProcess_LastKernel;
	ULARGE_INTEGER _ftProcess_LastUser;
};
