#pragma once

#include "Config.h"
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// For Session =================================================================

enum class NET_TYPE
{
	SEND_COMPLETE = 0,
	RECV_COMPLETE,
	SEND_POST,
	RECV_POST,
	RELEASE
};

struct NetworkOverlapped
{
	OVERLAPPED _ovl;
	NET_TYPE _type;
};

union ValidFlag
{
	struct
	{
		volatile short _releaseFlag;
		volatile short _useCount;
	};
	volatile long _flag;
};

// For Job =================================================================

enum class JOB_TYPE
{
	NONE,
	SYSTEM,
	CONTENT
};

enum class SYS_TYPE
{
	NONE,
	ACCEPT,
	RELEASE,
	TIMEOUT,
	TERMINATE
};
