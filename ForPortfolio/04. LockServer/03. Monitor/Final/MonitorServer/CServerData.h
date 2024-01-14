#pragma once
#include "CData.h"
class CServerData
{
public:
	CData CPU_TOTAL;
	CData NONPAGED_MEMORY;
	CData NETWORK_RECV;
	CData NETWORK_SEND;
	CData AVAILABLE_MEMORY;
};