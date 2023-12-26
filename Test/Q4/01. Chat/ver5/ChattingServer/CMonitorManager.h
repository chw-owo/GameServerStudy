#pragma once
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#include <windows.h>
#pragma comment(lib, "winmm.lib") 

#include <stdio.h>
#include <conio.h>
#include <pdh.h>
#include <pdhmsg.h>
#pragma comment(lib,"Pdh.lib")

class CNetServer;
class CLanServer;
class CLanClient;

class CMonitorManager
{
	friend CNetServer;
	friend CLanServer;
	friend CLanClient;

private:
	CMonitorManager(bool monitorServer);
	~CMonitorManager();

private:
	bool Initialize();
	void Terminate();

private:
	void GetCPUData();
	void GetMemoryData();
	void GetEthernetData();

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
	bool _monitorServer;

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

private:
	PDH_HQUERY _processMemQuery;
	PDH_HQUERY _nonpagedMemQuery;
	PDH_HQUERY _usableMemQuery;
	PDH_HCOUNTER _processMemTotal;
	PDH_HCOUNTER _nonpagedMemTotal;
	PDH_HCOUNTER _usableMemTotal;


private:
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
	st_ETHERNET _EthernetStruct[df_PDH_ETHERNET_MAX]; // 랜카드 별 PDH 정보
	PDH_HQUERY _netQuery;
};
