#pragma once
#include "NetworkManager.h"
#include "PlayerManager.h"

#include <windows.h>
#pragma comment(lib, "winmm.lib")

#define FPS 50
extern bool g_bShutdown;
bool SkipForFixedFrame();