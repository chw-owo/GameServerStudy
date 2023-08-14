#include "CSession.h"
unordered_map<__int64, CSession*> g_SessionMap;
SRWLOCK g_SessionMapLock;