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
	RELEASE
};

struct NetworkOverlapped
{
	OVERLAPPED _ovl;
	NET_TYPE _type;
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
