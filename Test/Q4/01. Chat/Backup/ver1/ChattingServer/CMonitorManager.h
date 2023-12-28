#pragma once

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

private:
	long GetProcessCPU();
	long GetProcessOnOff();
	long GetProcessMemory();

private:
	long GetCPUTotal();
	long GetNonpagedTotal();
	long GetUsableMemoryTotal();
	long GetRecvTotal();
	long GetSendTotal();
	
};
